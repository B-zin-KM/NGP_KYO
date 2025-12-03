#include "game.h"
#include "protocol.h"


// 충돌체크 함수 통합
bool CheckRectCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
	return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}


bool CheckWallCollision(GameState* pGame, int nextX, int nextY)
{
	// 보드판 가장자리 충돌검사
	const int MAP_LEFT = 335;
	const int MAP_TOP = 240;

	// [수정됨] 가로 20칸 x 타일크기 35
	const int MAP_RIGHT = 335 + (BOARDSIZE_x * boardsize);

	// [수정됨] 세로 15칸 x 타일크기 35
	const int MAP_BOTTOM = 240 + (BOARDSIZE_y * boardsize);

	if (nextX < MAP_LEFT) return true;
	if (nextY < MAP_TOP) return true;

	// 플레이어 크기(playersize)를 고려하여 우측/하단 경계 체크
	if (nextX > MAP_RIGHT - playersize) return true;
	if (nextY > MAP_BOTTOM - playersize) return true;

	// 검은타일과 충돌검사
	for (int i = 0; i < BOARD_SIZE; i++) {
		if (pGame->board_easy[i].value == TRUE) continue;
		if (CheckRectCollision(nextX, nextY, playersize, playersize,
			pGame->board_easy[i].x, pGame->board_easy[i].y, boardsize, boardsize))
		{
			return true;
		}
	}
	return false;
}


