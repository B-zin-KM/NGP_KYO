#pragma once
#include "Common.h"
#include "SharedData.h"

// 서버 스레드 

void AcceptLoop(SOCKET listenSocket);
unsigned __stdcall ClientThread(void* arg);
unsigned __stdcall GameLoopThread(void* arg);
// __stdcall은 스택정리를 위한 규약임 beginthreadex 쓸거면 반드시 써야됨

void ProcessPlayerMove(int playerIndex, char* data);
void ProcessPlayerAttack(int playerIndex, char* data);
void UpdateEnemyAI();
void InitGameMap();
void InitEnemies();
void UpdateBullets();
bool CheckRectCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
void  CheckCollisions();
bool CheckGameEndConditions();
void BroadcastPacket(char* packet, int size);
void ProcessPacket(Packet* pkt);

void HandleMatchingTimeout(SOCKET tempSockets[], int count);