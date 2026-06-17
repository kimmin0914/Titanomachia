#define _CRT_SECURE_NO_WARNINGS // 윈도우 보안 경고창 숨김
#include <stdio.h> // 표준 입출력 도구 포함
#include <stdlib.h> // 난수 생성 및 프로그램 제어 도구 포함
#include <time.h> // 실제 시간 연산 도구 포함
#include <windows.h> // 콘솔 화면 색상 및 커서 제어 도구 포함
#include <conio.h> // 비동기 키보드 입력 제어 도구 포함
#include <string.h> // 배열 메모리 일괄 초기화 도구 포함

#define ROWS 25 // 게임 화면 세로 길이 설정
#define COLS 60 // 게임 화면 가로 길이 설정

#define C_DEFAULT   15 // 기본 흰색 글씨 설정
#define C_PLAYER    11 // 플레이어 청록색 설정
#define C_BULLET    11 // 플레이어 총알 청록색 설정
#define C_BOSS      12 // 보스 본체 빨간색 설정
#define C_BOSS_HIT  240 // 보스 피격 시 흰 바탕 번쩍임 효과 설정
#define C_WARNING   14 // 레이저 발사 전 바닥 노란색 경고 설정
#define C_LASER     12 // 실제 레이저 공격 빨간색 설정
#define C_WAVE      9 // 파동 공격 파란색 설정
#define C_ORB       13 // 추적 유도탄 자주색 설정
#define C_STAR      8 // 우주 배경 별빛 회색 설정
#define C_STONE     8 // 보스 무적 기믹 시 회색 돌덩이 색상 설정
#define C_MINION    13 // 소환수 자주색 설정

#define FRAME_DELAY 16 // 초당 60프레임 유지를 위해 16밀리초 대기 시간 설정

// 화면을 굴러다니는 객체들의 정보를 담는 구조체 생성
typedef struct {
    int r; // 세로 위치
    int c; // 가로 위치
    int active; // 화면에 존재하는지 여부 확인
    int dir; // 좌우 이동 방향
} Entity;

char map[ROWS][COLS]; // 화면에 출력할 글자를 임시로 그려둘 2차원 배열 공간 생성
int color_map[ROWS][COLS]; // 위 글자들의 색상 번호를 기억할 2차원 배열 공간 생성

int player_r, player_c; // 플레이어 현재 세로 및 가로 위치
int player_hp, player_max_hp; // 플레이어 현재 체력 및 최대 체력

// 보스의 거대한 형태를 한 줄씩 쪼개어 저장한 아스키아트 문자열
const char* titan_art[] = {
    "      /( )                                  ( )\\      ",
    "     /( . )         _,,=########=,,_         ( . )\\     ",
    "    /( .  )       ,=################=,       (  . )\\    ",
    "   /  . (      /## [<@>] ###### [<@>] ##\\      ) .  \\   ",
    "   \\  . (      \\##  \"\"\"  ######  \"\"\"  ##/      ) .  /   ",
    "    \\    /      \\####,  .======.  ,####/      \\    /    ",
    "      \\  /        '####\\WWWWWWWW/####'        \\  /      ",
    "        \\/          '###\\MMMMMM/###'          \\/        "
};
int titan_width = 56; // 보스 그림 가로 너비
int titan_height = 8; // 보스 그림 세로 높이
int titan_hp, titan_max_hp; // 보스 현재 체력 및 최대 체력
int hit_flash_timer = 0; // 보스 피격 시 번쩍이는 연출 지속 시간
int shake_offset = 0; // 지진 연출 시 화면 좌우 흔들림 변위 값

Entity bullets[50]; // 플레이어 총알 최대 50발 생성
Entity orbs[10]; // 보스 유도탄 최대 10발 생성
Entity minions[5]; // 기믹 소환수 최대 5기 생성

int diff_level; // 난이도 숫자 기억
char diff_name[10]; // 난이도 이름 문자열 기억

int laser_warning[COLS]; // 세로줄마다 레이저 경고가 몇 초 남았는지 기억
int laser_firing[COLS]; // 세로줄마다 실제 레이저 타격이 몇 초 남았는지 기억

int laser_timer = 0, wave_timer = 0, orb_timer = 0; // 각 패턴이 발사되기 전까지 기다리는 대기 시간 시계
int wave_active = 0; // 파동이 현재 화면에 내려오고 있는지 여부 확인
int wave_r = 0; // 파동 현재 세로 위치
int wave_hole_c = 0; // 파동에서 플레이어가 피할 수 있는 구멍 위치
int tick_count = 0; // 게임 시작 후 프레임이 몇 번 지나갔는지 기록하는 시계

int phase = 1; // 보스의 현재 패턴 단계 설정
time_t ritual_end_time = 0; // 타임어택 기믹이 끝나는 실제 현실 시계 시간 기록
int ritual_time_left = 0; // 화면에 출력할 남은 초 계산
time_t stun_end_time = 0; // 보스 기절이 끝나는 실제 현실 시계 시간 기록
time_t enrage_end_time = 0; // 광폭화 지진 연출이 끝나는 실제 현실 시계 시간 기록
int has_done_ritual = 0; // 체력 60프로 기믹을 이미 수행했는지 확인
int has_done_berserk = 0; // 체력 30프로 광폭화를 이미 수행했는지 확인

