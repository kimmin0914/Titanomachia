graph TD
    A([프로그램 시작 main]) --> B[난수 초기화 및 화면 크기 고정]
    
    subgraph Main Loop [메인 메뉴 루프]
        B --> C[타이틀 화면 출력 show_title]
        C -->|2번 입력| D[도움말 출력 show_help]
        D --> C
        C -->|3번 입력| E([프로그램 종료 exit])
        C -->|1번 입력| F[난이도 선택 show_difficulty]
    end

    F --> G[게임 데이터 초기화 init_game]

    subgraph Game Engine Loop [게임 인게임 루프]
        G --> H((프레임 시작))
        H --> I[1. 사용자 입력 처리 handle_input]
        I --> J[2. 물리/패턴 연산 update_logic]
        J --> K[3. 2차원 배열 도화지 구성 build_frame]
        K --> L[4. 콘솔 화면 렌더링 draw_frame]
        
        L --> M{승패 조건 검사}
        M -->|플레이어 체력 0| N[패배 텍스트 출력]
        M -->|보스 체력 0| O[승리 텍스트 출력]
        M -->|둘 다 생존| P[30ms 대기 Sleep]
        P --> H
    end

    N -->|아무 키나 입력| C
    O -->|아무 키나 입력| C
