#pragma once

#define MAX_PLAYERS 3
#define MAX_ENEMIES 10

// --- 공용 헤더 ---
struct PacketHeader {
	short size; // 전체 패킷 크기
	short type; // 패킷 타입
};

// --- 클라이언트 -> 서버 (C_XXXX) ---

// 1. 이동 패킷 (WASD)
#define C_MOVE 1
struct C_MovePacket : public PacketHeader {
	int direction; // 1:D, 2:A, 3:S, 4:W
};

// 2. 공격(총알 발사) 패킷 (방향키)
#define C_ATTACK 2
struct C_AttackPacket : public PacketHeader {
	int direction; // 1:Right, 2:Left, 3:Down, 4:Up
};

// --- 서버 -> 클라이언트 (S_XXXX) ---
// (1) 매칭 성공 시, "당신은 0~2번입니다"라고 알려주는 패킷
#define S_MATCH_COMPLETE 101
struct S_MatchCompletePacket : public PacketHeader {
    int yourPlayerID; // 0, 1, 2 중 하나
};

// (2) 게임의 핵심 상태를 16ms마다 통째로 보내는 패킷
struct PlayerState {
    bool life;
    int x, y;
    int ammo;
};
struct EnemyState {
    bool life;
    int x, y;
};

#define S_GAME_STATE 102
struct S_GameStatePacket : public PacketHeader {
    // 모든 플레이어의 상태
    PlayerState players[MAX_PLAYERS];
    // 모든 적의 상태
    EnemyState enemies[MAX_ENEMIES];
    // ( 총알, 보드, 점수 등 추가)
};

#pragma pack(pop)