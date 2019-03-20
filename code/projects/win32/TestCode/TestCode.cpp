// TestCode.cpp : Defines the entry point for the application.
//
#include <winsock2.h>
#include "Ws2tcpip.h"
#include "stdafx.h"
#include <commctrl.h>
#include <Commdlg.h>
#include <winuser.h>
#include <shellapi.h>

#include "TestCode.h"
#include "CWndVideo.h"
#include "CWndSlider.h"

#include "CDlgOpenURL.h"
#include "CDlgDebug.h"
#include "CDlgSetting.h"

#include "qcPlayer.h"
#include "qcCodec.h"
#include "CVideoDecRnd.h"
#include "CDDrawRnd.h"
#include "CWaveOutRnd.h"
#include "CExtSource.h"

#include "COMBoxMng.h"

#include "COpenSSL.h"
#include "CHTTPClient.h"

#include "CQCVideoEnc.h"
#include "CFileIO.h"
#include "CDNSLookup.h"
#include "CMsgMng.h"

#include "UTestParser.h"
#include "ULibFunc.h"
#include "ULOgFunc.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE		g_hInst;								// current instance
HWND			g_hWnd;
CWndSlider *	g_sldPos = NULL;
CWndVideo *		g_wndVideo = NULL;
CDlgDebug *		g_dlgDebug = NULL;
CDlgSetting *	g_dlgSet = NULL;

COpenSSL *		g_pOpenSSL = NULL;
CHTTPClient *	g_pHTTPClient = NULL;
CExtSource *	g_pExtSource = NULL;

QC_VIDEO_BUFF *	g_pVideoRndBuff = NULL;

char szTitle[MAX_LOADSTRING];					// The title bar text
char szWindowClass[MAX_LOADSTRING];			// the main window class name

QCM_Player		g_player;
CVideoDecRnd *	g_pVDecRnd = NULL;
//CTestParser		g_testParser;

void * 			g_hDllPlay = NULL;
CDDrawRnd *		m_pRndVideo = NULL;
CWaveOutRnd *	m_pRndAudio = NULL;
double			g_aSpeed[] = {0.2, 0.5, 0.8, 1.0, 1.5, 2, 5,};
int				m_nStreamCount = 1;
TCHAR			g_szPlayURL[2048];
bool			g_bForceClosed = false;
int				g_nAutoTestIndex = 0;

#define			WM_MSG_EVENT	WM_USER + 101

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

BOOL				UpdateStreamMenu (void);
BOOL				OpenTestFile (char * pFile);

#pragma warning (disable : 4996)

#define			WMD_STREAM_BASE		40000
#define			WMD_VIDEO_BASE		41000
#define			WMD_AUDIO_BASE		42000
#define			WMD_SUBTT_BASE		43000

//unsigned char   g_szQiniuDrmKey[16] = { 0x6b, 0x64, 0x6e, 0x6c, 0x6a, 0x6a, 0x6c, 0x63, 0x6e, 0x32, 0x69, 0x75, 0x32, 0x33, 0x38, 0x34 };
unsigned char   g_szQiniuDrmKey[16] = { 0x64, 0x48, 0x38, 0x6a, 0x71, 0x53, 0x68, 0x78, 0x57, 0x43, 0x4a, 0x4e, 0x70, 0x77, 0x6c, 0x78 };

char			g_szTestFile[5][256];

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
	LoadString(hInstance, IDC_TESTCODE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
		return FALSE;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTCODE));
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

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTCODE));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TESTCODE);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	return RegisterClassEx(&wcex);
}

