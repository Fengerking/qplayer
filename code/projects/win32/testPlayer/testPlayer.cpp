#include "stdafx.h"
#include "testPlayer.h"
#include <commctrl.h>
#include <Commdlg.h>
#include <shellapi.h>

#include "CWndVideo.h"
#include "CWndSlider.h"

#include "CTestMng.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE	hInst;								// current instance
TCHAR		szTitle[MAX_LOADSTRING];			// The title bar text
TCHAR		szWindowClass[MAX_LOADSTRING];		// the main window class name

HWND		hWndMain	= NULL;
CWndVideo *	hWndVideo	= NULL;
CWndSlider* hWndSlide	= NULL;

HWND		hLBItem		= NULL;
HWND		hLBFunc		= NULL;
HWND		hLBMsg		= NULL;
HWND		hLBErr		= NULL;


CTestMng 	gTestMng;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR    lpCmdLine, _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TESTPLAYER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTPLAYER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	delete hWndVideo;
	delete hWndSlide;

	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTPLAYER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_GRAYTEXT);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TESTPLAYER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable
	hWndMain = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	if (hWndMain == NULL)
		return FALSE;


	int nScreenX = GetSystemMetrics(SM_CXSCREEN);
	int nScreenY = GetSystemMetrics(SM_CYSCREEN);
//	SetWindowPos(hWndMain, HWND_BOTTOM, 0, 0, nScreenX, nScreenY, 0);
	SetWindowPos(hWndMain, HWND_BOTTOM, 0, 0, nScreenX * 9 / 10, nScreenY * 9 / 10, 0);

	RECT rcView;
	GetClientRect(hWndMain, &rcView);
	hLBItem = CreateWindow ("LISTBOX", NULL,
							WS_CHILD | WS_VSCROLL | WS_VISIBLE | WS_VSCROLL | WS_BORDER| LBS_HASSTRINGS,
							rcView.left, rcView.top, rcView.right * 1 / 8, rcView.bottom / 2, hWndMain, (HMENU)1001, hInst, NULL);
	hLBFunc = CreateWindow("LISTBOX", NULL,
							WS_CHILD | WS_VSCROLL | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_HASSTRINGS,
							rcView.right * 1 / 8, rcView.top, rcView.right * 3 / 8, rcView.bottom / 2, hWndMain, (HMENU)1002, hInst, NULL);
	hLBErr = CreateWindow("LISTBOX", NULL,
							WS_CHILD | WS_VSCROLL | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_HASSTRINGS,
							rcView.left, rcView.bottom / 2, rcView.right / 2, rcView.bottom, hWndMain, (HMENU)1003, hInst, NULL);
	hLBMsg = CreateWindow("LISTBOX", NULL,
							WS_CHILD | WS_VSCROLL | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_HASSTRINGS,
							rcView.right / 2, rcView.bottom / 2, rcView.right, rcView.bottom, hWndMain, (HMENU)1004, hInst, NULL);
	//	SendMessage(hLBAction, LB_ADDSTRING, 0, (LPARAM)"Open URL");

	SetRect(&rcView, rcView.right / 2, rcView.top, rcView.right, rcView.bottom / 2 - 20);
	hWndVideo = new CWndVideo(hInst);
	hWndVideo->CreateWnd(hWndMain, rcView, RGB(0, 0, 0));

	GetClientRect(hWndMain, &rcView);
	SetRect(&rcView, rcView.right / 2, rcView.bottom / 2 - 20, rcView.right, rcView.bottom / 2);
	hWndSlide = new CWndSlider(hInst);
	hWndSlide->CreateWnd(hWndMain, rcView, RGB(100, 100, 100));

	gTestMng.GetInst()->m_hWndMain = hWndMain;
	gTestMng.GetInst()->m_hWndVideo = hWndVideo->GetWnd();
	gTestMng.GetInst()->m_pSlidePos = hWndSlide;
	gTestMng.GetInst()->m_hLBItem = hLBItem;
	gTestMng.GetInst()->m_hLBFunc = hLBFunc;
	gTestMng.GetInst()->m_hLBMsg = hLBMsg;
	gTestMng.GetInst()->m_hLBErr = hLBErr;

	ShowWindow(hWndMain, nCmdShow);
	UpdateWindow(hWndMain);

//	gTestMng.OpenTestFile("C:\\work\\qplayer\\bin\\win32\\autoTest\\baseTest.tst");
//	gTestMng.OpenTestFile("C:\\work\\qplayer\\bin\\win32\\autoTest\\mp4Test.tst");
//	gTestMng.OpenTestFile("C:\\work\\qplayer\\bin\\win32\\autoTest\\loopTest.tst");
//	gTestMng.OpenTestFile("http://opb95n9bi.bkt.clouddn.com/qplayer/autotest/baseTest.tst");

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_FILE_OPEN:
		{
			char				szFile[1024] = { 0 };
			OPENFILENAME		ofn;
			memset(szFile, 0, sizeof(szFile));
			memset(&(ofn), 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hWndMain;
			ofn.lpstrFilter = TEXT("QPlayer Test File (*.tst)\0*.tst\0");
			if (_tcsstr(szFile, _T(":/")) != NULL)
				_tcscpy(szFile, _T("*.*"));
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = MAX_PATH;

			ofn.lpstrTitle = TEXT("Open Test File");
			ofn.Flags = OFN_EXPLORER;
			if (!GetOpenFileName(&ofn))
					return FALSE;

			gTestMng.OpenTestFile(szFile);
		}
			break;

		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_TIMER:
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		gTestMng.ExitTest();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
