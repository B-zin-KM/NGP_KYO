#pragma once
#include "Common.h"

#define PORT 9000
#define MAX_PLAYERS_PER_ROOM 3
#define MATCHING_TIMEOUT_SEC 30 
#define MAX_ENEMIES 10

// 큐 관련 상수 (이것도 필요합니다)
#define MAX_PACKET_QUEUE_SIZE 200 
#define MAX_PACKET_DATA_SIZE 1024 

#define MAX_BULLETS 50 // 화면에 동시에 날아다닐 수 있는 최대 총알 수

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
} S_GameStatePacket;


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

} Player;

typedef struct {
    Player players[MAX_PLAYERS_PER_ROOM];
    int connectedPlayers;

    CRITICAL_SECTION lock;

    BulletState bullets[MAX_BULLETS];

    // (적 정보 등 추가 가능)

} GameRoom;

extern GameRoom g_GameRoom;

extern PacketQueue g_PacketQueue;