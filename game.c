#define _CRT_SECURE_NO_WARNINGS // scanf를 사용할 때 필수
#include <stdio.h>
#include <stdlib.h>  // 랜덤 숫자(난수)를 뽑아내거나 프로그램을 끄는 등 기능이 들어있다.
#include <time.h>    // 컴퓨터의 현재 시간 정보를 가져옵니다. 매번 다른 랜덤 패턴을 만들기 위해 시계 사용
#include <windows.h> // 화면 색상 바꾸기, 특정 위치로 커서 옮기기 등 윈도우 화면을 꾸미는 도구 상자
#include <conio.h>   // 키보드를 눌렀을 때 엔터키를 치지 않아도 즉시 눌렀다는 걸 인식하게 해줌

// #define은 상수를 지어주는 기능입니다. 
// 나중에 맵 크기를 바꿀 때 숫자 25나 60을 일일이 찾아서 고치지 않고 여기서 한 번만 바꾸면 전체에 적용
#define ROWS 25 // 게임 화면의 세로줄(행) 개수
#define COLS 60 // 게임 화면의 가로줄(열) 개수

// 화면에 글자를 그릴 때 쓸 색상 번호
#define C_DEFAULT   15  // 흰색 (기본 글씨)
#define C_PLAYER    11  // 밝은 청록색 (내 캐릭터)
#define C_BULLET    11  // 밝은 청록색 (내 총알)
#define C_BOSS      12  // 밝은 빨간색 (거신)
#define C_BOSS_HIT  240 // 흰색 바탕에 검은 글씨 (보스가 맞았을 때 번쩍이는 효과)
#define C_WARNING   14  // 노란색 (위험 경고)
#define C_LASER     12  // 빨간색 (보스의 레이저)
#define C_WAVE      9   // 파란색 (파동)
#define C_ORB       13  // 자주색 (유도탄)
#define C_STAR      8   // 회색 (배경의 별빛)
#define C_STONE     8   // 회색 (보스가 무적 상태일 때 돌덩이처럼 보이게 함)
#define C_MINION    13  // 자주색 (보스가 소환하는 광신도 몬스터 색상)

// 게임의 속도를 결정. 30은 컴퓨터를 0.03초마다 한 번씩 쉬게 한다는 뜻
#define FRAME_DELAY 30  

// 구조체는 관련된 변수들을 하나의 상자에 묶어두는 기능
// 게임 안에서 날아다니는 총알이나 유도탄들의 위치와 상태를 관리하기 위해 Entity라는 묶음을 만들었다.
typedef struct {
    int r;       // row(세로 위치): 현재 화면의 위아래 위치를 기억
    int c;       // col(가로 위치): 현재 화면의 좌우 위치를 기억
    int active;  // 활성화 상태: 1이면 현재 화면에 살아있는 거고 0이면 파괴되어 사라짐
    int dir;     // 이동 방향: -1이면 왼쪽으로, 1이면 오른쪽으로 이동한다는 뜻
} Entity;

// 2차원 배열
char map[ROWS][COLS];       // 화면의 각 칸에 어떤 문자를 그릴지 저장하는 도화지
int color_map[ROWS][COLS];  // 화면의 각 칸에 어떤 색을 칠할지 저장하는 도화지

int player_r, player_c;       // 내 캐릭터의 세로(r), 가로(c) 위치입니다.
int player_hp, player_max_hp; // 내 캐릭터의 현재 체력과 최대 체력입니다.

// 보스의 모습을 그리기 위한 문자열 묶음 [] 안에 한 줄씩 그림이 들어있다.
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
int titan_width = 56;    // 보스 그림의 가로 폭
int titan_height = 8;    // 보스 그림의 세로 높이
int titan_hp, titan_max_hp; // 보스의 현재 체력과 최대 체력
int hit_flash_timer = 0; // 보스가 맞았을 때 몇 프레임 동안 번쩍일지 기억하는 타이머
int shake_offset = 0;    // 보스가 광폭화할 때 좌우로 부들부들 떨리게 만드는 오차 값

// 위에서 만든 Entity를 바탕으로 여러 개의 물건을 찍어냅니다.
Entity bullets[50]; // 내 총알의 최대 개수
Entity orbs[10];    // 보스의 유도탄 최대 개수
Entity minions[5];  // 60% 패턴에서 보스를 돕기 위해 나오는 광신도 수

int diff_level;      // 1(쉬움), 2(보통), 3(어려움) 등 현재 난이도를 숫자로 기억
char diff_name[10];  // 화면에 출력할 난이도 이름을 담아둔다.

