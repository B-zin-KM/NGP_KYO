#include "Common.h"         
#include "ServerFramework.h"

// LLD 3. GameLoop 스레드 (게임 로직 돌리는 곳)
unsigned __stdcall GameLoopThread(void* arg)
{
    printf("[GameLoopThread] Started. Game is running!\n");

    while (1)
    {
        EnterCriticalSection(&g_GameRoom.lock);
        {
            UpdateBullets(); // 범진님 기능 (총알+벽파괴)
            UpdateEnemyAI(); // 적 이동
            CheckCollisions(); 
        }
        LeaveCriticalSection(&g_GameRoom.lock);

        // 전체 상태 전송 (S_GAME_STATE)
        BroadcastPacket(NULL, 0);

        if (CheckGameEndConditions() == true) {
            break;
        }
        Sleep(10); // 약 100FPS
    }

    printf("[GameLoopThread] Game Over.\n");
    return 0;
}

// 클라이언트 1명당 1개씩 생성되는 스레드 (패킷 수신 담당)
unsigned __stdcall ClientThread(void* arg)
{
    int playerIndex = (int)arg;
    SOCKET clientSocket = g_GameRoom.players[playerIndex].socket;
    char buffer[1024];

    printf("[ClientThread %d] Started.\n", playerIndex);

    while (1)
    {
        int recvBytes = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (recvBytes == SOCKET_ERROR || recvBytes == 0) {
            printf("[ClientThread %d] Connection lost.\n", playerIndex);
            closesocket(clientSocket);

            EnterCriticalSection(&g_GameRoom.lock);
            g_GameRoom.players[playerIndex].isConnected = false;
            LeaveCriticalSection(&g_GameRoom.lock);

            break;
        }

        // 패킷 처리
        PacketHeader* header = (PacketHeader*)buffer;
        short packetType = header->type;

        EnterCriticalSection(&g_GameRoom.lock);
        {
            // 1. 로비용 패킷 (준비 완료)
            if (packetType == C_REQ_READY) {
                g_GameRoom.players[playerIndex].isReady = !g_GameRoom.players[playerIndex].isReady;
                printf("Player %d Ready State: %d\n", playerIndex, g_GameRoom.players[playerIndex].isReady);
            }
            // 2. 게임용 패킷 (이동)
            else if (packetType == C_MOVE) {
                ProcessPlayerMove(playerIndex, buffer);
            }
            // 3. 게임용 패킷 (공격)
            else if (packetType == C_ATTACK) {
                ProcessPlayerAttack(playerIndex, buffer);
            }
        }
        LeaveCriticalSection(&g_GameRoom.lock);
    }
    return 0;
}

// 타임아웃 처리
void HandleMatchingTimeout(SOCKET tempSockets[], int count) {
    printf("Matching time out. Closing sockets.\n");
    char timeoutPacket[] = "Matching Failed";

    for (int i = 0; i < count; i++) {
        send(tempSockets[i], timeoutPacket, sizeof(timeoutPacket), 0);
        closesocket(tempSockets[i]);
    }
}

