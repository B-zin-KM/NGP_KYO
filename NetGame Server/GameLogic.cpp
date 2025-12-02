#include "Common.h"
#include "ServerFramework.h" 
#include "SharedData.h"

#define MOVE_SPEED 2 // 플레이어 이동 체크
#define BULLET_SPEED 15 // 총알 속도
#define MAX_AMMO 100 // 일단 총알 개수

#define BOARD_SIZE 35
#define PLAYER_SIZE 30
#define BULLET_LEN 18   // bulletlen
#define BULLET_THICK 8  // bulletthick


SERVER_BOARD g_Board[150];
void InitGameMap() {

    int boardnum = 0;
    // 1. 전체 맵 생성
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 15; j++) {
            g_Board[boardnum].x = j * BOARD_SIZE + 335;
            g_Board[boardnum].y = i * BOARD_SIZE + 240;
            g_Board[boardnum].value = false;
            boardnum++;
        }
    }

    // 2. 초기 이동 가능 구역 설정 (하얀색)
    // (클라의 j, k 루프와 동일)
    for (int j = 3; j < 7; j++) {
        for (int k = 5; k < 10; k++) {
            g_Board[j * 15 + k].value = true;
        }
    }
    printf("[Server Debug] 맵 생성 완료!\n");
}


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

void InitEnemies() {
    // (게임 시작 시 한 번 호출됨)
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        g_GameRoom.enemies[i].life = true;

        // 맵 크기(0~900, 0~800) 안에서 랜덤 위치
        g_GameRoom.enemies[i].x = rand() % 800 + 50;
        g_GameRoom.enemies[i].y = rand() % 700 + 50;

        g_GameRoom.enemies[i].direct = 0;
    }
    printf("[SERVER] 10 Enemies Spawned!\n");
}

void UpdateEnemyAI() {
    // 모든 적에 대해 반복
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        EnemyState* e = &g_GameRoom.enemies[i];
        if (e->life == false) continue; // 죽은 적은 무시

        // 1) 가장 가까운 플레이어 찾기
        int targetIndex = -1;
        double minDistance = 999999.0;

        for (int p = 0; p < MAX_PLAYERS_PER_ROOM; p++)
        {
            Player* player = &g_GameRoom.players[p];
            if (player->life == false || player->isConnected == false) continue;

            // 거리 계산 (피타고라스 정리)
            double dist = sqrt(pow(player->x - e->x, 2) + pow(player->y - e->y, 2));

            if (dist < minDistance) {
                minDistance = dist;
                targetIndex = p;
            }
        }

        // 2) 추적 이동 (타겟이 있다면)
        if (targetIndex != -1)
        {
            Player* target = &g_GameRoom.players[targetIndex];

            // X축 이동
            if (e->x < target->x) e->x += ENEMY_SPEED;
            else if (e->x > target->x) e->x -= ENEMY_SPEED;

            // Y축 이동
            if (e->y < target->y) e->y += ENEMY_SPEED;
            else if (e->y > target->y) e->y -= ENEMY_SPEED;
        }
    }
}

// 충돌체크 함수 통합
bool CheckRectCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