int laser_warning[COLS]; // 보스가 레이저를 쏘기 전 가로줄 어느 위치에 경고를 띄울지 저장
int laser_firing[COLS];  // 실제로 어느 위치에 데미지를 주는 레이저가 떨어지고 있는지 저장

// 게임 속 기믹들의 쿨타임을 재는 시계
int laser_timer = 0, wave_timer = 0, orb_timer = 0;
int wave_active = 0; // 거대 파동이 현재 화면에 내려오고 있는지(1) 아닌지(0)를 기억
int wave_r = 0;      // 거대 파동의 현재 세로 위치
int wave_hole_c = 0; // 거대 파동에서 우리가 피할 수 있는 안전지대의 가로 위치
int tick_count = 0;  // 게임이 시작된 후 시간이 얼마나 흘렀는지 틱 단위로 센다.

// 페이즈 관리 변수
// 보스의 체력에 따라 게임의 진행을 바꾸기 위해 현재 상태를 숫자로 기록
// 1: 일반, 2: 무적+광신도 소환, 3: 그로기, 4: 광폭화 연출, 5: 광폭화 전투
int phase = 1;
int ritual_timer = 0;   // 의식(60% 패턴)에 주어진 제한 시간 타이머
int stun_timer = 0;     // 보스가 기절했을 때 얼마나 오래 기절해 있을지 재는 타이머
int enrage_timer = 0;   // 보스가 광폭화할 때 쿠구구구 흔들리는 연출 시간을 잰다.
int has_done_ritual = 0; // 60% 패턴을 이미 했는지 안 했는지 체크 (체력 회복 시 두 번 하는 걸 막기 위함)
int has_done_berserk = 0;// 30% 패턴을 이미 했는지 체크


// 지정한 색상 번호를 받아서 앞으로 출력될 글자 색을 바꿔줍니다.
// 화면에 무언가를 그리는 draw_frame(), show_title() 등에서 글자를 찍기 직전에 사용
void set_color(int color_code);

// 화면을 지우지 않고 콘솔창의 글쓰기 커서를 (x, y) 좌표로 순간이동 (화면 깜빡임 방지용)
// draw_frame()의 맨 처음에 커서를 (0,0)으로 돌려 덮어쓰기를 하거나, 화면 중앙에 경고 텍스트를 띄울 때 사용
void gotoxy(int x, int y);

// 콘솔창 특유의 깜빡거리는 텍스트 입력 커서('_')를 투명하게 숨겨줌
// 게임이 본격적으로 시작될 때 딱 한 번 run_game()이나 main()의 맨 처음에 사용
void hide_cursor(void);

// 도화지와 팔레트를 텅 빈 공백(' ')으로 깨끗하게 지운다.
// 매 프레임마다 새로운 그림을 그리기 직전, build_frame()의 맨 첫 줄에서 사용
void clear_buffers(void);

// 게임을 처음 켰을 때 나오는 1. 시작 2. 설명 3. 종료 대문 화면을 그린다.
// 프로그램의 진짜 시작점인 main() 함수 안에서 무한 반복하며 사용자의 선택을 기다림
void show_title(void);

// 2번을 눌렀을 때 조작법과 보스의 기믹(60%, 30% 패턴)을 설명해 주는 페이지
// show_title() 안에서 플레이어가 2번 키를 눌렀을 때 출력
void show_help(void);

// 1번을 눌렀을 때, 쉬움 / 보통 / 어려움 중 하나를 고르게 한다.
// main() 함수에서 show_title()을 통과한 직후 게임 시작 직전에 사용
void show_difficulty(void);

// 내 체력, 보스 체력, 총알 위치, 타이머 등 모든 데이터를 게임 시작 전 상태로 되돌림
// 플레이어가 죽거나 이겨서 다시 게임을 시작할 때마다 run_game()의 맨 처음에 사용
void init_game(void);

// 플레이어가 누른 방향키와 스페이스바 입력을 낚아채서 내 캐릭터를 움직이거나 총알을 쏘게 한다.
// run_game() 안의 무한 루프에서 1단계로 불린다.
void handle_input(void);

// 보스가 레이저를 쏘고, 총알이 날아가고, 보스가 내 총알에 맞았는지 계산하는 물리 엔진
// run_game() 안의 무한 루프에서 2단계로 불린다.
void update_logic(void);

// update_logic()에서 계산된 모든 위치 데이터를 가져와서 실제로 2차원 배열에 찍어 넣는다.
// run_game() 안의 무한 루프에서 3단계로 불린다.
void build_frame(void);

// build_frame()이 완성한 도화지를 모니터 화면에 printf로 시원하게 쫙 뿌려줍니다. UI도 여기서 그린다.
// run_game() 안의 무한 루프에서 4단계로 불린다.
void draw_frame(void);