// --- 네트워크 패킷 처리 함수 ---
void ProcessPacket(GameState* pGame, PacketHeader* pHeader)
{
	if (pHeader->type == 5) { // 5번 = S_GAME_START
		printf("!!! [DEBUG] GAME START PACKET RECEIVED !!!\n");
	}
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

	case S_LOBBY_UPDATE:
	{
		S_LobbyUpdatePacket* pPkt = (S_LobbyUpdatePacket*)pHeader;
		pGame->connectedCount = pPkt->connectedCount;
		for (int i = 0; i < MAX_PLAYERS; ++i) {
			pGame->playerReadyState[i] = pPkt->players[i].isReady;
			pGame->playerConnected[i] = pPkt->players[i].connected;
		}
		break;
	}

	case S_GAME_START:
	{
		pGame->currentScene = SCENE_INGAME; 
		printf("Game Started!\n");
		break;
	}

	case S_EXPLOSION:
	{
		S_ExplosionPacket* pPkt = (S_ExplosionPacket*)pHeader;

		// 빈 이펙트 슬롯을 찾아 생성
		for (int i = 0; i < 20; ++i) {
			if (!pGame->effects[i].active) {
				pGame->effects[i].active = true;
				pGame->effects[i].x = pPkt->x;
				pGame->effects[i].y = pPkt->y;
				pGame->effects[i].size = pPkt->size; // 초기 크기
				pGame->effects[i].type = pPkt->type;
				pGame->effects[i].playerID = pPkt->playerID;
				pGame->effects[i].time = 0;
				break;
			}
		}
		break;
	}
	// (2) "게임 상태" 패킷 처리
	case S_GAME_STATE:
	{
		if (pGame->currentScene == SCENE_LOBBY) {
			pGame->currentScene = SCENE_INGAME;
			printf("Auto-switched to InGame Scene!\n");
		}
		S_GameStatePacket* pPkt = (S_GameStatePacket*)pHeader;
		
		if (pGame->myPlayerID != -1) {
			int myid = pGame->myPlayerID;
			printf("[client] recv state: mypos(%d, %d)\n",
				   pPkt->players[myid].x, pPkt->players[myid].y);
		}
		
		// 1. 모든 플레이어의 위치/생존 정보 갱신
		for (int i = 0; i < MAX_PLAYERS; i++) {

			// 이전 프레임의 생존 여부
			bool wasAlive = pGame->players[i].life;
			bool isAlive = pPkt->players[i].life;

			// 공통 정보 갱신
			pGame->players[i].direct = pPkt->players[i].direct;
			pGame->players[i].life = isAlive;
			pGame->players[i].ammo = pPkt->players[i].ammo;
			pGame->players[i].score = pPkt->players[i].score;

			// 좌표 정보
			int serverX = pPkt->players[i].x;
			int serverY = pPkt->players[i].y;

			if (wasAlive == FALSE && isAlive == TRUE) {
				pGame->players[i].x = serverX;
				pGame->players[i].y = serverY;
				continue;
			}

			if (isAlive == FALSE) continue;

			// --- 이동 보정 로직 ---
			int myX = pGame->players[i].x;
			int myY = pGame->players[i].y;
			float dist = sqrt(pow(serverX - myX, 2) + pow(serverY - myY, 2));

			if (i == pGame->myPlayerID) {
				if (dist > 20.0f) {
					pGame->players[i].x = serverX;
					pGame->players[i].y = serverY;
				}
				else if (dist > 2.0f) {
					pGame->players[i].x += (int)((serverX - myX) * 0.1f);
					pGame->players[i].y += (int)((serverY - myY) * 0.1f);
				}
			}
			else {
				if (dist > 30.0f) {
					pGame->players[i].x = serverX;
					pGame->players[i].y = serverY;
				}
				else if (dist < 5.0f) {
					pGame->players[i].x = serverX;
					pGame->players[i].y = serverY;
				}
				else {
					pGame->players[i].x += (int)((serverX - myX) * 0.6f);
					pGame->players[i].y += (int)((serverY - myY) * 0.6f);
				}
			}
		}

		// 2. 모든 적의 위치/생존 정보 갱신
		for (int i = 0; i < MAX_ENEMIES; i++) {
			pGame->enemies[i].x = pPkt->enemies[i].x;
			pGame->enemies[i].y = pPkt->enemies[i].y;
			pGame->enemies[i].life = pPkt->enemies[i].life;
		}

		for (int i = 0; i < MAX_BULLETS; i++) {
			pGame->serverBullets[i] = pPkt->bullets[i];
		}

		// 3. 보드판 갱신
		for (int i = 0; i < BOARD_SIZE; i++) {
			pGame->board_easy[i].value = pPkt->board[i];
		}

		pGame->timeLeft = pPkt->remainingTime;

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
	for (int i = 0; i < BOARDSIZE_y; i++) {
		for (int j = 0; j < BOARDSIZE_x; j++) {
			pGame->board_easy[pGame->boardnum].x = j * boardsize + 335;
			pGame->board_easy[pGame->boardnum].y = i * boardsize + 240;
			pGame->boardnum++;
		}
	}
	// 초기 하얀타일 설정
	for (int j = 3; j < 7; j++) {
		for (int k = 5; k < 10; k++) {
			pGame->board_easy[j * 15 + k].value = TRUE;
		}
	}

	// 플레이어 초기화
	for (int i = 0; i < MAX_PLAYERS; i++) {
		pGame->players[i].x = 600;
		pGame->players[i].y = 400;
		pGame->players[i].life = FALSE;
	}

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
	// 1. 매칭 전에는 조작 불가
	if (pGame->myPlayerID == -1) return;

	if (pGame->currentScene == SCENE_LOBBY) {
		if (wParam == 'r' || wParam == 'R') {
			// 준비 패킷 전송
			C_ReqReadyPacket pkt;
			pkt.size = sizeof(pkt);
			pkt.type = C_REQ_READY;
			pGame->networkManager.SendPacket((char*)&pkt, sizeof(pkt));
		}
		return; // 로비에서는 이동/공격 불가
	}

	//// 2. 보낼 패킷 준비
	//C_MovePacket pkt;
	//pkt.size = sizeof(C_MovePacket);
	//pkt.type = C_MOVE; // 10번 
	//pkt.direction = 0;

	switch (wParam) {
	case 'd': case 'D': pGame->keys['D'] = true; break;
	case 'a': case 'A': pGame->keys['A'] = true; break;
	case 's': case 'S': pGame->keys['S'] = true; break;
	case 'w': case 'W': pGame->keys['W'] = true; break;
	}

	//// 4. 이동 패킷 전송 (방향키 눌렀을 때만)
	//if (pkt.direction != 0) {
	//	printf("Send Move Packet: Dir %d\n", pkt.direction); // 디버깅용
	//	pGame->networkManager.SendPacket((char*)&pkt, sizeof(pkt));
	//	return;
	//}

	// 5. 공격 패킷 처리 (방향키)
	C_AttackPacket atkPkt;
	atkPkt.size = sizeof(C_AttackPacket);
	atkPkt.type = C_ATTACK; // 11번
	atkPkt.direction = 0;

	switch (wParam) {
	case VK_RIGHT: atkPkt.direction = 1; break;
	case VK_LEFT:  atkPkt.direction = 2; break;
	case VK_DOWN:  atkPkt.direction = 3; break;
	case VK_UP:    atkPkt.direction = 4; break;
	}

	if (atkPkt.direction != 0) {
		// 총알이 6발 미만일 때만 발사 가능
		if (pGame->bulletcount < 6) {
			pGame->networkManager.SendPacket((char*)&atkPkt, sizeof(atkPkt));
			pGame->bulletcount++; // 발사한 총알 수 증가
			printf("Bullet Fired! Count: %d\n", pGame->bulletcount); // 디버깅용
		}
	}

	// 기타 키 (R, 1 등) 처리
	if (wParam == 'r' || wParam == 'R') pGame->bulletcount = 0;
	if (wParam == '1') pGame->players[pGame->myPlayerID].moojuk = !pGame->players[pGame->myPlayerID].moojuk;
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

	for (int i = 0; i < 20; ++i) {
		if (pGame->effects[i].active) {
			pGame->effects[i].time++;
			pGame->effects[i].size += 2; // 크기 증가 (펑!)

			// 10프레임뒤에 사라짐
			if (pGame->effects[i].time > 10) {
				pGame->effects[i].active = false;
			}
		}
	}

	// 2. 이동 및 패킷 전송 로직
	static float moveTimer = 0.0f;
	moveTimer += deltaTime;

	// 0.010초(10ms)마다 실행 (빠릿한 반응 속도)
	if (moveTimer >= 0.010f)
	{
		moveTimer = 0.0f;

		// ----------------------------------------------
		// [X축] 가로 이동 처리
		// ----------------------------------------------
		int dirX = 0;
		if (pGame->keys['D']) dirX = 1;      // 우
		else if (pGame->keys['A']) dirX = 2; // 좌

		if (dirX != 0) {
			int finalMove = 0; // 실제로 이동할 거리

			// (1) 먼저 2칸 이동 시도 (기본 속도)
			int nextX = (dirX == 1) ? pGame->players[id].x + 2 : pGame->players[id].x - 2;

			if (CheckWallCollision(pGame, nextX, pGame->players[id].y) == false) {
				finalMove = 2; // 성공!
			}
			else {
				// (2) 2칸이 막혔다면? 1칸이라도 이동 시도! (빈틈 메우기)
				nextX = (dirX == 1) ? pGame->players[id].x + 1 : pGame->players[id].x - 1;
				if (CheckWallCollision(pGame, nextX, pGame->players[id].y) == false) {
					finalMove = 1; // 1칸 성공! (벽에 딱 붙음)
				}
			}

			// (3) 이동 가능한 거리가 있으면 적용 및 패킷 전송
			if (finalMove > 0) {
				// 로컬 좌표 갱신
				if (dirX == 1) pGame->players[id].x += finalMove;
				else pGame->players[id].x -= finalMove;

				// 서버 전송 (서버는 무조건 2씩 움직이지만, 미세 오차는 ProcessPacket에서 보정됨)
				C_MovePacket pkt;
				pkt.size = sizeof(C_MovePacket);
				pkt.type = C_MOVE;
				pkt.direction = dirX;
				pGame->networkManager.SendPacket((char*)&pkt, sizeof(pkt));
			}
		}

		// ----------------------------------------------
		// [Y축] 세로 이동 처리
		// ----------------------------------------------
		int dirY = 0;
		if (pGame->keys['S']) dirY = 3;      // 하
		else if (pGame->keys['W']) dirY = 4; // 상

		if (dirY != 0) {
			int finalMove = 0;

			// (1) 2칸 이동 시도
			int nextY = (dirY == 3) ? pGame->players[id].y + 2 : pGame->players[id].y - 2;

			if (CheckWallCollision(pGame, pGame->players[id].x, nextY) == false) {
				finalMove = 2;
			}
			else {
				// (2) 1칸 이동 시도
				nextY = (dirY == 3) ? pGame->players[id].y + 1 : pGame->players[id].y - 1;
				if (CheckWallCollision(pGame, pGame->players[id].x, nextY) == false) {
					finalMove = 1;
				}
			}

			// (3) 적용
			if (finalMove > 0) {
				if (dirY == 3) pGame->players[id].y += finalMove;
				else pGame->players[id].y -= finalMove;

				C_MovePacket pkt;
				pkt.size = sizeof(C_MovePacket);
				pkt.type = C_MOVE;
				pkt.direction = dirY;
				pGame->networkManager.SendPacket((char*)&pkt, sizeof(pkt));
			}
		}
	}


	// 3. 시각 효과 (회전 총알)
	// deltaTime을 곱해서 프레임이 변해도 일정한 속도로 회전하게 함
	int rotateSpeed = (int)(300.0f * deltaTime);
	if (rotateSpeed < 1) rotateSpeed = 1; // 최소 1도는 회전

	for (int i = 0; i < 6; i++) {
		pGame->angles[i] += rotateSpeed;
		if (pGame->angles[i] >= 360) pGame->angles[i] %= 360;
	}

	// 회전한 각도에 맞춰 실제 좌표(x, y) 갱신
	for (int i = 0; i < 6; i++) {
		double radians = pGame->angles[i] * 3.14159 / 180.0;

		pGame->readybullet[i].x = pGame->players[id].x + playersize / 2 +
			static_cast<int>((playersize / 2 - circleRadius - 0.5) * cos(radians)) - circleRadius;

		pGame->readybullet[i].y = pGame->players[id].y + playersize / 2 +
			static_cast<int>((playersize / 2 - circleRadius - 0.5) * sin(radians)) - circleRadius;
	}


	// 4. 보드 색상 갱신 (서버에서 벽이 뚫리면 여기서 색이 바뀜)
	for (int i = 0; i < BOARD_SIZE; i++) {
		if (pGame->board_easy[i].value) pGame->board_easy[i].color = 255; // 흰색 (이동 가능)
		else pGame->board_easy[i].color = 30;  // 검은색 (벽)
	}
}

// ---  그리기 전용 함수 ---
void Game_Render(HDC mDC, GameState* pGame)
{
	// --- 로비 화면 그리기 ---
	if (pGame->currentScene == SCENE_LOBBY)
	{
		TCHAR lpOut[100];

		// 배경 (흰색)
		Rectangle(mDC, 0, 0, 1200, 900);

		// 제목
		wsprintf(lpOut, L"=== GAME LOBBY ===");
		TextOut(mDC, 500, 200, lpOut, lstrlen(lpOut));

		// 플레이어 목록 표시
		for (int i = 0; i < MAX_PLAYERS; ++i) {
			if (pGame->playerConnected[i]) {
				if (pGame->playerReadyState[i]) {
					wsprintf(lpOut, L"Player %d : [READY]", i);
					SetTextColor(mDC, RGB(0, 200, 0));
				}
				else {
					wsprintf(lpOut, L"Player %d : Waiting...", i);
					SetTextColor(mDC, RGB(255, 0, 0));
				}
			}
			else {
				wsprintf(lpOut, L"Player %d : (Empty)", i);
				SetTextColor(mDC, RGB(100, 100, 100));
			}
			TextOut(mDC, 500, 300 + (i * 30), lpOut, lstrlen(lpOut));
		}

		// 내 상태 표시
		SetTextColor(mDC, RGB(0, 0, 0)); // 검은색 복귀
		if (pGame->myPlayerID != -1) {
			wsprintf(lpOut, L"My ID: %d (Press 'R' to Ready)", pGame->myPlayerID);
			TextOut(mDC, 500, 500, lpOut, lstrlen(lpOut));
		}
		else {
			TextOut(mDC, 500, 500, L"Connecting to Server...", 23);
		}
		return;
	}

	HBRUSH hBrush, oldBrush;
	TCHAR lpOut[100];

	// 1. 보드 그리기
	for (int i = 0; i < BOARD_SIZE; i++) {
		hBrush = (pGame->board_easy[i].value) ? pGame->hBrushWhite : pGame->hBrushGray;
		oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
		Rectangle(mDC, pGame->board_easy[i].x, pGame->board_easy[i].y, pGame->board_easy[i].x + boardsize, pGame->board_easy[i].y + boardsize);
		SelectObject(mDC, oldBrush);
	}

	
	oldBrush = (HBRUSH)SelectObject(mDC, pGame->hBrushBlack);
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (pGame->serverBullets[i].active) {
            int bx = pGame->serverBullets[i].x;
            int by = pGame->serverBullets[i].y;
            int dir = pGame->serverBullets[i].direct;

			if (dir == 1 || dir == 2) // 가로
				Rectangle(mDC, bx, by, bx + bulletlen, by + bulletthick);
			else // 세로
				Rectangle(mDC, bx, by, bx + bulletthick, by + bulletlen);
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
			TextOut(mDC, pGame->players[i].x, pGame->players[i].y - 16, lpOut, lstrlen(lpOut));
		}
	}

	// 4. 회전 총알 그리기 (내 것만)
	hBrush = pGame->hBrushWhite;
	oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
	for (int i = pGame->bulletcount; i < 6; i++) {
		Ellipse(mDC, pGame->readybullet[i].x, pGame->readybullet[i].y,
			pGame->readybullet[i].x + circleDiameter, pGame->readybullet[i].y + circleDiameter);
	}
	SelectObject(mDC, oldBrush);

	// 5. 적 그리기 (서버가 보내준 위치)
	oldBrush = (HBRUSH)SelectObject(mDC, pGame->hBrushRed);
	for (int i = 0; i < MAX_ENEMIES; i++) {
		if (pGame->enemies[i].life) {
			// 서버가 준 좌표대로 그리기
			RoundRect(mDC, pGame->enemies[i].x, pGame->enemies[i].y,
				pGame->enemies[i].x + playersize, pGame->enemies[i].y + playersize, 10, 10);
		}
	}
	SelectObject(mDC, oldBrush);

	// 폭발 이펙트 그리기
	for (int i = 0; i < 20; ++i) {
		if (pGame->effects[i].active) {
			HBRUSH effectBrush;

			// [0: 적] -> 빨강
			if (pGame->effects[i].type == 0) {
				effectBrush = pGame->hBrushRed;
			}
			// [1: 플레이어] -> 내꺼면 노랑, 남꺼면 검정
			else {
				if (pGame->effects[i].playerID == pGame->myPlayerID) {
					effectBrush = pGame->hBrushYellow; // 나 (노랑)
				}
				else {
					effectBrush = pGame->hBrushBlack;  // 남 (검정)
				}
			}

			oldBrush = (HBRUSH)SelectObject(mDC, effectBrush);
			int r = pGame->effects[i].size;
			Ellipse(mDC,
				pGame->effects[i].x - r, pGame->effects[i].y - r,
				pGame->effects[i].x + r, pGame->effects[i].y + r);
			SelectObject(mDC, oldBrush);
		}
	}

	// 점수판 UI 그리기
	{
		TCHAR uiBuf[100];
		SetBkMode(mDC, TRANSPARENT); // 텍스트 배경을 투명하게 설정

		// 위치 설정 (창 가로 크기가 1200이므로 오른쪽 끝인 1000 지점)
		int startX = 1000;
		int startY = 30;

		for (int i = 0; i < MAX_PLAYERS; i++) {
			// 점수 텍스트 준비
			if (i == pGame->myPlayerID) {
				// 내 점수는 파란색으로 표시하고 (ME) 붙이기
				SetTextColor(mDC, RGB(0, 0, 255));
				wsprintf(uiBuf, L"[P%d] Score : %d (ME)", i, pGame->players[i].score);
			}
			else {
				// 다른 플레이어는 검은색
				SetTextColor(mDC, RGB(0, 0, 0));
				wsprintf(uiBuf, L"[P%d] Score : %d", i, pGame->players[i].score);
			}

			// 텍스트 출력 (한 줄씩 띄워서)
			TextOut(mDC, startX, startY + (i * 25), uiBuf, lstrlen(uiBuf));
		}	
	}
	// 타이머 그리기
	int oldBkMode = SetBkMode(mDC, TRANSPARENT);
	COLORREF oldTextColor = SetTextColor(mDC, RGB(255, 255, 255));

	TCHAR timeText[64];
	wsprintf(timeText, L"TIME: %d", pGame->timeLeft);

	TextOut(mDC, 350, 10, timeText, lstrlen(timeText));

	if (pGame->timeLeft <= 0) {
		TCHAR overText[] = L"GAME OVER";
		TextOut(mDC, 380, 300, overText, lstrlen(overText));
	}
	SetBkMode(mDC, oldBkMode);
	SetTextColor(mDC, oldTextColor);
}

// --- 탄환 개수 반환 함수 ---
int GetAmmoCount(GameState* pGame, int playerID)
{
	if (playerID < 0 || playerID >= MAX_PLAYERS) return 0;
	return pGame->players[playerID].ammo;
}