void set_color(int color_code); // 색상 변경 함수 선언
void gotoxy(int x, int y); // 커서 이동 함수 선언
void hide_cursor(void); // 커서 숨김 함수 선언
void clear_buffers(void); // 그리기 배열 초기화 함수 선언
void clear_attacks(void); // 화면에 남은 공격 일괄 삭제 함수 선언

void show_title(void); // 타이틀 화면 함수 선언
void show_help(void); // 도움말 화면 함수 선언
void show_difficulty(void); // 난이도 선택 화면 함수 선언

void init_game(void); // 인게임 데이터 초기화 함수 선언
void handle_input(void); // 플레이어 조작 입력 함수 선언
void update_logic(void); // 물리 엔진 및 패턴 계산 함수 선언
void build_frame(void); // 그리기 배열 구성 함수 선언
void draw_frame(void); // 실제 화면 출력 함수 선언
int  run_game(void); // 인게임 반복 실행 엔진 함수 선언

void play_ending(void); // 승리 시 폭발 및 크레딧 연출 함수 선언

// 콘솔 텍스트 색상 변경 기능
void set_color(int color_code) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color_code); // 윈도우 기능 호출하여 색상 변경
}

// 콘솔 커서 위치 순간 이동 기능
void gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y }; // 입력받은 좌표값 구조체 생성
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos); // 윈도우 기능 호출하여 해당 좌표로 커서 점프
}

// 키보드 입력 커서 깜빡임 숨김 기능
void hide_cursor(void) {
    CONSOLE_CURSOR_INFO cci = { 1, FALSE }; // 커서 숨김 옵션 설정
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cci); // 윈도우 기능 호출하여 커서 숨김 적용
}

// 렌더링 배열을 빈칸으로 백지화하는 기능
void clear_buffers() {
    for (int r = 0; r < ROWS; r++) { // 세로줄 전체 순회
        for (int c = 0; c < COLS; c++) { // 가로줄 전체 순회
            map[r][c] = ' '; // 글자는 모두 빈칸으로 덮어쓰기
            color_map[r][c] = C_DEFAULT; // 색상은 모두 기본 하얀색으로 덮어쓰기
        }
    }
}

// 화면에 깔려있는 보스의 모든 공격과 쿨타임을 초기화하는 기능
void clear_attacks(void) {
    memset(laser_warning, 0, sizeof(laser_warning)); // 레이저 경고 배열 메모리 일괄 삭제
    memset(laser_firing, 0, sizeof(laser_firing)); // 진짜 레이저 배열 메모리 일괄 삭제
    wave_active = 0; // 파동 비활성화
    for (int o = 0; o < 10; o++) orbs[o].active = 0; // 돌아다니는 유도탄 전체 비활성화
    laser_timer = 0; wave_timer = 0; orb_timer = 0; // 공격 패턴별 대기 시간 모두 0으로 초기화
}

// 게임 최초 실행 시 출력되는 메인 타이틀 화면 기능
void show_title(void) {
    while (1) { // 정상적인 번호를 입력할 때까지 무한 반복
        system("cls"); // 콘솔 화면 전체 지우기
        set_color(C_DEFAULT); // 기본 흰색 설정
        printf("\n\n");
        printf("   ====================================================\n");
        printf("           T I T A N O M A C H I A  :  거신 토벌전       \n");
        printf("   ====================================================\n\n");
        printf("                    1. 게임 시작\n");
        printf("                    2. 게임 설명\n");
        printf("                    3. 게임 종료\n\n");
        printf("   번호를 입력하세요: ");

        char choice = _getch(); // 사용자 키보드 입력값 대기 및 즉시 수신
        if (choice == '1') return; // 1번 입력 시 함수 종료 후 다음 단계 진입
        else if (choice == '2') show_help(); // 2번 입력 시 설명서 화면 함수 호출
        else if (choice == '3') exit(0); // 3번 입력 시 프로그램 강제 종료
    }
}

// 게임 설명 및 조작법 안내 화면 기능
void show_help(void) {
    system("cls"); // 콘솔 화면 전체 지우기
    set_color(C_DEFAULT); // 기본 흰색 설정
    printf("\n   [ 게임 설명 ]\n");
    printf("   거신 티탄의 무수한 공격을 피하며 쓰러뜨리세요!\n");
    printf("   ※ 주의: 보스의 체력에 따라 특수 기믹과 광폭화가 발동됩니다.\n\n");
    printf("   [ 조작법 ]\n");
    printf("   방향키 상하좌우 : 플레이어 이동\n");
    printf("   스페이스바 : 공격 발사\n\n");
    printf("   [ 특수 페이즈 안내 ]\n");
    printf("   - 체력 60프로 도달: 보스가 무적이 되며 광신도를 소환합니다.\n");
    printf("     제한 시간 내에 처치하지 못하면 보스가 체력을 대폭 회복합니다.\n");
    printf("   - 체력 30프로 도달: 보스가 광폭화하여 공격 주기가 극단적으로 감소합니다.\n\n");
    printf("   아무 키나 누르면 메인 메뉴로 돌아갑니다...\n");

    (void)_getch(); // 키보드 아무 키나 입력할 때까지 대기
}