// 위에서 만든 4단계를 0.03초마다 무한히 뺑뺑이 돌려주는 루프입니다.
// main() 함수에서 난이도 선택이 끝난 직후에 딱 한 번 호출되어 게임 끝날 때까지 돌아간다.
int  run_game(void);

// 원하는 색 번호를 주면 콘솔창의 글자 색상을 바꿔주는 도구입니다.
void set_color(int color_code) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color_code);
}

// 화면을 지우지 않고 특정 x, y 좌표로 글쓰는 커서만 순간이동 시켜서 필요한 부분만 덮어쓰기 위한 도구
void gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

// 콘솔창 특유의 깜빡거리는 밑줄을 숨겨서 게임처럼 깔끔하게 보이게 만드는 도구입니다.
void hide_cursor(void) {
    CONSOLE_CURSOR_INFO cci = { 1, FALSE };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cci);
}

// 그림을 그리기 전에 도화지와 팔레트를 텅 빈 상태로 싹 지워주는 함수
void clear_buffers() {
    for (int r = 0; r < ROWS; r++) {       // 세로줄을 맨 위부터 아래까지
        for (int c = 0; c < COLS; c++) {   // 가로줄을 왼쪽부터 오른쪽까지
            map[r][c] = ' ';               // 빈칸으로 칠하고
            color_map[r][c] = C_DEFAULT;   // 색깔도 기본 흰색으로 초기화
        }
    }
}

// 게임을 처음 켰을 때 나오는 타이틀 화면을 그려주는 함수
void show_title(void) {
    while (1) { // while(1)은 무한 반복입니다. 사용자가 올바른 버튼을 누를 때까지 계속 화면을 띄워놓는다.
        system("cls"); // 화면을 깨끗하게 지운다.
        set_color(C_DEFAULT);
        printf("\n\n");
        printf("   ====================================================\n");
        printf("           T I T A N O M A C H I A  :  거신 토벌전       \n");
        printf("   ====================================================\n\n");
        printf("                    1. 게임 시작\n");
        printf("                    2. 게임 설명 (조작법)\n");
        printf("                    3. 게임 종료\n\n");
        printf("   번호를 입력하세요: ");

        char choice = _getch(); // 사용자가 키보드로 누른 키를 하나 가져온다.
        if (choice == '1') return; // 1번을 누르면 이 함수를 끝내고 게임 시작으로 나간다.
        else if (choice == '2') show_help(); // 2번을 누르면 설명서 화면 함수를 부른다.
        else if (choice == '3') exit(0);     // 3번을 누르면 프로그램 자체를 강제로 종료한다.
    }
}

// 2번 메뉴를 눌렀을 때 조작법과 규칙을 화면에 띄워주는 함수
void show_help(void) {
    system("cls");
    set_color(C_DEFAULT);
    printf("\n   [ 게임 설명 ]\n");
    printf("   거신 '티탄'의 공격을 피하며 쓰러뜨리세요!\n");
    printf("   **주의: 보스의 체력에 따라 특수 기믹과 광폭화가 발동됩니다!**\n\n");
    printf("   [ 조작법 ]\n");
    printf("   방향키 (상하좌우) : 빛의 파편(플레이어) 이동\n");
    printf("   스페이스바 (Space): 신의 쐐기(공격) 발사\n\n");
    printf("   [ 특수 페이즈 안내 ]\n");
    printf("   - 체력 60%%: 보스가 무적이 되며 광신도를 소환합니다.\n");
    printf("     제한 시간 내에 처치하지 못하면 보스가 체력을 회복합니다!\n");
    printf("   - 체력 30%%: 보스가 이성을 잃고 광폭화 패턴에 돌입합니다.\n\n");
    printf("   아무 키나 누르면 메인 메뉴로 돌아갑니다...\n");

    // void를 씌운 이유는 _getch()가 키보드 값을 뱉어내는데 우리는 그 값을 쓸 게 아니라
    // 그냥 아무 키나 누를 때까지 멈춰있는 용도
    (void)_getch();
}

// 1번 메뉴를 눌러 게임을 시작할 때 난이도를 선택받는 함수
void show_difficulty(void) {
    while (1) {
        system("cls");
        set_color(C_DEFAULT);
        printf("\n   [ 난이도 선택 ]\n\n");
        printf("   1. Easy   (체력 5 / 보스HP 150) - 기본 레이저 패턴\n");
        printf("   2. Normal (체력 4 / 보스HP 300) - 레이저 + 공간 파동\n");
        printf("   3. Hard   (체력 3 / 보스HP 500) - 레이저 + 파동 + 유도탄\n\n");
        printf("   난이도 번호를 선택하세요: ");

        char choice = _getch();
        // 선택한 번호에 따라 변수(난이도, 체력)들을 다르게 세팅하고 함수를 끝낸다.
        if (choice == '1') { diff_level = 1; player_max_hp = 5; titan_max_hp = 150; sprintf(diff_name, "Easy"); return; }
        if (choice == '2') { diff_level = 2; player_max_hp = 4; titan_max_hp = 300; sprintf(diff_name, "Normal"); return; }
        if (choice == '3') { diff_level = 3; player_max_hp = 3; titan_max_hp = 500; sprintf(diff_name, "Hard"); return; }
    }
}

