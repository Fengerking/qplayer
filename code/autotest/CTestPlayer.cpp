/*******************************************************************************
	File:		CTestPlayer.cpp

	Contains:	the buffer trace r class implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CTestPlayer.h"

#include "COMBoxMng.h"

#include "USystemFunc.h"

#define QCTEST_CHECK_FUNC(n, f, i, v)	\
			RecordFunc(n, f, i, v);		\
			return n;					\

CTestPlayer::CTestPlayer(CTestInst * pInst)
	: CTestBase(pInst)
	, m_pCurTask(NULL)
	, m_fCreate(NULL)
	, m_fDestroy(NULL)
	, m_pBaseInst (NULL)
{
#ifdef __QC_OS_WIN32__
	m_hDll = LoadLibrary("QPlayEng.Dll");
	m_fCreate = (QCCREATEPLAYER *)GetProcAddress(m_hDll, "qcCreatePlayer");
	m_fDestroy = (QCDESTROYPLAYER *)GetProcAddress(m_hDll, "qcDestroyPlayer");
#else
	m_fCreate = qcCreatePlayer;
	m_fDestroy = qcDestroyPlayer;
#endif // __QC_OS_WIN32__
	memset(&m_player, 0, sizeof(m_player));
}

CTestPlayer::~CTestPlayer(void)
{
	m_pInst->m_bExitTest = true;
	if (m_player.hPlayer != NULL)
		m_fDestroy(&m_player);
#ifdef __QC_OS_WIN32__	
	if (m_hDll != NULL)
		FreeLibrary(m_hDll);
#endif //__QC_OS_WIN32__
}

int	CTestPlayer::Create(void)
{
	m_pInst->m_bExitTest = false;
	if (m_player.hPlayer == NULL)
		m_fCreate(&m_player, NULL);
	m_pBaseInst = ((COMBoxMng *)m_player.hPlayer)->GetBaseInst();
	return QC_ERR_NONE;
}

int	CTestPlayer::SetNotify(QCPlayerNotifyEvent pFunc, void * pUserData)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	int nRC = m_player.SetNotify (m_player.hPlayer, pFunc, pUserData);
	QCTEST_CHECK_FUNC(nRC, __FUNCTION__, 0, NULL);
}

int	CTestPlayer::SetView(void * hView, RECT * pRect)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	int nRC = m_player.SetView(m_player.hPlayer, hView, pRect);
	QCTEST_CHECK_FUNC(nRC, __FUNCTION__, 0, NULL);
}

int	CTestPlayer::Open(const char * pURL, int nFlag)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
#ifdef __QC_OS_NDK__	
	m_pInst->m_pRndAudio->SetBaseInst (m_pBaseInst);	
	m_player.SetParam (m_player.hPlayer, QCPLAY_PID_EXT_AudioRnd, m_pInst->m_pRndAudio);
	if (m_pInst->m_pRndVideo != NULL)
		m_pInst->m_pRndVideo->SetBaseInst (m_pBaseInst);	
	if (m_pInst->m_pRndVidDec != NULL)
		m_pInst->m_pRndVidDec->SetBaseInst (m_pBaseInst);	
	if ((nFlag & QCPLAY_OPEN_VIDDEC_HW) == QCPLAY_OPEN_VIDDEC_HW)
		m_player.SetParam (m_player.hPlayer, QCPLAY_PID_EXT_VideoRnd, m_pInst->m_pRndVidDec);	
	else
		m_player.SetParam (m_player.hPlayer, QCPLAY_PID_EXT_VideoRnd, m_pInst->m_pRndVideo);
#elif defined __QC_OS_IOS__
    m_player.SetView(m_player.hPlayer, m_pInst->m_hWndVideo, NULL);
#endif // __QC_OS_NDK__
	m_pBaseInst->m_bForceClose = false;		
	int nRC = m_player.Open(m_player.hPlayer, pURL, nFlag);
	QCTEST_CHECK_FUNC(nRC, __FUNCTION__, 0, (void *)pURL);
}

int CTestPlayer::Close(void)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	int nRC = m_player.Close(m_player.hPlayer);
	QCTEST_CHECK_FUNC(nRC, __FUNCTION__, 0, NULL);
}

int CTestPlayer::Run(void)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	int nRC = m_player.Run(m_player.hPlayer);
	QCTEST_CHECK_FUNC(nRC, __FUNCTION__, 0, NULL);
}

int CTestPlayer::Pause(void)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	int nRC = m_player.Pause(m_player.hPlayer);
	QCTEST_CHECK_FUNC(nRC, __FUNCTION__, 0, NULL);
}

int CTestPlayer::Stop(void)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	int nRC = m_player.Stop(m_player.hPlayer);
	QCTEST_CHECK_FUNC(nRC, __FUNCTION__, 0, NULL);
}

QCPLAY_STATUS CTestPlayer::GetStatus(void)
{
	if (m_player.hPlayer == NULL)
		return QC_PLAY_MAX;
	return m_player.GetStatus(m_player.hPlayer);
}

long long CTestPlayer::GetDur(void)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	return m_player.GetDur(m_player.hPlayer);
}

long long CTestPlayer::GetPos(void)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	return m_player.GetPos(m_player.hPlayer);
}

long long CTestPlayer::SetPos(long long llPos)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	int nRC = (int)m_player.SetPos(m_player.hPlayer, llPos);
	QCTEST_CHECK_FUNC(nRC, __FUNCTION__, 0, &llPos);
}

int	CTestPlayer::SetVolume(int nVolume)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	int nRC = m_player.SetVolume(m_player.hPlayer, nVolume);
	QCTEST_CHECK_FUNC(nRC, __FUNCTION__, 0, NULL);
}

int	CTestPlayer::GetVolume(void)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	int nRC = m_player.GetVolume(m_player.hPlayer);
	QCTEST_CHECK_FUNC(nRC, __FUNCTION__, 0, NULL);
}

int CTestPlayer::GetParam(int nID, void * pParam)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	int nRC = m_player.GetParam(m_player.hPlayer, nID, pParam);
	QCTEST_CHECK_FUNC(nRC, __FUNCTION__, nID, pParam);
}

int CTestPlayer::SetParam(int nID, void * pParam)
{
	if (m_player.hPlayer == NULL)
		return QC_ERR_STATUS;
	int nRC = m_player.SetParam(m_player.hPlayer, nID, pParam);
	QCTEST_CHECK_FUNC(nRC, __FUNCTION__, nID, pParam);
}

int	CTestPlayer::RecordFunc(int nRC, const char * pFuncName, int nID, void * pParam)
{
	char * pResult = new char[4096];
	sprintf(pResult, "RC: 0X%08X  %s   ", nRC, pFuncName);
	if (strstr(pFuncName, "Open") != NULL)
	{
		strcat(pResult, (char *)pParam);
	}
	else if (strstr(pFuncName, "SetPos") != NULL)
	{
		sprintf(pResult + strlen (pResult) - 1, "  %lld", *(long long *)pParam);
	}
	else if (strstr(pFuncName, "SetParam") != NULL)
	{
//		char * pParam = pResult + strlen(pResult) - 1;
//		sprintf(pParam, "ID: 0X%08X", nID);
		char szParamInfo[256];
		szParamInfo[0] = 0;
		switch (nID)
		{
		case QCPLAY_PID_AspectRatio:
		{
			QCPLAY_ARInfo * pRatio = (QCPLAY_ARInfo *)pParam;
			sprintf(szParamInfo, "AspectRatio:  %d:%d", pRatio->nWidth, pRatio->nHeight);
			break;
		}

		case QCPLAY_PID_Speed:
			sprintf(szParamInfo, "Speed:  %f", *(double*)pParam);
			break;

		case QCPLAY_PID_Disable_Video:
			sprintf(szParamInfo, "DisableVideo:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_SetWorkPath:
			sprintf(szParamInfo, "SetWorkPath:  %s", (char*)pParam);
			break;

		case QCPLAY_PID_StreamPlay:
			sprintf(szParamInfo, "StreamPlay:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_Zoom_Video:
		{
			RECT * pZoom = (RECT *)pParam;
			sprintf(szParamInfo, "ZoomVideo:  %d : %d : %d : %d", pZoom->left, pZoom->top, pZoom->right, pZoom->bottom);
			break;
		}

		case QCPLAY_PID_Clock_OffTime:
			sprintf(szParamInfo, "OffsetTime:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_Seek_Mode:
			sprintf(szParamInfo, "SeekMode:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_Flush_Buffer:
			sprintf(szParamInfo, "FlushBuffer");
			break;

		case QCPLAY_PID_Download_Pause:
			sprintf(szParamInfo, "DownPause:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_Prefer_Protocol:
			sprintf(szParamInfo, "PreferProtocol:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_Prefer_Format:
			sprintf(szParamInfo, "PreferFormat:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_PD_Save_Path:
			sprintf(szParamInfo, "SavePath:  %s", (char*)pParam);
			break;

		case QCPLAY_PID_PD_Save_ExtName:
			sprintf(szParamInfo, "ExtName:  %s", (char*)pParam);
			break;

		case QCPLAY_PID_RTSP_UDPTCP_MODE:
			sprintf(szParamInfo, "UDPTCP:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_Socket_ConnectTimeout:
			sprintf(szParamInfo, "ConnectTimeOut:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_Socket_ReadTimeout:
			sprintf(szParamInfo, "ReadTimeOut:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_HTTP_HeadReferer:
			sprintf(szParamInfo, "HeadReferer:  %s", (char*)pParam);
			break;

		case QCPLAY_PID_DNS_SERVER:
			sprintf(szParamInfo, "DNSServer:  %s", (char*)pParam);
			break;

		case QCPLAY_PID_DNS_DETECT:
			sprintf(szParamInfo, "DNSDetect:  %s", (char*)pParam);
			break;

		case QCPLAY_PID_PlayBuff_MaxTime:
			sprintf(szParamInfo, "BuffMaxTime:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_PlayBuff_MinTime:
			sprintf(szParamInfo, "BuffMinTime:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_DRM_KeyText:
			sprintf(szParamInfo, "DRMKeyText:  %s", (char*)pParam);
			break;

		case QCPLAY_PID_Capture_Image:
			sprintf(szParamInfo, "CaptureImage:  %lld", *(long long*)pParam);
			break;

		case QCPLAY_PID_Log_Level:
			sprintf(szParamInfo, "LogLevel:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_Playback_Loop:
			sprintf(szParamInfo, "PlayLoop:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_MP4_PRELOAD:
			sprintf(szParamInfo, "PreLoad:  %d", *(int*)pParam);
			break;

		case QCPLAY_PID_SendOut_VideoBuff:
			sprintf(szParamInfo, "SendOutVideo:  %X", *(int*)pParam);
			break;

		case QCPLAY_PID_SendOut_AudioBuff:
			sprintf(szParamInfo, "SendOutAudio:  %X", *(int*)pParam);
			break;

		default:
			break;
		}
		strcat(pResult, szParamInfo);
	}
	m_pInst->AddInfoItem(m_pCurTask, QCTEST_INFO_Func, pResult);
	delete[]pResult;
	return nRC;
}
