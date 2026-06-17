#define _CRT_SECURE_NO_WARNINGS 
#include <stdio.h> 
#include <stdlib.h> 
#include <time.h> 
#include <windows.h> 
#include <conio.h> 
#include <string.h> 

/*
  [게임 환경 및 객체 색상 환경 변수]
  화면의 크기와 각 객체들이 출력될 고유 색상표를 상수로 고정합니다.
  초당 60프레임 구동을 위해 16밀리초의 지연 시간을 설정합니다.
 */
#define ROWS 25 
#define COLS 60 
#define C_DEFAULT   15 
#define C_PLAYER    11 
#define C_BULLET    11 
#define C_BOSS      12 
#define C_BOSS_HIT  240 
#define C_WARNING   14 
#define C_LASER     12 
#define C_WAVE      9 
#define C_ORB       13 
#define C_STAR      8 
#define C_STONE     8 
#define C_MINION    13 
#define FRAME_DELAY 16 

 /*
   [통합 객체 관리 구조체]
   총알, 유도탄, 소환수 등 화면을 돌아다니는 수많은 개체들을 하나하나 변수로 만들지 않고,
   세로 좌표, 가로 좌표, 화면 존재 유무, 이동 방향을 하나의 묶음으로 통합 관리합니다.
  */
typedef struct {
    int r;
    int c;
    int active;
    int dir;
} Entity;

/*
  [화면 렌더링 버퍼 및 전역 상태 변수]
  map과 color_map은 눈에 보이지 않는 도화지 역할을 하며 렌더링 최적화에 사용됩니다.
  플레이어와 보스의 좌표, 체력, 패턴 타이머, 위상 상태 등을 전역 변수로 저장하여
  모든 함수에서 데이터를 공유하고 조작할 수 있도록 설계했습니다.
 */
char map[ROWS][COLS];
int color_map[ROWS][COLS];

int player_r, player_c;
int player_hp, player_max_hp;

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
int titan_width = 56;
int titan_height = 8;
int titan_hp, titan_max_hp;
int hit_flash_timer = 0;
int shake_offset = 0;

Entity bullets[50];
Entity orbs[10];
Entity minions[5];

int diff_level;
char diff_name[10];

int laser_warning[COLS];
int laser_firing[COLS];

int laser_timer = 0, wave_timer = 0, orb_timer = 0;
int wave_active = 0;
int wave_r = 0;
int wave_hole_c = 0;
int tick_count = 0;

int phase = 1;
time_t ritual_end_time = 0;
int ritual_time_left = 0;
time_t stun_end_time = 0;
time_t enrage_end_time = 0;
int has_done_ritual = 0;
int has_done_berserk = 0;

/*
  [프로그램 핵심 기능 및 함수 선언부]
  게임 구동에 필요한 모든 서브루틴의 원형을 미리 선언합니다.
  기능별 모듈화를 통해 구조적 설계 수준과 가독성을 극대화했습니다.
 */

 /* 1. 시스템 프로그래밍 및 화면 제어 모듈 */
void set_color(int color_code); // 윈도우 콘솔 텍스트 색상 속성 변경
void gotoxy(int x, int y);      // 콘솔 출력 커서의 특정 좌표 순간 이동
void hide_cursor(void);         // 화면 내 텍스트 입력 커서 깜빡임 비활성화

/* 2. 메모리 및 데이터 풀 초기화 모듈 */
void clear_buffers(void);       // 2차원 도화지 배열 버퍼 공간 백지화
void clear_attacks(void);       // 패턴 전환 시 기존 잔존 탄막 데이터 일괄 소거
void init_game(void);           // 새로운 판 진입 시 플레이 데이터 완전 리셋

/* 3. 사용자 인터페이스 및 메뉴 세팅 모듈 */
void show_title(void);          // 메인 대문 타이틀 화면 출력
void show_help(void);           // 규칙 분석 및 상세 조작법 안내서 출력
void show_difficulty(void);     // 세 가지 난이도 분기 및 수치 변수 세팅

