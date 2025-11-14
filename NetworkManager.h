#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h> // Winsock 헤더
#include <ws2tcpip.h> // inet_pton
//#include <windows.h>  // HWND (에러 메시지창)
#include "protocol.h" 

#pragma comment(lib, "ws2_32.lib")

class NetworkManager
{
private:
    SOCKET serverSocket; // 서버와의 연결 소켓

public:
    NetworkManager();  
    ~NetworkManager(); 

    // 1. 초기화 (WSAStartup, connect)
    bool Init(HWND hWnd);

    // 2. 종료 (closesocket, WSACleanup)
    void Cleanup();

    // 3. 핵심 전송 함수 (기존 SendPacketToServer)
    void SendPacket(char* packet, int size);

    // 4. 헬퍼 함수
    void SendMovePacket(int direction);
    void SendAttackPacket(int direction);
};