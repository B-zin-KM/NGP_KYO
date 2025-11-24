#include "game.h"

//#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console") //콘솔창 띄우기

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Window Programming";
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

void ClientMainLoop(HWND hWnd) {
	MSG Message = { 0 };

	GameState* pGame = (GameState*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	DWORD prevTime = GetTickCount();

	HDC hDC = GetDC(hWnd);

	while (true) {
		// 1. 윈도우 메시지 처리 (키 입력, 종료 신호 등)
		if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) {
			if (Message.message == WM_QUIT) break;
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
		else {
			// 2. 게임 로직 실행 (메시지가 없을 때)

			if (!pGame) {
				pGame = (GameState*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				continue;
			}
			if (pGame->stop) continue;

			// 3. 시간 계산 (Delta Time)
			DWORD curTime = GetTickCount();
			DWORD deltaTime = curTime - prevTime;

			// 80프레임(약 10ms) 유지
			if (deltaTime < 10) {
				Sleep(1);
				continue;
			}
			prevTime = curTime;

			Game_Update(hWnd, pGame, deltaTime / 1000.0f);

			RECT rt;
			GetClientRect(hWnd, &rt);

			HDC mDC = CreateCompatibleDC(hDC);
			HBITMAP hBitmap = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
			HBITMAP hOldBitmap = (HBITMAP)SelectObject(mDC, hBitmap);

			PatBlt(mDC, 0, 0, rt.right, rt.bottom, WHITENESS);

			Game_Render(mDC, pGame);

			BitBlt(hDC, 0, 0, rt.right, rt.bottom, mDC, 0, 0, SRCCOPY);

			SelectObject(mDC, hOldBitmap);
			DeleteObject(hBitmap);
			DeleteDC(mDC);
		}
	}

	ReleaseDC(hWnd, hDC);
}

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
	//while (GetMessage(&Message, 0, 0, 0)) {
	//	TranslateMessage(&Message);
	//	DispatchMessage(&Message);
	//}
	ClientMainLoop(hWnd);
	return 0;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// GameState 포인터를 HWND의 '유저 데이터'로 관리
	GameState* pGame = (GameState*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	PAINTSTRUCT ps;
	HDC hDC, mDC;
	HBITMAP hBitmap;
	RECT rt;

	switch (uMsg) {
	case WM_CREATE:
	{
		// GameState 객체 생성 및 HWND에 포인터 저장
		GameState* pNewGame = new GameState();
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pNewGame);

		// 게임 초기화
		Game_Init(hWnd, pNewGame); // 분리된 함수 호출
		break;
	}
	case WM_KEYDOWN:
		if (pGame) {
			Game_HandleInput_Down(pGame, wParam); // 분리된 함수 호출

			// 일시정지, 종료 등 즉시 반응해야 하는 키
			switch (wParam) {
			case 'P':
				pGame->stop = !pGame->stop;
				break;
			case VK_ESCAPE:
				PostQuitMessage(0);
				break;
			}
		}
		break;

	case WM_KEYUP:
		if (pGame) {
			Game_HandleInput_Up(pGame, wParam); // 분리된 함수 호출
		}
		break;

	case WM_PAINT:
		GetClientRect(hWnd, &rt);
		hDC = BeginPaint(hWnd, &ps);

		// --- 더블 버퍼링 셋업 ---
		mDC = CreateCompatibleDC(hDC);
		hBitmap = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
		SelectObject(mDC, (HBITMAP)hBitmap);
		Rectangle(mDC, 0, 0, rt.right, rt.bottom); // 배경 흰색

		// --- 모든 그리기를 Game_Render 함수에 위임 ---
		if (pGame) {
			Game_Render(mDC, pGame); // 분리된 함수 호출
		}

		// --- 더블 버퍼링 완료 ---
		BitBlt(hDC, 0, 0, rt.right, rt.bottom, mDC, 0, 0, SRCCOPY);
		DeleteDC(mDC);
		DeleteObject(hBitmap);

		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		if (pGame) {
			Game_Cleanup(pGame); // 분리된 함수 호출
			delete pGame; // WM_CREATE에서 new 했으므로 delete
		}
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}