// 난이도 선택 화면 기능
void show_difficulty(void) {
    while (1) { // 정상적인 번호를 입력할 때까지 무한 반복
        system("cls"); // 콘솔 화면 전체 지우기
        set_color(C_DEFAULT); // 기본 흰색 설정
        printf("\n   [ 난이도 선택 ]\n\n");
        printf("   1. 쉬움   (내 체력 5 보스 체력 150) - 기본 레이저 공격\n");
        printf("   2. 보통   (내 체력 4 보스 체력 300) - 레이저와 파동 결합\n");
        printf("   3. 어려움 (내 체력 3 보스 체력 500) - 레이저와 파동과 유도탄 결합\n\n");
        printf("   난이도 번호를 선택하세요: ");

        char choice = _getch(); // 사용자 키보드 입력값 즉시 수신
        if (choice == '1') { diff_level = 1; player_max_hp = 5; titan_max_hp = 150; sprintf(diff_name, "Easy"); return; } // 쉬움 난이도 수치 설정 후 탈출
        if (choice == '2') { diff_level = 2; player_max_hp = 4; titan_max_hp = 300; sprintf(diff_name, "Normal"); return; } // 보통 난이도 수치 설정 후 탈출
        if (choice == '3') { diff_level = 3; player_max_hp = 3; titan_max_hp = 500; sprintf(diff_name, "Hard"); return; } // 어려움 난이도 수치 설정 후 탈출
    }
}

// 게임 진입 전 이전 플레이 데이터 지우기 및 변수 세팅 기능
void init_game() {
    player_r = ROWS - 2; // 플레이어 세로 위치를 화면 맨 아래쪽으로 설정
    player_c = COLS / 2; // 플레이어 가로 위치를 화면 정중앙으로 설정
    player_hp = player_max_hp; // 플레이어 체력 최대치로 회복
    titan_hp = titan_max_hp; // 보스 체력 최대치로 회복

    for (int i = 0; i < 50; i++) bullets[i].active = 0; // 잔여 총알 모두 비활성화
    for (int i = 0; i < 10; i++) orbs[i].active = 0; // 잔여 유도탄 모두 비활성화
    for (int i = 0; i < 5; i++) minions[i].active = 0; // 잔여 소환수 모두 비활성화

    clear_attacks(); // 잔여 공격 패턴 삭제
    tick_count = 0; // 인게임 누적 프레임 시계 초기화

    phase = 1; // 보스 상태 기본 단계로 초기화
    has_done_ritual = 0; has_done_berserk = 0; // 특수 기믹 발동 여부 초기화
    shake_offset = 0; // 지진 진동 효과 초기화
}

// 플레이어 비동기 조작 입력 처리 기능
void handle_input() {
    if (!_kbhit()) return; // 사용자가 키보드를 누르지 않았으면 코드 멈추지 않고 즉시 함수 탈출
    int key = _getch(); // 누른 키 값 확인

    if (key == 0 || key == 224) { // 방향키 입력 시 발생하는 확장 키코드 확인
        key = _getch(); // 실제 방향키 값 추가 확인
        if (key == 72 && player_r > titan_height) player_r--; // 위 방향키 입력 시 상단 경계선 검사 후 위로 1칸 이동
        if (key == 80 && player_r < ROWS - 1) player_r++;     // 아래 방향키 입력 시 하단 경계선 검사 후 아래로 1칸 이동
        if (key == 75 && player_c > 0) player_c--;            // 왼쪽 방향키 입력 시 좌측 경계선 검사 후 왼쪽으로 1칸 이동
        if (key == 77 && player_c < COLS - 1) player_c++;     // 오른쪽 방향키 입력 시 우측 경계선 검사 후 오른쪽으로 1칸 이동
        return; // 방향키 처리가 끝났으므로 함수 탈출
    }

    if (key == ' ') { // 스페이스바 입력 확인
        for (int i = 0; i < 50; i++) { // 장전된 50발의 총알 모두 검사
            if (!bullets[i].active) { // 사용 중이지 않은 빈 총알 발견
                bullets[i].active = 1; // 해당 총알 활성화 상태로 변경
                bullets[i].r = player_r - 1; // 총알 생성 위치를 플레이어 머리 위로 설정
                bullets[i].c = player_c; // 총알 생성 위치를 플레이어 가로 위치와 일치
                break; // 한 번에 한 발만 쏘기 위해 반복문 탈출
            }
        }
    }
}

