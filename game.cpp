#include "game.h"
#include "protocol.h"

bool bulletVSboard(int x, int y, int x2, int y2) {					//가로총알 vs 보드 충돌체크
	if ((x + bulletlen >= x2 && y + bulletthick >= y2) && (x <= x2 + boardsize && y <= y2 + boardsize)) {
		return TRUE;
	}
	else
		return FALSE;
}

bool bulletVSboard2(int x, int y, int x2, int y2) {					//세로총알 vs 보드 충돌체크
	if ((x + bulletthick >= x2 && y + bulletlen >= y2) && (x <= x2 + boardsize && y <= y2 + boardsize)) {
		return TRUE;
	}
	else
		return FALSE;
}

bool playerVSboard(int x, int y, int x2, int y2) {					//주인공 vs 보드 충돌체크
	if ((x + playersize > x2 && y + playersize > y2) && (x < x2 + boardsize && y < y2 + boardsize)) {
		return TRUE;
	}
	else
		return FALSE;
}

bool bulletVSplayer(int x, int y, int x2, int y2) {					//가로총알 vs 적 충돌체크
	if ((x + bulletlen >= x2 && y + bulletthick >= y2) && (x <= x2 + playersize && y <= y2 + playersize)) {
		return TRUE;
	}
	else
		return FALSE;
}

bool bulletVSplayer2(int x, int y, int x2, int y2) {				//세로총알 vs 적 충돌체크
	if ((x + bulletthick >= x2 && y + bulletlen >= y2) && (x <= x2 + playersize && y <= y2 + playersize)) {
		return TRUE;
	}
	else
		return FALSE;
}

bool enemyVSboom(int x, int y, int x2, int y2, int boomsize) {		//적 vs 폭발 충돌체크
	if ((x + playersize > x2 - boomsize && y + playersize > y2 - boomsize) && (x < x2 + boomsize && y < y2 + boomsize)) {
		return TRUE;
	}
	else
		return FALSE;
}

bool boardVSboom(int x, int y, int x2, int y2, int boomsize) {		//보드 vs 폭발 충돌체크
	if ((x + boardsize > x2 - boomsize && y + boardsize > y2 - boomsize) && (x < x2 + boomsize && y < y2 + boomsize)) {
		return TRUE;
	}
	else
		return FALSE;
}

bool playerVSenemy(int x, int y, int x2, int y2) {					//주인공 vs 적 충돌체크
	if ((x + playersize > x2 && y + playersize > y2) && (x < x2 + playersize && y < y2 + playersize)) {
		return TRUE;
	}
	else
		return FALSE;
}

// --- 네트워크 패킷 처리 함수 ---
void ProcessPacket(GameState* pGame, PacketHeader* pHeader)
{
	// 서버로부터 받은 패킷 타입에 따라 분기
	switch (pHeader->type)
	{
		// (1) "매칭 성공" 패킷 처리
	case S_MATCH_COMPLETE:
	{
		S_MatchCompletePacket* pPkt = (S_MatchCompletePacket*)pHeader;
		pGame->myPlayerID = pPkt->yourPlayerID;
		printf("[게임 시작] 매칭 성공! 내 플레이어 ID: %d\n", pGame->myPlayerID);
		break;
	}

	// (2) "게임 상태" 패킷 처리
	case S_GAME_STATE:
	{
		S_GameStatePacket* pPkt = (S_GameStatePacket*)pHeader;

		// 1. 모든 플레이어의 위치/생존 정보 갱신
		for (int i = 0; i < MAX_PLAYERS; i++) {
			pGame->players[i].x = pPkt->players[i].x;
			pGame->players[i].y = pPkt->players[i].y;
			pGame->players[i].life = pPkt->players[i].life;

			pGame->players[i].ammo = pPkt->players[i].ammo;
		}

		// 2. 모든 적의 위치/생존 정보 갱신
		for (int i = 0; i < MAX_ENEMIES; i++) {
			pGame->enemies[i].x = pPkt->enemies[i].x;
			pGame->enemies[i].y = pPkt->enemies[i].y;
			pGame->enemies[i].life = pPkt->enemies[i].life;
		}
		break;
	}

	default:
		printf("알 수 없는 패킷 타입 수신: %d\n", pHeader->type);
		break;
	}
}

