/*******************************************************************************
	File:		CTestTask.cpp

	Contains:	the buffer trace r class implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#include <stdlib.h>
#include "qcErr.h"
#include "CTestTask.h"

#include "USystemFunc.h"
#include "UMsgMng.h"

CTestTask::CTestTask(CTestInst * pInst)
	: CTestBase(pInst)
	, m_pPlayer(NULL)
	, m_pName(NULL)
	, m_pURL(NULL)
	, m_pExtSrc(NULL)
{
	m_nStartTime = 0;
	m_llStartPos = 0;
	m_nPlayComplete = 0;
	m_nExitClose = 1;

	m_nOpenFlag = 0;
	m_sRatio.nWidth = 1;
	m_sRatio.nHeight = 1;
	m_dSpeed = 1.0;
	m_nOffsetTime = 0;
	m_nSeekMode = 0;
	m_nPreProtocol = 0;
	m_nPreferFormat = 0;
	m_pSavePath = NULL;
	m_pExtName = NULL;
	m_nRTSPMode = 0;
	m_nConnectTimeOut = 0;
	m_nReadTimeOut = 0;
	m_pHeadText = NULL;
	m_pDNSServer = NULL;
	m_pDNSDetect = NULL;
	m_nMaxBuffTime = 0;
	m_nMinBuffTime = 0;
	m_pDrmKeyText = NULL;
	m_nLogLevel = -1;
	m_nPlayLoop = 0;
	m_nPreloadTime = 0;
    m_nExtSrcType = -1;

	m_pTxtMsg = new char[4096];
	m_pTxtTmp = new char[2048];
	m_pTxtErr = new char[2048];

	m_stsPlay = QC_PLAY_MAX;
	m_bHaveSeek = false;
    m_bPlaybackFromPos = false;
}

CTestTask::~CTestTask(void)
{
    QC_DEL_P(m_pExtSrc);
	CTestItem * pItem = m_lstItem.RemoveHead();
	while (pItem != NULL)
	{
		delete pItem;
		pItem = m_lstItem.RemoveHead();
	}
	QC_DEL_A(m_pDrmKeyText);
	QC_DEL_A(m_pDNSDetect);
	QC_DEL_A(m_pDNSServer);
	QC_DEL_A(m_pSavePath);
	QC_DEL_A(m_pExtName);
	QC_DEL_A(m_pHeadText);
	QC_DEL_A(m_pURL);
	QC_DEL_A(m_pName);

	QC_DEL_A(m_pTxtMsg);
	QC_DEL_A(m_pTxtTmp);
	QC_DEL_A(m_pTxtErr);
}

int	CTestTask::FillTask(char * pTask)
{
	int		nReadSize = 0;
	char	szLine[4086];
	char *	pLine = pTask;
	char *	pText = NULL;
	int		nLineSize = 0;
	CTestItem * pItem = NULL;

	QC_DEL_A(m_pName);
	while (*pLine != 0)
	{
		memset(szLine, 0, sizeof(szLine));
		nLineSize = qcReadTextLine(pLine, strlen(pLine), szLine, sizeof(szLine));
		nReadSize += nLineSize;
		pLine += nLineSize;
		if (szLine[0] == ';' || szLine[0] == '/')
			continue;
		if (nLineSize <= 4)
			break;

		if (szLine[0] == '[')
		{
			if (m_pName != NULL)
			{
				nReadSize -= nLineSize;
				break;
			}
			m_pName = new char[nLineSize];
			strcpy(m_pName, szLine + 1);
			m_pName[strlen(m_pName)-1] = 0;
		}
		else if (strncmp(szLine, "URL=", 4) == 0)
		{
			QC_DEL_A(m_pURL);
			m_pURL = new char[nLineSize];
			strcpy(m_pURL, szLine + 4);
		}
		else if (strncmp(szLine, "OPENFLAG=", 9) == 0)
		{
			pText = szLine + 9;
			//m_nOpenFlag = atoi(pText);
			int nFlag = 0;
			sscanf(pText, "0X%x", &nFlag);
			m_nOpenFlag = m_nOpenFlag | nFlag;
		}
		else if (strncmp(szLine, "HWDEC=", 6) == 0)
		{
			pText = szLine + 6;
			//m_nOpenFlag = atoi(pText);
			int nFlag = 0;
			sscanf(pText, "0X%x", &nFlag);
			m_nOpenFlag = m_nOpenFlag | nFlag;
		}
		else if (strncmp(szLine, "PLAYCOMPLETE=", 13) == 0)
		{
			pText = szLine + 13;
			m_nPlayComplete = atoi(pText);
		}
		else if (strncmp(szLine, "EXITCLOSE=", 10) == 0)
		{
			pText = szLine + 10;
			m_nExitClose = atoi(pText);
		}
		else if (strncmp(szLine, "STARTPOS=", 9) == 0)
		{
			pText = szLine + 9;
			m_llStartPos = atoi(pText);
		}
		else if (strncmp(szLine, "RATIO=", 6) == 0)
		{
			pText = szLine + 6;
			m_sRatio.nWidth = atoi(pText);
			pText = strchr(pText, ':') + 1;
			m_sRatio.nHeight = atoi(pText);
		}
		else if (strncmp(szLine, "SPEED=", 6) == 0)
		{
			pText = szLine + 6;
			m_dSpeed = atof(pText);
		}
		else if (strncmp(szLine, "OFFSETTIME=", 11) == 0)
		{
			pText = szLine + 11;
			m_nOffsetTime = atoi(pText);
		}
		else if (strncmp(szLine, "SEEKMODE=", 9) == 0)
		{
			pText = szLine + 9;
			m_nSeekMode = atoi(pText);
		}
		else if (strncmp(szLine, "PREPROTOCOL=", 12) == 0)
		{
			pText = szLine + 12;
			m_nPreProtocol = atoi(pText);
		}
		else if (strncmp(szLine, "PREFERFORMAT=", 13) == 0)
		{
			pText = szLine + 13;
			m_nPreferFormat = atoi(pText);
		}
		else if (strncmp(szLine, "SAVEPATH=", 9) == 0)
		{
			QC_DEL_A(m_pSavePath);
			pText = szLine + 9;
#ifdef __QC_OS_IOS__
            m_pSavePath = new char[1024];
            qcGetAppPath(NULL, m_pSavePath, 1024);
            strcat(m_pSavePath, "cache/");
#else
			m_pSavePath = new char[strlen(pText) + 1];
			strcpy(m_pSavePath, pText);
#endif
		}
		else if (strncmp(szLine, "EXTNAME=", 8) == 0)
		{
			QC_DEL_A(m_pExtName);
			pText = szLine + 8;
			m_pExtName = new char[strlen(pText) + 1];
			strcpy(m_pExtName, pText);
		}
		else if (strncmp(szLine, "RTSPMODE=", 9) == 0)
		{
			pText = szLine + 9;
			m_nRTSPMode = atoi(pText);
		}
		else if (strncmp(szLine, "CONNECTTIMEOUT=", 15) == 0)
		{
			pText = szLine + 15;
			m_nConnectTimeOut = atoi(pText);
		}
		else if (strncmp(szLine, "READTIMEOUT=", 12) == 0)
		{
			pText = szLine + 12;
			m_nReadTimeOut = atoi(pText);
		}
		else if (strncmp(szLine, "HEADTEXT=", 9) == 0)
		{
			QC_DEL_A(m_pHeadText);
			pText = szLine + 9;
			m_pHeadText = new char[strlen(pText) + 1];
			strcpy(m_pHeadText, pText);
		}
		else if (strncmp(szLine, "DNSSERVER=", 10) == 0)
		{
			QC_DEL_A(m_pDNSServer);
			pText = szLine + 10;
			m_pDNSServer = new char[strlen(pText) + 1];
			strcpy(m_pDNSServer, pText);
		}
		else if (strncmp(szLine, "DNSDETECT=", 10) == 0)
		{
			QC_DEL_A(m_pDNSDetect);
			pText = szLine + 10;
			m_pDNSDetect = new char[strlen(pText) + 1];
			strcpy(m_pDNSDetect, pText);
		}
		else if (strncmp(szLine, "MAXBUFFTIME=", 12) == 0)
		{
			pText = szLine + 12;
			m_nMaxBuffTime = atoi(pText);
		}
		else if (strncmp(szLine, "MINBUFFTIME=", 12) == 0)
		{
			pText = szLine + 12;
			m_nMinBuffTime = atoi(pText);
		}
		else if (strncmp(szLine, "DRMKEYTEXT=", 11) == 0)
		{
			QC_DEL_A(m_pDrmKeyText);
			pText = szLine + 11;
			m_pDrmKeyText = new char[strlen(pText) + 1];
			strcpy(m_pDrmKeyText, pText);
		}
		else if (strncmp(szLine, "LOGLEVEL=", 9) == 0)
		{
			pText = szLine + 9;
			m_nLogLevel = atoi(pText);
		}
		else if (strncmp(szLine, "PLAYLOOP=", 9) == 0)
		{
			pText = szLine + 9;
			m_nPlayLoop = atoi(pText);
		}
		else if (strncmp(szLine, "PRELOADTIME=", 12) == 0)
		{
			pText = szLine + 12;
			m_nPreloadTime = atoi(pText);
		}
		else if (strncmp(szLine, "ACTION=", 7) == 0)
		{
			if (strstr(szLine, "seek") != NULL)
				m_bHaveSeek = true;
			pItem = new CTestItem(this, m_pInst);
			pItem->AddItem(szLine);
			m_lstItem.AddTail(pItem);
		}
		else if (strncmp(szLine, "SETTING=", 8) == 0)
		{
			pItem = new CTestItem(this, m_pInst);
			pItem->AddItem(szLine);
			m_lstItem.AddTail(pItem);
		}
        else if (strncmp(szLine, "EXTSRC=", 7) == 0)
        {
            pText = szLine + 7;
            m_nExtSrcType = atoi(pText);
        }
	}

	return nReadSize;
}

int	CTestTask::Start(CTestPlayer * pPlayer)
{
	m_pPlayer = pPlayer;
	if (m_pPlayer == NULL)
		return QC_ERR_ARG;
	m_pPlayer->SetCurTask(this);

	m_pInst->AddInfoItem(this, QCTEST_INFO_Item, m_pName);
	m_pInst->AddInfoItem(this, QCTEST_INFO_Func, "RESET");
	m_pInst->AddInfoItem(this, QCTEST_INFO_Msg, "RESET");
	sprintf(m_pTxtErr, "%s  %s --------", m_pName, m_pURL);
	m_pInst->AddInfoItem(this, QCTEST_INFO_Err, m_pTxtErr);

	m_nStartTime = qcGetSysTime();
	m_nIndex = 0;

	m_pPlayer->Create();
	m_pPlayer->SetNotify(NotifyEvent, this);
	m_pPlayer->SetView(m_pInst->m_hWndVideo, NULL);

	if (m_pSavePath != NULL)
		m_pPlayer->SetParam(QCPLAY_PID_PD_Save_Path, m_pSavePath);
	if (m_pExtName != NULL)
		m_pPlayer->SetParam(QCPLAY_PID_PD_Save_ExtName, m_pExtName);
	if (m_pHeadText != NULL)
		m_pPlayer->SetParam(QCPLAY_PID_HTTP_HeadReferer, m_pHeadText);
	if (m_pDNSServer != NULL)
		m_pPlayer->SetParam(QCPLAY_PID_DNS_SERVER, m_pDNSServer);
	if (m_pDNSDetect != NULL)
		m_pPlayer->SetParam(QCPLAY_PID_DNS_DETECT, m_pDNSDetect);
	if (m_pDrmKeyText != NULL)
		m_pPlayer->SetParam(QCPLAY_PID_DRM_KeyText, m_pDrmKeyText);

	m_pPlayer->SetParam(QCPLAY_PID_Prefer_Protocol, &m_nPreProtocol);
	m_pPlayer->SetParam(QCPLAY_PID_Prefer_Format, &m_nPreferFormat);
	m_pPlayer->SetParam(QCPLAY_PID_RTSP_UDPTCP_MODE, &m_nRTSPMode);
	if (m_nConnectTimeOut > 0)
		m_pPlayer->SetParam(QCPLAY_PID_Socket_ConnectTimeout, &m_nConnectTimeOut);
	if (m_nReadTimeOut > 0)
		m_pPlayer->SetParam(QCPLAY_PID_Socket_ReadTimeout, &m_nReadTimeOut);
	if (m_nMaxBuffTime > 0)
		m_pPlayer->SetParam(QCPLAY_PID_PlayBuff_MaxTime, &m_nMaxBuffTime);
	if (m_nMinBuffTime > 0)
		m_pPlayer->SetParam(QCPLAY_PID_PlayBuff_MinTime, &m_nMinBuffTime);
	if (m_nLogLevel > 0)
		m_pPlayer->SetParam(QCPLAY_PID_Log_Level, &m_nLogLevel);
	m_pPlayer->SetParam(QCPLAY_PID_Playback_Loop, &m_nPlayLoop);
	if (m_nPreloadTime > 0)
		m_pPlayer->SetParam(QCPLAY_PID_MP4_PRELOAD, &m_nPreloadTime);
    if (m_llStartPos > 0)
        m_pPlayer->SetParam(QCPLAY_PID_START_POS, &m_llStartPos);
    m_pPlayer->SetParam(QCPLAY_PID_Seek_Mode, &m_nSeekMode);
    
    int nRC = QC_ERR_NONE;
    if(m_nExtSrcType != -1)
    {
        m_pExtSrc = new CExtSource(NULL);
        m_pExtSrc->SetURL(m_pURL);
        m_pExtSrc->SetSourceType(m_nExtSrcType);
        m_pExtSrc->SetPlayer(&m_pPlayer->m_player);
    }
    else
    {
        int nRC = m_pPlayer->Open(m_pURL, m_nOpenFlag);
        if (nRC != QC_ERR_NONE)
            return nRC;
    }

	CTestItem * pItem = NULL;
	NODEPOS pos = m_lstItem.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_lstItem.GetNext(pos);
		pItem->SetPlayer(m_pPlayer);
		pItem->ScheduleTask();
	}

	return nRC;
}

int	CTestTask::Stop(void)
{
    QC_DEL_P(m_pExtSrc);
	m_pInst->m_pTestMng->ResetTask();

	if (m_pPlayer != NULL && m_nExitClose > 0)
		m_pPlayer->Close();

	return QC_ERR_NONE;
}

int	CTestTask::ExcuteItem(int nItem)
{
	CTestItem * pItem = NULL;
	NODEPOS pos = m_lstItem.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_lstItem.GetNext(pos);
		if (pItem->m_nItemIndex == nItem)
			pItem->ExecuteCmd();
	}
	return QC_ERR_NONE;
}

int CTestTask::CheckStatus(void)
{
	if (m_stsPlay != QC_PLAY_Run)
		return QC_ERR_NONE;

	if (m_nVideoCount > 0 && m_nVideoError != m_nVideoCount && !m_bHaveSeek)
	{
		int nVideoTime = qcGetSysTime() - m_nLastSysVideo;
		if (nVideoTime > 200)
		{
			m_nVideoError = m_nVideoCount;
			sprintf(m_pTxtErr, "The video time % 8lld at frame % 8d   didn't render % 8d ms.", m_llVideoTime, m_nVideoCount, nVideoTime);
			m_pInst->AddInfoItem(this, QCTEST_INFO_Err, m_pTxtErr);
		}
	}

	return QC_ERR_NONE;
}

void CTestTask::NotifyEvent(void * pUserData, int nID, void * pValue)
{
	return ((CTestTask *)pUserData)->HandleEvent(nID, pValue);
}

void CTestTask::HandleEvent(int nID, void * pValue)
{
    if (m_pExtSrc != NULL)
        m_pExtSrc->NotifyEvent(this, nID, pValue);

	if (nID == QC_MSG_PLAY_RUN)
	{
		m_nLastSysVideo = qcGetSysTime();
		m_stsPlay = QC_PLAY_Run;
	}
	else if (nID == QC_MSG_PLAY_PAUSE)
		m_stsPlay = QC_PLAY_Pause;
	else if (nID == QC_MSG_PLAY_STOP)
		m_stsPlay = QC_PLAY_Stop;
	else
		m_stsPlay = QC_PLAY_Init;

	ShowStatus(nID, pValue);

	switch (nID)
	{
	case QC_MSG_PLAY_OPEN_DONE:
	case QC_MSG_PLAY_OPEN_FAILED:
		OnOpenDone(nID, pValue);
		break;
	case QC_MSG_PLAY_COMPLETE:
		OnPlayComplete();
		break;
    case QC_MSG_PLAY_SEEK_DONE:
        OnSeekDone();
        break;

	default:
		break;
	}
}

void CTestTask::OnOpenDone(int nMsgID, void * pValue)
{
	if (nMsgID == QC_MSG_PLAY_OPEN_DONE)
	{
#ifdef __QC_OS_WIN32__		
		InvalidateRect(m_pInst->m_hWndVideo, NULL, TRUE);
		long long llDur = m_pPlayer->GetDur();
		m_pInst->m_pSlidePos->SetRange(0, (int)llDur);		
#endif // __QC_OS_WIN32__	

		m_pPlayer->SetParam(QCPLAY_PID_SendOut_VideoBuff, (void *)ReceiveData);
		m_pPlayer->SetParam(QCPLAY_PID_SendOut_AudioBuff, (void *)ReceiveData);

		m_pPlayer->SetParam(QCPLAY_PID_AspectRatio, &m_sRatio);
		m_pPlayer->SetParam(QCPLAY_PID_Speed, &m_dSpeed);
		m_pPlayer->SetParam(QCPLAY_PID_Clock_OffTime, &m_nOffsetTime);

		RECT rcZoom;
		memset(&rcZoom, 0, sizeof(rcZoom));
		m_pPlayer->SetParam(QCPLAY_PID_Zoom_Video, &rcZoom);

		m_nVideoCount = 0;
		m_nVideoError = 0;
		m_llVideoTime = 0;
		m_nLastSysVideo = 0;
		m_nAudioCount = 0;
		m_llAudioTime = 0;
		m_nLastSysAudio = 0;
        
        long long llDuration = m_pPlayer->GetDur();
        // 0: live; >0 : VOD
        if (llDuration == 0)
        {
            sprintf(m_pTxtErr, "%s, %lld", "Duration error", llDuration);
            QCLOGT("qcAutotest", "%s", m_pTxtErr);
            m_pInst->AddInfoItem(this, QCTEST_INFO_Err, m_pTxtErr);
        }

        m_pPlayer->Run();
	}
	else
	{
		sprintf(m_pTxtErr, "Open %s failed. err: % 8x", m_pURL, *(int *)pValue);
		m_pInst->AddInfoItem(this, QCTEST_INFO_Err, m_pTxtErr);

		m_pInst->m_pTestMng->PostTask(QCTEST_TASK_EXIT, 0, 0, 0, NULL);
	}
}

void CTestTask::OnPlayComplete(void)
{
	if (m_nPlayLoop > 0)
	{
		m_pInst->AddInfoItem(this, QCTEST_INFO_Err, "Receive complete event, but it is loop.");
		return;
	}

    QC_DEL_P(m_pExtSrc);
	if (m_nPlayComplete == 0)
		m_pInst->m_pTestMng->PostTask(QCTEST_TASK_EXIT, 0, 0, 0, NULL);
	else
		m_pPlayer->SetPos(0);

	return;
}

void CTestTask::OnSeekDone(void)
{
    if(m_bPlaybackFromPos && m_llStartPos > 0)
    {
        m_bPlaybackFromPos = false;
        m_pPlayer->Run();
    }
}

int	CTestTask::ReceiveData(void * pUserData, QC_DATA_BUFF * pBuffer)
{
	return ((CTestTask*)pUserData)->HandleData(pBuffer);
}

int CTestTask::HandleData(QC_DATA_BUFF * pBuffer)
{
	CAutoLock lock(&m_mtData);
	if (pBuffer->nMediaType == QC_MEDIA_Video)
	{
		if (m_nVideoCount == 0)
		{
			int		nFirstTime = qcGetSysTime() - m_nStartTime;
			char	szInfo[256];
			sprintf(szInfo, "VFT = %d", nFirstTime);
			m_pInst->AddInfoItem(this, QCTEST_INFO_Item, szInfo);
		}
		else
		{
			if (!m_bHaveSeek)
			{
				int nVideoTime = qcGetSysTime() - m_nLastSysVideo - (int)(pBuffer->llTime - m_llVideoTime);
				if (nVideoTime > 200)
				{
					sprintf(m_pTxtErr, "The video time % 8lld at frame % 8d   didn't render % 8d ms.", m_llVideoTime, m_nVideoCount, nVideoTime);
					m_pInst->AddInfoItem(this, QCTEST_INFO_Err, m_pTxtErr);
				}
			}
		}
		m_nVideoCount++;
		m_nLastSysVideo = qcGetSysTime();
		m_llVideoTime = pBuffer->llTime;
	}
	else if (pBuffer->nMediaType == QC_MEDIA_Audio)
	{
		if (m_nAudioCount == 0)
		{
			int		nFirstTime = qcGetSysTime() - m_nStartTime;
			char	szInfo[256];
			sprintf(szInfo, "AFT = %d", nFirstTime);
			//m_pInst->AddInfoItem(this, QCTEST_INFO_Item, szInfo);
		}
		else if (m_nAudioCount > 10 && !m_bHaveSeek)
		{
			int nDiffTime = abs ((qcGetSysTime() - m_nLastSysAudio) - (int)(pBuffer->llTime - m_llAudioTime));
			if (nDiffTime > 200)
			{
				sprintf(m_pTxtErr, "The audio time % 8lld at frame % 8d   diff % 8d ms.", m_llAudioTime, m_nAudioCount, nDiffTime);
				m_pInst->AddInfoItem(this, QCTEST_INFO_Err, m_pTxtErr);
			}
		}
		m_nAudioCount++;
		m_nLastSysAudio = qcGetSysTime();
		m_llAudioTime = pBuffer->llTime;
	}

	return QC_ERR_NONE;
}

void CTestTask::ShowStatus(int nMsgID, void * pValue)
{
	if (m_pInst->m_bExitTest)
		return;
	if (nMsgID == QC_MSG_BUFF_SEI_DATA || nMsgID == QC_MSG_SNKA_RENDER || nMsgID == QC_MSG_SNKV_RENDER)
		return;
	m_nIndex++;

	bool	bErr = false;
	int		nStartPos = 0;
	int		nTxtLen = 4096;
	int		tmSec = (qcGetSysTime() - m_nStartTime) / 1000;
	int		tmMS = (qcGetSysTime() - m_nStartTime) % 1000;;

	memset(m_pTxtMsg, ' ', nTxtLen);

	sprintf(m_pTxtTmp, "%06d", m_nIndex++);
	memcpy(m_pTxtMsg + nStartPos, m_pTxtTmp, strlen(m_pTxtTmp));
	nStartPos += 10;

	sprintf(m_pTxtTmp, "%02d:%02d:%02d:%03d", tmSec / 3600, (tmSec % 3600) / 60, tmSec % 60, tmMS);
	memcpy(m_pTxtMsg + nStartPos, m_pTxtTmp, strlen(m_pTxtTmp));
	nStartPos += 20;

	QCMSG_ConvertName(nMsgID, m_pTxtTmp, 128);
	memcpy(m_pTxtMsg + nStartPos, m_pTxtTmp, strlen(m_pTxtTmp));
	nStartPos += 32;

	switch (nMsgID)
	{
	case QC_MSG_HTTP_CONTENT_SIZE:
	case QC_MSG_HTTP_BUFFER_SIZE:
		sprintf(m_pTxtMsg + nStartPos, "  %lld", *(long long *)pValue);
		nStartPos += 16;
		break;

	case QC_MSG_HTTP_DOWNLOAD_PERCENT:
	case QC_MSG_HTTP_DOWNLOAD_SPEED:
	case QC_MSG_RTMP_DOWNLOAD_SPEED:
	case QC_MSG_BUFF_VBUFFTIME:
	case QC_MSG_BUFF_ABUFFTIME:
	case QC_MSG_BUFF_GOPTIME:
	case QC_MSG_BUFF_VFPS:
	case QC_MSG_BUFF_AFPS:
	case QC_MSG_BUFF_VBITRATE:
	case QC_MSG_BUFF_ABITRATE:
	case QC_MSG_RENDER_VIDEO_FPS:
	case QC_MSG_RENDER_AUDIO_FPS:
		sprintf(m_pTxtMsg + nStartPos, "  %d", *(int *)pValue);
		nStartPos += 16;
		break;

	case QC_MSG_HTTP_CONNECT_FAILED:
	case QC_MSG_HTTP_RECONNECT_FAILED:
	case QC_MSG_RTMP_CONNECT_FAILED:
	case QC_MSG_RTMP_DISCONNECTED:
	case QC_MSG_RTMP_RECONNECT_FAILED:
	case QC_MSG_PARSER_M3U8_ERROR:
	case QC_MSG_PARSER_FLV_ERROR:
	case QC_MSG_PARSER_MP4_ERROR:
	case QC_MSG_VIDEO_HWDEC_FAILED:
	case QC_MSG_PLAY_SEEK_FAILED:
		bErr = true;
		break;

	default:
		break;
	}

	m_pTxtMsg[nStartPos] = 0;
	if (bErr)
		m_pInst->AddInfoItem(this, QCTEST_INFO_Err, m_pTxtMsg);
	else
		m_pInst->AddInfoItem(this, QCTEST_INFO_Msg, m_pTxtMsg);
}

int CTestTask::OpenURL()
{
    if(!m_pURL)
        return QC_ERR_FAILED;
    m_nStartTime = qcGetSysTime();
    return m_pPlayer->Open(m_pURL, m_nOpenFlag);
}

