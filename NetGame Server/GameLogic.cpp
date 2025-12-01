#include "Common.h"
#include "ServerFramework.h" 
#include "SharedData.h"

#define MOVE_SPEED 2 // 플레이어 이동 체크
#define BULLET_SPEED 15 // 총알 속도
#define MAX_AMMO 100 // 일단 총알 개수

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

int GetAmmoCout(int playerIndex) {
    return g_GameRoom.players[playerIndex].ammo;
}

void ProcessPlayerAttack(int playerIndex, char* data) {
    C_AttackPacket* pkt = (C_AttackPacket*)data;
    Player* p = &g_GameRoom.players[playerIndex];

    // 1. 총알이 있는지 확인
    if (p->ammo <= 0) return;

    // 2. 빈 총알 슬롯 찾기
    int bulletIdx = -1;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (g_GameRoom.bullets[i].active == false) {
            bulletIdx = i;
            break;
        }
    }

    // 3. 총알 발사 (빈 슬롯이 있다면)
    if (bulletIdx != -1) {
        BulletState* b = &g_GameRoom.bullets[bulletIdx];
        b->active = true;
        b->ownerID = playerIndex;
        b->direct = pkt->direction; // 클라가 보낸 방향

        // 총알 시작 위치 (플레이어 중앙에서 발사)
        // (클라 game.h의 playersize 30, bulletlen 18 등을 고려)
        b->x = p->x + 10;
        b->y = p->y + 10;

        // 총알 차감
        p->ammo--;

        printf("[SERVER] P%d Shot Bullet! (Rem: %d)\n", playerIndex, p->ammo);
    }
}

void UpdateEnemyAI() {

}

void CheckCollisions() {

}

bool CheckGameEndConditions() {
    return false;
}

void UpdateBullets() {
    // 보드 크기 (game.h 참고: 15칸 * 35픽셀 = 525 + 여백)
    // 일단 간단하게 화면 밖으로 나가면 삭제
    const int MAP_MIN_X = 0;
    const int MAP_MAX_X = 900;
    const int MAP_MIN_Y = 0;
    const int MAP_MAX_Y = 800;

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        BulletState* b = &g_GameRoom.bullets[i];
        if (b->active == false) continue;

        // (1) 이동
        switch (b->direct) {
        case 1: b->x += BULLET_SPEED; break; // 우
        case 2: b->x -= BULLET_SPEED; break; // 좌
        case 3: b->y += BULLET_SPEED; break; // 하
        case 4: b->y -= BULLET_SPEED; break; // 상
        }

        // (2) 맵 밖으로 나가면 삭제
        if (b->x < MAP_MIN_X || b->x > MAP_MAX_X ||
            b->y < MAP_MIN_Y || b->y > MAP_MAX_Y) {
            b->active = false;
        }

        // (3) 벽 충돌 체크 (나중에 구현)
        // if (CheckBulletCollision(b)) b->active = false;
    }
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

        for (int i = 0; i < MAX_BULLETS; i++) {
            statePkt.bullets[i] = g_GameRoom.bullets[i]; // 통째로 복사
        }

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