// 투사체 이동 및 충돌 판정 등 핵심 연산 기능
void update_logic() {
    tick_count++; // 전체 누적 프레임 1 증가
    if (hit_flash_timer > 0) hit_flash_timer--; // 피격 섬광 효과 잔여 시간 1 감소

    // 체력 도달에 따른 특수 기믹 단계 진입 판정
    if (phase == 1 || phase == 5) { // 전투 진행 중일 때만 체력 확인
        if (titan_hp <= titan_max_hp * 0.3 && !has_done_berserk) { // 체력이 30프로 이하이고 광폭화를 안 거쳤다면
            has_done_berserk = 1; // 광폭화 진행 완료로 체크
            phase = 4; // 광폭화 연출 상태로 전환
            enrage_end_time = time(NULL) + 3; // 현실 시계 기준으로 3초 동안 지진 연출 예약

            clear_attacks(); // 억울한 피격 방지를 위해 남아있는 공격 삭제
        }
        else if (titan_hp <= titan_max_hp * 0.6 && !has_done_ritual) { // 체력이 60프로 이하이고 소환 기믹을 안 거쳤다면
            has_done_ritual = 1; // 소환 기믹 진행 완료로 체크
            phase = 2; // 무적 및 소환 기믹 상태로 전환
            ritual_end_time = time(NULL) + 10; // 현실 시계 기준으로 정확히 10초 뒤 기믹 종료 예약

            clear_attacks(); // 남아있는 공격 삭제

            for (int i = 0; i < 5; i++) { // 소환수 5기 생성
                minions[i].active = 1; // 소환수 활성화
                minions[i].r = titan_height + 2 + (rand() % 4); // 보스 아래 공간에 세로 위치 랜덤 배치
                minions[i].c = 10 + (i * 8); // 서로 겹치지 않게 가로 위치 간격 배치
                minions[i].dir = (rand() % 2 == 0) ? 1 : -1; // 좌우 이동 방향 랜덤 결정
            }
        }
    }

    // 단계별 특수 규칙 적용 영역
    if (phase == 2) { // 소환 타임어택 진행 중일 경우
        ritual_time_left = ritual_end_time - time(NULL); // 예약된 종료 시간에서 현재 시간을 빼서 남은 초 계산
        int minions_alive = 0; // 살아있는 소환수 숫자 기억할 변수 생성

        if (tick_count % 3 == 0) { // 소환수 이동 속도를 늦추기 위해 3프레임마다 한 번씩 이동
            for (int i = 0; i < 5; i++) { // 소환수 5기 순회
                if (minions[i].active) { // 소환수가 살아있다면
                    minions_alive++; // 살아있는 소환수 숫자 1 증가
                    minions[i].c += minions[i].dir; // 가로 방향으로 1칸 이동

                    if (minions[i].c < 2 || minions[i].c >= COLS - 2) minions[i].dir *= -1; // 맵 좌우 경계선에 닿으면 이동 방향 반대로 전환
                }
            }
        }

        if (ritual_time_left <= 0) { // 제한 시간이 0초 이하로 떨어졌을 경우
            if (minions_alive > 0) { // 소환수가 한 마리라도 살아있으면 기믹 실패
                titan_hp = (int)(titan_max_hp * 0.8); // 보스 체력을 80프로로 대폭 회복
                phase = 1; // 일반 전투 상태로 강제 복귀
                for (int i = 0; i < 5; i++) minions[i].active = 0; // 화면에 남은 소환수 일괄 삭제
            }
        }
        else { // 아직 제한 시간이 남았을 경우
            int check_alive = 0; // 검사용 변수 생성
            for (int i = 0; i < 5; i++) if (minions[i].active) check_alive++; // 활성화된 소환수 숫자 세기

            if (check_alive == 0) { // 살아있는 소환수가 0명이면 기믹 성공
                phase = 3; // 보스 기절 상태로 전환
                stun_end_time = time(NULL) + 4; // 현실 시계 기준으로 4초 동안 기절 예약
                clear_attacks(); // 공격 초기화
            }
        }
    }
    else if (phase == 3) { // 보스 기절 상태 진행 중일 경우
        if (time(NULL) >= stun_end_time) phase = 1; // 예약된 기절 시간이 끝났으면 일반 전투 상태로 깸
    }
    else if (phase == 4) { // 광폭화 변신 연출 진행 중일 경우
        shake_offset = (tick_count % 4 < 2) ? -2 : 2; // 화면 전체를 좌우 2칸씩 흔들어 지진 효과 구현

        if (time(NULL) >= enrage_end_time) { // 변신 연출 시간이 끝났으면
            phase = 5; // 진짜 광폭화 전투 상태로 돌입
            shake_offset = 0; // 지진 효과 멈춤
        }
    }

    // 플레이어 총알 이동 및 적 타격 판정 영역
    for (int i = 0; i < 50; i++) { // 총알 50발 순회
        if (bullets[i].active) { // 날아가는 총알 발견
            bullets[i].r--; // 위로 한 칸 전진

            if (bullets[i].r < 0) { bullets[i].active = 0; continue; } // 화면 밖으로 나갔으면 삭제 후 다음 총알 검사

            if (phase != 2 && phase != 4 && bullets[i].r < titan_height) { // 보스가 타격 가능한 상태고 총알이 보스 높이까지 도달했으면
                int start_c = (COLS - titan_width) / 2 + shake_offset; // 지진 오차까지 계산하여 보스 왼쪽 경계선 파악
                int end_c = start_c + titan_width; // 보스 오른쪽 경계선 파악

                if (bullets[i].c >= start_c && bullets[i].c < end_c) { // 총알 가로 위치가 보스 가로 범위 안에 들어왔으면
                    if (titan_art[bullets[i].r][bullets[i].c - start_c] != ' ') { // 여백이 아니라 진짜 보스 그림에 맞았으면
                        titan_hp--; // 보스 체력 1 삭감
                        bullets[i].active = 0; // 적중한 총알 삭제
                        hit_flash_timer = 4; // 화면 4프레임 동안 하얗게 번쩍임 예약
                        continue; // 해당 총알 검사 종료
                    }
                }
            }

            if (phase == 2) { // 기믹 소환수 타격 가능 상태일 경우
                for (int m = 0; m < 5; m++) { // 소환수 전체 순회
                    if (minions[m].active && bullets[i].r == minions[m].r && bullets[i].c == minions[m].c) { // 총알 위치와 소환수 위치가 정확히 일치하면
                        minions[m].active = 0; // 맞은 소환수 삭제
                        bullets[i].active = 0; // 적중한 총알 삭제
                    }
                }
            }

            if (diff_level == 3) { // 유도탄 등장하는 최고 난이도일 경우
                for (int o = 0; o < 10; o++) { // 유도탄 전체 순회
                    if (orbs[o].active && bullets[i].r == orbs[o].r && bullets[i].c == orbs[o].c) { // 내 총알 위치와 유도탄 위치가 정확히 일치하면
                        bullets[i].active = 0; orbs[o].active = 0; // 내 총알과 유도탄 동반 삭제 처리
                    }
                }
            }
        }
    }

    // 보스 지능 및 공격 쿨타임 계산 영역
    if (phase != 3 && phase != 4) { // 기절 상태나 연출 상태가 아닐 때만 공격 실행
        int hp_percent = (titan_hp * 100) / titan_max_hp; // 보스 현재 체력을 백분율로 환산
        if (hp_percent < 0) hp_percent = 0; // 체력 비율이 음수가 되는 것 방지

        // 체력이 낮을수록 뒷부분 수식이 작아져 전체 대기 시간이 훅 줄어드는 동적 난이도 연산
        int laser_interval = 20 + (30 * hp_percent / 100); // 레이저 대기 시간 설정
        int wave_interval = 60 + (60 * hp_percent / 100); // 파동 대기 시간 설정
        int orb_interval = 30 + (40 * hp_percent / 100); // 유도탄 대기 시간 설정

        if (phase == 5) { // 광폭화 돌입 시 동적 난이도 무시하고 최저치 강제 고정
            laser_interval = 15; wave_interval = 50; orb_interval = 20; // 쿨타임 극한으로 단축
        }

        laser_timer++; // 레이저 발사 시계 1 증가
        if (laser_timer >= laser_interval) { // 레이저 쿨타임이 꽉 찼으면
            laser_timer = 0; // 레이저 시계 초기화
            int attack_count = (phase == 5) ? 6 : (3 + (100 - hp_percent) / 20); // 광폭화 시 6가닥 고정 발사, 평시엔 잃은 체력 비례 발사량 증가

            for (int i = 0; i < attack_count; i++) { // 발사 횟수만큼 반복
                int target_c = (i == 0) ? player_c : (rand() % COLS); // 첫 발은 플레이어 조준, 나머지는 무작위 조준
                laser_warning[target_c] = 20; // 지정된 바닥 위치에 20프레임 동안 레이저 경고 발령
            }
        }

        for (int c = 0; c < COLS; c++) { // 바닥 가로축 전체 순회
            if (laser_warning[c] > 0) { // 레이저 경고가 깔려있으면
                laser_warning[c]--; // 경고 잔여 시간 1 감소
                if (laser_warning[c] == 0) laser_firing[c] = 10; // 경고 시간 끝나면 실제 타격 레이저 10프레임 동안 생성
            }
            if (laser_firing[c] > 0) { // 실제 타격 레이저가 쏟아지는 중이면
                laser_firing[c]--; // 타격 잔여 시간 1 감소
                if (player_c == c) { player_hp--; laser_firing[c] = 0; } // 레이저 위치와 플레이어 위치가 일치하면 플레이어 체력 1 삭감 후 해당 레이저 삭제
            }
        }

        // 보통 난이도 이상부터 추가되는 공간 파동 패턴 제어 로직
        if (diff_level >= 2) {
            if (wave_active) { // 파동이 내려오는 중이면
                if (tick_count % 4 == 0) { // 파동 이동 속도 조절을 위해 4프레임마다 연산 실행
                    wave_r++; // 파동을 아래로 1칸 이동시킴
                    if (wave_r >= ROWS) wave_active = 0; // 화면 바닥을 뚫고 나가면 파동 삭제 처리함

                    // 파동 세로 위치와 플레이어 세로 위치가 똑같은데 플레이어가 구멍 범위 밖에 있으면 피격 판정 수행
                    if (wave_active && wave_r == player_r && (player_c < wave_hole_c || player_c > wave_hole_c + 6)) {
                        player_hp--; // 플레이어 체력 1 삭감 처리
                    }
                }
            }
            else { // 파동 공격이 대기 중이면
                wave_timer++; // 파동 재생성 타이머 1 증가
                if (wave_timer >= wave_interval) { // 파동 쿨타임 꽉 찼으면
                    wave_timer = 0; wave_active = 1; wave_r = titan_height; // 파동 활성화 후 보스 바로 아래 세로줄로 위치 지정
                    wave_hole_c = rand() % (COLS - 8) + 1; // 플레이어가 생존할 수 있는 6칸 짜리 여백 구멍 가로 위치 무작위 설정
                }
            }
        }

        if (diff_level >= 3) { // 어려움 난이도부터 유도탄 공격 추가
            orb_timer++; // 유도탄 시계 1 증가
            if (orb_timer >= orb_interval) { // 유도탄 쿨타임 꽉 찼으면
                orb_timer = 0; // 유도탄 시계 초기화
                for (int i = 0; i < 10; i++) { // 유도탄 배열 전체 순회
                    if (!orbs[i].active) { // 사용 대기 중인 빈 유도탄 발견
                        orbs[i].active = 1; orbs[i].r = titan_height; // 보스 아래쪽으로 세로 위치 설정
                        orbs[i].c = (COLS - titan_width) / 2 + rand() % titan_width; // 보스 가로 넓이 안쪽으로 가로 위치 무작위 설정
                        break; // 1발만 생성하고 반복문 탈출
                    }
                }
            }
            if (tick_count % 2 == 0) { // 유도탄 이동 속도 조절
                for (int i = 0; i < 10; i++) { // 화면 내 유도탄 전체 순회
                    if (orbs[i].active) { // 날아다니는 유도탄 발견
                        if (orbs[i].r < player_r) orbs[i].r++; else if (orbs[i].r > player_r) orbs[i].r--; // 플레이어 세로 위치 쫓아가게 방향 설정
                        if (orbs[i].c < player_c) orbs[i].c++; else if (orbs[i].c > player_c) orbs[i].c--; // 플레이어 가로 위치 쫓아가게 방향 설정

                        if (orbs[i].r == player_r && orbs[i].c == player_c) { // 플레이어 좌표와 일치하면
                            player_hp--; orbs[i].active = 0; // 플레이어 체력 1 삭감 후 폭발 삭제
                        }
                    }
                }
            }
        }
    }
}