/* 4. 동적 프레임 제어 및 실시간 엔진 모듈 (초당 60회 반복) */
void handle_input(void);        // 비동기 키보드 조작 입력 실시간 감지
void update_logic(void);        // 물리 연산, 충돌 판정 및 보스 패턴 갱신
void build_frame(void);         // 연산 완료된 상식을 도화지 배열에 배치
void draw_frame(void);          // 도화지 데이터를 콘솔 창에 최적화하여 출력
int  run_game(void);            // 위 프레임 사이클을 총괄 통제하는 핵심 모터

/* 5. 보스 파괴 연출 및 결과 모듈 */
void play_ending(void);         // 토벌 성공 시 파티클 폭발 및 크레딧 스크롤 출력

/*
  [시스템 제어 유틸리티 영역]
  윈도우 운영체제의 기능들을 호출하여 콘솔의 텍스트 색상을 바꾸거나,
  출력 커서의 위치를 옮기고, 깜빡이는 입력 커서를 숨기는 역할을 담당합니다.
 */
void set_color(int color_code) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color_code);
}
void gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}
void hide_cursor(void) {
    CONSOLE_CURSOR_INFO cci = { 1, FALSE };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cci);
}

/*
  [데이터 초기화 제어 영역]
  매 프레임마다 이전 그림을 지우고 새 그림을 그리기 위해 도화지를 하얗게 지우거나
  기믹이 전환될 때 억울한 피격을 막고자 맵에 남아있는 모든 공격 판정을 단번에 소거합니다.
 */
void clear_buffers() {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            map[r][c] = ' ';
            color_map[r][c] = C_DEFAULT;
        }
    }
}
void clear_attacks(void) {
    memset(laser_warning, 0, sizeof(laser_warning));
    memset(laser_firing, 0, sizeof(laser_firing));
    wave_active = 0;
    for (int o = 0; o < 10; o++) orbs[o].active = 0;
    laser_timer = 0; wave_timer = 0; orb_timer = 0;
}

/*
  [사용자 인터페이스 출력 영역]
  게임 시작 전 메인 타이틀, 조작법, 난이도 선택 화면을 렌더링하고
  플레이어의 키보드 입력을 받아 해당 난이도의 세부 수치를 전역 변수에 세팅합니다.
 */
void show_title(void) {
    while (1) {
        system("cls");
        set_color(C_DEFAULT);
        printf("\n\n");
        printf("   ====================================================\n");
        printf("           T I T A N O M A C H I A  :  거신 토벌전       \n");
        printf("   ====================================================\n\n");
        printf("                    1. 게임 시작\n");
        printf("                    2. 게임 설명\n");
        printf("                    3. 게임 종료\n\n");
        printf("   번호를 입력하세요: ");

        char choice = _getch();
        if (choice == '1') return;
        else if (choice == '2') show_help();
        else if (choice == '3') exit(0);
    }
}
void show_help(void) {
    system("cls");
    set_color(C_DEFAULT);
    printf("\n   [ 게임 설명 ]\n");
    printf("   거신 티탄의 무수한 공격을 피하며 쓰러뜨리십시오!\n");
    printf("   ※ 주의: 보스의 체력에 따라 특수 기믹과 광폭화가 발동됩니다.\n\n");
    printf("   [ 조작법 ]\n");
    printf("   방향키 상하좌우 : 플레이어 이동\n");
    printf("   스페이스바 : 공격 발사\n\n");
    printf("   [ 특수 페이즈 안내 ]\n");
    printf("   - 체력 60프로 도달: 보스가 무적이 되며 광신도를 소환합니다.\n");
    printf("     제한 시간 내에 처치하지 못하면 보스가 체력을 대폭 회복합니다.\n");
    printf("   - 체력 30프로 도달: 보스가 광폭화하여 공격 주기가 극단적으로 감소합니다.\n\n");
    printf("   아무 키나 누르면 메인 메뉴로 돌아갑니다...\n");
    (void)_getch();
}
void show_difficulty(void) {
    while (1) {
        system("cls");
        set_color(C_DEFAULT);
        printf("\n   [ 난이도 선택 ]\n\n");
        printf("   1. 쉬움   (내 체력 5 보스 체력 150) - 기본 레이저 공격\n");
        printf("   2. 보통   (내 체력 4 보스 체력 300) - 레이저와 파동 결합\n");
        printf("   3. 어려움 (내 체력 3 보스 체력 500) - 레이저와 파동과 유도탄 결합\n\n");
        printf("   난이도 번호를 선택하세요: ");

        char choice = _getch();
        if (choice == '1') { diff_level = 1; player_max_hp = 5; titan_max_hp = 150; sprintf(diff_name, "Easy"); return; }
        if (choice == '2') { diff_level = 2; player_max_hp = 4; titan_max_hp = 300; sprintf(diff_name, "Normal"); return; }
        if (choice == '3') { diff_level = 3; player_max_hp = 3; titan_max_hp = 500; sprintf(diff_name, "Hard"); return; }
    }
}

