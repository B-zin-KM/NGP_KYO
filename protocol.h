#pragma once

#define MAX_PLAYERS 3
#define MAX_ENEMIES 10

// 1바이트 정렬 시작
#pragma pack(push, 1)

struct PacketHeader {
    short size; // 패킷 전체 크기
    short type; // 패킷 종류 (ID)
};

// ==========================================================
// [서버 -> 클라이언트] (번호: 1 ~ 9)
// ==========================================================

// 1. 매칭 성공 패킷
#define S_MATCH_COMPLETE 1
struct S_MatchCompletePacket : public PacketHeader {
    int yourPlayerID; // 0, 1, 2 중 하나
};

// 2. 게임 상태 전체 패킷 (적, 플레이어 위치 등)
#define S_GAME_STATE 2

struct PlayerState {
    bool life;
    int x, y;
    int direct; // 방향
    int ammo;   // 총알 개수
};

struct EnemyState {
    bool life;
    int x, y;
    int direct;
};

struct S_GameStatePacket : public PacketHeader {
    // 플레이어 3명의 정보
    PlayerState players[MAX_PLAYERS];
    // 적 10마리의 정보
    EnemyState enemies[MAX_ENEMIES];
};


// ==========================================================
// [클라이언트 -> 서버] (번호: 10 ~ 19)
// ==========================================================

// 10. 이동 요청 패킷
#define C_MOVE 10
struct C_MovePacket : public PacketHeader {
    int direction; // 1:D(우), 2:A(좌), 3:S(하), 4:W(상)
};

// 11. 공격 요청 패킷
#define C_ATTACK 11
struct C_AttackPacket : public PacketHeader {
    int direction; // 1:Right, 2:Left, 3:Down, 4:Up
};

// [중요] 정렬 해제
#pragma pack(pop)