int RecvVideoBuff(void * pUserData, QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || pBuff->pBuffPtr == NULL)
		return QC_ERR_ARG;
	if (pBuff->uBuffType != QC_BUFF_TYPE_Video)
		return QC_ERR_ARG;

	QC_VIDEO_BUFF * pVideoBuff = (QC_VIDEO_BUFF *)pBuff->pBuffPtr;
	if (g_pVideoRndBuff == NULL)
	{
		g_pVideoRndBuff = new QC_VIDEO_BUFF();
		memset(g_pVideoRndBuff, 0, sizeof(QC_VIDEO_BUFF));
	}
	if (g_pVideoRndBuff->nWidth < pVideoBuff->nWidth || g_pVideoRndBuff->nHeight < pVideoBuff->nHeight)
	{
		QC_DEL_A(g_pVideoRndBuff->pBuff[0]);
		QC_DEL_A(g_pVideoRndBuff->pBuff[1]);
		QC_DEL_A(g_pVideoRndBuff->pBuff[2]);
	}
	g_pVideoRndBuff->nType = pVideoBuff->nType;
	g_pVideoRndBuff->nWidth = pVideoBuff->nWidth;
	g_pVideoRndBuff->nHeight = pVideoBuff->nHeight;
	g_pVideoRndBuff->nRatioDen = pVideoBuff->nRatioDen;
	g_pVideoRndBuff->nRatioNum = pVideoBuff->nRatioNum;
	if (g_pVideoRndBuff->pBuff[0] == NULL)
	{
		g_pVideoRndBuff->pBuff[0] = new unsigned char[pVideoBuff->nWidth * pVideoBuff->nHeight];
		g_pVideoRndBuff->pBuff[1] = new unsigned char[pVideoBuff->nWidth * pVideoBuff->nHeight / 4];
		g_pVideoRndBuff->pBuff[2] = new unsigned char[pVideoBuff->nWidth * pVideoBuff->nHeight / 4];
		g_pVideoRndBuff->nStride[0] = pVideoBuff->nWidth;
		g_pVideoRndBuff->nStride[1] = g_pVideoRndBuff->nStride[0] / 2;
		g_pVideoRndBuff->nStride[2] = g_pVideoRndBuff->nStride[0] / 2;
	}
	int h = 0;
	for (h = 0; h < pVideoBuff->nHeight; h++)
	{
		memcpy(g_pVideoRndBuff->pBuff[0] + h * g_pVideoRndBuff->nStride[0], pVideoBuff->pBuff[0] + pVideoBuff->nStride[0] * h, pVideoBuff->nWidth);
	}
	for (h = 0; h < pVideoBuff->nHeight / 2; h++)
	{
		memcpy(g_pVideoRndBuff->pBuff[1] + h * g_pVideoRndBuff->nStride[1], pVideoBuff->pBuff[1] + pVideoBuff->nStride[1] * h, pVideoBuff->nWidth / 2);
		memcpy(g_pVideoRndBuff->pBuff[2] + h * g_pVideoRndBuff->nStride[2], pVideoBuff->pBuff[2] + pVideoBuff->nStride[2] * h, pVideoBuff->nWidth / 2);
	}

	// Draw Box
	int nLeft = 100;
	int nTop = 80;
	int nRight = 160;
	int nBottom = 140;
	unsigned char * pY = g_pVideoRndBuff->pBuff[0] + nTop * g_pVideoRndBuff->nStride[0] + nLeft;
	memset(pY, 255, nRight - nLeft);
	pY = g_pVideoRndBuff->pBuff[0] + nBottom * g_pVideoRndBuff->nStride[0] + nLeft;
	memset(pY, 255, nRight - nLeft);
	for (h = nTop; h < nBottom; h++)
	{
		pY = g_pVideoRndBuff->pBuff[0] + h * g_pVideoRndBuff->nStride[0] + nLeft;
		*pY = 255;
		pY = g_pVideoRndBuff->pBuff[0] + h * g_pVideoRndBuff->nStride[0] + nRight;
		*pY = 255;
	}

	unsigned char * pU = g_pVideoRndBuff->pBuff[1] + nTop * g_pVideoRndBuff->nStride[1] / 2 + nLeft / 2;
	unsigned char * pV = g_pVideoRndBuff->pBuff[2] + nTop * g_pVideoRndBuff->nStride[2] / 2 + nLeft / 2;
	memset(pU, 127, (nRight - nLeft) / 2);
	memset(pV, 127, (nRight - nLeft) / 2);
	for (h = nTop / 2; h < nBottom / 2; h++)
	{
		pU = g_pVideoRndBuff->pBuff[1] + h * g_pVideoRndBuff->nStride[1] + nLeft / 2;
		*pU = 127;
		pU = g_pVideoRndBuff->pBuff[1] + h * g_pVideoRndBuff->nStride[1] + nRight / 2;
		*pU = 127;

		pV = g_pVideoRndBuff->pBuff[2] + h * g_pVideoRndBuff->nStride[2] + nLeft / 2;
		*pV = 127;
		pV = g_pVideoRndBuff->pBuff[2] + h * g_pVideoRndBuff->nStride[2] + nRight / 2;
		*pV = 127;
	}

	pBuff->pData = g_pVideoRndBuff;
	pBuff->nDataType = QC_BUFF_TYPE_Video;

	return QC_ERR_NONE;
}

