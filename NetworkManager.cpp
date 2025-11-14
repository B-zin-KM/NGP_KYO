#include "NetworkManager.h"
#include <stdio.h> 


NetworkManager::NetworkManager()
{
    serverSocket = INVALID_SOCKET;
}

NetworkManager::~NetworkManager()
{
    Cleanup();
}

bool NetworkManager::Init(HWND hWnd)
{
    // 1. WSAStartup
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        MessageBox(hWnd, L"WSAStartup failed", L"Error", MB_OK);
        return false;
    }

    // 2. socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        MessageBox(hWnd, L"socket() failed", L"Error", MB_OK);
        return false;
    }

    // 3. connect
    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(7777); // 서버 포트
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // 서버 IP

    if (connect(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        MessageBox(hWnd, L"connect() failed", L"Error", MB_OK);
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
        return false; // 연결 실패
    }

    printf("서버 연결 성공!\n");
    return true; // 연결 성공
}

void NetworkManager::Cleanup()
{
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    WSACleanup();
}

void NetworkManager::SendPacket(char* packet, int size)
{
    if (serverSocket == INVALID_SOCKET) {
        return; // 연결이 안되어있으면 무시
    }

    int result = send(serverSocket, packet, size, 0);
    if (result == SOCKET_ERROR) {
        printf("send error: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
}

void NetworkManager::SendMovePacket(int direction)
{
    C_MovePacket pkt;
    pkt.size = sizeof(C_MovePacket);
    pkt.type = C_MOVE;
    pkt.direction = direction;
    SendPacket((char*)&pkt, pkt.size);
}

void NetworkManager::SendAttackPacket(int direction)
{
    C_AttackPacket pkt;
    pkt.size = sizeof(C_AttackPacket);
    pkt.type = C_ATTACK;
    pkt.direction = direction;
    SendPacket((char*)&pkt, pkt.size);
}