// 연산 끝난 데이터들을 보이지 않는 도화지에 그리는 기능
void build_frame() {
    clear_buffers(); // 지난 프레임 그림 백지화

    for (int r = 0; r < ROWS; r++) { // 세로줄 전체 순회
        for (int c = 0; c < COLS; c++) { // 가로줄 전체 순회
            if (rand() % 50 == 0) { map[r][c] = '.'; color_map[r][c] = C_STAR; } // 확률적으로 배경 별빛 도장 찍기
        }
    }

    int start_c = ((COLS - titan_width) / 2) + shake_offset; // 지진 오차값 반영하여 보스 그림 시작 가로 위치 지정
    int titan_color = C_BOSS; // 보스 본체 기본 빨간색 설정

    if (phase == 2) titan_color = C_STONE; // 무적 상태일 경우 회색 돌덩이 색상으로 덮어쓰기
    else if (phase == 4 || phase == 5) { // 광폭화 상태일 경우
        titan_color = (tick_count % 8 < 4) ? C_BOSS : C_WARNING; // 시간에 따라 빨간색과 노란색 번갈아가며 사이렌 점멸 설정
    }
    if (hit_flash_timer > 0 && phase != 2 && phase != 4) titan_color = C_BOSS_HIT; // 피격 타이머 돌아가는 중이면 하얀색으로 덮어쓰기

    for (int r = 0; r < titan_height; r++) { // 보스 세로 크기만큼 반복
        for (int c = 0; c < titan_width; c++) { // 보스 가로 크기만큼 반복
            if (titan_art[r][c] != ' ' && (start_c + c >= 0 && start_c + c < COLS)) { // 아스키아트 여백 부분이 아니면
                map[r][start_c + c] = titan_art[r][c]; // 도화지에 보스 글자 찍기
                color_map[r][start_c + c] = titan_color; // 색상표에 보스 상태 색상 찍기
            }
        }
    }

    if (phase == 2) { // 60프로 소환수 기믹 중이면
        for (int i = 0; i < 5; i++) { // 소환수 전체 순회
            if (minions[i].active) { // 살아있으면
                map[minions[i].r][minions[i].c] = 'W'; // 해당 좌표에 몬스터 형태 도장 찍기
                color_map[minions[i].r][minions[i].c] = C_MINION; // 자주색 물감 설정
            }
        }
    }

    if (wave_active) { // 파동 내려오는 중이면
        for (int c = 0; c < COLS; c++) { // 가로축 전체 순회
            if (c < wave_hole_c || c > wave_hole_c + 6) { // 플레이어 피하는 구멍 영역만 제외하고
                map[wave_r][c] = '~'; color_map[wave_r][c] = C_WAVE; // 파동 모양 파란색 도장 찍기
            }
        }
    }

    for (int c = 0; c < COLS; c++) { // 가로축 전체 순회
        if (laser_warning[c] > 0) { // 해당 세로줄에 경고 있으면
            for (int r = titan_height; r < ROWS; r++) { // 보스 바닥부터 화면 맨 아래까지 세로로 순회
                if (r % 2 == (tick_count / 4) % 2) { // 시간에 따라 점선 모양 썼다 지웠다 반복 설정
                    map[r][c] = '!'; color_map[r][c] = C_WARNING; // 느낌표 모양 노란색 도장 찍기
                }
            }
        }
        else if (laser_firing[c] > 0) { // 해당 세로줄에 진짜 레이저 타격 있으면
            for (int r = titan_height; r < ROWS; r++) { // 세로로 순회
                map[r][c] = '|'; color_map[r][c] = C_LASER; // 직선 모양 빨간색 도장 찍기
            }
        }
    }

    for (int i = 0; i < 10; i++) { // 유도탄 배열 전체 순회
        if (orbs[i].active) { map[orbs[i].r][orbs[i].c] = '@'; color_map[orbs[i].r][orbs[i].c] = C_ORB; } // 날아다니는 유도탄 모양 도장 찍기
    }

    for (int i = 0; i < 50; i++) { // 총알 배열 전체 순회
        if (bullets[i].active) { map[bullets[i].r][bullets[i].c] = '^'; color_map[bullets[i].r][bullets[i].c] = C_BULLET; } // 내 총알 모양 도장 찍기
    }
    map[player_r][player_c] = 'A'; color_map[player_r][player_c] = C_PLAYER; // 플레이어 기체 모양 최종 도장 찍기
}

