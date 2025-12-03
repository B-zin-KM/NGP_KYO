#pragma once
#include "Common.h"

#define PORT 9000
#define MAX_PLAYERS_PER_ROOM 3
#define MATCHING_TIMEOUT_SEC 30 
#define MAX_ENEMIES 10
#define ENEMY_SPEED 1

#define BOARD_SIZE 35
#define PLAYER_SIZE 30
#define BULLET_SIZE_W 18 
#define BULLET_SIZE_H 8

// 큐 관련 상수 
#define MAX_PACKET_QUEUE_SIZE 200 
#define MAX_PACKET_DATA_SIZE 2048

#define MAX_BULLETS 50 // 화면에 동시에 날아다닐 수 있는 최대 총알 수

#define GAME_DURATION_SEC 120 // 게임시간 120초 설정

// 클라와 통일하기 위해 1바이트 정렬
#pragma pack(push, 1)

// 패킷 헤더 정의
typedef struct {
    short size;
    short type;
} PacketHeader;

typedef struct {
    bool active;
    int x, y;
    int direct;
    int ownerID;
}BulletState;

// 1. 매칭 완료 패킷
#define S_MATCH_COMPLETE 1
typedef struct {
    PacketHeader header;
    int yourPlayerID;
} SC_MatchingCompletePacket;

// 2. 게임 상태 전체 패킷
#define S_GAME_STATE 2
typedef struct {
    bool life;
    int x, y;
    int direct;
    int ammo;
} PlayerState;

typedef struct {
    bool life;
    int x, y;
    int direct;
} EnemyState;

typedef struct {
    PacketHeader header;
    PlayerState players[MAX_PLAYERS_PER_ROOM];
    EnemyState enemies[MAX_ENEMIES];

    BulletState bullets[MAX_BULLETS];

    bool board[150];

    int remainingTime;

} S_GameStatePacket;

#define S_LOBBY_UPDATE 4
typedef struct {
    PacketHeader header;
    int connectedCount;
    struct {
        bool connected;
        bool isReady;
    } players[MAX_PLAYERS_PER_ROOM];
} S_LobbyUpdatePacket;

#define S_GAME_START 5
typedef struct {
    PacketHeader header;
} S_GameStartPacket;

#define C_REQ_READY 3
typedef struct {
    PacketHeader header;
} C_ReqReadyPacket;

#define S_EXPLOSION 6
typedef struct {
	PacketHeader header;
    int x, y;      // 폭발 위치
    int size;      // 폭발 크기 (기본 15)
    int type;      // 0: 적 사망, 1: 플레이어 사망 
    int playerID;
} S_ExplosionPacket;

// 10. 이동 요청 패킷
#define C_MOVE 10
typedef struct {
    PacketHeader header;
    int direction;
} C_MovePacket;

// 11. 공격 요청 패킷
#define C_ATTACK 11
typedef struct {
    PacketHeader header;
    int direction;
} C_AttackPacket;

#pragma pack(pop) // 정렬 끝

typedef struct {
    int playerIndex; // 누가 보냈는가
    int dataSize;    // 실제 데이터 크기
    char data[MAX_PACKET_DATA_SIZE]; // 실제 데이터
} Packet;

typedef struct {
    Packet packets[MAX_PACKET_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    CRITICAL_SECTION lock;
} PacketQueue;
// ==========================================================

// 서버 내부 보드 구조체
struct SERVER_BOARD {
    int x, y;
    bool value; // TRUE: 이동 가능(흰색), FALSE: 벽(검은색)
};
extern SERVER_BOARD g_Board[150];

// 서버 내부 Player 구조체
typedef struct {
    SOCKET socket;
    int id;
    bool isConnected;

    // TCP 수신 버퍼용 (필요하다면 추가)
    char recvBuffer[2048];
    int bufferPos;

    // 게임 로직용 변수
    int x, y;
    int direct;
    bool life;
    int ammo;

	bool isReady; // 로비 준비 상태
} Player;

typedef struct {
    Player players[MAX_PLAYERS_PER_ROOM];
    int connectedPlayers;

    CRITICAL_SECTION lock;

    BulletState bullets[MAX_BULLETS];

    EnemyState enemies[MAX_ENEMIES];

    bool board[150];

    time_t gameStartTime;

} GameRoom;

extern GameRoom g_GameRoom;

extern PacketQueue g_PacketQueue;