void NotifyEvent (void * pUserData, int nID, void * pValue1)
{
//	QCLOGT ("TestCode", "Notify Event id: %08X", nID);
	if (g_pExtSource != NULL)
		g_pExtSource->NotifyEvent(pUserData, nID, pValue1);

	if (nID == QC_MSG_PLAY_OPEN_DONE)
	{
		//g_player.SetParam(g_player.hPlayer, QCPLAY_PID_SendOut_VideoBuff, RecvVideoBuff);

		InvalidateRect (g_wndVideo->GetWnd (), NULL, TRUE);
		long long llDur = g_player.GetDur (g_player.hPlayer);
		g_sldPos->SetRange (0, (int)llDur);
//		g_player.SetPos (g_player.hPlayer, 6000);

		UpdateStreamMenu ();

		int nSeekMode = 1;
		//g_player.SetParam(g_player.hPlayer, QCPLAY_PID_Seek_Mode, &nSeekMode);
		// g_player.SetParam(g_player.hPlayer, QCPLAY_PID_NET_CHANGED, NULL);

		g_player.SetParam(g_player.hPlayer, QCPLAY_PID_DEL_Cache, g_szTestFile[2]);

		g_player.Run (g_player.hPlayer);
	}
	else if (nID == QC_MSG_PLAY_OPEN_FAILED)
	{
		if (g_bForceClosed == true)
			return;
		InvalidateRect (g_wndVideo->GetWnd (), NULL, TRUE);
		if (!g_bForceClosed)
			SetWindowText(g_hWnd, "Open File failed!");
		//MessageBox (g_hWnd, "Open File failed!", "Error", MB_OK);
	}
	else if (nID == QC_MSG_PLAY_SEEK_DONE)
	{
		if (!g_bForceClosed)
			SetWindowText(g_hWnd, "The Seek done!");
//		g_player.Run (g_player.hPlayer);
	}
	else if (nID == QC_MSG_PLAY_SEEK_FAILED)
	{
		if (!g_bForceClosed)
			SetWindowText(g_hWnd, "The Seek failed!");
	}
	else if (nID == QC_MSG_PLAY_DURATION)
	{
		long long llDur = g_player.GetDur(g_player.hPlayer);
		g_sldPos->SetRange(0, (int)llDur);
	}
	else if (nID == QC_MSG_PLAY_COMPLETE)
	{
		g_player.SetPos (g_player.hPlayer, 0);
		//PostMessage(g_hWnd, WM_MSG_EVENT, nID, 0);

		int nStatus = *(int*)pValue1;
//		g_player.Pause (g_player.hPlayer);
		//OpenTestFile ("D:\\Work\\TestClips\\YouTube_0.flv");
		//g_player.Open(g_player.hPlayer, "http://mus-oss.muscdn.com/reg02/2017/07/06/14/247382630843777024.mp4", 0);
	}
	else if (nID == QC_MSG_HTTP_DISCONNECTED)
	{
		if (!g_bForceClosed)
			SetWindowText(g_hWnd, "The network was disconnected!");
	}
	else if (nID == QC_MSG_HTTP_RECONNECT_FAILED)
	{
		if (!g_bForceClosed)
			SetWindowText(g_hWnd, "The network reconnect failed!");
	}
	else if (nID == QC_MSG_HTTP_RECONNECT_SUCESS)
	{
		if (!g_bForceClosed)
			SetWindowText(g_hWnd, "The network reconnect sucessed!");
	}
	else if (nID == QC_MSG_PLAY_CAPTURE_IMAGE)
	{
		QC_DATA_BUFF * pData = (QC_DATA_BUFF *)pValue1;
		CFileIO filIO (NULL);
		char szFile[256];
		qcGetAppPath(NULL, szFile, sizeof(szFile));
		strcat(szFile, "0001.jpg");

		filIO.Open(szFile, 0, QCIO_FLAG_WRITE);
		filIO.Write(pData->pBuff, pData->uSize);
		filIO.Close();
	}
	else if (nID == QC_MSG_BUFF_SEI_DATA)
	{
		QC_DATA_BUFF * pData = (QC_DATA_BUFF *)pValue1;
		pData = pData;
	}
	else if (nID == QC_MSG_PLAY_CACHE_DONE || nID == QC_MSG_PLAY_CACHE_FAILED)
	{
		char * pSource = (char *)pValue1;
	//	SetWindowText(g_hWnd, pSource);
	}
}