// 그려진 도화지 데이터를 지연 없이 콘솔 화면에 즉시 출력하는 기능
void draw_frame() {
    gotoxy(0, 0); // 화면 지우기 대신 출력 커서만 좌측 상단 원점으로 돌려보냄

    set_color(C_DEFAULT); // 하얀색 설정
    printf("============================================================\n"); // 상단 테두리 선 긋기
    printf(" [난이도: %-6s] 거신 HP: ", diff_name); // 난이도 문자열 출력
    set_color(C_BOSS); // 빨간색 설정

    int hp_bar_len = (titan_hp * 35) / titan_max_hp; // 보스 체력을 35칸짜리 막대기로 변환
    for (int i = 0; i < 35; i++) { // 35번 반복
        if (i < hp_bar_len) printf("■"); else printf("-"); // 남은 체력만큼 사각형 출력 후 깎인 부분 작대기 출력
    }
    set_color(C_DEFAULT); // 하얀색 설정
    printf("\n"); // 줄바꿈 실행
    printf(" [신의 권능] HP: "); // 플레이어 체력 이름 출력
    set_color(C_PLAYER); // 청록색 설정
    for (int i = 0; i < player_max_hp; i++) { // 플레이어 최대 체력만큼 반복
        if (i < player_hp) printf("♥ "); else printf("♡ "); // 남은 체력 하트 아이콘 채워서 출력
    }
    printf("                               \n"); // 남은 줄 빈칸으로 밀어버림
    set_color(C_DEFAULT); // 하얀색 설정
    printf("============================================================\n"); // UI 하단 테두리 선 긋기

    int current_color = -1; // 현재 설정된 펜 색상 기억 변수 생성
    for (int r = 0; r < ROWS; r++) { // 세로줄 순회
        for (int c = 0; c < COLS; c++) { // 가로줄 순회
            if (color_map[r][c] != current_color) { // 출력해야 할 글자 색상이 이전 글자 색상과 다를 때만
                set_color(color_map[r][c]); // 윈도우 시스템 호출하여 펜 색상 교체
                current_color = color_map[r][c]; // 교체된 색상 기록 갱신
            }
            printf("%c", map[r][c]); // 도화지에 적힌 글자 하나 모니터에 출력
        }
        printf("\n"); // 한 줄 끝날 때마다 줄바꿈 실행
    }

    if (phase == 2) { // 기믹 진행 중일 경우
        gotoxy(6, 12); set_color(C_WARNING); // 화면 한가운데 노란색 경고 세팅
        printf("[ 심판의 의식 진행 중! 광신도를 모두 파괴하십시오! ]"); // 기믹 텍스트 출력
        gotoxy(23, 14); set_color(C_DEFAULT); // 바로 아래 하얀색 세팅
        if (ritual_time_left < 0) ritual_time_left = 0; // 남은 시간 마이너스 방어 코드
        printf("남은 시간 : %d초", ritual_time_left); // 초시계 타이머 출력
    }
    else if (phase == 3) { // 보스 기절 상태일 경우
        gotoxy(9, 12); set_color(C_PLAYER); // 청록색 세팅
        printf("[ 그로기 상태! 치명적인 피해를 입히세요! ]"); // 프리딜 기회 텍스트 출력
    }
    else if (phase == 4) { // 광폭화 변신 중일 경우
        gotoxy(9, 12); set_color(C_BOSS); // 빨간색 세팅
        printf("보스가 광폭화를 시작합니다!!!"); // 광폭화 경고 텍스트 출력
    }
}