// 게임을 시작하기 전에 플레이어 위치, 보스 체력, 총알 상태 등 모든 것을 초기 상태로 셋팅
// 안 하면 이전에 플레이했던 시체나 총알이 그대로 남아서 버그가 생긴다.
void init_game() {
    player_r = ROWS - 2; // 화면 맨 밑에서 살짝 위
    player_c = COLS / 2; // 화면 가로 한가운데
    player_hp = player_max_hp; // 체력을 꽉 채워준다.
    titan_hp = titan_max_hp;

    for (int i = 0; i < 50; i++) bullets[i].active = 0; // 총알 50발 모두 비활성화
    for (int i = 0; i < 10; i++) orbs[i].active = 0;    // 유도탄 10발 모두 비활성화
    for (int i = 0; i < 5; i++) minions[i].active = 0;  // 몬스터 5마리 모두 비활성화

    for (int c = 0; c < COLS; c++) {
        laser_warning[c] = 0; laser_firing[c] = 0; // 바닥에 깔린 레이저 경고도 모두 지운다.
    }

    laser_timer = 0; wave_timer = 0; orb_timer = 0;
    wave_active = 0; tick_count = 0;

    phase = 1; // 1단계로 시작
    has_done_ritual = 0; has_done_berserk = 0; // 특수 패턴 아직 안 썼다고 기록
    shake_offset = 0; // 화면 흔들림 없음
}

// 플레이어가 키보드를 누르는 것을 감지해서 캐릭터를 움직이거나 총알을 쏜다.
void handle_input() {
    if (!_kbhit()) return; // _kbhit()은 아무 키도 안 눌렀으면 멈춰있지 않고 바로 넘어감 (게임이 안 끊기게 해줌)
    int key = _getch();    // 무슨 키를 눌렀는지 가져온다.

    // 방향키는 컴퓨터가 두 번의 신호로 인식하기 때문에 첫 신호가 0이나 224면 한 번 더 읽어준다.
    if (key == 0 || key == 224) {
        key = _getch();
        // 화면 밖으로 나가지 못하도록 조건(예: player_c > 0)을 걸고 위치(r, c)를 변경한다.
        if (key == 72 && player_r > titan_height) player_r--; // 위 화살표
        if (key == 80 && player_r < ROWS - 1) player_r++;     // 아래 화살표
        if (key == 75 && player_c > 0) player_c--;            // 왼쪽 화살표
        if (key == 77 && player_c < COLS - 1) player_c++;     // 오른쪽 화살표
        return;
    }

    // 스페이스바를 누르면 빈 총알을 찾아서 쏜다.
    if (key == ' ') {
        for (int i = 0; i < 50; i++) {
            if (!bullets[i].active) {     // 아직 안 쏘고 쉬고 있는 총알을 찾으면
                bullets[i].active = 1;    // 활성화시키고
                bullets[i].r = player_r - 1; // 내 캐릭터 바로 머리 위로 위치를 잡는다.
                bullets[i].c = player_c;
                break; // 한 번에 여러 발 쏘지 않도록 하나만 쏘고 그만 찾는다.
            }
        }
    }
}

