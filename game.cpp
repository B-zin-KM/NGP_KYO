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

void Game_Init(HWND hWnd, GameState* pGame)
{

	// --- 네트워크 초기화 ---
	if (pGame->networkManager.Init(hWnd) == false) {
		printf("서버 연결 실패.\n");
	}

	// 기존 WM_CREATE 로직
	pGame->boardnum = 0;
	//보드 생성
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
	pGame->boardnum = 0;
	//주인공 초기위치
	pGame->player.x = 600;
	pGame->player.y = 400;
	//회전총알 초기위치
	for (int i = 0; i < 6; i++) {
		pGame->angles[i] = 60 * i;
	}
	for (int i = 0; i < 6; i++) {
		double radians = pGame->angles[i] * 3.14159 / 180.0;
		pGame->readybullet[i].x = pGame->player.x + playersize / 2 + static_cast<int>((playersize / 2 - circleRadius - 0.5) * cos(radians)) - circleRadius;
		pGame->readybullet[i].y = pGame->player.y + playersize / 2 + static_cast<int>((playersize / 2 - circleRadius - 0.5) * sin(radians)) - circleRadius;
	}
	//적 초기위치
	for (int i = 0; i < 10; i++) {
		pGame->enemyloc[i] = rand() % 150;
	}
	for (int j = 0; j < 10; j++) {
		pGame->enemy_easy[j].x = pGame->board_easy[pGame->enemyloc[j]].x;
		pGame->enemy_easy[j].y = pGame->board_easy[pGame->enemyloc[j]].y;
	}

	// GDI 리소스 미리 생성
	pGame->hBrushBlack = CreateSolidBrush(RGB(0, 0, 0));
	pGame->hBrushWhite = CreateSolidBrush(RGB(255, 255, 255));
	pGame->hBrushGray = CreateSolidBrush(RGB(30, 30, 30));
	pGame->hBrushRed = CreateSolidBrush(RGB(255, 0, 0));
	pGame->hBrushYellow = CreateSolidBrush(RGB(255, 255, 0));

	// 단일 게임 루프 타이머 시작
	SetTimer(hWnd, ID_GAME_LOOP, GAME_TICK_MS, NULL);
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
	// 이동 키 (상태 저장)
	switch (wParam) {
	case 'd': case 'D':
		if (pGame->keys['D'] == false) {
			pGame->networkManager.SendMovePacket(1);
		}
		pGame->keys['D'] = true;
		pGame->player.direct = 1;
		break;
	case 'a': case 'A':
		if (pGame->keys['A'] == false) {
			pGame->networkManager.SendMovePacket(2);
		}
		pGame->keys['A'] = true;
		pGame->player.direct = 2;
		break;
	case 's': case 'S':
		if (pGame->keys['S'] == false) {
			pGame->networkManager.SendMovePacket(3);
		}
		pGame->keys['S'] = true;
		pGame->player.direct = 3;
		break;
	case 'w': case 'W':
		if (pGame->keys['W'] == false) {
			pGame->networkManager.SendMovePacket(4);
		}
		pGame->keys['W'] = true;
		pGame->player.direct = 4;
		break;
	}

	// 발사 키 (즉시 실행)
	switch (wParam) {
	case VK_RIGHT:
		pGame->networkManager.SendAttackPacket(1); // 1:Right

		if (pGame->bulletcount < 6) {
			pGame->bulletcount++;
			for (int i = 0; i < pGame->bulletcount; i++) {
				if (!pGame->bullet[i].shot) {
					pGame->bullet[i].direct = 1;
					pGame->bullet[i].x = pGame->player.x + 6; pGame->bullet[i].y = pGame->player.y + 11;
					pGame->bullet[i].shot = TRUE;
					break; 
				}
			}
		}
		break;
	case VK_LEFT:
		pGame->networkManager.SendAttackPacket(2); // 2:Left

		if (pGame->bulletcount < 6) {
			pGame->bulletcount++;
			for (int i = 0; i < pGame->bulletcount; i++) {
				if (!pGame->bullet[i].shot) {
					pGame->bullet[i].direct = 2;
					pGame->bullet[i].x = pGame->player.x + 6; pGame->bullet[i].y = pGame->player.y + 11;
					pGame->bullet[i].shot = TRUE;
					break; 
				}
			}
		}
		break;
	case VK_DOWN:
		pGame->networkManager.SendAttackPacket(3); // 3:Down

		if (pGame->bulletcount < 6) {
			pGame->bulletcount++;
			for (int i = 0; i < pGame->bulletcount; i++) {
				if (!pGame->bullet[i].shot) {
					pGame->bullet[i].direct = 3;
					pGame->bullet[i].x = pGame->player.x + 11; pGame->bullet[i].y = pGame->player.y + 6;
					pGame->bullet[i].shot = TRUE;
					break; 
				}
			}
		}
		break;
	case VK_UP:
		pGame->networkManager.SendAttackPacket(4); // 4:Up

		if (pGame->bulletcount < 6) {
			pGame->bulletcount++;
			for (int i = 0; i < pGame->bulletcount; i++) {
				if (!pGame->bullet[i].shot) {
					pGame->bullet[i].direct = 4;
					pGame->bullet[i].x = pGame->player.x + 11; pGame->bullet[i].y = pGame->player.y + 6;
					pGame->bullet[i].shot = TRUE;
					break; 
				}
			}
		}
		break;
	case 'r': case 'R':
		pGame->bulletcount = 0;
		for (int i = 0; i < 6; i++) {
			pGame->bullet[i].shot = FALSE;
		}
		break;
	case '1':
		pGame->player.moojuk = !pGame->player.moojuk;
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


// --- 메인 게임 업데이트 함수 (모든 로직) ---
void Game_Update(HWND hWnd, GameState* pGame, float deltaTime)
{
	// --- 1. 주인공 이동 (기존 Timer 1) ---
	if (pGame->keys['D']) {
		pGame->player.x += 4;
		for (int i = 0; i < 150; i++) {
			if (playerVSboard(pGame->player.x, pGame->player.y, pGame->board_easy[i].x, pGame->board_easy[i].y)) {
				if (!pGame->board_easy[i].value) {
					if (pGame->player.life && !pGame->player.moojuk)
						pGame->player.x = pGame->board_easy[i].x - boardsize;
				}
			}
		}
		if (pGame->player.x + playersize >= pGame->board_easy[14].x + boardsize) {
			pGame->player.x = pGame->board_easy[14].x;
		}
	}
	if (pGame->keys['A']) {
		pGame->player.x -= 4;
		for (int i = 0; i < 150; i++) {
			if (playerVSboard(pGame->player.x, pGame->player.y, pGame->board_easy[i].x, pGame->board_easy[i].y)) {
				if (!pGame->board_easy[i].value) {
					if (pGame->player.life && !pGame->player.moojuk)
						pGame->player.x = pGame->board_easy[i].x + boardsize * 2 - playersize;
				}
			}
		}
		if (pGame->player.x <= pGame->board_easy[0].x) {
			pGame->player.x = pGame->board_easy[0].x + boardsize - playersize;
		}
	}
	if (pGame->keys['S']) {
		pGame->player.y += 4;
		for (int i = 0; i < 150; i++) {
			if (playerVSboard(pGame->player.x, pGame->player.y, pGame->board_easy[i].x, pGame->board_easy[i].y)) {
				if (!pGame->board_easy[i].value) {
					if (pGame->player.life && !pGame->player.moojuk)
						pGame->player.y = pGame->board_easy[i].y - boardsize;
				}
			}
		}
		if (pGame->player.y + playersize >= pGame->board_easy[149].y + boardsize) {
			pGame->player.y = pGame->board_easy[149].y;
		}
	}
	if (pGame->keys['W']) {
		pGame->player.y -= 4;
		for (int i = 0; i < 150; i++) {
			if (playerVSboard(pGame->player.x, pGame->player.y, pGame->board_easy[i].x, pGame->board_easy[i].y)) {
				if (!pGame->board_easy[i].value) {
					if (pGame->player.life && !pGame->player.moojuk)
						pGame->player.y = pGame->board_easy[i].y + boardsize * 2 - playersize;
				}
			}
		}
		if (pGame->player.y <= pGame->board_easy[0].y) {
			pGame->player.y = pGame->board_easy[0].y + boardsize - playersize;
		}
	}
	// 회전총알 및 사망좌표 갱신 (기존 Timer 1의 일부)
	for (int i = 0; i < 6; i++) {
		double radians = pGame->angles[i] * 3.14159 / 180.0;
		pGame->readybullet[i].x = pGame->player.x + playersize / 2 + static_cast<int>((playersize / 2 - circleRadius - 0.5) * cos(radians)) - circleRadius;
		pGame->readybullet[i].y = pGame->player.y + playersize / 2 + static_cast<int>((playersize / 2 - circleRadius - 0.5) * sin(radians)) - circleRadius;
	}
	pGame->player.diex = pGame->player.x + 15;
	pGame->player.diey = pGame->player.y + 15;


	// --- 2. 총알 발사/충돌 (기존 Timer 2) ---
	// 총알 이동
	for (int i = 0; i < pGame->bulletcount; i++) {
		if (pGame->bullet[i].shot) {
			if (pGame->bullet[i].direct == 1) pGame->bullet[i].x += 15;
			if (pGame->bullet[i].direct == 2) pGame->bullet[i].x -= 15;
			if (pGame->bullet[i].direct == 3) pGame->bullet[i].y += 15;
			if (pGame->bullet[i].direct == 4) pGame->bullet[i].y -= 15;
		}
	}
	// 맵 밖으로 나간 총알 제거 (요청사항 반영)
	for (int i = 0; i < 6; i++) {
		if (pGame->bullet[i].shot) {
			if (pGame->bullet[i].x < 330 || pGame->bullet[i].x > 865 || pGame->bullet[i].y < 235 || pGame->bullet[i].y > 595) {
				pGame->bullet[i].shot = FALSE;
				pGame->bullet[i].x = -1000;
			}
		}
	}
	// 총알 vs 보드
	for (int i = 0; i < 150; i++) {
		for (int j = 0; j < pGame->bulletcount; j++) {
			if (pGame->bullet[j].shot) { // 활성화된 총알만 검사
				if (pGame->bullet[j].direct == 1 || pGame->bullet[j].direct == 2) {
					if (bulletVSboard(pGame->bullet[j].x, pGame->bullet[j].y, pGame->board_easy[i].x, pGame->board_easy[i].y)) {
						pGame->board_easy[i].value = TRUE;
						// (요청사항: 보드 맞아도 안사라짐)
					}
				}
				else {
					if (bulletVSboard2(pGame->bullet[j].x, pGame->bullet[j].y, pGame->board_easy[i].x, pGame->board_easy[i].y)) {
						pGame->board_easy[i].value = TRUE;
					}
				}
			}
		}
	}
	// 총알 vs 적
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < pGame->bulletcount; j++) {
			if (pGame->bullet[j].shot) { // 활성화된 총알만 검사
				if (pGame->bullet[j].direct == 1 || pGame->bullet[j].direct == 2) {
					if (bulletVSplayer(pGame->bullet[j].x, pGame->bullet[j].y, pGame->enemy_easy[i].x, pGame->enemy_easy[i].y)) {
						if (pGame->enemy_easy[i].life) {
							pGame->enemy_easy[i].boomx = pGame->enemy_easy[i].x + 15;
							pGame->enemy_easy[i].boomy = pGame->enemy_easy[i].y + 15;
							pGame->enemy_easy[i].boom = TRUE;
							pGame->enemyloc[i] = rand() % 150;
							pGame->enemy_easy[i].x = pGame->board_easy[pGame->enemyloc[i]].x;
							pGame->enemy_easy[i].y = pGame->board_easy[pGame->enemyloc[i]].y;
							pGame->enemy_easy[i].life = FALSE;
							pGame->bullet[j].shot = FALSE; // 총알 비활성화
							pGame->bullet[j].x = -1000;
							pGame->combo++;
						}
					}
				}
				else {
					if (bulletVSplayer2(pGame->bullet[j].x, pGame->bullet[j].y, pGame->enemy_easy[i].x, pGame->enemy_easy[i].y)) {
						if (pGame->enemy_easy[i].life) {
							pGame->enemy_easy[i].boomx = pGame->enemy_easy[i].x + 15;
							pGame->enemy_easy[i].boomy = pGame->enemy_easy[i].y + 15;
							pGame->enemy_easy[i].boom = TRUE;
							pGame->enemyloc[i] = rand() % 150;
							pGame->enemy_easy[i].x = pGame->board_easy[pGame->enemyloc[i]].x;
							pGame->enemy_easy[i].y = pGame->board_easy[pGame->enemyloc[i]].y;
							pGame->enemy_easy[i].life = FALSE;
							pGame->bullet[j].shot = FALSE; // 총알 비활성화
							pGame->bullet[j].x = -1000;
							pGame->score += 100;
							pGame->combo++;
						}
					}
				}
			}
		}
	}

	// --- 3. 총알 회전 (기존 Timer 3) ---
	// (16ms마다 실행되므로 완벽히 동일)
	for (int i = 0; i < 6; i++) {
		pGame->angles[i] += 5;
		if (pGame->angles[i] >= 360) pGame->angles[i] -= 360;
	}
	// (회전총알 이동은 1번에서 이미 갱신됨)

	// --- 4. 적 생성 (기존 Timer 4) ---
	pGame->scoreTimer += deltaTime;
	if (pGame->scoreTimer >= 1.0f) { // 1초마다 실행
		pGame->scoreTimer -= 1.0f; // 0으로 리셋하면 오차가 누적됨

		pGame->time++;
		pGame->score += 10;
		if (pGame->time % 5 == 0) {
			pGame->enemycnt += pGame->enemyspawn;
			if (pGame->time % 20 == 0)
				pGame->enemyspawn++;
			for (int i = 0; i < pGame->enemycnt; i++) {
				pGame->enemy_easy[i].life = TRUE;
				pGame->enemy_easy[i].boom = FALSE;
			}
		}
		if (pGame->time == 120) {
			KillTimer(hWnd, ID_GAME_LOOP); // 게임 루프 타이머 중지
		}
	}

	// --- 5. 적 이동 (기존 Timer 5) ---
	pGame->enemyMoveTimer += deltaTime;
	if (pGame->enemyMoveTimer >= 0.04f) { // 40ms마다 실행
		pGame->enemyMoveTimer -= 0.04f;

		for (int i = 0; i < 10; i++) {
			if (pGame->enemy_easy[i].life) {
				pGame->enemy_easy[i].deltaX = pGame->player.x - pGame->enemy_easy[i].x;
				pGame->enemy_easy[i].deltaY = pGame->player.y - pGame->enemy_easy[i].y;
				float distance = sqrt(pGame->enemy_easy[i].deltaX * pGame->enemy_easy[i].deltaX + pGame->enemy_easy[i].deltaY * pGame->enemy_easy[i].deltaY);
				if (distance > 0) {
					pGame->enemy_easy[i].directionX = pGame->enemy_easy[i].deltaX / distance;
					pGame->enemy_easy[i].directionY = pGame->enemy_easy[i].deltaY / distance;
					pGame->enemy_easy[i].x += pGame->enemy_easy[i].directionX * 1.5;
					pGame->enemy_easy[i].y += pGame->enemy_easy[i].directionY * 1.5;
				}
			}
		}
		for (int j = 0; j < 10; j++) {
			for (int i = 0; i < 150; i++) {
				if (playerVSboard(pGame->enemy_easy[j].x, pGame->enemy_easy[j].y, pGame->board_easy[i].x, pGame->board_easy[i].y)) {
					if (pGame->enemy_easy[j].life)
						pGame->board_easy[i].value = FALSE;
				}
			}
		}
		for (int i = 0; i < 10; i++) {
			if (playerVSenemy(pGame->player.x, pGame->player.y, pGame->enemy_easy[i].x, pGame->enemy_easy[i].y)) {
				if (pGame->player.life) {
					if (!pGame->player.moojuk) {
						if (pGame->enemy_easy[i].life) {
							pGame->player.life = FALSE;
							pGame->player.diex = pGame->player.x + 15;
							pGame->player.diey = pGame->player.y + 15;
							pGame->score -= 500;
							pGame->combo = 0;
						}
					}
				}
			}
		}
	}

	// --- 6. 적 폭발 (기존 Timer 6) ---
	// (기존 5ms -> 16ms 루프로 변경됨에 따라 속도/시간 조절)
	for (int i = 0; i < 10; i++) {
		if (pGame->enemy_easy[i].boom) {
			// 10 * 5ms = 50ms간 지속
			// 3 * 16ms = 48ms간 지속 (비슷하게 맞춤)
			// 총 40의 크기 증가 -> 프레임당 13씩 증가
			if (pGame->enemy_easy[i].boomtime != 4) {
				pGame->enemy_easy[i].boomtime++;
				pGame->enemy_easy[i].boomsize += 10; // (4 * 10 = 40)
			}
			if (pGame->enemy_easy[i].boomtime == 4) {
				pGame->enemy_easy[i].boomtime = 0;
				pGame->enemy_easy[i].boomsize = 15;
				pGame->enemy_easy[i].boom = FALSE;
			}
		}
	}
	// 폭발 연쇄
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			if (enemyVSboom(pGame->enemy_easy[i].x, pGame->enemy_easy[i].y, pGame->enemy_easy[j].boomx, pGame->enemy_easy[j].boomy, pGame->enemy_easy[j].boomsize)) {
				if (pGame->enemy_easy[i].life && pGame->enemy_easy[j].boom && i != j) {
					pGame->enemy_easy[i].boomx = pGame->enemy_easy[i].x + 15;
					pGame->enemy_easy[i].boomy = pGame->enemy_easy[i].y + 15;
					pGame->enemy_easy[i].boom = TRUE;
					pGame->enemyloc[i] = rand() % 150;
					pGame->enemy_easy[i].x = pGame->board_easy[pGame->enemyloc[i]].x;
					pGame->enemy_easy[i].y = pGame->board_easy[pGame->enemyloc[i]].y;
					pGame->enemy_easy[i].life = FALSE;
					pGame->score += 100;
					pGame->combo++;
				}
			}
		}
	}
	// 보드 vs 폭발
	for (int i = 0; i < 150; i++) {
		for (int j = 0; j < 10; j++) {
			if (boardVSboom(pGame->board_easy[i].x, pGame->board_easy[i].y, pGame->enemy_easy[j].boomx, pGame->enemy_easy[j].boomy, pGame->enemy_easy[j].boomsize)) {
				if (pGame->enemy_easy[j].boom) {
					pGame->board_easy[i].value = TRUE;
				}
			}
		}
	}

	// --- 7. 주인공 사망 이펙트 (기존 Timer 7) ---
	// (16ms마다 실행되므로 완벽히 동일)
	if (!pGame->player.life) {
		if (pGame->player.dietime != 40) {
			pGame->player.dietime++;
			pGame->player.diesize += 1;
		}
		if (pGame->player.dietime == 40) {
			pGame->player.dietime = 0;
			pGame->player.diesize = 15;
			pGame->player.life = TRUE;
		}
	}
	// 사망 이펙트 vs 적
	for (int i = 0; i < 10; i++) {
		if (enemyVSboom(pGame->enemy_easy[i].x, pGame->enemy_easy[i].y, pGame->player.diex, pGame->player.diey, pGame->player.diesize)) {
			if (pGame->enemy_easy[i].life) {
				pGame->enemy_easy[i].boomx = pGame->enemy_easy[i].x + 15;
				pGame->enemy_easy[i].boomy = pGame->enemy_easy[i].y + 15;
				pGame->enemy_easy[i].boom = TRUE;
				pGame->enemyloc[i] = rand() % 150;
				pGame->enemy_easy[i].x = pGame->board_easy[pGame->enemyloc[i]].x;
				pGame->enemy_easy[i].y = pGame->board_easy[pGame->enemyloc[i]].y;
				pGame->enemy_easy[i].life = FALSE;
				pGame->combo++;
			}
		}
	}

	// --- 8. 보드 색상 갱신 (WM_PAINT에서 이동) ---
	for (int i = 0; i < 150; i++) {
		if (pGame->board_easy[i].value) {
			pGame->board_easy[i].color = 255;
		}
		else pGame->board_easy[i].color = 30;
	}
}


