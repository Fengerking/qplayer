#include "RTSPClientAPI.h"
#include "RtspClientWrapper.h"

TaskScheduler* gpScheduler = NULL;
UsageEnvironment* gpEnv = NULL;

static unsigned g_rtspClientCount = 0;
static char g_StopFlag = 1;


#ifdef _WIN32
HANDLE   	gThreadHandle = NULL;
unsigned long             gThreadID = 0;

#endif

int ThreadFunc(void*  pArg)
{
	UsageEnvironment* pEnv = (UsageEnvironment*)(pArg);
	pEnv->taskScheduler().doEventLoop(&g_StopFlag);
	return 0;
}

int StartThread()
{
#ifdef _WIN32

	gThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadFunc, (void*)(gpEnv), 0, &gThreadID);
	return 0;
#endif
}

typedef struct  
{
	RTSPClient*   pRtspClient;
	RTSPSourceCallBack    fRtspCallback;
	void*                 pUser;
	int                   iState;
} S_RTSP_Ins;

static S_RTSP_Ins g_rtspIns[4096] = { 0 };

RTSP_API int RTSP_APICALL RTSP_GetErrCode(RTSP_Handle handle)
{
	return 0;
}

RTSP_API int RTSP_APICALL RTSP_Init(RTSP_Handle *handle)
{
	if (gpScheduler == NULL || gpEnv == NULL)
	{
		gpScheduler = BasicTaskScheduler::createNew();
		gpEnv = BasicUsageEnvironment::createNew(*gpScheduler);
	}

	*handle = (void*)&(g_rtspIns[g_rtspClientCount]);
	return 0;
}

RTSP_API int RTSP_APICALL RTSP_Deinit(RTSP_Handle *handle)
{
	S_RTSP_Ins*   pRtspIns = (S_RTSP_Ins*)(*handle);
	if (pRtspIns != NULL)
	{
		pRtspIns->fRtspCallback = NULL;
	}

	return 0;
}

RTSP_API int RTSP_APICALL RTSP_SetCallback(RTSP_Handle handle, RTSPSourceCallBack _callback)
{
	S_RTSP_Ins*   pRtspIns = (S_RTSP_Ins*)handle;
	if (pRtspIns != NULL)
	{
		pRtspIns->fRtspCallback = _callback;
		SetCallback(_callback);
	}
	return 0;
}

RTSP_API int RTSP_APICALL RTSP_OpenStream(RTSP_Handle handle, int _channelid, char *_url, RTSP_RTP_CONNECT_TYPE _connType, unsigned int _mediaType, char *_username, char *_password,
	void *userPtr, int _reconn, int outRtpPacke, int heartbeatType, int _verbosity)
{
	S_RTSP_Ins*   pRtspIns = (S_RTSP_Ins*)handle;
	if (pRtspIns != NULL)
	{
		pRtspIns->pUser = userPtr;
		pRtspIns->pRtspClient = openURL(*gpEnv, "RTSP Client", _url);
	}

	if (g_StopFlag != 0)
	{
		g_StopFlag = 0;
		StartThread();
	}

	
	return 0;
}

RTSP_API int RTSP_APICALL RTSP_CloseStream(RTSP_Handle handle)
{
	S_RTSP_Ins*   pRtspIns = (S_RTSP_Ins*)handle;
	shutdownStream(pRtspIns->pRtspClient);
	g_rtspClientCount--;
	return 0;
}