// 총알 이동, 보스의 공격 패턴, 충돌 판정 등 게임 안에서 일어나는 물리 법칙을 계산하는 함수
void update_logic() {
    tick_count++; // 시간 1 증가
    if (hit_flash_timer > 0) hit_flash_timer--; // 보스가 맞아서 번쩍이는 효과 시간을 줄인다.

    // 1. 페이즈(Phase) 전환 관리
    // 체력이 일정 비율 아래로 떨어지면 게임의 페이즈를 바꾼다.
    if (phase == 1 || phase == 5) { // 보스가 싸우고 있는 상태일 때만 체력을 검사
        // 보스 피가 30% 이하이고 아직 광폭화를 안 했다면?
        if (titan_hp <= titan_max_hp * 0.3 && !has_done_berserk) {
            has_done_berserk = 1;
            phase = 4;         // 4단계(광폭화 연출)로 돌입!
            enrage_timer = 90; // 90프레임(약 3초) 동안 덜덜덜 떠는 연출
        }
        // 보스 피가 60% 이하이고, 아직 의식을 안 했다면?
        else if (titan_hp <= titan_max_hp * 0.6 && !has_done_ritual) {
            has_done_ritual = 1;
            phase = 2;          // 2단계(심판의 의식: 무적+광신도 소환)로 돌입
            ritual_timer = 300; // 300프레임(약 10초)의 타임어택 시간을 준다.

            // 광신도 5마리를 맵에 뿌려준다.
            for (int i = 0; i < 5; i++) {
                minions[i].active = 1;
                minions[i].r = titan_height + 2 + (rand() % 4); // 세로 위치 랜덤
                minions[i].c = 10 + (i * 8);                    // 가로 위치는 겹치지 않게 간격을 띄움
                minions[i].dir = (rand() % 2 == 0) ? 1 : -1;    // 왼쪽으로 갈지 오른쪽으로 갈지 랜덤
            }
        }
    }

    //  2. 특수 페이즈 처리 로직
    if (phase == 2) { // 심판의 의식 - 10초 타임어택
        ritual_timer--; // 남은 시간을 깎는다.
        int minions_alive = 0;

        // 3프레임마다 한 번씩 광신도들이 좌우로 움직인다.
        if (tick_count % 3 == 0) {
            for (int i = 0; i < 5; i++) {
                if (minions[i].active) {
                    minions_alive++; // 아직 살아있는 광신도 수 카운트
                    minions[i].c += minions[i].dir; // 방향만큼 가로 위치 이동

                    // 벽에 부딪히면 방향을 반대로(-1 곱하기) 바꾼다.
                    if (minions[i].c < 2 || minions[i].c >= COLS - 2) minions[i].dir *= -1;
                }
            }
        }

        // 시간이 다 떨어졌을 때의 심판
        if (ritual_timer <= 0) {
            if (minions_alive > 0) {
                // 패턴 실패 광신도가 1마리라도 살아있다면 보스가 80%로 체력을 회복
                // (int)를 붙여 소수점을 떼고 정수로 강제 변환하여 오류를 막는다.
                titan_hp = (int)(titan_max_hp * 0.8);
                phase = 1; // 다시 일반 전투로 복귀
                for (int i = 0; i < 5; i++) minions[i].active = 0; // 남은 몬스터는 지운다.
            }
        }
        else {
            // 시간 안에 광신도를 다 잡으면?
            int check_alive = 0;
            for (int i = 0; i < 5; i++) if (minions[i].active) check_alive++;

            if (check_alive == 0) {
                // 패턴 성공 보스 기절
                phase = 3; // 3단계(그로기)로 전환
                stun_timer = 120; // 120프레임(약 4초) 동안 무력화 상태
            }
        }
    }
    else if (phase == 3) { // 그로기 상태 
        stun_timer--;
        if (stun_timer <= 0) phase = 1; // 기절 시간이 다 되면 다시 일반 전투(1단계)로 돌아간다.
    }
    else if (phase == 4) { // 광폭화 연출
        enrage_timer--;
        // 4프레임 주기로 왼쪽으로 2칸, 오른쪽으로 2칸 흔들리게 만들어 지진이 나는 듯한 효과를 준다.
        shake_offset = (enrage_timer % 4 < 2) ? -2 : 2;

        if (enrage_timer <= 0) {
            phase = 5; // 연출이 끝나면 5단계(광폭화 실전 전투) 진행
            shake_offset = 0;
        }
    }

    // 3. 플레이어 총알 처리 및 충돌 판정
    for (int i = 0; i < 50; i++) {
        if (bullets[i].active) { // 날아가고 있는 총알이라면 
            bullets[i].r--;      // 한 칸 위로 올린다.

            if (bullets[i].r < 0) { bullets[i].active = 0; continue; } // 끝까지 가면 총알 삭제

            // 보스를 때렸는지 판정 (2단계 무적이나 4단계 연출 중에는 무시)
            if (phase != 2 && phase != 4 && bullets[i].r < titan_height) {
                // 보스 그림이 있는 가로 범위 계산 (흔들림 오차까지 포함)
                int start_c = (COLS - titan_width) / 2 + shake_offset;
                int end_c = start_c + titan_width;

                if (bullets[i].c >= start_c && bullets[i].c < end_c) {
                    // 보스 그림 배열에서 빈 공간(' ')이 아닌 진짜 껍데기에 맞았는지 확인
                    if (titan_art[bullets[i].r][bullets[i].c - start_c] != ' ') {
                        titan_hp--; // 보스 체력 1 감소
                        bullets[i].active = 0; // 총알 파괴
                        hit_flash_timer = 2;   // 화면 번쩍
                        continue;
                    }
                }
            }

            // 60% 패턴 중이라면 총알이 광신도를 맞췄는지 판정
            if (phase == 2) {
                for (int m = 0; m < 5; m++) {
                    if (minions[m].active && bullets[i].r == minions[m].r && bullets[i].c == minions[m].c) {
                        minions[m].active = 0; // 광신도 파괴
                        bullets[i].active = 0; // 총알도 파괴
                    }
                }
            }

            // 어려움 난이도라면 내 총알로 보스의 유도탄을 맞춰서 부술 수 있다.
            if (diff_level == 3) {
                for (int o = 0; o < 10; o++) {
                    if (orbs[o].active && bullets[i].r == orbs[o].r && bullets[i].c == orbs[o].c) {
                        bullets[i].active = 0; orbs[o].active = 0;
                    }
                }
            }
        }
    }

    // 4. 보스 공격 패턴 (광폭화 적용)
    // 기절 상태(3)나 광폭화 연출 중(4)에는 공격을 멈춤
    if (phase != 3 && phase != 4) {
        // 보스의 남은 체력 퍼센트를 계산 (100 ~ 0%)
        int hp_percent = (titan_hp * 100) / titan_max_hp;
        if (hp_percent < 0) hp_percent = 0;

        // 핵심 기믹: 체력이 낮을수록 패턴이 튀어나오는 쿨타임이 짧아진다.

        // 1. 레이저: 기본 35틱에서 시작해, 피를 완전히 잃으면 최대 20틱을 깎아서 15틱이 됨
        int laser_interval = 15 + (20 * hp_percent / 100);
        // 2. 공간 파동: 기본 150틱에서 시작 피를 완전히 잃으면 최대 90틱을 깎아서 60틱이 됨
        int wave_interval = 60 + (90 * hp_percent / 100);
        // 3. 추적 유도탄: 기본 80틱에서 시작해, 피를 완전히 잃으면 최대 50틱을 깎아서 30틱이 됨
        int orb_interval = 30 + (50 * hp_percent / 100);

        // 5단계 광폭화에 돌입하면 쿨타임을 최하로 고정
        if (phase == 5) {
            laser_interval = 10; wave_interval = 50; orb_interval = 20;
        }

        // 레이저 패턴
        laser_timer++;
        if (laser_timer >= laser_interval) { // 쿨타임이 찼다면
            laser_timer = 0; // 타이머 초기화
            // 한 번에 쏘는 레이저 개수도 광폭화 때는 6개로 고정 아니면 체력에 따라 늘어난다.
            int attack_count = (phase == 5) ? 6 : (3 + (100 - hp_percent) / 20);

            for (int i = 0; i < attack_count; i++) {
                int target_c = (i == 0) ? player_c : (rand() % COLS); // 첫 발은 무조건 날 조준 나머지는 랜덤
                laser_warning[target_c] = 20; // 20프레임 동안 바닥에 경고 표시
            }
        }
        for (int c = 0; c < COLS; c++) {
            if (laser_warning[c] > 0) {
                laser_warning[c]--; // 경고 시간 감소
                if (laser_warning[c] == 0) laser_firing[c] = 10; // 경고가 끝나면 10프레임짜리 레이저 발사
            }
            if (laser_firing[c] > 0) {
                laser_firing[c]--;
                // 레이저가 쏴지고 있는데 내 캐릭터가 그 세로줄(c)에 있다면 체력 감소
                if (player_c == c) { player_hp--; laser_firing[c] = 0; }
            }
        }

        // 거대 공간 파동 패턴 (보통 난이도 이상)
        if (diff_level >= 2) {
            if (wave_active) {
                if (tick_count % 2 == 0) wave_r++; // 파동이 두 프레임마다 한 칸씩 내려온다.
                if (wave_r >= ROWS) wave_active = 0; // 바닥에 닿으면 사라진다.

                // 파동이 내 캐릭터 줄까지 내려왔는데 내가 안전지대에 들어가 있지 않다면 체력 감소
                if (wave_r == player_r && (player_c < wave_hole_c || player_c > wave_hole_c + 6)) player_hp--;
            }
            else {
                wave_timer++;
                if (wave_timer >= wave_interval) { // 쿨타임이 차면 파동 생성
                    wave_timer = 0; wave_active = 1; wave_r = titan_height;
                    wave_hole_c = rand() % (COLS - 8) + 1; // 폭 6칸짜리 구멍을 랜덤 위치에 뚫어준다.
                }
            }
        }

        // 유도탄 패턴 (어려움 난이도 전용)
        if (diff_level >= 3) {
            orb_timer++;
            if (orb_timer >= orb_interval) { // 쿨타임이 차면 보스 몸에서 유도탄 1개 발사
                orb_timer = 0;
                for (int i = 0; i < 10; i++) {
                    if (!orbs[i].active) {
                        orbs[i].active = 1; orbs[i].r = titan_height;
                        orbs[i].c = (COLS - titan_width) / 2 + rand() % titan_width;
                        break;
                    }
                }
            }
            // 4프레임마다 유도탄이 내 캐릭터를 향해 한 칸씩 다가온다.
            if (tick_count % 4 == 0) {
                for (int i = 0; i < 10; i++) {
                    if (orbs[i].active) {
                        // 세로로 쫓아오기
                        if (orbs[i].r < player_r) orbs[i].r++; else if (orbs[i].r > player_r) orbs[i].r--;
                        // 가로로 쫓아오기
                        if (orbs[i].c < player_c) orbs[i].c++; else if (orbs[i].c > player_c) orbs[i].c--;

                        // 나랑 부딪히면 사라지며 체력 감소
                        if (orbs[i].r == player_r && orbs[i].c == player_c) {
                            player_hp--; orbs[i].active = 0;
                        }
                    }
                }
            }
        }
    }
}

