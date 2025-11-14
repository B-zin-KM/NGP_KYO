#pragma once

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <math.h>
#include "NetworkManager.h"

#define boardsize 35
#define playersize 30
#define bulletlen 18
#define bulletthick 8
#define circleDiameter 9
#define circleRadius 4.5

#define ID_GAME_LOOP 1
#define GAME_TICK_MS 16

struct BOARD {
	int x;
	int y;
	int color;
	bool value = FALSE;
};

struct PLAYER {
	int x;
	int y;
	int direct;
	int diex;
	int diey;
	int dietime = 0;
	int diesize = 15;
	bool life = TRUE;
	bool moojuk = FALSE;
};

struct BULLET {
	int x = -100;
	int y = -100;
	int direct;
	bool shot = FALSE;
};

struct READYBULLET {
	int x;
	int y;
	bool shot = FALSE;
};

struct ENEMY {
	int x;
	int y;
	int boomx = -1000;
	int boomy = -1000;
	int boomtime = 0;
	int boomsize = 15;
	float deltaX;
	float deltaY;
	float directionX;
	float directionY;
	bool life = FALSE;
	bool boom = FALSE;
};

struct GameState {
	
	PLAYER player;
	BOARD board_easy[150];
	ENEMY enemy_easy[10];
	BULLET bullet[6];
	READYBULLET readybullet[6];

	// 기존 WndProc의 static 변수
	int boardnum = 0;
	int bulletcount = 0;
	int angles[6];
	int enemyloc[10];
	int time = 0;
	int enemycnt = 0;
	int enemyspawn = 0;
	int score = 0;
	int combo = 0;
	bool stop = FALSE;

	// 입력 처리용 (WASD)
	bool keys[256] = { false };

	// 시간 관리용 (기존 타이머 4, 5, 6을 대체)
	float scoreTimer = 0.0f;
	float enemyMoveTimer = 0.0f;

	// GDI 리소스 (WM_PAINT 최적화)
	HBRUSH hBrushBlack;
	HBRUSH hBrushWhite;
	HBRUSH hBrushGray;
	HBRUSH hBrushRed;
	HBRUSH hBrushYellow;

	// 네트워크 매니저
	NetworkManager networkManager;
};




// --- 2. 충돌 함수 선언 ---
bool bulletVSboard(int x, int y, int x2, int y2);
bool bulletVSboard2(int x, int y, int x2, int y2);
bool playerVSboard(int x, int y, int x2, int y2);
bool bulletVSplayer(int x, int y, int x2, int y2);
bool bulletVSplayer2(int x, int y, int x2, int y2);
bool enemyVSboom(int x, int y, int x2, int y2, int boomsize);
bool boardVSboom(int x, int y, int x2, int y2, int boomsize);
bool playerVSenemy(int x, int y, int x2, int y2);


// --- 게임 로직 함수 프로토타입 ---
void Game_Init(HWND hWnd, GameState* pGame);
void Game_Cleanup(GameState* pGame);
void Game_HandleInput_Down(GameState* pGame, WPARAM wParam);
void Game_HandleInput_Up(GameState* pGame, WPARAM wParam);
void Game_Update(HWND hWnd, GameState* pGame, float deltaTime);
void Game_Render(HDC hdc, GameState* pGame);