#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h> // Winsock 헤더
#include <ws2tcpip.h> // inet_pton
//#include <windows.h>  // HWND (에러 메시지창)
#include <queue>         
#include <mutex>
#include "protocol.h" 

#pragma comment(lib, "ws2_32.lib")

class NetworkManager
{
private:
    SOCKET serverSocket; // 서버와의 연결 소켓

    // --- 수신 스레드용 ---
    HANDLE hRecvThread;
    volatile bool bIsRunning; // 스레드 중지 플래그

    // --- 패킷 큐 (스레드 안전) ---
    std::queue<char*> packetQueue; // (패킷 데이터 포인터)
    std::mutex queueMutex;         // (큐 전용 잠금)

    // --- 수신 스레드 함수 (static으로 선언) ---
    static unsigned __stdcall RecvThread(void* arg);

public:
    NetworkManager();  
    ~NetworkManager(); 

    // 1. 초기화 (WSAStartup, connect)
    bool Init(HWND hWnd);

    // 2. 종료 (closesocket, WSACleanup)
    void Cleanup();

    // 3. 핵심 전송 함수 
    void SendPacket(char* packet, int size);

    // 4. 헬퍼 함수
    void SendMovePacket(int direction);
    void SendAttackPacket(int direction);

    // --- 큐 관련 함수 ---
    void PushPacket(char* packetData);
    bool PopPacket(char* buffer, int bufferSize);
};