// --- 게임 초기화 함수 ---
void Game_Init(HWND hWnd, GameState* pGame)
{
	// 네트워크 초기화
	if (pGame->networkManager.Init(hWnd) == false) {
		printf("서버 연결 실패 \n");
	}

	pGame->myPlayerID = -1; // 아직 매칭 안 됨

	// GDI 리소스
	pGame->hBrushBlack = CreateSolidBrush(RGB(0, 0, 0));
	pGame->hBrushWhite = CreateSolidBrush(RGB(255, 255, 255));
	pGame->hBrushGray = CreateSolidBrush(RGB(30, 30, 30));
	pGame->hBrushRed = CreateSolidBrush(RGB(255, 0, 0));
	pGame->hBrushYellow = CreateSolidBrush(RGB(255, 255, 0));

	// 보드 생성 (일단 로컬에서 초기화, 나중에 서버 동기화 필요)
	pGame->boardnum = 0;
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 15; j++) {
			pGame->board_easy[pGame->boardnum].x = j * boardsize + 335;
			pGame->board_easy[pGame->boardnum].y = i * boardsize + 240;
			pGame->boardnum++;
		}
	}
	for (int j = 3; j < 7; j++) {
		for (int k = 5; k < 10; k++) {
			pGame->board_easy[j * 15 + k].value = TRUE;
		}
	}

	// 플레이어 초기화
	for (int i = 0; i < MAX_PLAYERS; i++) {
		pGame->players[i].x = -100;
		pGame->players[i].y = -100;
		pGame->players[i].life = FALSE;
	}

	// 초기 스폰 위치 (임시)
	pGame->players[0].x = 400; pGame->players[0].y = 400; pGame->players[0].life = TRUE;
	pGame->players[1].x = 600; pGame->players[1].y = 400; pGame->players[1].life = TRUE;
	pGame->players[2].x = 800; pGame->players[2].y = 400; pGame->players[2].life = TRUE;

	// 회전총알 각도
	for (int i = 0; i < 6; i++) {
		pGame->angles[i] = 60 * i;
	}
}

// --- 게임 리소스 해제 함수 ---
void Game_Cleanup(GameState* pGame)
{
	// Game_Init에서 생성한 GDI 리소스 해제
	DeleteObject(pGame->hBrushBlack);
	DeleteObject(pGame->hBrushWhite);
	DeleteObject(pGame->hBrushGray);
	DeleteObject(pGame->hBrushRed);
	DeleteObject(pGame->hBrushYellow);
}

// --- 키보드 입력 처리 함수 ---
void Game_HandleInput_Down(GameState* pGame, WPARAM wParam)
{
	if (pGame->myPlayerID == -1) return; // 매칭 전엔 조작 불가
	int id = pGame->myPlayerID;

	// 이동 키 (WASD) -> 내 캐릭터(id)만 조작
	switch (wParam) {
	case 'd': case 'D':
		if (pGame->keys['D'] == false) pGame->networkManager.SendMovePacket(1);
		pGame->keys['D'] = true;
		pGame->players[id].direct = 1;
		break;
	case 'a': case 'A':
		if (pGame->keys['A'] == false) pGame->networkManager.SendMovePacket(2);
		pGame->keys['A'] = true;
		pGame->players[id].direct = 2;
		break;
	case 's': case 'S':
		if (pGame->keys['S'] == false) pGame->networkManager.SendMovePacket(3);
		pGame->keys['S'] = true;
		pGame->players[id].direct = 3;
		break;
	case 'w': case 'W':
		if (pGame->keys['W'] == false) pGame->networkManager.SendMovePacket(4);
		pGame->keys['W'] = true;
		pGame->players[id].direct = 4;
		break;
	}

	// 발사 키 (방향키)
	switch (wParam) {
	case VK_RIGHT:
		pGame->networkManager.SendAttackPacket(1);
		// (클라 예측 총알 발사 로직은 생략하거나 필요시 추가)
		break;
	case VK_LEFT:
		pGame->networkManager.SendAttackPacket(2);
		break;
	case VK_DOWN:
		pGame->networkManager.SendAttackPacket(3);
		break;
	case VK_UP:
		pGame->networkManager.SendAttackPacket(4);
		break;

	// 재장전 로직
	case 'r': case 'R':
		pGame->bulletcount = 0; 
		break;
	case '1':
		pGame->players[id].moojuk = !pGame->players[id].moojuk;
		break;
	}
}
void Game_HandleInput_Up(GameState* pGame, WPARAM wParam)
{
	// 이동 키 (상태 해제)
	switch (wParam) {
	case 'd': case 'D': pGame->keys['D'] = false; break;
	case 'a': case 'A': pGame->keys['A'] = false; break;
	case 's': case 'S': pGame->keys['S'] = false; break;
	case 'w': case 'W': pGame->keys['W'] = false; break;
	}
}

