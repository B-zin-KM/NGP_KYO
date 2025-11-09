#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include <math.h>
//#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console") //콘솔창 띄우기

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Window Programming";
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR ipszCmdParam, int nCmdShow)
{
	srand(time(NULL));
	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	g_hInst = hInstance;
	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&WndClass);
	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL, 0, 0, 1200, 900, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return Message.wParam;
}

#define boardsize 35
#define playersize 30
#define bulletlen 18
#define bulletthick 8
#define circleDiameter 9
#define circleRadius 4.5

struct BOARD {
	int x;
	int y;
	int color;
	bool value = FALSE;
};
//15 X 10
BOARD board_easy[150];
//20 X 15
BOARD board_mid[300];
//25 X 20
BOARD board_hard[500];

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
}player;

struct BULLET {
	int x = -100;
	int y = -100;
	int direct;
	bool shot = FALSE;
}bullet[6];

struct READYBULLET {
	int x;
	int y;
	bool shot = FALSE;
}readybullet[6];

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
//easy 난이도
ENEMY enemy_easy[10];
//mid 난이도
ENEMY enemy_mid[20];
//hard 난이도
ENEMY enemy_hard[30];

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

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC, mDC;
	HBITMAP hBitmap;
	RECT rt;
	HBRUSH hBrush, oldBrush;
	TCHAR lpOut[100];
	static int boardnum = 0;
	static int bulletcount = 0;
	static int angles[6];
	static int enemyloc[10];
	static int time = -10;
	static int enemycnt = 0;
	static int enemyspawn = 0;
	static int score = 0;
	static int combo = 0;
	static bool stop = FALSE;

	switch (uMsg) {
	case WM_CREATE:
		//발사총알 타이머
		SetTimer(hWnd, 2, 1, NULL);
		//회전총알 타이머
		SetTimer(hWnd, 3, 16, NULL);
		//적 생성 타이머
		SetTimer(hWnd, 4, 1000, NULL);
		//적 이동 타이머
		SetTimer(hWnd, 5, 40, NULL);
		//적 폭발 타이머
		SetTimer(hWnd, 6, 5, NULL);
		//주인공 사망이펙트 타이머
		SetTimer(hWnd, 7, 16, NULL);
		//보드 생성
		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 15; j++) {
				board_easy[boardnum].x = j * boardsize + 335;
				board_easy[boardnum].y = i * boardsize + 240;
				boardnum++;
			}
		}
		for (int j = 3; j < 7; j++) {
			for (int k = 5; k < 10; k++) {
				board_easy[j * 15 + k].value = TRUE;
			}
		}
		boardnum = 0;
		//주인공 초기위치
		player.x = 600;
		player.y = 400;
		//회전총알 초기위치
		for (int i = 0; i < 6; i++) {
			angles[i] = 60 * i;
		}
		for (int i = 0; i < 6; i++) {
			double radians = angles[i] * 3.14159 / 180.0;
			readybullet[i].x = player.x + playersize / 2 + static_cast<int>((playersize / 2 - circleRadius - 0.5) * cos(radians)) - circleRadius;
			readybullet[i].y = player.y + playersize / 2 + static_cast<int>((playersize / 2 - circleRadius - 0.5) * sin(radians)) - circleRadius;
		}
		//적 초기위치
		for (int i = 0; i < 10; i++) {
			enemyloc[i] = rand() % 150;
		}
		for (int j = 0; j < 10; j++) {
			enemy_easy[j].x = board_easy[enemyloc[j]].x;
			enemy_easy[j].y = board_easy[enemyloc[j]].y;
		}
		break;
	case WM_TIMER:
		switch (wParam) {
			//주인공 이동
		case 1:
			if (player.direct == 1) {
				player.x += 4;
				for (int i = 0; i < 150; i++) {
					if (playerVSboard(player.x, player.y, board_easy[i].x, board_easy[i].y)) {
						if (!board_easy[i].value) {
							if (player.life) {
								if (!player.moojuk)
									player.x = board_easy[i].x - boardsize;
							}
						}
					}
				}
				if (player.x + playersize >= board_easy[14].x + boardsize) {
					player.x = board_easy[14].x;
				}
			}
			if (player.direct == 2) {
				player.x -= 4;
				for (int i = 0; i < 150; i++) {
					if (playerVSboard(player.x, player.y, board_easy[i].x, board_easy[i].y)) {
						if (!board_easy[i].value) {
							if (player.life) {
								if (!player.moojuk)
									player.x = board_easy[i].x + boardsize * 2 - playersize;
							}
						}
					}
				}
				if (player.x <= board_easy[0].x) {
					player.x = board_easy[0].x + boardsize - playersize;
				}
			}
			if (player.direct == 3) {
				player.y += 4;
				for (int i = 0; i < 150; i++) {
					if (playerVSboard(player.x, player.y, board_easy[i].x, board_easy[i].y)) {
						if (!board_easy[i].value) {
							if (player.life) {
								if (!player.moojuk)
									player.y = board_easy[i].y - boardsize;
							}
						}
					}
				}
				if (player.y + playersize >= board_easy[149].y + boardsize) {
					player.y = board_easy[149].y;
				}
			}
			if (player.direct == 4) {
				player.y -= 4;
				for (int i = 0; i < 150; i++) {
					if (playerVSboard(player.x, player.y, board_easy[i].x, board_easy[i].y)) {
						if (!board_easy[i].value) {
							if (player.life) {
								if (!player.moojuk)
									player.y = board_easy[i].y + boardsize * 2 - playersize;
							}
						}
					}
				}
				if (player.y <= board_easy[0].y) {
					player.y = board_easy[0].y + boardsize - playersize;
				}
			}
			//회전총알 이동
			for (int i = 0; i < 6; i++) {
				double radians = angles[i] * 3.14159 / 180.0;
				readybullet[i].x = player.x + playersize / 2 + static_cast<int>((playersize / 2 - circleRadius - 0.5) * cos(radians)) - circleRadius;
				readybullet[i].y = player.y + playersize / 2 + static_cast<int>((playersize / 2 - circleRadius - 0.5) * sin(radians)) - circleRadius;
			}
			player.diex = player.x + 15;
			player.diey = player.y + 15;
			break;
			//총알 발사
		case 2:
			for (int i = 0; i < bulletcount; i++) {
				if (bullet[i].shot) {
					if (bullet[i].direct == 1) {
						bullet[i].x += 15;
					}
					if (bullet[i].direct == 2) {
						bullet[i].x -= 15;
					}
					if (bullet[i].direct == 3) {
						bullet[i].y += 15;
					}
					if (bullet[i].direct == 4) {
						bullet[i].y -= 15;
					}
				}
			}
			for (int i = 0; i < 150; i++) {
				for (int j = 0; j < bulletcount; j++) {
					if (bullet[j].direct == 1 || bullet[j].direct == 2) {
						if (bulletVSboard(bullet[j].x, bullet[j].y, board_easy[i].x, board_easy[i].y)) {
							board_easy[i].value = TRUE;
						}
					}
					else {
						if (bulletVSboard2(bullet[j].x, bullet[j].y, board_easy[i].x, board_easy[i].y)) {
							board_easy[i].value = TRUE;
						}
					}
				}
			}
			for (int i = 0; i < 10; i++) {
				for (int j = 0; j < bulletcount; j++) {
					if (bullet[j].direct == 1 || bullet[j].direct == 2) {
						if (bulletVSplayer(bullet[j].x, bullet[j].y, enemy_easy[i].x, enemy_easy[i].y)) {
							if (enemy_easy[i].life) {
								enemy_easy[i].boomx = enemy_easy[i].x + 15;
								enemy_easy[i].boomy = enemy_easy[i].y + 15;
								enemy_easy[i].boom = TRUE;
								enemyloc[i] = rand() % 150;
								enemy_easy[i].x = board_easy[enemyloc[i]].x;
								enemy_easy[i].y = board_easy[enemyloc[i]].y;
								enemy_easy[i].life = FALSE;
								bullet[j].x = -1000;
								bullet[j].y = -1000;
								combo++;
							}
						}
					}
					else {
						if (bulletVSplayer2(bullet[j].x, bullet[j].y, enemy_easy[i].x, enemy_easy[i].y)) {
							if (enemy_easy[i].life) {
								enemy_easy[i].boomx = enemy_easy[i].x + 15;
								enemy_easy[i].boomy = enemy_easy[i].y + 15;
								enemy_easy[i].boom = TRUE;
								enemyloc[i] = rand() % 150;
								enemy_easy[i].x = board_easy[enemyloc[i]].x;
								enemy_easy[i].y = board_easy[enemyloc[i]].y;
								enemy_easy[i].life = FALSE;
								bullet[j].x = -1000;
								bullet[j].y = -1000;
								score += 100;
								combo++;
							}
						}
					}
				}
			}
			break;
			//총알 회전
		case 3:
			for (int i = 0; i < 6; i++) {
				angles[i] += 5;
				if (angles[i] >= 360) angles[i] -= 360;
			}
			//회전총알 이동
			for (int i = 0; i < 6; i++) {
				double radians = angles[i] * 3.14159 / 180.0;
				readybullet[i].x = player.x + playersize / 2 + static_cast<int>((playersize / 2 - circleRadius - 0.5) * cos(radians)) - circleRadius;
				readybullet[i].y = player.y + playersize / 2 + static_cast<int>((playersize / 2 - circleRadius - 0.5) * sin(radians)) - circleRadius;
			}
			break;
			//적 생성
		case 4:
			time++;
			score += 10;
			if (time % 5 == 0) {
				enemycnt += enemyspawn;
				if (time % 20 == 0)
					enemyspawn++;
				for (int i = 0; i < enemycnt; i++) {
					enemy_easy[i].life = TRUE;
					enemy_easy[i].boom = FALSE;
				}
			}
			if (time == 120) {
				KillTimer(hWnd, 2);
				KillTimer(hWnd, 3);
				KillTimer(hWnd, 4);
				KillTimer(hWnd, 5);
				KillTimer(hWnd, 6);
				KillTimer(hWnd, 7);
			}
			break;
			//적 이동
		case 5:
			for (int i = 0; i < 10; i++) {
				if (enemy_easy[i].life) {
					enemy_easy[i].deltaX = player.x - enemy_easy[i].x;
					enemy_easy[i].deltaY = player.y - enemy_easy[i].y;
					float distance = sqrt(enemy_easy[i].deltaX * enemy_easy[i].deltaX + enemy_easy[i].deltaY * enemy_easy[i].deltaY);
					if (distance > 0) {
						enemy_easy[i].directionX = enemy_easy[i].deltaX / distance;
						enemy_easy[i].directionY = enemy_easy[i].deltaY / distance;
						enemy_easy[i].x += enemy_easy[i].directionX * 1.5;
						enemy_easy[i].y += enemy_easy[i].directionY * 1.5;
					}
				}
			}
			for (int j = 0; j < 10; j++) {
				for (int i = 0; i < 150; i++) {
					if (playerVSboard(enemy_easy[j].x, enemy_easy[j].y, board_easy[i].x, board_easy[i].y)) {
						if (enemy_easy[j].life)
							board_easy[i].value = FALSE;
					}
				}
			}
			for (int i = 0; i < 10; i++) {
				if (playerVSenemy(player.x, player.y, enemy_easy[i].x, enemy_easy[i].y)) {
					if (player.life) {
						if (!player.moojuk) {
							if (enemy_easy[i].life) {
								player.life = FALSE;
								player.diex = player.x + 15;
								player.diey = player.y + 15;
								score -= 500;
								combo = 0;
							}
						}
					}
				}
			}
			break;
			//적 폭발
		case 6:
			for (int i = 0; i < 10; i++) {
				if (enemy_easy[i].boom) {
					if (enemy_easy[i].boomtime != 10) {
						enemy_easy[i].boomtime++;
						enemy_easy[i].boomsize += 4;
					}
					if (enemy_easy[i].boomtime == 10) {
						enemy_easy[i].boomtime = 0;
						enemy_easy[i].boomsize = 15;
						enemy_easy[i].boom = FALSE;
					}
				}
			}
			for (int i = 0; i < 10; i++) {
				for (int j = 0; j < 10; j++) {
					if (enemyVSboom(enemy_easy[i].x, enemy_easy[i].y, enemy_easy[j].boomx, enemy_easy[j].boomy, enemy_easy[j].boomsize)) {
						if (enemy_easy[i].life) {
							if (enemy_easy[j].boom) {
								if (i != j) {
									enemy_easy[i].boomx = enemy_easy[i].x + 15;
									enemy_easy[i].boomy = enemy_easy[i].y + 15;
									enemy_easy[i].boom = TRUE;
									enemyloc[i] = rand() % 150;
									enemy_easy[i].x = board_easy[enemyloc[i]].x;
									enemy_easy[i].y = board_easy[enemyloc[i]].y;
									enemy_easy[i].life = FALSE;
									score += 100;
									combo++;
								}
							}
						}
					}
				}
			}
			for (int i = 0; i < 150; i++) {
				for (int j = 0; j < 10; j++) {
					if (boardVSboom(board_easy[i].x, board_easy[i].y, enemy_easy[j].boomx, enemy_easy[j].boomy, enemy_easy[j].boomsize)) {
						if (enemy_easy[j].boom) {
							board_easy[i].value = TRUE;
						}
					}
				}
			}
			break;
			//주인공 사망 이펙트
		case 7:
			if (!player.life) {
				if (player.dietime != 40) {
					player.dietime++;
					player.diesize += 1;
				}
				if (player.dietime == 40) {
					player.dietime = 0;
					player.diesize = 15;
					player.life = TRUE;
				}
			}
			for (int i = 0; i < 10; i++) {
				if (enemyVSboom(enemy_easy[i].x, enemy_easy[i].y, player.diex, player.diey, player.diesize)) {
					if (enemy_easy[i].life) {
						enemy_easy[i].boomx = enemy_easy[i].x + 15;
						enemy_easy[i].boomy = enemy_easy[i].y + 15;
						enemy_easy[i].boom = TRUE;
						enemyloc[i] = rand() % 150;
						enemy_easy[i].x = board_easy[enemyloc[i]].x;
						enemy_easy[i].y = board_easy[enemyloc[i]].y;
						enemy_easy[i].life = FALSE;
						combo++;
					}
				}
			}
			break;
		}
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_KEYDOWN:
		hDC = GetDC(hWnd);
		switch (wParam) {
		case 'd':
		case 'D':
			player.direct = 1;
			SetTimer(hWnd, 1, 1, NULL);
			break;
		case 'a':
		case 'A':
			player.direct = 2;
			SetTimer(hWnd, 1, 1, NULL);
			break;
		case 's':
		case 'S':
			player.direct = 3;
			SetTimer(hWnd, 1, 1, NULL);
			break;
		case 'w':
		case 'W':
			player.direct = 4;
			SetTimer(hWnd, 1, 1, NULL);
			break;
		case VK_RIGHT:
			if (bulletcount < 6) {
				bulletcount++;
				for (int i = 0; i < bulletcount; i++) {
					if (!bullet[i].shot) {
						bullet[i].direct = 1;
						bullet[i].x = player.x + 6; bullet[i].y = player.y + 11;
						bullet[i].shot = TRUE;
					}
				}
			}
			break;
		case VK_LEFT:
			if (bulletcount < 6) {
				bulletcount++;
				for (int i = 0; i < bulletcount; i++) {
					if (!bullet[i].shot) {
						bullet[i].direct = 2;
						bullet[i].x = player.x + 6; bullet[i].y = player.y + 11;
						bullet[i].shot = TRUE;
					}
				}
			}
			break;
		case VK_DOWN:
			if (bulletcount < 6) {
				bulletcount++;
				for (int i = 0; i < bulletcount; i++) {
					if (!bullet[i].shot) {
						bullet[i].direct = 3;
						bullet[i].x = player.x + 11; bullet[i].y = player.y + 6;
						bullet[i].shot = TRUE;
					}
				}
			}
			break;
		case VK_UP:
			if (bulletcount < 6) {
				bulletcount++;
				for (int i = 0; i < bulletcount; i++) {
					if (!bullet[i].shot) {
						bullet[i].direct = 4;
						bullet[i].x = player.x + 11; bullet[i].y = player.y + 6;
						bullet[i].shot = TRUE;
					}
				}
			}
			break;
		case 'r':
		case 'R':
			bulletcount = 0;
			for (int i = 0; i < 6; i++) {
				bullet[i].shot = FALSE;
			}
			break;
		case '1':
			if (!player.moojuk) player.moojuk = TRUE;
			else player.moojuk = FALSE;
			break;
		case 'p':
		case 'P':
			if (!stop) {
				KillTimer(hWnd, 2);
				KillTimer(hWnd, 3);
				KillTimer(hWnd, 4);
				KillTimer(hWnd, 5);
				KillTimer(hWnd, 6);
				KillTimer(hWnd, 7);
				stop = TRUE;
			}
			else {
				SetTimer(hWnd, 2, 1, NULL);
				SetTimer(hWnd, 3, 16, NULL);
				SetTimer(hWnd, 4, 1000, NULL);
				SetTimer(hWnd, 5, 40, NULL);
				SetTimer(hWnd, 6, 5, NULL);
				SetTimer(hWnd, 7, 16, NULL);
				stop = FALSE;
			}
			break;
		case VK_ESCAPE:
			PostQuitMessage(0);
			break;
		}
		InvalidateRect(hWnd, NULL, FALSE);
		ReleaseDC(hWnd, hDC);
		break;
	case WM_KEYUP:
		hDC = GetDC(hWnd);
		switch (wParam) {
		case 'd':
		case 'D':
			KillTimer(hWnd, 1);
			break;
		case 'a':
		case 'A':
			KillTimer(hWnd, 1);
			break;
		case 's':
		case 'S':
			KillTimer(hWnd, 1);
			break;
		case 'w':
		case 'W':
			KillTimer(hWnd, 1);
			break;
		}
		InvalidateRect(hWnd, NULL, FALSE);
		ReleaseDC(hWnd, hDC);
		break;
	case WM_PAINT:
		GetClientRect(hWnd, &rt);                                       //--- 윈도우 크기 얻어오기
		hDC = BeginPaint(hWnd, &ps);
		mDC = CreateCompatibleDC(hDC);                                  //--- 메모리 DC 만들기
		hBitmap = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);     //--- 메모리 DC와 연결할 비트맵 만들기
		SelectObject(mDC, (HBITMAP)hBitmap);                            //--- 메모리 DC와 비트맵 연결하기
		Rectangle(mDC, 0, 0, rt.right, rt.bottom);
		//보드 그리기
		Rectangle(mDC, 334, 239, 861, 591);
		for (int i = 0; i < 150; i++) {
			if (board_easy[i].value) {
				board_easy[i].color = 255;
			}
			else board_easy[i].color = 30;
		}
		for (int i = 0; i < 150; i++) {
			hBrush = CreateSolidBrush(RGB(board_easy[i].color, board_easy[i].color, board_easy[i].color));
			oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
			Rectangle(mDC, board_easy[i].x, board_easy[i].y, board_easy[i].x + boardsize, board_easy[i].y + boardsize);
			SelectObject(mDC, oldBrush);
			DeleteObject(hBrush);
		}
		//총알 그리기
		hBrush = CreateSolidBrush(RGB(0, 0, 0));
		oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
		for (int i = 0; i < bulletcount; i++) {
			if (bullet[i].direct == 1) {
				Rectangle(mDC, bullet[i].x, bullet[i].y, bullet[i].x + bulletlen, bullet[i].y + bulletthick);
			}
			if (bullet[i].direct == 2) {
				Rectangle(mDC, bullet[i].x, bullet[i].y, bullet[i].x + bulletlen, bullet[i].y + bulletthick);
			}
			if (bullet[i].direct == 3) {
				Rectangle(mDC, bullet[i].x, bullet[i].y, bullet[i].x + bulletthick, bullet[i].y + bulletlen);
			}
			if (bullet[i].direct == 4) {
				Rectangle(mDC, bullet[i].x, bullet[i].y, bullet[i].x + bulletthick, bullet[i].y + bulletlen);
			}
		}
		SelectObject(mDC, oldBrush);
		DeleteObject(hBrush);
		//주인공 그리기
		if (!player.moojuk) {
			hBrush = CreateSolidBrush(RGB(0, 0, 0));
			oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
			RoundRect(mDC, player.x, player.y, player.x + playersize, player.y + playersize, 10, 10);
			SelectObject(mDC, oldBrush);
			DeleteObject(hBrush);
		}
		else {
			hBrush = CreateSolidBrush(RGB(255, 255, 0));
			oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
			RoundRect(mDC, player.x, player.y, player.x + playersize, player.y + playersize, 10, 10);
			SelectObject(mDC, oldBrush);
			DeleteObject(hBrush);
		}
		//회전총알 그리기
		if (!player.moojuk) {
			hBrush = CreateSolidBrush(RGB(255, 255, 255));
			oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
			for (int i = 0; i < 6 - bulletcount; i++) {
				Ellipse(mDC, readybullet[i].x, readybullet[i].y, readybullet[i].x + circleDiameter, readybullet[i].y + circleDiameter);
			}
			SelectObject(mDC, oldBrush);
			DeleteObject(hBrush);
		}
		else {
			hBrush = CreateSolidBrush(RGB(0, 0, 0));
			oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
			for (int i = 0; i < 6 - bulletcount; i++) {
				Ellipse(mDC, readybullet[i].x, readybullet[i].y, readybullet[i].x + circleDiameter, readybullet[i].y + circleDiameter);
			}
			SelectObject(mDC, oldBrush);
			DeleteObject(hBrush);
		}
		//적 그리기
		hBrush = CreateSolidBrush(RGB(255, 0, 0));
		oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
		for (int i = 0; i < 10; i++) {
			if (enemy_easy[i].life)
				RoundRect(mDC, enemy_easy[i].x, enemy_easy[i].y, enemy_easy[i].x + playersize, enemy_easy[i].y + playersize, 10, 10);
		}
		SelectObject(mDC, oldBrush);
		DeleteObject(hBrush);
		//적 폭발 그리기
		hBrush = CreateSolidBrush(RGB(255, 0, 0));
		oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
		for (int i = 0; i < 10; i++) {
			if (enemy_easy[i].boom && !enemy_easy[i].life)
				Ellipse(mDC, enemy_easy[i].boomx - enemy_easy[i].boomsize, enemy_easy[i].boomy - enemy_easy[i].boomsize, enemy_easy[i].boomx + enemy_easy[i].boomsize, enemy_easy[i].boomy + enemy_easy[i].boomsize);
		}
		SelectObject(mDC, oldBrush);
		DeleteObject(hBrush);
		//주인공 사망이펙트 그리기
		hBrush = CreateSolidBrush(RGB(0, 0, 0));
		oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
		if (!player.life)
			Ellipse(mDC, player.diex - player.diesize, player.diey - player.diesize, player.diex + player.diesize, player.diey + player.diesize);
		SelectObject(mDC, oldBrush);
		DeleteObject(hBrush);
		//시간 출력
		wsprintf(lpOut, L"TIME : %d", time);
		TextOut(mDC, 570, 50, lpOut, lstrlen(lpOut));
		//스코어 출력
		wsprintf(lpOut, L"SCORE : %d", score);
		TextOut(mDC, 50, 50, lpOut, lstrlen(lpOut));
		//콤보 출력
		wsprintf(lpOut, L"COMBO : %d", combo);
		TextOut(mDC, 50, 100, lpOut, lstrlen(lpOut));
		//게임 결과 출력
		if (score >= 10000) {
			wsprintf(lpOut, L"승리!");
			TextOut(mDC, 50, 500, lpOut, lstrlen(lpOut));
		}
		if (time == 120 && score < 10000) {
			wsprintf(lpOut, L"패배");
			TextOut(mDC, 50, 500, lpOut, lstrlen(lpOut));
		}
		//더블 버퍼링
		BitBlt(hDC, 0, 0, rt.right, rt.bottom, mDC, 0, 0, SRCCOPY);     //--- 마지막에 메모리 DC의 내용을 화면 DC로 복사한다.
		DeleteDC(mDC);                                                  //--- 생성한 메모리 DC 삭제
		DeleteObject(hBitmap);                                          //--- 생성한 비트맵 삭제
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}