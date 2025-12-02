#pragma once

#define MAX_PLAYERS 3
#define MAX_ENEMIES 10
#define MAX_BULLETS 50
#define ENEMY_SPEED 1
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

struct BulletState {
    bool active; // 활성화 여부
    int x, y;
    int direct;  // 방향
    int ownerID; // 주인 ID
};

struct S_GameStatePacket : public PacketHeader {
    // 플레이어 3명의 정보
    PlayerState players[MAX_PLAYERS];
    // 적 10마리의 정보
    EnemyState enemies[MAX_ENEMIES];
    // 총알 정보 (MAX 50)
    BulletState bullets[MAX_BULLETS];
    // 보드 정보
    bool board[150];
};

#define C_REQ_READY 3  
struct C_ReqReadyPacket : public PacketHeader {
    // 내용 없음 
};

#define S_LOBBY_UPDATE 4
struct S_LobbyUpdatePacket : public PacketHeader {
    int connectedCount; // 현재 접속 인원
    struct LobbyPlayerInfo {
        bool connected; // 접속 여부
        bool isReady;   // 준비 완료 여부
    } players[MAX_PLAYERS];
};

#define S_GAME_START 5
struct S_GameStartPacket : public PacketHeader {
    // 내용 없음 (받으면 게임 화면으로 전환)
};

#define S_EXPLOSION 6
struct S_ExplosionPacket : public PacketHeader { 
    int x, y;      // 폭발 위치
    int size;      // 폭발 크기 (기본 15)
    int type;      // 0: 적 사망, 1: 플레이어 사망 
    int playerID;
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

// 정렬 해제
#pragma pack(pop)