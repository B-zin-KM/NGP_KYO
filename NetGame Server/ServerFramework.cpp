#include "Common.h"         
#include "ServerFramework.h"

// 메인 스레드 루프
void AcceptLoop(SOCKET listenSocket)
{
    while (1)
    {
        printf("[AcceptLoop] Waiting for %d players...\n", MAX_PLAYERS_PER_ROOM);

        SOCKET tempSockets[MAX_PLAYERS_PER_ROOM];
        g_GameRoom.connectedPlayers = 0;

        bool matchingComplete = false;
        time_t firstConnectionTime = 0;

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

            if (selResult == SOCKET_ERROR) {
                err_display("Select() failed!");
                break;
            }
            if (selResult == 0) {
                HandleMatchingTimeout(tempSockets, g_GameRoom.connectedPlayers);
                break;
            }

            if (FD_ISSET(listenSocket, &readSet))
            {
                SOCKET clientSocket = accept(listenSocket, NULL, NULL);
                if (clientSocket == INVALID_SOCKET) {
                    err_display("Accept failed!");
                    continue;
                }

                // 락 사용해서 gameroom 보호
                EnterCriticalSection(&g_GameRoom.lock);
                int index = g_GameRoom.connectedPlayers;
                g_GameRoom.players[index].socket = clientSocket;
                g_GameRoom.players[index].id = index;
                g_GameRoom.players[index].isConnected = true;

                // 초기 위치 초기화
                g_GameRoom.players[index].x = 400 + (index * 100);
                g_GameRoom.players[index].y = 400;
                g_GameRoom.players[index].direct = 3;
                g_GameRoom.players[index].life = true;

                g_GameRoom.players[index].ammo = MAX_BULLETS;

                g_GameRoom.connectedPlayers++;
                LeaveCriticalSection(&g_GameRoom.lock);

                tempSockets[index] = clientSocket;

                if (index == 0) {
                    firstConnectionTime = time(NULL);
                }

                printf("[AcceptLoop] Player %d connected. (%d/%d)\n",
                    index, g_GameRoom.connectedPlayers, MAX_PLAYERS_PER_ROOM);

                if (g_GameRoom.connectedPlayers == MAX_PLAYERS_PER_ROOM) {
                    matchingComplete = true;
                }
            }
        }


        if (matchingComplete)
        {
            printf("Matching complete! Starting Game...\n");

            InitEnemies();

            HANDLE hGameLoopThread = (HANDLE)_beginthreadex(NULL, 0, GameLoopThread, NULL, 0, NULL);
            if (hGameLoopThread) CloseHandle(hGameLoopThread);

            for (int i = 0; i < MAX_PLAYERS_PER_ROOM; ++i)
            {
                HANDLE hClientThread = (HANDLE)_beginthreadex(NULL, 0, ClientThread, (void*)i, 0, NULL);
                if (hClientThread) CloseHandle(hClientThread);
            }

            for (int i = 0; i < MAX_PLAYERS_PER_ROOM; ++i)
            {
                SC_MatchingCompletePacket pkt;
                pkt.header.size = sizeof(SC_MatchingCompletePacket);
                pkt.header.type = S_MATCH_COMPLETE; // 1번
                pkt.yourPlayerID = i;

                send(g_GameRoom.players[i].socket, (char*)&pkt, sizeof(pkt), 0);
            }
        }
    }
}

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

        PacketHeader* header = (PacketHeader*)buffer;
        short packetType = header->type;
        short packetSize = header->size;


        EnterCriticalSection(&g_GameRoom.lock);
        {
            if (packetType == C_MOVE) { // 10번
                ProcessPlayerMove(playerIndex, buffer);
            }
            else if (packetType == C_ATTACK) {
                ProcessPlayerAttack(playerIndex, buffer);
            }
        }
        LeaveCriticalSection(&g_GameRoom.lock);
    }
    return 0;
}

// LLD 3. GameLoop 스레드
unsigned __stdcall GameLoopThread(void* arg)
{
    printf("[GameLoopThread] Started.\n");

    while (1)
    {
        EnterCriticalSection(&g_GameRoom.lock);
        {
            UpdateBullets();

            UpdateEnemyAI();
            CheckCollisions();
        }
        LeaveCriticalSection(&g_GameRoom.lock);

        BroadcastPacket(NULL, 0);

        if (CheckGameEndConditions() == true) {
            break;
        }
        Sleep(10); // 프레임 올려뒀음 16 -> 10
    }

    // 종료 처리...
    return 0;
}
// 타임아웃 규칙 
void HandleMatchingTimeout(SOCKET tempSockets[], int count) {
    printf("Matching time out. Closing sockets.\n");
    char timeoutPacket[] = "Matching Failed";

    for (int i = 0; i < count; i++) {
        send(tempSockets[i], timeoutPacket, sizeof(timeoutPacket), 0);
        closesocket(tempSockets[i]);
    }
}