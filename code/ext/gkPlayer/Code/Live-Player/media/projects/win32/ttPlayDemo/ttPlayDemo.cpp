// ttPlayDemo.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ttPlayDemo.h"
#include <commctrl.h>
#include <Commdlg.h>
#include <winuser.h>
#include <shellapi.h>

#include "GKMediaPlayer.h"
#include "CWndVideo.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE		g_hInst;								// current instance
HWND			g_hWnd = NULL;
HWND			g_hSldPos = NULL;
CWndVideo *		g_wndVideo = NULL;

TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name


class GPlayListener : public IGKMediaPlayerObserver
{
public:
	virtual void PlayerNotifyEvent(GKNotifyMsg aMsg, TTInt aArg1, TTInt aArg2, const TTChar* aArg3)
	{
		switch (aMsg)
		{
		case ENotifyPrepare:
			break;

		default:
			break;
		}

	};
};


CGKMediaPlayer * gPlayer = NULL;
GPlayListener *	 gListen = NULL;


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TTPLAYDEMO, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TTPLAYDEMO));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TTPLAYDEMO));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TTPLAYDEMO);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   g_hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
   if (!hWnd)
      return FALSE;
   g_hWnd = hWnd;

	SetWindowLong (hWnd, GWL_EXSTYLE, WS_EX_ACCEPTFILES);
	SetWindowPos (hWnd, NULL, (GetSystemMetrics (SM_CXSCREEN) - 818) / 2, (GetSystemMetrics (SM_CYSCREEN) - 498 - 100 ) / 2, 818, 498, 0);

	RECT	rcWnd;
	GetClientRect (hWnd, &rcWnd);
	rcWnd.bottom = rcWnd.bottom - 24;

	g_wndVideo = new CWndVideo (g_hInst);
	g_wndVideo->CreateWnd (hWnd, rcWnd, RGB (0, 0, 0));

	rcWnd.top = rcWnd.bottom;
	rcWnd.bottom = rcWnd.bottom + 24;
	g_hSldPos = CreateWindow (_T("msctls_trackbar32"), _T(""), WS_VISIBLE | WS_CHILD | TBS_BOTH | TBS_NOTICKS, 
							rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top,
							hWnd, (HMENU)1001, g_hInst, NULL);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);


	gListen = new GPlayListener ();
	gPlayer = new CGKMediaPlayer (gListen, NULL);
	gPlayer->SetView (g_wndVideo->GetWnd ());

	SetTimer (hWnd, 2002, 500, NULL);
	return TRUE;
}

int OpenMediaFile (HWND hWnd)
{
	DWORD				dwID = 0;
	OPENFILENAME		ofn;
	TCHAR				szFile[256];

	memset (szFile, 0, sizeof (szFile));
	memset( &(ofn), 0, sizeof(ofn));
	ofn.lStructSize	= sizeof(ofn);
	ofn.hwndOwner = hWnd;

	ofn.lpstrFilter = TEXT("Media File (*.*)\0*.*\0");	
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;

	ofn.lpstrTitle = TEXT("Open Media File");
	ofn.Flags = OFN_EXPLORER;
			
	if (!GetOpenFileName(&ofn))
		return -1;

	int nRC = 0;
//	gPlayer->Stop ();
//	strcpy (szFile, "http://42.121.109.19/Files/hls/v10/bipbop_16x9_variant_v10_2.m3u8");
	strcpy (szFile, "http://42.121.109.19/Files/hls/v8/bipbopall.m3u8");
	nRC = gPlayer->SetDataSourceSync (szFile, 0);
	if (nRC == 0)
		gPlayer->Play ();

	return nRC;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
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
			OpenMediaFile (hWnd);
			break;

		case IDM_ABOUT:
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_HSCROLL:
		if (gPlayer != NULL)
		{
			int nPos = SendMessage (g_hSldPos, TBM_GETPOS, 0, 0);
			gPlayer->SetPosition (nPos * 1000);
		}
		break;

	case WM_TIMER:
		if (gPlayer != NULL)
			SendMessage (g_hSldPos, TBM_SETPOS, TRUE, (LPARAM)(gPlayer->GetPosition ()/1000));
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		if (gPlayer != NULL)
		{
			delete gPlayer;
			delete gListen;
		}
		if (g_wndVideo != NULL)
		{
			SendMessage (g_wndVideo->GetWnd (), WM_CLOSE, 0, 0);
			delete g_wndVideo;
			g_wndVideo = NULL;
		}

		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
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
