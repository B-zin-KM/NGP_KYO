#pragma once

// 1바이트 크기로 정렬
#pragma pack(push, 1) 

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
// (나중에 서버가 보낼 패킷도 여기에 추가)


#pragma pack(pop)