/*
  [인게임 진입 및 조작 제어 영역]
  전투가 시작될 때 변수들을 초기 상태로 되돌리고
  플레이어의 키보드 입력을 비동기적으로 감지하여 방향키 이동과 스페이스바 공격을 처리합니다.
 */
void init_game() {
    player_r = ROWS - 2;
    player_c = COLS / 2;
    player_hp = player_max_hp;
    titan_hp = titan_max_hp;

    for (int i = 0; i < 50; i++) bullets[i].active = 0;
    for (int i = 0; i < 10; i++) orbs[i].active = 0;
    for (int i = 0; i < 5; i++) minions[i].active = 0;

    clear_attacks();
    tick_count = 0;
    phase = 1;
    has_done_ritual = 0; has_done_berserk = 0;
    shake_offset = 0;
}
void handle_input() {
    if (!_kbhit()) return;
    int key = _getch();

    if (key == 0 || key == 224) {
        key = _getch();
        if (key == 72 && player_r > titan_height) player_r--;
        if (key == 80 && player_r < ROWS - 1) player_r++;
        if (key == 75 && player_c > 0) player_c--;
        if (key == 77 && player_c < COLS - 1) player_c++;
        return;
    }

    if (key == ' ') {
        for (int i = 0; i < 50; i++) {
            if (!bullets[i].active) {
                bullets[i].active = 1;
                bullets[i].r = player_r - 1;
                bullets[i].c = player_c;
                break;
            }
        }
    }
}

/*
  [핵심 물리 연산 및 패턴 제어 엔진]
  게임 내 모든 객체의 이동, 정밀 충돌 판정, 보스의 인공지능 패턴을 연산합니다.
  매 프레임(초당 60회)마다 호출되어 화면을 그리기 전 전체 논리 상태를 결정합니다.
 */