// 메인 스레드 루프 (접속 대기 -> 로비 -> 게임 시작)
void AcceptLoop(SOCKET listenSocket)
{
    while (1)
    {
        printf("[AcceptLoop] Waiting for %d players...\n", MAX_PLAYERS_PER_ROOM);

        SOCKET tempSockets[MAX_PLAYERS_PER_ROOM];
        g_GameRoom.connectedPlayers = 0;

        bool matchingComplete = false;
        time_t firstConnectionTime = 0;

        // 1. 접속 대기 (기존 코드 유지)
        while (g_GameRoom.connectedPlayers < MAX_PLAYERS_PER_ROOM)
        {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(listenSocket, &readSet);

            struct timeval timeout;
            if (g_GameRoom.connectedPlayers == 0) {
                timeout.tv_sec = LONG_MAX;
                timeout.tv_usec = 0;
            }
            else {
                time_t elapsed = time(NULL) - firstConnectionTime;
                timeout.tv_sec = (MATCHING_TIMEOUT_SEC - elapsed);
                timeout.tv_usec = 0;
                if (timeout.tv_sec <= 0) {
                    HandleMatchingTimeout(tempSockets, g_GameRoom.connectedPlayers);
                    break;
                }
            }

            int selResult = select(0, &readSet, NULL, NULL, &timeout);

            if (selResult == SOCKET_ERROR) break;
            if (selResult == 0) {
                HandleMatchingTimeout(tempSockets, g_GameRoom.connectedPlayers);
                break;
            }

            if (FD_ISSET(listenSocket, &readSet))
            {
                SOCKET clientSocket = accept(listenSocket, NULL, NULL);
                if (clientSocket == INVALID_SOCKET) continue;

                EnterCriticalSection(&g_GameRoom.lock);
                int index = g_GameRoom.connectedPlayers;
                g_GameRoom.players[index].socket = clientSocket;
                g_GameRoom.players[index].id = index;
                g_GameRoom.players[index].isConnected = true;

                // 초기 위치 설정
                g_GameRoom.players[index].x = 400 + (index * 100);
                g_GameRoom.players[index].y = 400;
                g_GameRoom.players[index].direct = 3;
                g_GameRoom.players[index].life = true;
                g_GameRoom.players[index].ammo = MAX_BULLETS;

                g_GameRoom.connectedPlayers++;
                LeaveCriticalSection(&g_GameRoom.lock);

                tempSockets[index] = clientSocket;

                if (index == 0) firstConnectionTime = time(NULL);

                printf("[AcceptLoop] Player %d connected. (%d/%d)\n",
                    index, g_GameRoom.connectedPlayers, MAX_PLAYERS_PER_ROOM);

                if (g_GameRoom.connectedPlayers == MAX_PLAYERS_PER_ROOM) {
                    matchingComplete = true;
                }
            }
        }

        // 2. 매칭 완료 -> 로비 진입
        if (matchingComplete)
        {
            printf("Matching complete! Entering Lobby...\n");

            // [통합] 여기서 맵과 적을 모두 초기화합니다!
            InitGameMap();  // 범진님 (맵 생성)
            InitEnemies();  // 태웅님 (적 생성)

            // 각 클라이언트 패킷 수신 스레드 시작
            for (int i = 0; i < MAX_PLAYERS_PER_ROOM; ++i)
            {
                HANDLE hClientThread = (HANDLE)_beginthreadex(NULL, 0, ClientThread, (void*)i, 0, NULL);
                if (hClientThread) CloseHandle(hClientThread);
            }

            // 매칭 완료 패킷 전송
            EnterCriticalSection(&g_GameRoom.lock);
            for (int i = 0; i < MAX_PLAYERS_PER_ROOM; ++i)
            {
                g_GameRoom.players[i].isReady = false;
                SC_MatchingCompletePacket pkt;
                pkt.header.size = sizeof(pkt);
                pkt.header.type = S_MATCH_COMPLETE; // 1번
                pkt.yourPlayerID = i;
                send(g_GameRoom.players[i].socket, (char*)&pkt, sizeof(pkt), 0);
            }
            LeaveCriticalSection(&g_GameRoom.lock);

            // 3. 로비 대기 루프 (모두 레디할 때까지)
            while (true)
            {
                int readyCount = 0;
                int connectedCount = 0;

                S_LobbyUpdatePacket lobbyPkt;
                lobbyPkt.header.size = sizeof(S_LobbyUpdatePacket);
                lobbyPkt.header.type = S_LOBBY_UPDATE; // 4번

                EnterCriticalSection(&g_GameRoom.lock);
                {
                    for (int i = 0; i < MAX_PLAYERS_PER_ROOM; ++i) {
                        if (g_GameRoom.players[i].isConnected) {
                            connectedCount++;
                            if (g_GameRoom.players[i].isReady) readyCount++;
                        }
                        lobbyPkt.players[i].connected = g_GameRoom.players[i].isConnected;
                        lobbyPkt.players[i].isReady = g_GameRoom.players[i].isReady;
                    }
                    lobbyPkt.connectedCount = connectedCount;
                    BroadcastPacket((char*)&lobbyPkt, lobbyPkt.header.size);
                }
                LeaveCriticalSection(&g_GameRoom.lock);

                // ★ 모두 준비 완료되면 게임 시작!
                if (readyCount == MAX_PLAYERS_PER_ROOM) {
                    printf("All players Ready! Starting Game...\n");

                    S_GameStartPacket startPkt;
                    startPkt.header.size = sizeof(startPkt);
                    startPkt.header.type = S_GAME_START; // 5번

                    EnterCriticalSection(&g_GameRoom.lock);
                    BroadcastPacket((char*)&startPkt, startPkt.header.size);
                    LeaveCriticalSection(&g_GameRoom.lock);

                    break; // 로비 탈출 -> 게임 시작
                }

                if (connectedCount == 0) break;
                Sleep(100);
            }

            // 4. 게임 루프 스레드 시작 (여기서 총알, 적, 벽 파괴 로직이 돌아감)
            HANDLE hGameLoopThread = (HANDLE)_beginthreadex(NULL, 0, GameLoopThread, NULL, 0, NULL);
            if (hGameLoopThread) {
                WaitForSingleObject(hGameLoopThread, INFINITE); // 게임 끝날 때까지 여기서 대기
                CloseHandle(hGameLoopThread);
            }
        }
    }
}