// 화면을 화면에 그리기 직전 여태까지 계산한 모든 결과를 배열에 스탬프처럼 찍는 함수
void build_frame() {
    clear_buffers(); // 지난 프레임의 잔상을 깨끗이 지운다.

    // 1. 우주 배경 별빛 그리기
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (rand() % 50 == 0) { map[r][c] = '.'; color_map[r][c] = C_STAR; }
        }
    }

    // 2. 보스 렌더링 (페이즈에 따라 색깔을 다르게 칠한다.)
    int start_c = ((COLS - titan_width) / 2) + shake_offset;
    int titan_color = C_BOSS;

    if (phase == 2) titan_color = C_STONE; // 의식 중엔 회색으로 무적 표시
    else if (phase == 4 || phase == 5) {
        // 광폭화 중에는 붉은색과 노란색이 번쩍거림
        titan_color = (tick_count % 4 < 2) ? C_BOSS : C_WARNING;
    }
    // 평소에 맞았을 땐 하얗게 번쩍임
    if (hit_flash_timer > 0 && phase != 2 && phase != 4) titan_color = C_BOSS_HIT;

    for (int r = 0; r < titan_height; r++) {
        for (int c = 0; c < titan_width; c++) {
            if (titan_art[r][c] != ' ' && (start_c + c >= 0 && start_c + c < COLS)) {
                map[r][start_c + c] = titan_art[r][c]; // 2차원 문자 배열에 보스 스탬프 찍어줌
                color_map[r][start_c + c] = titan_color; // 색상 배열에도 찍어줌
            }
        }
    }

    // 3. 광신도 그리기 (60% 의식 페이즈에만)
    if (phase == 2) {
        for (int i = 0; i < 5; i++) {
            if (minions[i].active) {
                map[minions[i].r][minions[i].c] = 'W';
                color_map[minions[i].r][minions[i].c] = C_MINION;
            }
        }
    }

    // 4. 공간 파동 그리기
    if (wave_active) {
        for (int c = 0; c < COLS; c++) {
            if (c < wave_hole_c || c > wave_hole_c + 6) { // 구멍 범위가 아니면 파동을 그린다.
                map[wave_r][c] = '~'; color_map[wave_r][c] = C_WAVE;
            }
        }
    }

    // 5. 레이저 경고와 발사 그리기
    for (int c = 0; c < COLS; c++) {
        if (laser_warning[c] > 0) {
            for (int r = titan_height; r < ROWS; r++) {
                if (r % 2 == (tick_count / 2) % 2) { // 깜빡이는 애니메이션 효과
                    map[r][c] = '!'; color_map[r][c] = C_WARNING; // 느낌표 기호로 헷갈림 방지
                }
            }
        }
        else if (laser_firing[c] > 0) {
            for (int r = titan_height; r < ROWS; r++) {
                map[r][c] = '|'; color_map[r][c] = C_LASER; // 레이저 기둥 그리기
            }
        }
    }

    // 6. 유도탄 그리기
    for (int i = 0; i < 10; i++) {
        if (orbs[i].active) { map[orbs[i].r][orbs[i].c] = '@'; color_map[orbs[i].r][orbs[i].c] = C_ORB; }
    }

    // 7. 내 총알과 플레이어 캐릭터 그리기
    for (int i = 0; i < 50; i++) {
        if (bullets[i].active) { map[bullets[i].r][bullets[i].c] = '^'; color_map[bullets[i].r][bullets[i].c] = C_BULLET; }
    }
    map[player_r][player_c] = 'A'; color_map[player_r][player_c] = C_PLAYER;
}