void update_logic() {
    tick_count++;
    if (hit_flash_timer > 0) hit_flash_timer--;

    /*
      [1. 체력 비례 특수 기믹 발동 판별]
      보스의 체력이 특정 비율(60프로, 30프로) 이하로 떨어졌을 때 검사하여,
      기존 패턴을 정지시키고 무적 소환 기믹이나 광폭화 지진 연출로 상태를 전환합니다.
     */
    if (phase == 1 || phase == 5) {
        if (titan_hp <= titan_max_hp * 0.3 && !has_done_berserk) {
            has_done_berserk = 1;
            phase = 4;
            enrage_end_time = time(NULL) + 3;
            clear_attacks();
        }
        else if (titan_hp <= titan_max_hp * 0.6 && !has_done_ritual) {
            has_done_ritual = 1;
            phase = 2;
            ritual_end_time = time(NULL) + 10;
            clear_attacks();

            for (int i = 0; i < 5; i++) {
                minions[i].active = 1;
                minions[i].r = titan_height + 2 + (rand() % 4);
                minions[i].c = 10 + (i * 8);
                minions[i].dir = (rand() % 2 == 0) ? 1 : -1;
            }
        }
    }

    /*
      [2. 위상별 특수 규칙 적용]
      소환 타임어택: 10초 제한 시간 검사 및 소환수 이동 로직 처리 실패 시 패널티 부여
      그로기: 제한 시간이 끝났을 때 일반 전투로 복귀
      변신 연출: 3초 동안 화면 흔들림 효과 부여 후 진짜 광폭화 전투 돌입
     */
    if (phase == 2) {
        ritual_time_left = ritual_end_time - time(NULL);
        int minions_alive = 0;

        if (tick_count % 3 == 0) {
            for (int i = 0; i < 5; i++) {
                if (minions[i].active) {
                    minions_alive++;
                    minions[i].c += minions[i].dir;
                    if (minions[i].c < 2 || minions[i].c >= COLS - 2) minions[i].dir *= -1;
                }
            }
        }

        if (ritual_time_left <= 0) {
            if (minions_alive > 0) {
                titan_hp = (int)(titan_max_hp * 0.8);
                phase = 1;
                for (int i = 0; i < 5; i++) minions[i].active = 0;
            }
        }
        else {
            int check_alive = 0;
            for (int i = 0; i < 5; i++) if (minions[i].active) check_alive++;

            if (check_alive == 0) {
                phase = 3;
                stun_end_time = time(NULL) + 4;
                clear_attacks();
            }
        }
    }
    else if (phase == 3) {
        if (time(NULL) >= stun_end_time) phase = 1;
    }
    else if (phase == 4) {
        shake_offset = (tick_count % 4 < 2) ? -2 : 2;

        if (time(NULL) >= enrage_end_time) {
            phase = 5;
            shake_offset = 0;
        }
    }

    /*
      [3. 정밀 물리 충돌 판정 제어]
      플레이어의 총알을 전진시키고, 보스의 가로/세로 영역에 도달했을 때
      배열의 빈 여백이 아닌 실제 그림 표면에 닿았는지를 정밀하게 판별하여 체력을 차감합니다.
      또한 최상위 난이도에서는 유도탄을 총알로 요격할 수 있는 시스템을 적용합니다.
     */
    for (int i = 0; i < 50; i++) {
        if (bullets[i].active) {
            bullets[i].r--;

            if (bullets[i].r < 0) { bullets[i].active = 0; continue; }

            if (phase != 2 && phase != 4 && bullets[i].r < titan_height) {
                int start_c = (COLS - titan_width) / 2 + shake_offset;
                int end_c = start_c + titan_width;

                if (bullets[i].c >= start_c && bullets[i].c < end_c) {
                    if (titan_art[bullets[i].r][bullets[i].c - start_c] != ' ') {
                        titan_hp--;
                        bullets[i].active = 0;
                        hit_flash_timer = 4;
                        continue;
                    }
                }
            }

            if (phase == 2) {
                for (int m = 0; m < 5; m++) {
                    if (minions[m].active && bullets[i].r == minions[m].r && bullets[i].c == minions[m].c) {
                        minions[m].active = 0;
                        bullets[i].active = 0;
                    }
                }
            }

            if (diff_level == 3) {
                for (int o = 0; o < 10; o++) {
                    if (orbs[o].active && bullets[i].r == orbs[o].r && bullets[i].c == orbs[o].c) {
                        bullets[i].active = 0; orbs[o].active = 0;
                    }
                }
            }
        }
    }

    /*
      [4. 동적 인공지능 패턴 설계]
      보스의 남은 체력 비율을 사칙연산으로 계산하여 공격 주기가 자동으로 짧아지도록 만듭니다.
      광폭화 상태에서는 동적 공식을 무시하고 모든 공격 쿨타임을 최저 수치로 강제 고정합니다.
     */
    if (phase != 3 && phase != 4) {
        int hp_percent = (titan_hp * 100) / titan_max_hp;
        if (hp_percent < 0) hp_percent = 0;

        int laser_interval = 20 + (30 * hp_percent / 100);
        int wave_interval = 60 + (60 * hp_percent / 100);
        int orb_interval = 30 + (40 * hp_percent / 100);

        if (phase == 5) {
            laser_interval = 15; wave_interval = 50; orb_interval = 20;
        }

        /* [패턴 A: 지면 레이저 타격] */
        laser_timer++;
        if (laser_timer >= laser_interval) {
            laser_timer = 0;
            int attack_count = (phase == 5) ? 6 : (3 + (100 - hp_percent) / 20);

            for (int i = 0; i < attack_count; i++) {
                int target_c = (i == 0) ? player_c : (rand() % COLS);
                laser_warning[target_c] = 20;
            }
        }
        for (int c = 0; c < COLS; c++) {
            if (laser_warning[c] > 0) {
                laser_warning[c]--;
                if (laser_warning[c] == 0) laser_firing[c] = 10;
            }
            if (laser_firing[c] > 0) {
                laser_firing[c]--;
                if (player_c == c) { player_hp--; laser_firing[c] = 0; }
            }
        }

        /* [패턴 B: 무작위 안전구역 거대 공간 파동] */
        if (diff_level >= 2) {
            if (wave_active) {
                if (tick_count % 4 == 0) {
                    wave_r++;
                    if (wave_r >= ROWS) wave_active = 0;

                    if (wave_active && wave_r == player_r && (player_c < wave_hole_c || player_c > wave_hole_c + 6)) {
                        player_hp--;
                    }
                }
            }
            else {
                wave_timer++;
                if (wave_timer >= wave_interval) {
                    wave_timer = 0; wave_active = 1; wave_r = titan_height;
                    wave_hole_c = rand() % (COLS - 8) + 1;
                }
            }
        }

        /* [패턴 C: 플레이어 위치 추적 유도탄] */
        if (diff_level >= 3) {
            orb_timer++;
            if (orb_timer >= orb_interval) {
                orb_timer = 0;
                for (int i = 0; i < 10; i++) {
                    if (!orbs[i].active) {
                        orbs[i].active = 1; orbs[i].r = titan_height;
                        orbs[i].c = (COLS - titan_width) / 2 + rand() % titan_width;
                        break;
                    }
                }
            }
            if (tick_count % 2 == 0) {
                for (int i = 0; i < 10; i++) {
                    if (orbs[i].active) {
                        if (orbs[i].r < player_r) orbs[i].r++; else if (orbs[i].r > player_r) orbs[i].r--;
                        if (orbs[i].c < player_c) orbs[i].c++; else if (orbs[i].c > player_c) orbs[i].c--;

                        if (orbs[i].r == player_r && orbs[i].c == player_c) {
                            player_hp--; orbs[i].active = 0;
                        }
                    }
                }
            }
        }
    }
}