// 보스 처치 시 발생되는 웅장한 연출 및 엔딩 크레딧 기능
void play_ending(void) {
    for (int i = 0; i < 20; i++) { // 연쇄 폭발 20회 반복
        shake_offset = (rand() % 5) - 2; // 화면 무작위로 흔들리게 변위 설정
        clear_attacks(); // 공격 데이터 지우기
        build_frame(); // 배경 및 보스 도화지에 다시 그리기

        int start_c = ((COLS - titan_width) / 2) + shake_offset; // 보스 시작 위치 파악
        for (int r = 0; r < titan_height; r++) { // 보스 세로 크기 순회
            for (int c = 0; c < titan_width; c++) { // 보스 가로 크기 순회
                if (titan_art[r][c] != ' ') { // 여백이 아니라 몸통일 경우
                    int rand_val = rand() % 100; // 랜덤 난수 추출
                    if (rand_val < 20) { // 20퍼센트 확률 적용
                        map[r][start_c + c] = '*'; // 별 모양 찍어서 폭발하는 시각 효과 적용
                        color_map[r][start_c + c] = (rand() % 2 == 0) ? C_WARNING : C_BOSS_HIT; // 노랑 및 흰색 번갈아 터지도록 색상 부여
                    }
                    else if (rand_val < 50) { // 30퍼센트 확률 적용
                        map[r][start_c + c] = ' '; // 빈 공간 뚫어서 몸통 파괴되는 시각 효과 적용
                    }
                }
            }
        }
        draw_frame(); // 폭발 및 붕괴되는 화면 프린트
        Sleep(100); // 폭발마다 0.1초 대기 시간 부여
    }

    for (int r = 0; r < ROWS; r++) { // 화면 전체 배열 순회
        for (int c = 0; c < COLS; c++) { // 화면 전체 배열 순회
            map[r][c] = ' '; // 글자 모두 삭제
            color_map[r][c] = 240; // 전체 배경 하얀색으로 도배하여 시야 멀게 하는 연출 적용
        }
    }
    draw_frame(); // 새하얀 화면 프린트
    Sleep(150); // 번쩍 하고 정지 상태 부여
    system("cls"); // 콘솔 화면 전체 삭제하여 완전한 암전 상태 조성

    const char* credits[] = { // 올라갈 엔딩 크레딧 문자열 배열 등록
        "============================================",
        "          T I T A N O M A C H I A           ",
        "============================================",
        "                                            ",
        "            [ 거신 토벌 성공! ]             ",
        "                                            ",
        "    우주를 위협하던 거신을 파괴했습니다.    ",
        "    빛의 파편은 다시 깊은 잠에 듭니다...    ",
        "                                            ",
        "    - 개발자: 김민준                        ",
        "    - 제작: C언어 프로그래밍                ",
        "                                            ",
        "    플레이해주셔서 감사합니다.              ",
        "============================================",
        "        엔터 키를 누르면 종료               "
    };
    int credit_lines = 15; // 크레딧 총 15줄 선언

    for (int y = ROWS; y >= 6; y--) { // 출력 Y좌표를 화면 밑에서부터 한 칸씩 빼며 위로 올리기 설정
        system("cls"); // 이전 글씨 지우기
        for (int i = 0; i < credit_lines; i++) { // 15줄 전체 순회
            if (y + i >= 0 && y + i < ROWS) { // 글씨 위치가 화면 위아래 밖으로 나가지 않았을 때만
                gotoxy((COLS - 44) / 2, y + i); // 텍스트 화면 중앙 정렬
                set_color(C_PLAYER); // 플레이어 색상 부여
                printf("%s", credits[i]); // 문자열 한 줄씩 출력
            }
        }
        Sleep(150); // 영화 크레딧처럼 천천히 올라가도록 대기 시간 부여
    }

    while (_kbhit()) _getch(); // 조작 과정에서 쌓인 키보드 잔여 데이터 모두 빼내어 삭제
    while (1) { // 무한 대기
        int key = _getch(); // 키보드 입력 대기
        if (key == '\r' || key == '\n') break; // 엔터 키 확인 시 무한 대기 종료
    }
}