// 다 그려진 배열을 가져다가 실제로 까만 콘솔창에 텍스트를 출력하는 함수
void draw_frame() {
    gotoxy(0, 0); // 화면을 지우지 않고 맨 윗줄부터 덮어쓰기 시작

    // 화면 맨 위쪽 UI
    set_color(C_DEFAULT);
    printf("============================================================\n");
    printf(" [난이도: %-6s] 거신 HP: ", diff_name);
    set_color(C_BOSS);

    // 보스 체력을 막대기 35칸짜리로 그림
    int hp_bar_len = (titan_hp * 35) / titan_max_hp;
    for (int i = 0; i < 35; i++) {
        if (i < hp_bar_len) printf("■"); else printf("-");
    }
    set_color(C_DEFAULT);
    printf("\n");
    printf(" [신의 권능] HP: ");
    set_color(C_PLAYER);
    // 내 체력을 하트로 그림
    for (int i = 0; i < player_max_hp; i++) {
        if (i < player_hp) printf("♥ "); else printf("♡ ");
    }
    printf("                               \n");
    set_color(C_DEFAULT);
    printf("============================================================\n");

    // 게임 화면 출력 영역
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            set_color(color_map[r][c]); // 배열에 적힌 색깔로 물감을 짜서
            printf("%c", map[r][c]);    // 배열에 적힌 문자를 찍는다.
        }
        printf("\n"); // 가로줄 하나 끝나면 줄바꿈
    }

    // 특수 페이즈 때 화면 한가운데에 띄워주는 경고 문구
    if (phase == 2) {
        gotoxy(6, 12); set_color(C_WARNING);
        printf("[ 심판의 의식 진행 중! 광신도(W)를 모두 파괴하라! ]");
        gotoxy(23, 14); set_color(C_DEFAULT);
        printf("남은 시간 : %d초", ritual_timer / 30); // 30틱이 1초니까 30으로 나누면 남은 초가 된다.
    }
    else if (phase == 3) {
        gotoxy(9, 12); set_color(C_PLAYER);
        printf("[ 그로기 상태! 극딜(치명적인 피해)을 넣으세요! ]");
    }
    else if (phase == 4) {
        gotoxy(9, 12); set_color(C_BOSS);
        printf("보스가 광폭화를 시작합니다 !!!");
    }
}