/*
  [화면 렌더링 버퍼 오프스크린 연산 영역]
  직접 콘솔에 글씨를 찍는 대신 2차원 배열로 만든 도화지에 미리 모든 기호와 색상을 그려둡니다.
  이 방식을 통해 콘솔 창에서 여러 번 텍스트를 출력할 때 생기는 찢어짐 현상을 방지합니다.
 */
void build_frame() {
    clear_buffers();

    // 우주 배경 별빛 및 보스 본체 도장 매핑
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (rand() % 50 == 0) { map[r][c] = '.'; color_map[r][c] = C_STAR; }
        }
    }

    int start_c = ((COLS - titan_width) / 2) + shake_offset;
    int titan_color = C_BOSS;

    if (phase == 2) titan_color = C_STONE;
    else if (phase == 4 || phase == 5) {
        titan_color = (tick_count % 8 < 4) ? C_BOSS : C_WARNING;
    }
    if (hit_flash_timer > 0 && phase != 2 && phase != 4) titan_color = C_BOSS_HIT;

    for (int r = 0; r < titan_height; r++) {
        for (int c = 0; c < titan_width; c++) {
            if (titan_art[r][c] != ' ' && (start_c + c >= 0 && start_c + c < COLS)) {
                map[r][start_c + c] = titan_art[r][c];
                color_map[r][start_c + c] = titan_color;
            }
        }
    }

    // 소환수 기믹, 공간 파동, 레이저, 유도탄 순차 텍스트 매핑
    if (phase == 2) {
        for (int i = 0; i < 5; i++) {
            if (minions[i].active) {
                map[minions[i].r][minions[i].c] = 'W';
                color_map[minions[i].r][minions[i].c] = C_MINION;
            }
        }
    }

    if (wave_active) {
        for (int c = 0; c < COLS; c++) {
            if (c < wave_hole_c || c > wave_hole_c + 6) {
                map[wave_r][c] = '~'; color_map[wave_r][c] = C_WAVE;
            }
        }
    }

    for (int c = 0; c < COLS; c++) {
        if (laser_warning[c] > 0) {
            for (int r = titan_height; r < ROWS; r++) {
                if (r % 2 == (tick_count / 4) % 2) {
                    map[r][c] = '!'; color_map[r][c] = C_WARNING;
                }
            }
        }
        else if (laser_firing[c] > 0) {
            for (int r = titan_height; r < ROWS; r++) {
                map[r][c] = '|'; color_map[r][c] = C_LASER;
            }
        }
    }

    // 내 발사체 및 플레이어 기체 좌표 버퍼 최종 매핑
    for (int i = 0; i < 10; i++) {
        if (orbs[i].active) { map[orbs[i].r][orbs[i].c] = '@'; color_map[orbs[i].r][orbs[i].c] = C_ORB; }
    }
    for (int i = 0; i < 50; i++) {
        if (bullets[i].active) { map[bullets[i].r][bullets[i].c] = '^'; color_map[bullets[i].r][bullets[i].c] = C_BULLET; }
    }
    map[player_r][player_c] = 'A'; color_map[player_r][player_c] = C_PLAYER;
}