void CheckCollisions()
{

    //플레이어 vs 적 충돌 검사
    for (int i = 0; i < MAX_PLAYERS_PER_ROOM; ++i) {
        // 접속 안 했거나 이미 죽은 플레이어는 무시
        if (!g_GameRoom.players[i].isConnected || !g_GameRoom.players[i].life) continue;

        // (옵션) 무적 상태라면 충돌 안 함
        // if (g_GameRoom.players[i].moojuk) continue;

        for (int j = 0; j < MAX_ENEMIES; ++j) {
            if (!g_GameRoom.enemies[j].life) continue;

            // 충돌 판정 (플레이어 크기 vs 적 크기)
            // (둘 다 PLAYER_SIZE=30 이라고 가정)
            if (CheckRectCollision(
                (int)g_GameRoom.players[i].x, (int)g_GameRoom.players[i].y, PLAYER_SIZE, PLAYER_SIZE,
                (int)g_GameRoom.enemies[j].x, (int)g_GameRoom.enemies[j].y, PLAYER_SIZE, PLAYER_SIZE))
            {
                // --- 충돌 발생! ---
                printf("[Crash] Player %d touched Enemy %d -> DIE\n", i, j);

                // 1. 플레이어 사망 처리
                g_GameRoom.players[i].life = false;

                // (선택 사항) 적도 같이 죽일지? -> 일단 플레이어만 죽게 둠
                // g_GameRoom.enemies[j].life = false; 

                // 2. 폭발 이펙트 전송 (플레이어 위치)
                S_ExplosionPacket expPkt;
                expPkt.header.size = sizeof(S_ExplosionPacket);
                expPkt.header.type = S_EXPLOSION; 
                expPkt.x = (int)g_GameRoom.players[i].x + 15; // 중심점
                expPkt.y = (int)g_GameRoom.players[i].y + 15;
                expPkt.size = 25; 
                expPkt.type = 1;  
                expPkt.playerID = j;

                BroadcastPacket((char*)&expPkt, expPkt.header.size);
            }
        }
    }
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

        // (3) 벽 충돌 체크
        int bW = (b->direct == 1 || b->direct == 2) ? BULLET_SIZE_W : BULLET_SIZE_H;
        int bH = (b->direct == 1 || b->direct == 2) ? BULLET_SIZE_H : BULLET_SIZE_W;

        for (int k = 0; k < 150; k++) {

            if (g_Board[k].value == true) continue;

            if (CheckRectCollision(b->x, b->y, bW, bH,
                g_Board[k].x, g_Board[k].y, BOARD_SIZE, BOARD_SIZE))
            {
                printf("벽vs총알 충돌! \n");
                g_Board[k].value = true;
                break;
            }
        }

        for (int j = 0; j < MAX_ENEMIES; ++j) {
            if (g_GameRoom.enemies[j].life == false) continue; // 죽은 적은 패스

            // 적 크기는 PLAYER_SIZE (30)
            if (CheckRectCollision((int)b->x, (int)b->y, bW, bH,
                (int)g_GameRoom.enemies[j].x, (int)g_GameRoom.enemies[j].y, PLAYER_SIZE, PLAYER_SIZE))
            {
                // 충돌 발생!
                printf("[Hit] Enemy %d killed by Bullet %d\n", j, i);

                // 상태 변경
                b->active = false;               // 총알 삭제
                g_GameRoom.enemies[j].life = false; // 적 사망

                // 폭발 패킷 전송
                S_ExplosionPacket expPkt;
                expPkt.header.size = sizeof(S_ExplosionPacket);
                expPkt.header.type = S_EXPLOSION; // 103번
                expPkt.x = (int)g_GameRoom.enemies[j].x + 15;
                expPkt.y = (int)g_GameRoom.enemies[j].y + 15;
                expPkt.size = 20;
                expPkt.type = 0; // 0: 적 사망

                BroadcastPacket((char*)&expPkt, expPkt.header.size);

                break; // 총알 하나가 적 하나만 죽이고 사라짐
            }
        }
    }
}

void BroadcastPacket(char* packet, int size)
{
    if (packet != NULL)
    {
        for (int i = 0; i < MAX_PLAYERS_PER_ROOM; i++)
        {
            if (g_GameRoom.players[i].isConnected) {
                send(g_GameRoom.players[i].socket, packet, size, 0);
            }
        }
        return; 
    }

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

        }
        LeaveCriticalSection(&g_GameRoom.lock);

        for (int i = 0; i < MAX_BULLETS; i++) {
            statePkt.bullets[i] = g_GameRoom.bullets[i]; // 통째로 복사
        }

        for (int i = 0; i < MAX_ENEMIES; i++) {
            statePkt.enemies[i] = g_GameRoom.enemies[i];
        }

        for (int i = 0; i < 150; i++) {
            statePkt.board[i] = g_Board[i].value;
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