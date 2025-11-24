#include "Common.h"          
#include "ServerFramework.h" 

GameRoom g_GameRoom;

int main()
{
   
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        err_quit("WSAStartup failed");
    }

    // lock 초기화
    InitializeCriticalSection(&g_GameRoom.lock);

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        err_quit("Listen socket failed");
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        err_quit("Bind failed.");
    }

    
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        err_quit("Listen failed.");
    }

    printf("서버 준비 완료. 대기중 %d...\n", PORT);

    AcceptLoop(listenSocket); // 루프

    closesocket(listenSocket);

    // lock 제거
    DeleteCriticalSection(&g_GameRoom.lock);

    WSACleanup();
    return 0;
}