/*******************************************************************************
	File:		qcPlayerEng.cpp

	Contains:	qc media engine  implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-18		Bangfei			Create file

*******************************************************************************/
#ifdef __QC_OS_WIN32__
#include <windows.h>
#endif // __QC_OS_WIN32__

#include "qcPlayer.h"
#include "COMBoxMng.h"

#ifdef __QC_OS_WIN32__
HINSTANCE	qc_hInst = NULL;
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	qc_hInst = (HINSTANCE) hModule;
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
	}
    return TRUE;
}
#else
void *	g_hInst = NULL;
#endif // __QC_OS_WIN32__

int	qcPlayer_SetNotify (void * hPlayer, QCPlayerNotifyEvent pFunc, void * pUserData)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	((COMBoxMng *)hPlayer)->SetNotifyFunc (pFunc, pUserData);
	return QC_ERR_NONE;
}

int	qcPlayer_SetView (void * hPlayer, void * hView, RECT * pRect)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	((COMBoxMng *)hPlayer)->SetView (hView, pRect);
	return QC_ERR_NONE;
}

int	qcPlayer_Open (void * hPlayer, const char * pURL, int nFlag)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	return ((COMBoxMng *)hPlayer)->Open (pURL, nFlag);
}

int qcPlayer_Close (void * hPlayer)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	return ((COMBoxMng *)hPlayer)->Close ();
}

int qcPlayer_Run (void * hPlayer)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	return ((COMBoxMng *)hPlayer)->Start ();
}

int qcPlayer_Pause (void * hPlayer)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	return ((COMBoxMng *)hPlayer)->Pause ();
}

int qcPlayer_Stop (void * hPlayer)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	return ((COMBoxMng *)hPlayer)->Stop ();
}

QCPLAY_STATUS qcPlayer_GetStatus (void * hPlayer)
{
	if (hPlayer == NULL)
		return QC_PLAY_Init;
	return ((COMBoxMng *)hPlayer)->GetStatus ();
}

long long qcPlayer_GetDur	(void * hPlayer)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	return ((COMBoxMng *)hPlayer)->GetDuration ();
}

long long qcPlayer_GetPos	(void * hPlayer)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	return ((COMBoxMng *)hPlayer)->GetPos ();
}

long long qcPlayer_SetPos	(void * hPlayer, long long llPos)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	return ((COMBoxMng *)hPlayer)->SetPos (llPos);
}

int	qcPlayer_SetVolume (void * hPlayer, int nVolume)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	return ((COMBoxMng *)hPlayer)->SetVolume (nVolume);
}

int	qcPlayer_GetVolume (void * hPlayer)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	return ((COMBoxMng *)hPlayer)->GetVolume ();
}

int qcPlayer_GetParam (void * hPlayer, int nID, void * pParam)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	return ((COMBoxMng *)hPlayer)->GetParam (nID, pParam);
}

int qcPlayer_SetParam (void * hPlayer, int nID, void * pParam)
{
	if (hPlayer == NULL)
		return QC_ERR_ARG;
	return ((COMBoxMng *)hPlayer)->SetParam (nID, pParam);
}

int qcCreatePlayer (QCM_Player * fPlayer, void * hInst)
{
	if (fPlayer == NULL)
		return QC_ERR_ARG;
	fPlayer->SetNotify = qcPlayer_SetNotify;	
	fPlayer->SetView = qcPlayer_SetView;		
	fPlayer->Open = qcPlayer_Open;			
	fPlayer->Close = qcPlayer_Close;			
	fPlayer->Run = qcPlayer_Run;		
	fPlayer->Pause = qcPlayer_Pause;		
	fPlayer->Stop = qcPlayer_Stop;	
	fPlayer->GetStatus = qcPlayer_GetStatus;		
	fPlayer->GetDur = qcPlayer_GetDur;		
	fPlayer->GetPos = qcPlayer_GetPos;		
	fPlayer->SetPos = qcPlayer_SetPos;		
	fPlayer->SetVolume = qcPlayer_SetVolume;	
	fPlayer->GetVolume = qcPlayer_GetVolume;	
	fPlayer->GetParam = qcPlayer_GetParam;	
	fPlayer->SetParam = qcPlayer_SetParam;
    
    //qcCreateSnd();
    
#ifdef __QC_OS_WIN32__
	if (qc_hInst != NULL)
		fPlayer->hPlayer = new COMBoxMng (qc_hInst);
	else
		fPlayer->hPlayer = new COMBoxMng (hInst);
#else
	fPlayer->hPlayer = new COMBoxMng (hInst);
#endif // __QC_OS_WIN32__
    fPlayer->nVersion = ((COMBoxMng*)fPlayer->hPlayer)->GetSDKVersion();
	return QC_ERR_NONE;
}

int qcDestroyPlayer (QCM_Player * fPlayer)
{
	if (fPlayer == NULL || fPlayer->hPlayer == NULL)
		return QC_ERR_ARG;

	COMBoxMng * pMng = (COMBoxMng *)fPlayer->hPlayer;
	delete pMng;
	fPlayer->hPlayer = NULL;

	return QC_ERR_NONE;
}