BOOL OpenTestFile (char * pFile)
{
	int					nFlag = 0;
	char				szFile[1024] = {0};
	DWORD				dwID = 0;
	OPENFILENAME		ofn;
	if (pFile != NULL)
	{
		strcpy (szFile, pFile);
	}
	else
	{
		memset (szFile, 0, sizeof (szFile));
		memset( &(ofn), 0, sizeof(ofn));
		ofn.lStructSize	= sizeof(ofn);
		ofn.hwndOwner = g_hWnd;
		ofn.lpstrFilter = TEXT("Media File (*.*)\0*.*\0");	
		if (_tcsstr (szFile, _T(":/")) != NULL)
			_tcscpy (szFile, _T("*.*"));
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;

		ofn.lpstrTitle = TEXT("Open Media File");
		ofn.Flags = OFN_EXPLORER;
				
		if (!GetOpenFileName(&ofn))
			return FALSE;
		//nFlag = QCPLAY_OPEN_SAME_SOURCE;
	}

	if (g_pVDecRnd != NULL)
	{
		g_pVDecRnd->SetView (g_wndVideo->GetWnd ());
		nFlag = QCPLAY_OPEN_VIDDEC_HW;
	}
	g_nDebugTime01 = qcGetSysTime ();

//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_DRM_KeyText, g_szQiniuDrmKey);
	long long llStartPos = 30000;
//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_START_POS, &llStartPos);
	int nProtocol = g_dlgSet->m_bHttpPD ? QC_IOPROTOCOL_HTTPPD : QC_IOPROTOCOL_NONE;
	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_Prefer_Protocol, &nProtocol);
	int nLoop = g_dlgSet->m_bLoop ? 1 : 0;
	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_Playback_Loop, &nLoop);

	//g_player.SetParam(g_player.hPlayer, QCPLAY_PID_FILE_KeyText, "rckv");
	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_COMP_KeyText, "TYKG47AT");

	if (g_dlgSet->m_bSameSource)
		nFlag |= QCPLAY_OPEN_SAME_SOURCE;
	if (g_dlgSet->m_bHWDec)
		nFlag |= QCPLAY_OPEN_VIDDEC_HW;
	g_player.Open (g_player.hPlayer, szFile, nFlag);
	_tcscpy (g_szPlayURL, szFile);

	SetWindowText (g_hWnd, szFile);

	return TRUE;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_hInst = hInstance; // Store instance handle in our global variable
	g_hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	if (!g_hWnd)
	  return FALSE;

	g_dlgSet = new CDlgSetting(g_hInst, g_hWnd);
	g_dlgSet->OpenDlg();
	RECT rcDlg;
	GetClientRect(g_dlgSet->GetDlg(), &rcDlg);

	RECT	rcWnd;
	GetClientRect (g_hWnd, &rcWnd);
	rcWnd.left = rcDlg.right;
	rcWnd.bottom = rcWnd.bottom - 24;
	g_wndVideo = new CWndVideo (g_hInst);
	g_wndVideo->CreateWnd (g_hWnd, rcWnd, RGB (0, 0, 0));

	SetRect (&rcWnd, rcWnd.left, rcWnd.bottom, rcWnd.right, rcWnd.bottom + 24);
	g_sldPos = new CWndSlider (g_hInst);
	g_sldPos->CreateWnd (g_hWnd, rcWnd, RGB (100, 100, 100));

	g_dlgDebug = new CDlgDebug (g_hInst, g_hWnd);
	
	GetClientRect (g_hWnd, &rcWnd);
	int nScreenX = GetSystemMetrics (SM_CXSCREEN);
	int nScreenY = GetSystemMetrics (SM_CYSCREEN);
	SetWindowPos (g_hWnd, HWND_BOTTOM, (nScreenX - rcWnd.right) / 2, 100, 0, 0, SWP_NOSIZE);

	SetWindowText (g_hWnd, "corePlayer test application");
	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);
	_tcscpy (g_szPlayURL, _T(""));
    
	memset (&g_player, 0, sizeof (g_player));
#if 1
	qcCreatePlayer (&g_player, g_hInst);
#else
	m_pRndVideo = new CDDrawRnd ();
	m_pRndAudio = new CWaveOutRnd ();
	g_hDllPlay = qcLibLoad ("qcPlayEng", 0);
	QCCREATEPLAYER * fCreate = (QCCREATEPLAYER *)qcLibGetAddr (g_hDllPlay, "qcCreatePlayer", 0);
	fCreate (&g_player, g_hInst);
#endif 

	CBaseInst * pBaseInst = ((COMBoxMng *)g_player.hPlayer)->GetBaseInst ();
	if (pBaseInst != NULL && pBaseInst->m_pMsgMng != NULL)
		pBaseInst->m_pMsgMng->RegNotify(g_dlgDebug);
	//	g_pVDecRnd = new CVideoDecRnd (g_hInst);
	if (g_pVDecRnd != NULL)
	{
		g_pVDecRnd->SetView (g_wndVideo->GetWnd ());
		g_pVDecRnd->SetPlayer (&g_player);
	}

	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_PD_Save_Path, "c:\\Temp\\qplayer\\pdfile\\");
	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_PD_Save_ExtName, "mp4");

	//int nExtAITracking = 1;
	//g_player.SetParam(g_player.hPlayer, QCPLAY_PID_EXT_AITracking, &nExtAITracking);
	int irtspUDPTCPFlag = 1;
	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_RTSP_UDPTCP_MODE, &irtspUDPTCPFlag);


	char szDNSServer[64];
	strcpy(szDNSServer, "114.114.114.114");
//	strcpy(szDNSServer, "8.8.8.8");
	strcpy(szDNSServer, "127.0.0.1");
	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_DNS_SERVER, szDNSServer);
//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_DNS_DETECT, "www.qiniu.com");

	g_player.SetNotify (g_player.hPlayer, NotifyEvent, g_hWnd);
	g_player.SetView (g_player.hPlayer, g_wndVideo->GetWnd (), NULL);

