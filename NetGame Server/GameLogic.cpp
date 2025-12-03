#include "Common.h"
#include "ServerFramework.h" 
#include "SharedData.h"

#define MOVE_SPEED 2 // 플레이어 이동 체크
#define BULLET_SPEED 15 // 총알 속도
#define MAX_AMMO 100 // 일단 총알 개수

#define MAX_BOARD_x 32   // 보드칸 가로 개수
#define MAX_BOARD_y 22   // 보드칸 세로 개수
#define BOARD_SIZE 35    // 보드칸 크기
#define PLAYER_SIZE 30
#define BULLET_LEN 18   // bulletlen
#define BULLET_THICK 8  // bulletthick

#define GAME_LIMIT_SEC 120


SERVER_BOARD g_Board[MAX_BOARD];
void InitGameMap() {

    int boardnum = 0;
    // 1. 전체 맵 생성
    for (int i = 0; i < MAX_BOARD_y; i++) {
        for (int j = 0; j < MAX_BOARD_x; j++) {
            g_Board[boardnum].x = j * BOARD_SIZE + 135;
            g_Board[boardnum].y = i * BOARD_SIZE + 140;
            g_Board[boardnum].value = false;
            boardnum++;
        }
    }

    // --- 2. 초기 하얀타일 설정 (3군데: 좌상단, 우상단, 우하단) ---

    int width = MAX_BOARD_x;
    int height = MAX_BOARD_y;

    // 1. 좌상단 (Top-Left) 3x3
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            g_Board[y * width + x].value = true;
        }
    }
    // 2. 우상단 (Top-Right) 3x3
    for (int y = 0; y < 3; y++) {
        for (int x = width - 3; x < width; x++) {
            g_Board[y * width + x].value = true;
        }
    }
    // 3. 우하단 (Bottom-Right) 3x3
    for (int y = height - 3; y < height; y++) {
        for (int x = width - 3; x < width; x++) {
            g_Board[y * width + x].value = true;
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

        // 맵 크기(0~1400, 0~1000) 안에서 랜덤 위치
        g_GameRoom.enemies[i].x = rand() % 1300 + 50;
        g_GameRoom.enemies[i].y = rand() % 900 + 50;

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
        // 맵 전체 타일(150개)을 검사
        for (int k = 0; k < 150; k++)
        {
            // 1. 이미 검정색(false)인 타일은 건드리지 않음 (최적화 & 조건 만족)
            if (g_Board[k].value == false) continue;

            // 2. 적과 타일이 조금이라도 겹치는지 검사 (AABB 충돌)
            // 적 크기: PLAYER_SIZE(30), 타일 크기: BOARD_SIZE(35)
            if (CheckRectCollision(
                (int)e->x, (int)e->y, PLAYER_SIZE, PLAYER_SIZE,
                g_Board[k].x, g_Board[k].y, BOARD_SIZE, BOARD_SIZE))
            {
                // 3. 겹쳤다면 검정색(false)으로 변경
                g_Board[k].value = false;

            }
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
                g_GameRoom.players[i].score -= 100;
                //g_GameRoom.players[i].deadTime = GetTickCount();

                // 2. 폭발 이펙트 전송 (플레이어 위치)
                S_ExplosionPacket expPkt;
                expPkt.header.size = sizeof(S_ExplosionPacket);
                expPkt.header.type = S_EXPLOSION; 
                expPkt.x = (int)g_GameRoom.players[i].x + 15; // 중심점
                expPkt.y = (int)g_GameRoom.players[i].y + 15;
                expPkt.size = 25; 
                expPkt.type = 1;  
                expPkt.playerID = i;

                BroadcastPacket((char*)&expPkt, expPkt.header.size);
            }
        }
    }
}

bool CheckGameEndConditions() {
    time_t currentTime = time(NULL);
    int elapsed = (int)(currentTime - g_GameRoom.gameStartTime);

    // 시간이 다 됐으면 true 반환 -> 서버 스레드 종료
    if (elapsed >= GAME_LIMIT_SEC) {
        printf("[SERVER] Time Over! (120s passed)\n");
        return true;
    }

    return false;
}

