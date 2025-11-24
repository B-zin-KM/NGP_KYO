#include "NetworkManager.h"
#include <stdio.h> 
#include <windows.h>
#include <process.h> 

NetworkManager::NetworkManager()
{
    serverSocket = INVALID_SOCKET;

    hRecvThread = NULL;    
    bIsRunning = false;
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
    serverAddr.sin_port = htons(9000); // 서버 포트
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // 서버 IP

    if (connect(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        MessageBox(hWnd, L"connect() failed", L"Error", MB_OK);
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
        return false; // 연결 실패
    }

    printf("서버 연결 성공!\n");

    // --- 연결 성공 시 수신 스레드 생성 ---
    bIsRunning = true;
    hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvThread, this, 0, NULL);
    if (hRecvThread == NULL) {
        MessageBox(hWnd, L"RecvThread 생성 실패", L"Error", MB_OK);
        return false;
    }

    return true; 
}

void NetworkManager::Cleanup()
{
    // --- 수신 스레드 종료 ---
    bIsRunning = false; // 1. 스레드 루프 중지 신호

    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket); // 2. recv()를 강제 종료
        serverSocket = INVALID_SOCKET;
    }

    if (hRecvThread != NULL) {
        // 3. 스레드가 완전히 끝날 때까지 1초간 대기
        WaitForSingleObject(hRecvThread, 1000);
        CloseHandle(hRecvThread); // 4. 스레드 핸들 닫기
        hRecvThread = NULL;
    }

    WSACleanup();

    // --- 큐에 남은 패킷 메모리 해제 ---
    std::lock_guard<std::mutex> lock(queueMutex);
    while (!packetQueue.empty()) {
        delete[] packetQueue.front();
        packetQueue.pop();
    }
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

// ---  수신 스레드 함수 (핵심) ---
unsigned __stdcall NetworkManager::RecvThread(void* arg)
{
    NetworkManager* pNetwork = (NetworkManager*)arg;
    SOCKET sock = pNetwork->serverSocket;

    // (TCP는 패킷이 나뉘거나 합쳐져서 오므로, 버퍼에 쌓아두고 파싱해야 함)
    char recvBuffer[2048]; // 수신 버퍼
    int bufferPos = 0;     // 현재 버퍼에 쌓인 데이터 위치

    while (pNetwork->bIsRunning)
    {
        // 1. 버퍼의 남은 공간만큼 데이터 수신
        int recvBytes = recv(sock, recvBuffer + bufferPos, 2048 - bufferPos, 0);

        if (recvBytes == SOCKET_ERROR || recvBytes == 0) {
            // (서버 연결 끊김)
            printf("서버 연결 끊김. RecvThread 종료.\n");
            pNetwork->bIsRunning = false;
            break;
        }

        // 2. 버퍼에 쌓인 데이터 크기 증가
        bufferPos += recvBytes;

        // 3. 버퍼에서 완성된 패킷을 모두 처리
        while (bufferPos >= sizeof(PacketHeader))
        {
            PacketHeader* pHeader = (PacketHeader*)recvBuffer;

            // 3-1. 패킷 하나가 완전히 도착했는지 확인
            if (bufferPos >= pHeader->size)
            {
                // 3-2. 패킷 큐에 넣기 (메모리 복사)
                char* packetData = new char[pHeader->size];
                memcpy(packetData, recvBuffer, pHeader->size);
                pNetwork->PushPacket(packetData); // 큐에 넣음

                // 3-3. 처리한 패킷만큼 버퍼에서 제거 (남은 데이터를 앞으로 당김)
                memmove(recvBuffer, recvBuffer + pHeader->size, bufferPos - pHeader->size);
                bufferPos -= pHeader->size;
            }
            else
            {
                // (아직 패킷이 덜 왔음. 다음 recv() 대기)
                break;
            }
        }
    }
    return 0;
}

// --- 큐 관련 함수 구현 ---
void NetworkManager::PushPacket(char* packetData)
{
    std::lock_guard<std::mutex> lock(queueMutex); // 자동으로 락/언락
    packetQueue.push(packetData);
}

bool NetworkManager::PopPacket(char* buffer, int bufferSize)
{
    std::lock_guard<std::mutex> lock(queueMutex); // 자동으로 락/언락

    if (packetQueue.empty()) {
        return false;
    }

    // 큐에서 패킷 포인터 가져오기
    char* packetData = packetQueue.front();
    PacketHeader* pHeader = (PacketHeader*)packetData;

    if (pHeader->size > bufferSize) {
        // (버퍼가 너무 작아서 패킷을 복사할 수 없음. 에러)
        delete[] packetData; // 메모리 해제
        packetQueue.pop();
        return false;
    }

    // 인자로 받은 버퍼에 패킷 내용 복사
    memcpy(buffer, packetData, pHeader->size);

    // 원본 데이터 삭제
    delete[] packetData;
    packetQueue.pop();

    return true;
}