//	OpenTestFile("http://ojpjb7lbl.bkt.clouddn.com/bipbopall.m3u8");
//	OpenTestFile("http://ogtoywd4d.bkt.clouddn.com/sOwaKtEVhwgXuF29LtnAQx_HXoE=/lhsawSRA9-0L8b0s-cXmojaMhGqn");
//	OpenTestFile ("http://pili-playback.qshare.qiniucdn.com/qshare/20170317-ai-linyining.m3u8?start=1489728111&end=1489736318");
//	OpenTestFile("http://dx-video-test.itangchao.me/ÉäµñÓ¢ÐÛ´«02_m3u8_240P_480P_720P_1080P_0325_2.m3u8");
//	OpenTestFile ("http://video.youxiangtv.com/HD_MOMOšg˜·¹ÈµÚÆß¼¾_º†ów°æ_ÓÐÅÔ°×_EPS01_m3u8_240P_480P_720P_1080P_0325_2.m3u8");
//	OpenTestFile ("http://musically.muscdn.com/reg02/2017/05/31/02/234148590598897664.mp4");
//	OpenTestFile ("http://221.228.219.61/reg02/2017/05/31/02/234148590598897664.mp4?domain=musically.muscdn.com");
//	OpenTestFile("http://ojpjb7lbl.bkt.clouddn.com/bipbopall.m3u8");
//	OpenTestFile("rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov");
//	OpenTestFile("rtmp://183.146.213.65/live/hks?domain=live.hkstv.hk.lxdns.com");
//	OpenTestFile("https://oigovwije.qnssl.com/7bNOdFMmkSAixm2ID2IhIsrF5yM=/Fg35h9Mi85ClJdNLXwE7WD24HJ4h");
//	OpenTestFile("https://ypyvideo.dailyyoga.com.cn/session/package/Shaping%20flat%20lower%20abdomen.mp4");
//	OpenTestFile("http://oh4yf3sig.cvoda.com/cWEtZGlhbmJvdGVzdDpY54m56YGj6ZifLumfqeeJiC5IRGJ1eHVhbnppZG9uZzAwNC5tcDQ=_q00030002.mp4");
//	OpenTestFile("http://hcluploadffiles.oss-cn-hangzhou.aliyuncs.com/%E7%8E%AF%E4%BF%9D%E5%B0%8F%E8%A7%86%E9%A2%91.mp4");
//	OpenTestFile("http://video.qiniu.3tong.com/720_201883248781950976.mp4");
//	qcTestParserOpen ();

//	OpenTestFile("c:/work/media/1111.mp4");
//	OpenTestFile("http://demo-videos.qnsdk.com/movies/apple.mp4");
//	OpenTestFile("c:/temp/15346515019356_origin.mp4");

	char szCacheSource[256];

	strcpy(szCacheSource, "http://demo-videos.qnsdk.com/movies/apple.mp4");
//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_ADD_Cache, szCacheSource);
	
	strcpy(szCacheSource, "http://video.qiniu.3tong.com/720_201883248781950976.mp4");
//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_ADD_Cache, szCacheSource);
	strcpy(szCacheSource, "http://video.qiniu.3tong.com/720_182584969019785216.mp4");
	strcpy(szCacheSource, "http://demo-videos.qnsdk.com/movies/qiniu.mp4");
//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_ADD_Cache, szCacheSource);
	strcpy(szCacheSource, "http://video.qiniu.3tong.com/720_179737636708024320.mp4");
//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_ADD_Cache, szCacheSource);
	strcpy(szCacheSource, "http://video.qiniu.3tong.com/720_188810429944823808.mp4");
//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_ADD_Cache, szCacheSource);
	strcpy(szCacheSource, "http://demo-videos.qnsdk.com/movies/apple.mp4");
//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_ADD_Cache, szCacheSource);


	strcpy(g_szTestFile[0], "http://video.qiniu.3tong.com/720_201883248781950976.mp4");
	strcpy(g_szTestFile[1], "http://video.qiniu.3tong.com/720_182584969019785216.mp4");
	strcpy(g_szTestFile[2], "http://demo-videos.qnsdk.com/movies/qiniu.mp4");
	strcpy(g_szTestFile[3], "http://video.qiniu.3tong.com/720_179737636708024320.mp4");
	strcpy(g_szTestFile[4], "http://video.qiniu.3tong.com/720_188810429944823808.mp4");

	strcpy(g_szTestFile[0], "http://demo-videos.qnsdk.com/movies/snowday.mp4");
	strcpy(g_szTestFile[0], "http ://pili-media.meilihuli.com/recordings/z1.meili.product_239/app_product_239.mp4");
	strcpy(g_szTestFile[0], "http ://down.ttdtweb.com/test/MTV.mp4");

	strcpy(szCacheSource, "http://test.xuhc.me/new_15346515019356_origin.mp4");
	strcpy(szCacheSource, "c:/temp/cam_gen.ts");
	//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_ADD_IOCache, szCacheSource);
//	Sleep(10);


	strcpy(szCacheSource, "Z:\\work\\Media\\Demo\\YouTube.flv");
	//strcpy(szCacheSource, "c:\\temp\\IMG_1660.mov");
	//OpenTestFile(szCacheSource);


	g_nAutoTestIndex = 0;
	SetTimer (g_hWnd, 1001, 50, NULL);
//	SetTimer (g_hWnd, 1002, 5000, NULL);
//	SetTimer(g_hWnd, 1003, 4000, NULL);
//	SetTimer(g_hWnd, 1004, 1500, NULL);

//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_ADD_Cache, g_szTestFile[2]);

