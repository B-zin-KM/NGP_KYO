#pragma once
#include "Common.h"

#define PORT 9000
#define MAX_PLAYERS_PER_ROOM 3
#define MATCHING_TIMEOUT_SEC 30 // 우리가 하기로한 매칭 타임아웃 규칙 선언

// 클라와 통일하기 위해
#pragma pack(push, 1)

// 패킷 헤더 정의 (클라와 통일한거)
typedef struct {
    short size;
    short type;
} PacketHeader;

typedef struct {
    PacketHeader header;
    int yourPlayerID;
} SC_MatchingCompletePacket;

#pragma pack(pop)

typedef struct {
    SOCKET socket;
    int id;
    // 플레이어 구조체
    // float x, y; 같은거
    bool isConnected;
} Player;

typedef struct {
    Player players[MAX_PLAYERS_PER_ROOM];
    int connectedPlayers;
    
    CRITICAL_SECTION lock;
    
    // 게임 공용데이터 구조체 부분
    // 적같은거 넣으면 될거같음
    // 점수도 넣어 줘야 될듯
    

} GameRoom;

extern GameRoom g_GameRoom;