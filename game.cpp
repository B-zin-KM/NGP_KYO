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

			// [CASE 1] 나
			if (i == pGame->myPlayerID) {
				// 1. 큰 오차 (20픽셀 이상) -> 렉 걸림/충돌 -> 즉시 강제 이동
				if (dist > 20.0f) {
					pGame->players[i].x = serverX;
					pGame->players[i].y = serverY;
				}
				// 2. 작은 오차 (2픽셀 ~ 20픽셀) -> 미세하게 틀어짐 -> 아주 살살 당겨옴
				else if (dist > 2.0f) {
					pGame->players[i].x += (int)((serverX - myX) * 0.1f);
					pGame->players[i].y += (int)((serverY - myY) * 0.1f);
				}
			}
			// [CASE 2] 다른 사람
			else {
				// 1. 너무 멀면(30픽셀) 순간이동 (렉 복구)
				if (dist > 30.0f) {
					pGame->players[i].x = serverX;
					pGame->players[i].y = serverY;
				}
				// 2. 평소에는 부드럽게 추적 (Lerp)
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
		pGame->networkManager.SendPacket((char*)&atkPkt, sizeof(atkPkt));
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


	// 이동로직 수정
	static float moveTimer = 0.0f;
	moveTimer += deltaTime;

	if (moveTimer >= 0.010f)
	{
		moveTimer = 0.0f;

		// 1. 가로 이동 (X축) 체크
		int dirX = 0;
		if (pGame->keys['D']) dirX = 1;      // 우
		else if (pGame->keys['A']) dirX = 2; // 좌

		if (dirX != 0) {
			// (1) 로컬 이동: 패킷 보낼 때 같이 움직임! (속도 3)
			if (dirX == 1) pGame->players[id].x += 2;
			else pGame->players[id].x -= 2;

			// (2) 패킷 전송
			C_MovePacket pkt;
			pkt.size = sizeof(C_MovePacket);
			pkt.type = C_MOVE;
			pkt.direction = dirX;
			pGame->networkManager.SendPacket((char*)&pkt, sizeof(pkt));
		}

		// --- 2. 세로 이동 (Y축) ---
		int dirY = 0;
		if (pGame->keys['S']) dirY = 3;      // 하
		else if (pGame->keys['W']) dirY = 4; // 상

		if (dirY != 0) {
			// (1) 로컬 이동 (속도 3)
			if (dirY == 3) pGame->players[id].y += 2;
			else pGame->players[id].y -= 2;

			// (2) 패킷 전송
			C_MovePacket pkt;
			pkt.size = sizeof(C_MovePacket);
			pkt.type = C_MOVE;
			pkt.direction = dirY;
			pGame->networkManager.SendPacket((char*)&pkt, sizeof(pkt));
		}
	}

	// 로컬 예측 이동 잠깐 주석으로 함
	// 서버가 보내주는 좌표로만 움직이는지 확인할라고
	// 근데 이게 왜 필요한건지 모르겠음 설명좀
	// 위에 합쳤음 서버반응 차이때문에 클라에서 먼저 이동시키고 서버로 올리는게 이동이 부드러움

	// 3. 시각 효과 (회전 총알)
	int rotateSpeed = (int)(300.0f * deltaTime);
	if (rotateSpeed < 1) rotateSpeed = 1;

	for (int i = 0; i < 6; i++) {
		pGame->angles[i] += rotateSpeed;
		if (pGame->angles[i] >= 360) pGame->angles[i] %= 360;
	}

	for (int i = 0; i < 6; i++) {
		double radians = pGame->angles[i] * 3.14159 / 180.0;

		pGame->readybullet[i].x = pGame->players[id].x + playersize / 2 +
			static_cast<int>((playersize / 2 - circleRadius - 0.5) * cos(radians)) - circleRadius;

		pGame->readybullet[i].y = pGame->players[id].y + playersize / 2 +
			static_cast<int>((playersize / 2 - circleRadius - 0.5) * sin(radians)) - circleRadius;
	}

	// 적 이동, 총알 이동 같은건 서버가 처리해서 S_GAME_STATE로 보내주니까 로직 제거함

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
