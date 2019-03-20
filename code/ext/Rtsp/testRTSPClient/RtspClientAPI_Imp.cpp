#include "RtspClientAPI.h"
#include "CRtspSession.h"

/*******************************************************************************
File:		RtspClientAPI_Imp.cpp

Contains:	
Written by:	Shenqichao

Change History (most recent first):
2018-06-11		Qichao			Create file

*******************************************************************************/
#ifdef __QC_OS_WIN32__
#include <windows.h>
#endif // __QC_OS_WIN32__

#include "RtspClient_Common.h"

#ifdef __QC_OS_WIN32__
HINSTANCE	qc_hInst = NULL;
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	qc_hInst = (HINSTANCE)hModule;
	return TRUE;
}
#else
void *	g_hInst = NULL;
#endif // __QC_OS_WIN32__

int	RTSP_GetErrCode(CM_RTSP_Handle pHandle)
{
	if (pHandle == NULL)
		return RTSP_BadArgument;
	return 0;
}

int	RTSP_SetCallback(CM_RTSP_Handle hHandle, RTSPSourceCallBack _callback)
{
	int iRet = 0;
	CRtspSession*   pSession = (CRtspSession*)(hHandle);
	if (pSession != NULL)
	{
		pSession->SetCallback(_callback);
	}
	else
	{
		iRet = RTSP_BadArgument;
	}

	return iRet;
}

int RTSP_OpenStream(CM_RTSP_Handle hHandle, int _channelid, char *_url, int _connType, unsigned int _mediaType, void *userPtr, int _verbosity)
{
	int iRet = 0;
	CRtspSession*   pSession = (CRtspSession*)(hHandle);
	if (pSession != NULL)
	{
		pSession->SetUserForCallback(userPtr);
		iRet = pSession->StartRtspClient("RTSP Client", _url, _verbosity);
	}
	else
	{
		iRet = RTSP_BadArgument;
	}

	return iRet;
}

int  RTSP_CloseStream(CM_RTSP_Handle hHandle)
{
	int iRet = 0;
	CRtspSession*   pSession = (CRtspSession*)(hHandle);
	if (pSession != NULL)
	{
		iRet = pSession->StopRTSPClient();
	}
	else
	{
		iRet = RTSP_BadArgument;
	}

	return iRet;
}

int qcCreateRtspIns(CM_Rtsp_Ins * pHandle, void * hInst)
{
	int iRet = 0;
	CRtspSession*   pSession = NULL;

	if (pHandle == NULL)
	{
		return RTSP_BadArgument;
	}

	pHandle->RTSP_GetErrCode = RTSP_GetErrCode;
	pHandle->RTSP_OpenStream = RTSP_OpenStream;
	pHandle->RTSP_SetCallback = RTSP_SetCallback;
	pHandle->RTSP_CloseStream = RTSP_CloseStream;

	pSession = new CRtspSession;
	if (pSession != NULL)
	{
		pHandle->hRtspIns = (void*)pSession;
	}
	else
	{
		printf("create Ins Error!\n");
		iRet = RTSP_MallocError;
	}

	return iRet;
}


int qcDestroyRtspIns(CM_Rtsp_Ins * pHandle)
{
	int  iRet = 0;
	CRtspSession*   pSession = NULL;

	if (pHandle == NULL || pHandle->hRtspIns == NULL)
	{
		return RTSP_BadArgument;
	}

	pSession = (CRtspSession *)pHandle->hRtspIns;
	delete pSession;
	pHandle->hRtspIns = NULL;
	return iRet;
}