void UpdateBullets() {

    // 맵 경계 설정
    const int MAP_LEFT = 135;
    const int MAP_TOP = 140;
    const int MAP_RIGHT = MAP_LEFT + (MAX_BOARD_x * BOARD_SIZE);
    const int MAP_BOTTOM = MAP_TOP + (MAX_BOARD_y * BOARD_SIZE);

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
        if (b->x < MAP_LEFT || b->x > MAP_RIGHT ||
            b->y < MAP_TOP || b->y > MAP_BOTTOM) {
            b->active = false;
        }

        // (3) 벽 충돌 체크
        int bW = (b->direct == 1 || b->direct == 2) ? BULLET_SIZE_W : BULLET_SIZE_H;
        int bH = (b->direct == 1 || b->direct == 2) ? BULLET_SIZE_H : BULLET_SIZE_W;

        for (int k = 0; k < MAX_BOARD; k++) {

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
                // g_GameRoom.enemies[j].life = false; // 리스폰으로 할거니까 잠시 
                int deadX = (int)g_GameRoom.enemies[j].x;
                int deadY = (int)g_GameRoom.enemies[j].y; // 죽은위치를 백업 

                int centerX = deadX + 15; // 적의 중심점 (크기 30의 절반)
                int centerY = deadY + 15;
                int blastRadius = 45;     // 폭발 반경 (이 값을 조절하면 파괴 범위가 달라짐)
                int blastSize = blastRadius * 2;

                for (int k = 0; k < MAX_BOARD; k++) {
                    // 이미 하얀색(뚫린 곳)이면 패스
                    if (g_Board[k].value == true) continue;

                    // 폭발 범위(사각형)와 벽돌이 겹치는지 확인
                    if (CheckRectCollision(
                        centerX - blastRadius, centerY - blastRadius, blastSize, blastSize,
                        g_Board[k].x, g_Board[k].y, BOARD_SIZE, BOARD_SIZE))
                    {
                        g_Board[k].value = true; // 벽 파괴! (하얀색으로 변경)
                    }
                }

				// 점수 추가
                int owner = b->ownerID;
                if (owner >= 0 && owner < MAX_PLAYERS_PER_ROOM) {
                    g_GameRoom.players[owner].score += 500;
                    printf("[Score] P%d Score: %d\n", owner, g_GameRoom.players[owner].score);
                }

                g_GameRoom.enemies[j].x = rand() % 1300 + 50;
                g_GameRoom.enemies[j].y = rand() % 900 + 50;

                g_GameRoom.enemies[j].life = true;

                printf("[Respawn] Enemy %d moved to (%d, %d)\n", j, g_GameRoom.enemies[j].x, g_GameRoom.enemies[j].y);

                // 폭발 패킷 전송
                S_ExplosionPacket expPkt;
                expPkt.header.size = sizeof(S_ExplosionPacket);
                expPkt.header.type = S_EXPLOSION; // 103번
                expPkt.x = deadX+ 15;
                expPkt.y = deadY + 15;
                expPkt.size = 20;
                expPkt.type = 0; // 0: 적 사망
                expPkt.playerID = -1;

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
            statePkt.players[i].score = g_GameRoom.players[i].score;

        }
        time_t currentTime = time(NULL);
        int elapsed = (int)(currentTime - g_GameRoom.gameStartTime);
        int timeLeft = GAME_LIMIT_SEC - elapsed;

        if (timeLeft < 0) timeLeft = 0;
        statePkt.remainingTime = timeLeft;

        LeaveCriticalSection(&g_GameRoom.lock);

        for (int i = 0; i < MAX_BULLETS; i++) {
            statePkt.bullets[i] = g_GameRoom.bullets[i]; // 통째로 복사
        }

        for (int i = 0; i < MAX_ENEMIES; i++) {
            statePkt.enemies[i] = g_GameRoom.enemies[i];
        }

        for (int i = 0; i < MAX_BOARD; i++) {
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