// 위에서 만든 입력, 연산, 그리기 등을 모아서 무한루프를 돌려주는 게임 엔진
int run_game(void) {
    init_game();   // 상태 초기화
    system("cls"); // 화면 지우기
    hide_cursor(); // 커서 숨기기

    while (1) {
        handle_input(); // 1. 키보드 누른 거 확인
        update_logic(); // 2. 총알 옮기고 보스 공격 계산
        build_frame();  // 3. 도화지에 계산 결과 도장 찍기
        draw_frame();   // 4. 모니터에 출력하기

        // 게임 오버 조건 검사 (내 체력이 0이하)
        if (player_hp <= 0) {
            set_color(C_BOSS);
            gotoxy(12, ROWS / 2 + 3);
            printf(" >>> 거신의 압도적인 힘에 짓눌려 소멸했습니다... <<< ");
            (void)_getch(); // 확인 버튼(아무 키) 누를 때까지 정지
            return 0; // 패배했다는 뜻의 0을 남기고 메뉴로 튕겨 나감
        }

        // 승리 조건 검사 (보스 체력이 0이하)
        if (titan_hp <= 0) {
            set_color(C_PLAYER);
            gotoxy(12, ROWS / 2 + 3);
            printf(" >>> 거신이 쓰러졌습니다! 우주에 평화가 찾아옵니다. <<< ");
            (void)_getch();
            return 1; // 승리했다는 뜻의 1을 남기고 메뉴로 튕겨 나감
        }

        Sleep(FRAME_DELAY); // 0.03초(30ms) 동안 컴퓨터를 멈춘다 게임 속도 조절용
    }
}

// C언어 프로그램이 맨 처음 실행될 때 가장 먼저 찾는 입구
// 여기서부터 모든 게 시작
int main(void) {
    srand((unsigned int)time(NULL));      // 랜덤 함수를 켜기 위한 주문
    system("mode con: cols=62 lines=32"); // 게임 창 크기를 내가 원하는 비율로 고정

    // 1(타이틀) -> 2(난이도) -> 3(본게임)을 무한루프
    // 죽거나 깨도 안 꺼지고 다시 타이틀로 돌아가는 오락기 구조
    while (1) {
        show_title();
        show_difficulty();
        run_game();
    }
    return 0; // 프로그램 정상 종료
}