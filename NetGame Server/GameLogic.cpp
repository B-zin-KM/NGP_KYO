#include "Common.h"
#include "ServerFramework.h" 
#include "SharedData.h"

#define MOVE_SPEED 5 // 플레이어 이동 체크

void ProcessPlayerMove(int playerIndex, char* data) {
    C_MovePacket* pkt = (C_MovePacket*)data;
    Player* p = &g_GameRoom.players[playerIndex];

    int oldX = p->x;
    int oldY = p->y;

    switch (pkt->direction)
    {
    case 1: p->x += MOVE_SPEED; p->direct = 1; break;
    case 2: p->x -= MOVE_SPEED; p->direct = 2; break;
    case 3: p->y += MOVE_SPEED; p->direct = 3; break;
    case 4: p->y -= MOVE_SPEED; p->direct = 4; break;
    }

    printf("[SERVER] P%d MoveReq: Dir %d | (%d, %d) -> (%d, %d)\n",
        playerIndex, pkt->direction, oldX, oldY, p->x, p->y);
}

void ProcessPlayerAttack(int playerIndex, char* data) {

}

void UpdateEnemyAI() {

}

void CheckCollisions() {

}

bool CheckGameEndConditions() {
    return false;
}

void BroadcastPacket(char* packet, int size)
{
  
    if (packet == NULL)
    {
        S_GameStatePacket statePkt;

        statePkt.header.size = sizeof(S_GameStatePacket);
        statePkt.header.type = S_GAME_STATE; // 2번 패킷
        EnterCriticalSection(&g_GameRoom.lock); // 데이터 읽으니까 잠금
        for (int i = 0; i < MAX_PLAYERS_PER_ROOM; i++)
        {
            statePkt.players[i].x = g_GameRoom.players[i].x;
            statePkt.players[i].y = g_GameRoom.players[i].y;
            statePkt.players[i].direct = g_GameRoom.players[i].direct;
            statePkt.players[i].life = true; // 일단 다 살았다고 가정

            // 나중에 적 정보도 여기서 복사
        }
        LeaveCriticalSection(&g_GameRoom.lock);

        for (int i = 0; i < MAX_PLAYERS_PER_ROOM; i++)
        {
            if (g_GameRoom.players[i].isConnected) {
                send(g_GameRoom.players[i].socket, (char*)&statePkt, sizeof(statePkt), 0);
            }
        }
    }
}

void ProcessPacket(Packet* pkt)
{
    PacketHeader* pHeader = (PacketHeader*)pkt->data;

    switch (pHeader->type)
    {
    case C_MOVE: // 10번
        // 디버깅
        printf("[Packet] P%d Move Request\n", pkt->playerIndex);
        ProcessPlayerMove(pkt->playerIndex, pkt->data);
        break;

    case C_ATTACK: // 11번
        printf("[Packet] P%d Attack Request\n", pkt->playerIndex);
        ProcessPlayerAttack(pkt->playerIndex, pkt->data);
        break;

    }
}