/*
  [버퍼 기반 콘솔 최적화 출력 영역]
  화면 전체를 지우는 대신 커서만 (0,0)으로 이동시킨 뒤 도화지의 그림을 그대로 모니터로 전송합니다.
  글자마다 색상을 바꿀 때 생기는 심각한 렉 현상을 막기 위해
  출력할 글자의 색상이 기존 색상과 다를 때만 윈도우 색상 변경 함수를 호출하도록 최적화했습니다.
 */
void draw_frame() {
    gotoxy(0, 0);

    set_color(C_DEFAULT);
    printf("============================================================\n");
    printf(" [난이도: %-6s] 거신 HP: ", diff_name);
    set_color(C_BOSS);

    int hp_bar_len = (titan_hp * 35) / titan_max_hp;
    for (int i = 0; i < 35; i++) {
        if (i < hp_bar_len) printf("■"); else printf("-");
    }
    set_color(C_DEFAULT);
    printf("\n");
    printf(" [신의 권능] HP: ");
    set_color(C_PLAYER);
    for (int i = 0; i < player_max_hp; i++) {
        if (i < player_hp) printf("♥ "); else printf("♡ ");
    }
    printf("                               \n");
    set_color(C_DEFAULT);
    printf("============================================================\n");

    int current_color = -1;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (color_map[r][c] != current_color) {
                set_color(color_map[r][c]);
                current_color = color_map[r][c];
            }
            printf("%c", map[r][c]);
        }
        printf("\n");
    }

    if (phase == 2) {
        gotoxy(6, 12); set_color(C_WARNING);
        printf("[ 심판의 의식 진행 중! 광신도를 모두 파괴하십시오! ]");
        gotoxy(23, 14); set_color(C_DEFAULT);
        if (ritual_time_left < 0) ritual_time_left = 0;
        printf("남은 시간 : %d초", ritual_time_left);
    }
    else if (phase == 3) {
        gotoxy(9, 12); set_color(C_PLAYER);
        printf("[ 그로기 상태! 치명적인 피해를 입히십시오! ]");
    }
    else if (phase == 4) {
        gotoxy(9, 12); set_color(C_BOSS);
        printf("보스가 광폭화를 시작합니다!!!");
    }
}