//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_START_MUX_FILE, "c:/temp/0000/mux_000.mp4");
//	g_pExtSource = new CExtSource(hInstance);
//	g_pExtSource->SetPlayer(&g_player);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	int nParam = 0;
	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
		{
		//	int nFlag = QCPLAY_OPEN_SAME_SOURCE;
		//	char szFile[2048];
		//	strcpy(szFile, "http://mus-oss.muscdn.com/reg02/2017/07/02/00/245712223036194816.mp4");
		//	g_player.Open(g_player.hPlayer, szFile, nFlag);
		//	g_player.SetParam(g_player.hPlayer, QCPLAY_PID_Flush_Buffer, NULL);

			RECT rcZoom;
			rcZoom.left = 100;
			rcZoom.top = 80;
			rcZoom.right = 200;
			rcZoom.bottom = 160;
			//g_player.SetParam(g_player.hPlayer, QCPLAY_PID_Zoom_Video, &rcZoom);

			//if (g_player.hPlayer != NULL)
			//	qcDestroyPlayer(&g_player);
			//g_player.hPlayer = NULL;

			//g_player.SetVolume(g_player.hPlayer, 300);
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
		}
			break;
		case IDM_EXIT:
			//g_player.SetVolume(g_player.hPlayer, 150);
			DestroyWindow(hWnd);
			break;
		case ID_FILE_OPEN:
			OpenTestFile (NULL);
			break;
		case ID_FILE_OPENURL:
		{
			CDlgOpenURL dlgURL (g_hInst, hWnd);
			if (dlgURL.OpenDlg (g_szPlayURL) == IDOK)
			{
				if (_tcslen (dlgURL.GetURL ()) > 6)
					OpenTestFile (dlgURL.GetURL ());
			}
		}
			break;
		case ID_FILE_DEBUG:
			g_dlgDebug->OpenDlg ();
			break;

		case ID_FILE_STARTMUX:
			g_player.SetParam(g_player.hPlayer, QCPLAY_PID_START_MUX_FILE, "c:/temp/0000/mux_000.mp4");
			break;

		case ID_FILE_STOPMUX:
			nParam = 0;
			g_player.SetParam(g_player.hPlayer, QCPLAY_PID_STOP_MUX_FILE, &nParam);
			break;
		case ID_FILE_PAUSEMUX:
			nParam = 1;
			g_player.SetParam(g_player.hPlayer, QCPLAY_PID_STOP_MUX_FILE, &nParam);
			break;
		case ID_FILE_RESTARTMUX:
			nParam = 2;
			g_player.SetParam(g_player.hPlayer, QCPLAY_PID_STOP_MUX_FILE, &nParam);
			break;

		case ID_PLAY_RUN:
			if (g_player.hPlayer != NULL)
				g_player.Run (g_player.hPlayer);
			break;
		case ID_PLAY_PAUSE:
			if (g_player.hPlayer != NULL)
				g_player.Pause (g_player.hPlayer);
			break;
		case ID_PLAY_STOP:
			if (g_player.hPlayer != NULL)
				g_player.Stop (g_player.hPlayer);
			break;
		case ID_PLAY_CAPTURE:
			if (g_player.hPlayer != NULL)
			{
				long long llTime = 0;
				g_player.SetParam(g_player.hPlayer, QCPLAY_PID_Capture_Image, &llTime);
			}
			break;


		case ID_STREAM_STREAM1:
		case ID_STREAM_STREAM1+1:
		case ID_STREAM_STREAM1+2:
		case ID_STREAM_STREAM1+3:
		case ID_STREAM_STREAM1+4:
		case ID_STREAM_STREAM1+5:
		case ID_STREAM_STREAM1+6:
		case ID_STREAM_STREAM1+7:
		{
			int nStream = wmId -ID_STREAM_STREAM1 - 1;
			if (g_player.hPlayer != NULL)
				g_player.SetParam (g_player.hPlayer, QCPLAY_PID_StreamPlay, &nStream);
			HMENU mMain = GetSubMenu (GetMenu (g_hWnd), 2);
			HMENU hStream = GetSubMenu (mMain, 0);
			for (int i = 0; i <= m_nStreamCount; i++)
				CheckMenuItem (hStream, ID_STREAM_STREAM1 + i, MF_UNCHECKED);
			CheckMenuItem (hStream, wmId, MF_CHECKED);
		}
			break;

		case ID_VIDEO_VIDEO1:
			break;

		case ID_AUDIO_AUDIO1:
		case ID_AUDIO_AUDIO1+1:
		case ID_AUDIO_AUDIO1+2:
		case ID_AUDIO_AUDIO1+3:
		{
			int nTrack = wmId -ID_AUDIO_AUDIO1;
			if (g_player.hPlayer != NULL)
				g_player.SetParam (g_player.hPlayer, QCPLAY_PID_AudioTrackPlay, &nTrack);
		}
			break;

		case ID_SUBTITLE_SUBTT1:
			break;

		case ID_SPEED_0:
		case ID_SPEED_1:
		case ID_SPEED_2:
		case ID_SPEED_3:
		case ID_SPEED_4:
		case ID_SPEED_5:
		case ID_SPEED_6:
		{
			double fSpeed = g_aSpeed[wmId -ID_SPEED_0];
			if (g_player.hPlayer != NULL)
				g_player.SetParam (g_player.hPlayer, QCPLAY_PID_Speed, &fSpeed);
		}
			break;

		case ID_PLAY_PASUEDOWNLOAD:
		{
			int nPause = 0;
			if (g_player.hPlayer != NULL)
				nPause = g_player.GetParam(g_player.hPlayer, QCPLAY_PID_Download_Pause, NULL);
			if (nPause == 0)
				nPause = 1;
			else
				nPause = 0;
			if (g_player.hPlayer != NULL)
				g_player.SetParam(g_player.hPlayer, QCPLAY_PID_Download_Pause, &nPause);
			HMENU mMain = GetSubMenu(GetMenu(g_hWnd), 1);
			if (nPause == 1)
				CheckMenuItem(mMain, wmId, MF_CHECKED);
			else
				CheckMenuItem(mMain, wmId, MF_UNCHECKED);
		}
			break;


		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_YYSLD_NEWPOS:
	{
		long long llPos = g_sldPos->GetPos ();
		if (g_player.hPlayer != NULL)
			g_player.SetPos (g_player.hPlayer, llPos);
		//QCLOGT("TestPos", "SetPos --------  % 8lld", llPos);
	}
		break;

	case WM_MSG_EVENT:
	{
		if (wParam == QC_MSG_PLAY_COMPLETE)
			g_player.SetPos(g_player.hPlayer, 0);
			//OpenTestFile(g_szPlayURL);
	}
		break;

	case WM_TIMER:
	{
		if (wParam == 1001)
		{
			long long llPos = 0;
			if (g_player.hPlayer != NULL)
				llPos = g_player.GetPos(g_player.hPlayer);
			g_sldPos->SetPos((int)llPos);
			//QCLOGT("TestPos", "GetPos ********  % 8lld", llPos);
		}
		else if (wParam == 1002)
		{
			//nRC = g_player.Close(g_player.hPlayer);
			//OpenTestFile ("C:\\work\\Media\\179b5bd6ce990b8a8ebd141b091419ac.mp4");
			/*
			if (g_player.hPlayer != NULL)
				qcDestroyPlayer(&g_player);
			g_player.hPlayer = NULL;

			qcCreatePlayer(&g_player, g_hInst);
			g_player.SetNotify(g_player.hPlayer, NotifyEvent, g_hWnd);
			g_player.SetView(g_player.hPlayer, g_wndVideo->GetWnd(), NULL);
			OpenTestFile ("http://ojpjb7lbl.bkt.clouddn.com/bipbopall.m3u8");
			*/
			//OpenTestFile ("C:\\work\\Media\\111.mp4");
			/*
			g_nAutoTestIndex++;
			if (g_nAutoTestIndex % 2 == 1)
				g_player.Open(g_player.hPlayer, "https://oigovwije.qnssl.com/shfpahbclahjbdoa.mp4", QCPLAY_OPEN_SAME_SOURCE);
			else
				//g_player.Open(g_player.hPlayer, "http://gslb.miaopai.com/stream/E26J9j~FuMDu0lX--GALbHiXg~LEH0wrGDyv4w__.mp4", QCPLAY_OPEN_SAME_SOURCE);
				g_player.Open(g_player.hPlayer, "http://musically.muscdn.com/reg02/2017/05/31/02/234148590598897664.mp4", QCPLAY_OPEN_SAME_SOURCE);

			KillTimer(hWnd, 1002);
			int nTimer = 5000;// (rand() + 10) % 500;
			SetTimer(hWnd, 1002, nTimer, NULL);
			*/

//			COMBoxMng * pMng = (COMBoxMng *)g_player.hPlayer;
//			pMng->GetBaseInst()->NotifyNetChanged();
		}
		else if (wParam == 1003)
		{
			int nIndex = rand() % 3;
			g_player.SetParam(g_player.hPlayer, QCPLAY_PID_ADD_Cache, g_szTestFile[nIndex]);
			OpenTestFile(g_szTestFile[nIndex]);
			nIndex = (rand() + 1) % 5;
			//g_player.SetParam(g_player.hPlayer, QCPLAY_PID_DEL_Cache, g_szTestFile[nIndex]);
		}
		else if (wParam == 1004)
		{
			int nIndex = rand() % 5;
			g_player.SetParam(g_player.hPlayer, QCPLAY_PID_ADD_Cache, g_szTestFile[nIndex]);
		}
	}
		break;

	case WM_VIEW_FullScreen:
		if (g_player.hPlayer != NULL)
			g_player.SetView (g_player.hPlayer, g_wndVideo->GetWnd (), NULL);
		if (g_pVDecRnd != NULL)
			g_pVDecRnd->SetView (g_wndVideo->GetWnd ());
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		if (g_pExtSource != NULL)
			g_pExtSource->Close();

		g_bForceClosed = true;
		if (g_player.hPlayer != NULL)
		{
			if (g_dlgDebug != NULL)
			{
				CBaseInst * pBaseInst = ((COMBoxMng *)g_player.hPlayer)->GetBaseInst();
				if (pBaseInst != NULL && pBaseInst->m_pMsgMng != NULL)
					pBaseInst->m_pMsgMng->RemNotify(g_dlgDebug);
			}
			g_player.Close (g_player.hPlayer);

			if (g_pExtSource != NULL)
				delete g_pExtSource;

			if (g_hDllPlay != NULL)
			{
				QCDESTROYPLAYER * fDestroy = (QCDESTROYPLAYER *)qcLibGetAddr (g_hDllPlay, "qcDestroyPlayer", 0);
				fDestroy (&g_player);
			}
			else
			{
				qcDestroyPlayer (&g_player);
			}

			if (g_pVideoRndBuff != NULL)
			{
				QC_DEL_A(g_pVideoRndBuff->pBuff[0]);
				QC_DEL_A(g_pVideoRndBuff->pBuff[1]);
				QC_DEL_A(g_pVideoRndBuff->pBuff[2]);
				delete g_pVideoRndBuff;
			}
		}
		if (m_pRndVideo != NULL)
			delete m_pRndVideo;
		if (m_pRndAudio != NULL)
			delete m_pRndAudio;
		if (g_pVDecRnd != NULL)
			delete g_pVDecRnd;

		SendMessage (g_wndVideo->GetWnd (), WM_CLOSE, 0, 0);
		SendMessage (g_sldPos->GetWnd (), WM_CLOSE, 0, 0);

		if (g_dlgSet != NULL)
			delete g_dlgSet;
		if (g_dlgDebug != NULL)
			delete g_dlgDebug;

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