// --- 게임 로직 업데이트 함수 ---
void Game_Update(HWND hWnd, GameState* pGame, float deltaTime)
{
	// 1. 네트워크 패킷 처리 (서버 상태 동기화)
	char packetBuffer[2048];
	while (pGame->networkManager.PopPacket(packetBuffer, 2048))
	{
		ProcessPacket(pGame, (PacketHeader*)packetBuffer);
	}

	if (pGame->myPlayerID == -1) return; // 매칭 대기 중
	int id = pGame->myPlayerID;


	// 2. 로컬 예측 이동 (내 캐릭터만)
	// (서버 응답을 기다리면 뚝뚝 끊기므로 미리 움직여 놓음)
	if (pGame->keys['D']) {
		pGame->players[id].x += 4;
		// (벽 충돌 검사는 로컬에서도 해야 함 - playerVSboard 사용)
	}
	if (pGame->keys['A']) pGame->players[id].x -= 4;
	if (pGame->keys['S']) pGame->players[id].y += 4;
	if (pGame->keys['W']) pGame->players[id].y -= 4;


	// 3. 시각 효과 (회전 총알)
	for (int i = 0; i < 6; i++) {
		pGame->angles[i] += 5;
		if (pGame->angles[i] >= 360) pGame->angles[i] -= 360;
	}
	// 내 캐릭터 주위의 회전 총알 위치 갱신
	for (int i = 0; i < 6; i++) {
		double radians = pGame->angles[i] * 3.14159 / 180.0;
		pGame->readybullet[i].x = pGame->players[id].x + playersize / 2 + static_cast<int>((playersize / 2 - circleRadius - 0.5) * cos(radians)) - circleRadius;
		pGame->readybullet[i].y = pGame->players[id].y + playersize / 2 + static_cast<int>((playersize / 2 - circleRadius - 0.5) * sin(radians)) - circleRadius;
	}

	// (적 이동, 총알 이동 등은 서버가 처리해서 S_GAME_STATE로 보내주므로 여기서 로직 제거)

	// 4. 보드 색상 갱신
	for (int i = 0; i < 150; i++) {
		if (pGame->board_easy[i].value) pGame->board_easy[i].color = 255;
		else pGame->board_easy[i].color = 30;
	}
}

// ---  그리기 전용 함수 ---
void Game_Render(HDC mDC, GameState* pGame)
{
	HBRUSH hBrush, oldBrush;
	TCHAR lpOut[100];

	if (pGame->myPlayerID == -1) {
		wsprintf(lpOut, L"매칭 대기 중..."); 
		TextOut(mDC, 500, 400, lpOut, lstrlen(lpOut));
		return;
	}

	// 1. 보드 그리기
	for (int i = 0; i < 150; i++) {
		hBrush = (pGame->board_easy[i].value) ? pGame->hBrushWhite : pGame->hBrushGray;
		oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
		Rectangle(mDC, pGame->board_easy[i].x, pGame->board_easy[i].y, pGame->board_easy[i].x + boardsize, pGame->board_easy[i].y + boardsize);
		SelectObject(mDC, oldBrush);
	}

	// 2. 총알 그리기 (서버에서 총알 정보를 아직 안 보내주므로, 로컬 bullet 배열 사용 - 추후 수정 필요)
	oldBrush = (HBRUSH)SelectObject(mDC, pGame->hBrushBlack);
	for (int i = 0; i < pGame->bulletcount; i++) {
		if (pGame->bullet[i].shot) {
			if (pGame->bullet[i].direct == 1 || pGame->bullet[i].direct == 2)
				Rectangle(mDC, pGame->bullet[i].x, pGame->bullet[i].y, pGame->bullet[i].x + bulletlen, pGame->bullet[i].y + bulletthick);
			else
				Rectangle(mDC, pGame->bullet[i].x, pGame->bullet[i].y, pGame->bullet[i].x + bulletthick, pGame->bullet[i].y + bulletlen);
		}
	}

	SelectObject(mDC, oldBrush);

	// 3. 플레이어 그리기 (3명 모두)
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (pGame->players[i].life) {
			// 내 캐릭터만 노란색, 남은 검은색
			bool isMe = (i == pGame->myPlayerID);
			hBrush = (isMe) ? pGame->hBrushYellow : pGame->hBrushBlack;

			oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
			RoundRect(mDC, pGame->players[i].x, pGame->players[i].y, pGame->players[i].x + playersize, pGame->players[i].y + playersize, 10, 10);
			SelectObject(mDC, oldBrush);

			// ID 표시 (디버깅용)
			wsprintf(lpOut, L"P%d", i);
			TextOut(mDC, pGame->players[i].x, pGame->players[i].y - 15, lpOut, lstrlen(lpOut));
		}
	}

	// 4. 회전 총알 그리기 (내 것만)
	hBrush = pGame->hBrushWhite;
	oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
	for (int i = 0; i < 6; i++) {
		Ellipse(mDC, pGame->readybullet[i].x, pGame->readybullet[i].y, pGame->readybullet[i].x + circleDiameter, pGame->readybullet[i].y + circleDiameter);
	}
	SelectObject(mDC, oldBrush);

	// 5. 적 그리기 (서버가 보내준 위치)
	oldBrush = (HBRUSH)SelectObject(mDC, pGame->hBrushRed);
	for (int i = 0; i < MAX_ENEMIES; i++) {
		if (pGame->enemies[i].life) {
			RoundRect(mDC, pGame->enemies[i].x, pGame->enemies[i].y, pGame->enemies[i].x + playersize, pGame->enemies[i].y + playersize, 10, 10);
		}
	}
	SelectObject(mDC, oldBrush);
}

// --- 탄환 개수 반환 함수 ---
int GetAmmoCount(GameState* pGame, int playerID)
{
	if (playerID < 0 || playerID >= MAX_PLAYERS) return 0;
	return pGame->players[playerID].ammo;
}