// 인게임 진행 및 출력 사이클을 통제하는 메인 모터 기능
int run_game(void) {
    init_game(); // 변수 및 배열 완전 초기화
    system("cls"); // 화면 완전 지우기
    hide_cursor(); // 밑줄 깜빡임 비활성화

    while (1) { // 승패 결정 전까지 게임 무한 동작
        handle_input(); // 방향키 조작 확인
        update_logic(); // 이동 수식 연산 및 상태 판단
        build_frame();  // 안보이는 도화지에 화면 기록
        draw_frame();   // 모니터에 단번에 출력

        if (player_hp <= 0) { // 내 체력 0 이하 소진 판별
            set_color(C_BOSS); // 텍스트 빨간색 설정
            gotoxy(12, ROWS / 2 + 3); // 안내 문자열 중앙 배치
            printf(" >>> 거신의 압도적인 힘에 짓눌려 소멸했습니다... <<< "); // 게임 오버 안내

            gotoxy(14, ROWS / 2 + 5); // 버튼 안내 중앙 배치
            set_color(C_DEFAULT); // 텍스트 흰색 설정
            printf("[ 엔터 키를 눌러 메뉴로 복귀 ]"); // 키보드 조작 안내

            while (_kbhit()) _getch(); // 남아있는 조작 데이터 일괄 삭제
            while (1) { // 무한 대기
                int key = _getch(); // 키보드 입력 대기
                if (key == '\r' || key == '\n') break; // 엔터 키 확인 시 대기 종료
            }
            return 0; // 패배 상태값 0 반환 후 함수 탈출
        }

        if (titan_hp <= 0) { // 보스 체력 0 이하 소진 판별
            play_ending(); // 폭발 및 크레딧 출력 함수 호출
            return 1; // 승리 상태값 1 반환 후 함수 탈출
        }

        Sleep(FRAME_DELAY); // 지정된 16밀리초 시간 대기하여 화면 초당 60프레임 속도 동기화
    }
}

// C언어 프로그램 최초 시작 진입점
int main(void) {
    srand((unsigned int)time(NULL)); // 난수 시작점 설정
    system("mode con: cols=62 lines=32"); // 게임 창 크기 내 맘대로 고정시켜버림

    while (1) { // 끄기 전까지 메인 화면으로 돌아가는 무한 반복
        show_title(); // 대문 보여주기
        show_difficulty(); // 난이도 선택 메뉴 보여주기
        run_game(); // 본격적인 게임 연산 함수 호출
    }
    return 0; // 프로그램 완전 종료
}