BOOL UpdateStreamMenu (void)
{
	HMENU mMain = GetSubMenu (GetMenu (g_hWnd), 2);
	HMENU hStream = GetSubMenu (mMain, 0);
	HMENU hVideo = GetSubMenu (mMain, 1);
	HMENU hAudio = GetSubMenu (mMain, 2);
	HMENU hSubtt = GetSubMenu (mMain, 3);

	int nCount = GetMenuItemCount (hStream);
	while (nCount > 1){
		DeleteMenu (hStream, nCount-1, MF_BYPOSITION);
		nCount = GetMenuItemCount (hStream);
	}
	nCount = GetMenuItemCount (hVideo);
	while (nCount > 1){
		DeleteMenu (hVideo, nCount-1, MF_BYPOSITION);
		nCount = GetMenuItemCount (hVideo);
	}
	nCount = GetMenuItemCount (hAudio);
	while (nCount > 1){
		DeleteMenu (hAudio, nCount-1, MF_BYPOSITION);
		nCount = GetMenuItemCount (hAudio);
	}
	nCount = GetMenuItemCount (hSubtt);
	while (nCount > 1){
		DeleteMenu (hSubtt, nCount-1, MF_BYPOSITION);
		nCount = GetMenuItemCount (hSubtt);
	}

	int		i;
	char	szItem[32];
	int nRC = g_player.GetParam (g_player.hPlayer, QCPLAY_PID_StreamNum, &nCount);
	if (nCount > 1)
	{
		QC_STREAM_FORMAT stmInfo;
		memset (&stmInfo, 0, sizeof (stmInfo));
		for (i = 1; i <= nCount; i++)
		{
			stmInfo.nID = i - 1;
			g_player.GetParam (g_player.hPlayer, QCPLAY_PID_StreamInfo, &stmInfo);
			sprintf (szItem, "Stream %d - %d", i, stmInfo.nBitrate);
			AppendMenu (hStream, MF_STRING | MF_BYCOMMAND, WMD_STREAM_BASE + i, szItem);
		}
	}
	CheckMenuItem (hStream, ID_STREAM_STREAM1, MF_CHECKED);
	m_nStreamCount = nCount;
	nRC = g_player.GetParam (g_player.hPlayer, QCPLAY_PID_AudioTrackNum, &nCount);
	if (nCount > 1)
	{
		for (i = 1; i < nCount; i++)
		{
			sprintf (szItem, "Audio%d", i+1);
			AppendMenu(hAudio, MF_STRING | MF_BYCOMMAND, WMD_AUDIO_BASE + i, szItem);
		}
	}
	nRC = g_player.GetParam (g_player.hPlayer, QCPLAY_PID_VideoTrackNum, &nCount);
	if (nCount > 1)
	{
		for (i = 1; i < nCount; i++)
		{
			sprintf (szItem, "Video%d", i+1);
			AppendMenu(hVideo, MF_STRING | MF_BYCOMMAND, WMD_VIDEO_BASE + i, szItem);
		}
	}
	nRC = g_player.GetParam (g_player.hPlayer, QCPLAY_PID_SubttTrackNum, &nCount);
	if (nCount > 1)
	{
		for (i = 1; i < nCount; i++)
		{
			sprintf (szItem, "Subtt%d", i+1);
			AppendMenu (hSubtt, MF_STRING | MF_BYCOMMAND, WMD_SUBTT_BASE + i, szItem);
		}
	}


//	ModifyMenu (hMenuS1, 0, MF_BYPOSITION, 0, "Stream 00000");
	return TRUE;
}