/*
  [보스 토벌 엔딩 연출 및 스크롤 크레딧 제어 영역]
  보스 체력 소진 시 난수 확률 기반 붕괴 파티클과 화면 진동을 출력하여 달성감을 주고
  스크롤 좌표 연산을 통해 영화 엔딩과 같은 스태프 롤 연출을 구현했습니다.
  종료 직전 스페이스바 연타로 인한 강제 튕김 방지를 위해 키보드 입력 버퍼를 완전히 비워냅니다.
 */
void play_ending(void) {
    for (int i = 0; i < 20; i++) {
        shake_offset = (rand() % 5) - 2;
        clear_attacks();
        build_frame();

        int start_c = ((COLS - titan_width) / 2) + shake_offset;
        for (int r = 0; r < titan_height; r++) {
            for (int c = 0; c < titan_width; c++) {
                if (titan_art[r][c] != ' ') {
                    int rand_val = rand() % 100;
                    if (rand_val < 20) {
                        map[r][start_c + c] = '*';
                        color_map[r][start_c + c] = (rand() % 2 == 0) ? C_WARNING : C_BOSS_HIT;
                    }
                    else if (rand_val < 50) {
                        map[r][start_c + c] = ' ';
                    }
                }
            }
        }
        draw_frame();
        Sleep(100);
    }

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            map[r][c] = ' ';
            color_map[r][c] = 240;
        }
    }
    draw_frame();
    Sleep(150);
    system("cls");

    const char* credits[] = {
        "============================================",
        "          T I T A N O M A C H I A           ",
        "============================================",
        "                                            ",
        "            [ 거신 토벌 성공! ]             ",
        "                                            ",
        "    우주를 위협하던 거신을 파괴했습니다.    ",
        "    빛의 파편은 다시 깊은 잠에 듭니다...    ",
        "                                            ",
        "                                            ",
        "                                            ",
        "                                            ",
        "        플레이해주셔서 감사합니다.          ",
        "============================================",
        "        엔터 키를 누르면 종료됩니다.        "
    };
    int credit_lines = 15;

    for (int y = ROWS; y >= 6; y--) {
        system("cls");
        for (int i = 0; i < credit_lines; i++) {
            if (y + i >= 0 && y + i < ROWS) {
                gotoxy((COLS - 44) / 2, y + i);
                set_color(C_PLAYER);
                printf("%s", credits[i]);
            }
        }
        Sleep(150);
    }

    while (_kbhit()) _getch();
    while (1) {
        int key = _getch();
        if (key == '\r' || key == '\n') break;
    }
}

/*
  [게임 라이프사이클 엔진 모터]
  입력 확인, 논리 연산, 렌더링, 출력 순으로 진행되는 프레임 갱신 주기를 제어하며
  체력 상태에 따라 승리 또는 패배 화면으로 진입할 수 있는 분기점을 관리합니다.
 */
int run_game(void) {
    init_game();
    system("cls");
    hide_cursor();

    while (1) {
        handle_input();
        update_logic();
        build_frame();
        draw_frame();

        if (player_hp <= 0) {
            set_color(C_BOSS);
            gotoxy(12, ROWS / 2 + 3);
            printf(" >>> 거신의 압도적인 힘에 짓눌려 소멸했습니다... <<< ");

            gotoxy(14, ROWS / 2 + 5);
            set_color(C_DEFAULT);
            printf("[ 엔터 키를 눌러 메뉴로 복귀하십시오. ]");

            while (_kbhit()) _getch();
            while (1) {
                int key = _getch();
                if (key == '\r' || key == '\n') break;
            }
            return 0;
        }

        if (titan_hp <= 0) {
            play_ending();
            return 1;
        }

        Sleep(FRAME_DELAY);
    }
}

/*
  [프로그램 최초 진입점]
  콘솔창의 해상도를 고정하고 사용자가 프로그램을 끄기 전까지
  메뉴와 인게임을 영구적으로 반복 순환하도록 메인 스레드를 구성합니다.
 */
int main(void) {
    srand((unsigned int)time(NULL));
    system("mode con: cols=62 lines=32");

    while (1) {
        show_title();
        show_difficulty();
        run_game();
    }
    return 0;
}