// ---  그리기 전용 함수 ---
void Game_Render(HDC mDC, GameState* pGame)
{
	HBRUSH hBrush, oldBrush;
	TCHAR lpOut[100];

	//보드 그리기 (미리 생성한 브러시 사용)
	Rectangle(mDC, 334, 239, 861, 591);
	for (int i = 0; i < 150; i++) {
		hBrush = (pGame->board_easy[i].value) ? pGame->hBrushWhite : pGame->hBrushGray;
		oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
		Rectangle(mDC, pGame->board_easy[i].x, pGame->board_easy[i].y, pGame->board_easy[i].x + boardsize, pGame->board_easy[i].y + boardsize);
		SelectObject(mDC, oldBrush);
		// DeleteObject(hBrush) 불필요! (성능 향상)
	}

	//총알 그리기
	oldBrush = (HBRUSH)SelectObject(mDC, pGame->hBrushBlack);
	for (int i = 0; i < pGame->bulletcount; i++) {
		if (pGame->bullet[i].shot) { // 활성화된 총알만 그리기
			if (pGame->bullet[i].direct == 1 || pGame->bullet[i].direct == 2) {
				Rectangle(mDC, pGame->bullet[i].x, pGame->bullet[i].y, pGame->bullet[i].x + bulletlen, pGame->bullet[i].y + bulletthick);
			}
			else {
				Rectangle(mDC, pGame->bullet[i].x, pGame->bullet[i].y, pGame->bullet[i].x + bulletthick, pGame->bullet[i].y + bulletlen);
			}
		}
	}
	SelectObject(mDC, oldBrush);

	//주인공 그리기
	hBrush = (pGame->player.moojuk) ? pGame->hBrushYellow : pGame->hBrushBlack;
	oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
	RoundRect(mDC, pGame->player.x, pGame->player.y, pGame->player.x + playersize, pGame->player.y + playersize, 10, 10);
	SelectObject(mDC, oldBrush);

	//회전총알 그리기
	hBrush = (pGame->player.moojuk) ? pGame->hBrushBlack : pGame->hBrushWhite;
	oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
	for (int i = 0; i < 6 - pGame->bulletcount; i++) {
		Ellipse(mDC, pGame->readybullet[i].x, pGame->readybullet[i].y, pGame->readybullet[i].x + circleDiameter, pGame->readybullet[i].y + circleDiameter);
	}
	SelectObject(mDC, oldBrush);

	//적 그리기
	oldBrush = (HBRUSH)SelectObject(mDC, pGame->hBrushRed);
	for (int i = 0; i < 10; i++) {
		if (pGame->enemy_easy[i].life)
			RoundRect(mDC, pGame->enemy_easy[i].x, pGame->enemy_easy[i].y, pGame->enemy_easy[i].x + playersize, pGame->enemy_easy[i].y + playersize, 10, 10);
	}
	SelectObject(mDC, oldBrush);

	//적 폭발 그리기
	oldBrush = (HBRUSH)SelectObject(mDC, pGame->hBrushRed);
	for (int i = 0; i < 10; i++) {
		if (pGame->enemy_easy[i].boom && !pGame->enemy_easy[i].life)
			Ellipse(mDC, pGame->enemy_easy[i].boomx - pGame->enemy_easy[i].boomsize, pGame->enemy_easy[i].boomy - pGame->enemy_easy[i].boomsize, pGame->enemy_easy[i].boomx + pGame->enemy_easy[i].boomsize, pGame->enemy_easy[i].boomy + pGame->enemy_easy[i].boomsize);
	}
	SelectObject(mDC, oldBrush);

	//주인공 사망이펙트 그리기
	oldBrush = (HBRUSH)SelectObject(mDC, pGame->hBrushBlack);
	if (!pGame->player.life)
		Ellipse(mDC, pGame->player.diex - pGame->player.diesize, pGame->player.diey - pGame->player.diesize, pGame->player.diex + pGame->player.diesize, pGame->player.diey + pGame->player.diesize);
	SelectObject(mDC, oldBrush);

	//시간 출력
	wsprintf(lpOut, L"TIME : %d", pGame->time);
	TextOut(mDC, 570, 50, lpOut, lstrlen(lpOut));
	//스코어 출력
	wsprintf(lpOut, L"SCORE : %d", pGame->score);
	TextOut(mDC, 50, 50, lpOut, lstrlen(lpOut));
	//콤보 출력
	wsprintf(lpOut, L"COMBO : %d", pGame->combo);
	TextOut(mDC, 50, 100, lpOut, lstrlen(lpOut));
	//게임 결과 출력
	if (pGame->score >= 10000) {
		wsprintf(lpOut, L"승리!");
		TextOut(mDC, 50, 500, lpOut, lstrlen(lpOut));
	}
	if (pGame->time == 120 && pGame->score < 10000) {
		wsprintf(lpOut, L"패배");
		TextOut(mDC, 50, 500, lpOut, lstrlen(lpOut));
	}
}

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

	// (2) "게임 상태" 패킷 처리 (가장 중요)
	case S_GAME_STATE:
	{
		S_GameStatePacket* pPkt = (S_GameStatePacket*)pHeader;

		// 서버가 보내준 정보로 클라이언트의 GameState를 강제로 덮음.

		// 1. 모든 플레이어의 위치/생존 정보 갱신
		for (int i = 0; i < MAX_PLAYERS; i++) {
			pGame->players[i].x = pPkt->players[i].x;
			pGame->players[i].y = pPkt->players[i].y;
			pGame->players[i].life = pPkt->players[i].life;
		}

		// 2. 모든 적의 위치/생존 정보 갱신
		for (int i = 0; i < MAX_ENEMIES; i++) {
			pGame->enemies[i].x = pPkt->enemies[i].x;
			pGame->enemies[i].y = pPkt->enemies[i].y;
			pGame->enemies[i].life = pPkt->enemies[i].life;
		}
		break;
	}

	// (나중에 S_PLAYER_DIE, S_ATTACK_BROADCAST 등 추가...)

	default:
		printf("알 수 없는 패킷 타입 수신: %d\n", pHeader->type);
		break;
	}
}
