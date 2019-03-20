/*******************************************************************************
	File:		COMBoxMng.cpp

	Contains:	The media engine implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-15		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#ifdef __QC_OS_WIN32__
#include "windows.h"
#include "psapi.h"
#include "CTrackMng.h"
#elif defined __QC_OS_NDK__
#include <sys/system_properties.h>
#elif defined __QC_OS_IOS__
#include "CiOSPlayer.h"
#endif // _OS_WINPC

#include "COMBoxMng.h"
#include "CBoxAudioDec.h"
#include "CBoxVideoDec.h"

#include "AdaptiveStreamParser.h"
#include "CAnalysisMng.h"

#include "CMsgMng.h"
#include "USystemFunc.h"
#include "USocketFunc.h"
#include "ULogFunc.h"
#include "CQCMuxer.h"

COMBoxMng::COMBoxMng(void * hInst)
	: CBaseObject (NULL)
	, m_hInst (hInst)
	, m_fNotifyEvent (NULL)
	, m_pUserData (NULL)
	, m_bExit(false)
	, m_stsPlay (QC_PLAY_Init)
	// Source open
	, m_nOpenFlag (0)
	, m_llDur (0)
	, m_bOpening (false)
	, m_bClosed (false)
	, m_llStartTime (-1)
	, m_nParserFormat(QC_PARSER_NONE)
	, m_llReopenPos(0)
	, m_nTotalRndCount(0)
	, m_bEOS(false)
	// render parameters
	, m_hView (NULL)
	, m_nDisVideoLevel (QC_PLAY_VideoEnable)
	// seek parameters
	, m_nSeekMode (0)
	, m_llSeekPos (0)
	, m_bSeeking (false)
	, m_nLastSeekTime (0)
	// box paramters
	, m_pBoxSource (NULL)
	, m_pRndAudio (NULL)
	, m_pRndVideo (NULL)
	, m_pClock (NULL)
	, m_pClockMng (NULL)
	, m_pExtRndAudio (NULL)
	, m_pExtRndVideo (NULL)
	, m_pThreadWork (NULL)
    , m_pAnlMng(NULL)
	, m_bFirstFrameRendered(false)
	, m_pURL(NULL)
{
	SetObjectName ("COMBoxMng");
    
    int nVersion = GetSDKVersion();
	char szCPU[16];
	strcpy (szCPU, " ");
#ifdef __QC_CPU_X86__ 
	strcpy (szCPU, "CPU: X86");
#elif defined __QC_CPU_ARMV8__
	strcpy (szCPU, "CPU: ARMV8");
#elif defined  __QC_CPU_ARMV7__
	strcpy (szCPU, "CPU: ARMV7");
#else
	strcpy (szCPU, "CPU: ARMV6");
#endif // __QC_CPU_X86__
    QCLOGI("SDK version %d.%d.%d.%d, %s %s. %s", (nVersion>>24) & 0xFF, (nVersion>>16) & 0xFF, (nVersion>>8) & 0xFF, nVersion&0xFF, __TIME__,  __DATE__, szCPU);

	m_pBaseInst = new CBaseInst();

	memset (&m_rcView, 0, sizeof (RECT));
	m_pBoxMonitor = new CBoxMonitor(m_pBaseInst);

	m_pThreadWork = new CThreadWork(m_pBaseInst);
	m_pThreadWork->SetOwner (m_szObjName);
	m_pThreadWork->Start ();
	m_pBaseInst->m_pMsgMng = new CMsgMng(m_pBaseInst);
	m_pBaseInst->m_pMsgMng->RegNotify(this);

#ifdef __QC_OS_IOS__
    m_pPlayer = new CiOSPlayer(m_pBaseInst);
#endif
    
    UpdateAnal();

	memset(m_szQiniuDrmKey, 0, sizeof(m_szQiniuDrmKey));
	memset(m_szPreName, 0, sizeof(m_szPreName));

	m_llStartPos = 0;
	m_nPreloadTimeDef = m_pBaseInst->m_pSetting->g_qcs_nMoovPreLoadItem;
	m_nPreloadTimeSet = 0;

	strcpy(m_szCacheURL, "");

    PostAsyncTask(QC_TASK_CHECK, 0, 0, NULL, 10000);
}

COMBoxMng::~COMBoxMng(void)
{
	//make sure it can exit application.
	m_bOpening = false;
//	CAutoLock lock(&m_mtFunc);
	m_bExit = true;
	QCLOGI("Try to close when exit.");
	Close ();

    m_pBaseInst->SetForceClose(true);

	QCLOGI("Try to remove notify when exit.");
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->RemNotify(this);

	QCLOGI("Try to stop thread when exit.");
	if (m_pThreadWork != NULL)
	{
		m_pThreadWork->Stop ();
		QC_DEL_P (m_pThreadWork);
	}
	QCLOGI("Try to delete source when exit.");
	QC_DEL_P(m_pBoxSource);

#ifdef __QC_OS_IOS__
    if(m_pPlayer)
        m_pPlayer->DestroyRender(&m_pExtRndVideo);
    QC_DEL_P(m_pPlayer);
#endif

	QCLOGI("Try to delete analyse when exit.");
	QC_DEL_P(m_pAnlMng);

#ifdef __QC_OS_WIN32__
	QCLOGI("Try to delete track manager when exit.");
	QC_DEL_P(m_pBaseInst->m_pTrackMng);
#endif // __QC_OS_WIN32_

	QCLOGI("Try to delte monitor when exit.");
	QC_DEL_P (m_pBoxMonitor);
	QC_DEL_P (m_pClockMng);
    QC_DEL_A (m_pURL);

	QCLOGI("Try to free codec lib when exit.");
	if (m_pBaseInst->m_pMuxer != NULL)
	{
		QC_DEL_P(m_pBaseInst->m_pMuxer);
	}
	if (m_pBaseInst->m_hLibCodec != NULL)
	{
		qcLibFree(m_pBaseInst->m_hLibCodec, 0);
		m_pBaseInst->m_hLibCodec = NULL;
	}

	QCLOGI("Try to delete instance when exit.");
	delete m_pBaseInst;

#ifdef __QC_DEBUG__
	char outDebugName[128];
	for (int i = 0; i < 2048; i++)
	{
		if (g_lstObj[i] != NULL)
		{
			if (g_lstObj[i] == this || g_lstObj[i] == &m_mtFunc)
				continue;
			strcpy(outDebugName, "Find rest object  ");
			strcat(outDebugName, g_lstObj[i]->m_szObjName);
			strcat(outDebugName, "\r\n");
			OutputDebugString(outDebugName);
		}
	}
#endif // __QC_DEBUG__
    QCLOGI("Box manager exit safely!");
    qcCheckLogCache();
}

void COMBoxMng::SetNotifyFunc (QCPlayerNotifyEvent pFunc, void * pUserData)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	m_fNotifyEvent = pFunc;
	m_pUserData = pUserData;
}

void COMBoxMng::SetView (void * hView, RECT * pRect)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	CAutoLock lock(&m_mtView);
    
    m_hView = hView;
	
	if (pRect != NULL)
		memcpy (&m_rcView, pRect, sizeof (RECT));
	if (m_pRndVideo != NULL)
		m_pRndVideo->SetView (m_hView, &m_rcView);
    
#ifdef __QC_OS_IOS__
    if(!m_pExtRndVideo && m_pPlayer)
        m_pExtRndVideo = m_pPlayer->CreateRender(m_hView, &m_rcView);
    // view size maybe re-size during open, it need process on main-thread
    if(m_pExtRndVideo && m_pRndVideo==NULL)
        m_pExtRndVideo->SetView(m_hView, &m_rcView);
#endif
}

int COMBoxMng::Open (const char * pSource, int nFlag)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, nFlag);
	if (!pSource)
	{
		nRC = QC_ERR_EMPTYPOINTOR;
		return nRC;
	}
#ifndef __QC_LIB_ONE__
	if (m_pBaseInst->m_hLibCodec == NULL)
		m_pBaseInst->m_hLibCodec = (qcLibHandle)qcLibLoad("qcCodec", 0);
#endif // __QC_LIB_ONE__
	m_pBaseInst->SetForceClose(true);
    if (CheckOpenStatus (2000) != QC_ERR_NONE)
    {
        QCLOGI ("Open failed for error status!");
		nRC = QC_ERR_STATUS;
		return nRC;
	}
	m_pBaseInst->SetForceClose(false);

	m_pBaseInst->m_nOpenSysTime = qcGetSysTime();
    
    if (pSource != NULL)
    {
        QC_DEL_A (m_pURL);
        m_pURL = new char[strlen(pSource) + 1];
        sprintf(m_pURL, "%s", pSource);
    }

    if ((nFlag & QCPLAY_OPEN_SAME_SOURCE) == QCPLAY_OPEN_SAME_SOURCE)
    {
        // it must consider open fail for the first time playback, audio and video render was not created.
        if (m_pBoxSource != NULL && m_pRndAudio && m_pRndVideo)
        {
            if(m_pAnlMng)
                m_pAnlMng->OnOpen(pSource);
#ifdef __QC_OS_IOS__
            if(m_pPlayer)
                m_pPlayer->Open(pSource);
#endif
			if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_OPEN_START, 0, 0);

            //			DoSeek (0, false);
            m_stsPlay = QC_PLAY_Init;
            m_bOpening = true;
			if (m_pBoxSource != NULL)
				m_pBoxSource->CancelCache();
			PostAsyncTask(QC_TASK_OPEN, nFlag, 0, pSource);
			nRC = QC_ERR_NONE;
			return nRC;
		}
        else
            nFlag &= ~QCPLAY_OPEN_SAME_SOURCE;
    }
    
	if (Close() < 0)
	{
		nRC = QC_ERR_STATUS;
		return nRC;
	}

    if(m_pAnlMng)
        m_pAnlMng->OnOpen(pSource);
#ifdef __QC_OS_IOS__
    if(m_pPlayer)
        m_pPlayer->Open(pSource);
#endif
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_OPEN_START, 0, 0);

	m_pBaseInst->SetForceClose(false);

	CAutoLock lock (&m_mtFunc);

#ifdef __QC_OS_IOS__
    // Open should be called on main thread
    if(!m_pExtRndVideo && m_pPlayer)
        m_pExtRndVideo = m_pPlayer->CreateRender(m_hView, &m_rcView);
#endif

	m_nOpenFlag = nFlag;
	m_bClosed = false;
	m_bOpening = true;
	m_nTotalRndCount = 0;
	m_bEOS = false;

	if (m_nPreloadTimeSet > 0)
		m_pBaseInst->m_pSetting->g_qcs_nMoovPreLoadItem = (m_nPreloadTimeSet / 1000) * 30;
	else
	{
		if (m_llStartPos > 0)
			m_pBaseInst->m_pSetting->g_qcs_nMoovPreLoadItem = (m_llStartPos / 500) * 30;
		else
			m_pBaseInst->m_pSetting->g_qcs_nMoovPreLoadItem = m_nPreloadTimeDef;
	}

	if (m_pBoxSource != NULL)
	{
		if (strcmp(pSource, m_szCacheURL) != 0)
			m_pBoxSource->CancelCache();
	}
    PostAsyncTask(QC_TASK_OPEN, nFlag, 0, pSource);

	return QC_ERR_NONE;
}

int COMBoxMng::PostAsyncTask(int nID, int nValue, long long llValue, const char * pName, int nDelay)
{
    if (m_pThreadWork != NULL)
    {
        CThreadEvent * pEvent = m_pThreadWork->GetFree ();
        if (pEvent == NULL)
        {
            pEvent = new CThreadEvent (nID, nValue, llValue, pName);
            pEvent->SetEventFunc (this, &CThreadFunc::OnEvent);
        }
        else
        {
            pEvent->m_nID = nID;
            pEvent->m_nValue = nValue;
            pEvent->m_llValue = llValue;
            pEvent->SetName (pName);
        }
        m_pThreadWork->PostEvent (pEvent, nDelay);
    }

    return QC_ERR_NONE;
}

int COMBoxMng::Close (void)
{	
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, 0);

	if (m_pBoxSource == NULL)
		return QC_ERR_NONE;

	m_pBaseInst->SetForceClose(true);
	if (CheckOpenStatus(2000) != QC_ERR_NONE)
	{
		QCLOGI ("Try to close failed for the status was error!");
		nRC = QC_ERR_STATUS;
		return nRC;
	}
	CAutoLock lock (&m_mtFunc);
	Stop ();
	m_pBaseInst->SetForceClose(true);	
	m_bClosed = true;
	CBoxBase * pBoxIn = NULL;
	CBoxBase * pBox = NULL;
    {
        CAutoLock lock(&m_mtView);
        pBox = m_pRndVideo;
        while (pBox != NULL)
        {
            pBoxIn = pBox->GetSource ();
            if (pBox != m_pBoxSource)
                delete pBox;
            pBox = pBoxIn;
        }
        m_pRndVideo = NULL;
    }
	pBox = m_pRndAudio;
	while (pBox != NULL)
	{
		pBoxIn = pBox->GetSource ();
		if (pBox != m_pBoxSource)
			delete pBox;
		pBox = pBoxIn;
	}
	m_pRndAudio = NULL;
	m_pBoxSource->Close();
	m_lstBox.RemoveAll ();
	m_pClock = NULL;

	if (m_pBoxMonitor != NULL)
		m_pBoxMonitor->ReleaseItems ();
    
	m_llSeekPos = 0;
	m_stsPlay = QC_PLAY_Init;
	m_pBaseInst->SetForceClose(false);

	m_pBaseInst->ResetLogFunc();
	if (m_pBaseInst->m_pMsgMng != NULL)
		((CMsgMng *)m_pBaseInst->m_pMsgMng)->ReleaseItem();

	return QC_ERR_NONE;
}

int COMBoxMng::CheckOpenStatus (int nWaitTime)
{
	int nStartTime = qcGetSysTime ();
	while (m_bOpening || m_bSeeking || m_pBaseInst->m_bCheckReopn)
	{
		qcSleep (2000);
		if ((qcGetSysTime ()-nStartTime) > nWaitTime)
			break;
	}
	if (m_bOpening || m_pBaseInst->m_bCheckReopn)
	{
		QCLOGW ("CheckOpenStatus failed! %d, %d", m_bOpening, m_pBaseInst->m_bCheckReopn);
		return QC_ERR_STATUS;
	}
	return QC_ERR_NONE;
}

int COMBoxMng::DoFastOpen (const char * pSource, int nFlag)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, nFlag);
	
	if (!m_pBoxSource)
	{
		nRC = QC_ERR_STATUS;
		return nRC;
	}
    
    CAutoLock lock (&m_mtFunc);
	m_bSendIPAddr = false;
    m_llDur = 0;
    m_bFirstFrameRendered = false;
	m_pBaseInst->m_nDownloadPause = 0;
    m_pBaseInst->m_pSetting->g_qcs_nVideoRotateAngle = 0;
    
    if (m_pRndAudio != NULL)
        m_pRndAudio->SetNewSource (true);
    if (m_pRndVideo != NULL)
        m_pRndVideo->SetNewSource (true);
    if (m_szQiniuDrmKey[0] != 0)
        m_pBoxSource->SetParam(QC_PARSER_SET_QINIU_DRM_INFO, m_szQiniuDrmKey);
    memset(m_szQiniuDrmKey, 0, sizeof(m_szQiniuDrmKey));
    nRC = m_pBoxSource->OpenSource (pSource, nFlag);
    m_llSeekPos = 0;
    m_llDur = m_pBoxSource->GetDuration ();
    if (m_llDur == 0)
        m_llDur = -1;
    m_llStartTime = -1;
    m_stsPlay = QC_PLAY_Open;
    
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_DURATION, 0, m_llDur);

    return nRC;
}

int COMBoxMng::DoReopen (void)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, 0);
    
    CAutoLock lock(&m_mtFunc);

	if (m_pBoxSource == NULL || m_pURL == NULL)
	{
		nRC = QC_ERR_STATUS;
		return nRC;
	}

    if (m_pBaseInst != NULL)
        m_pBaseInst->NotifyReopen();
	m_pBaseInst->m_bCheckReopn = true;
	if (m_pRndAudio != NULL)
		m_pRndAudio->Flush ();
	if (m_pRndVideo != NULL)
		m_pRndVideo->Flush();
	nRC = m_pBoxSource->OpenSource(m_pURL, QCPLAY_OPEN_SAME_SOURCE);
	if (nRC == QC_ERR_NONE)
	{
		if (m_llReopenPos > 0)
			m_pBoxSource->SetPos(m_llReopenPos);
        if (m_pClock != NULL)
            m_pClock->SetTime(m_llReopenPos);
        m_pBoxSource->Start();
		m_llReopenPos = 0;
		m_nTotalRndCount = 0;
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		{
			if (m_pBoxSource->GetIOType() == QC_IOTYPE_RTMP)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_RECONNECT_SUCESS, 0, 0);
			else
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_RECONNECT_SUCESS, 0, 0);
		}
	}
    else
    {
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		{
			if (m_pBoxSource->GetIOType() == QC_IOTYPE_RTMP)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_RECONNECT_FAILED, 0, 0);
			else
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_RECONNECT_FAILED, 0, 0);
		}
    }
	m_pBaseInst->m_bCheckReopn = false;
	return nRC;
}

int COMBoxMng::DoCheckStatus(void)
{
	if (m_stsPlay == QC_PLAY_Run)
	{
		int nRndCount = 0;
		if (m_pRndVideo != NULL)
			nRndCount = m_pRndVideo->GetRndCount();
		if (m_pRndAudio != NULL)
			nRndCount += m_pRndAudio->GetRndCount();
		if (!m_bEOS && m_nTotalRndCount > 0 && !m_bSeeking && !m_pBaseInst->m_bForceClose)
		{
			//QCLOGI("CheckStatus TotalRnd = % 8d    AVRnd = % 8d", m_nTotalRndCount, nRndCount);
			if (m_nTotalRndCount == nRndCount)
			{
				if (m_nParserFormat == QC_PARSER_M3U8 || (m_nParserFormat == QC_PARSER_FLV && GetDuration() <= 0) || (m_nParserFormat == QC_PARSER_RTSP && GetDuration() <= 0))
				{
					if (m_llDur > 0 && m_llReopenPos == 0)
						m_llReopenPos = GetPos();
					PostAsyncTask(QC_TASK_REOPEN, 0, 0, NULL, 50);
				}
			}
		}
		m_nTotalRndCount = nRndCount;
	}

	PostAsyncTask(QC_TASK_CHECK, 0, 0, NULL, 10000);

	return QC_ERR_NONE;
}

int COMBoxMng::DoOpen (const char * pSource, int nFlag)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, nFlag);

	CAutoLock lock(&m_mtFunc);
	m_bSendIPAddr = false;
	m_llDur = 0;
    m_bFirstFrameRendered = false;
	m_pBaseInst->m_nDownloadPause = 0;
	m_pBaseInst->m_pSetting->g_qcs_nVideoRotateAngle = 0;

	if (m_pBoxSource == NULL)
		m_pBoxSource = new CBoxSource(m_pBaseInst, m_hInst);
	if (m_pBoxSource == NULL)
	{
		QCLOGE ("m_pBoxSource is NULL!");
		nRC = QC_ERR_MEMORY;
		return nRC;
	}
	m_lstBox.AddTail (m_pBoxSource);
	nRC = m_pBoxSource->OpenSource (pSource, nFlag);
	if (nRC != QC_ERR_NONE)
	{
		QCLOGE ("Open Source failed!");
		return nRC;
	}
	if (m_pBoxSource->GetStreamCount (QC_MEDIA_Audio) <= 0 &&
		m_pBoxSource->GetStreamCount (QC_MEDIA_Video) <= 0)
	{
		QCLOGE ("It was no audio and video!");
		nRC = QC_ERR_FORMAT;
		return nRC;
	}
	m_pBaseInst->m_bAudioDecErr = false;
	m_pBaseInst->m_bVideoDecErr = false;
	if (m_szQiniuDrmKey[0] != 0)
		m_pBoxSource->SetParam(QC_PARSER_SET_QINIU_DRM_INFO, m_szQiniuDrmKey);
	memset(m_szQiniuDrmKey, 0, sizeof(m_szQiniuDrmKey));
	m_pBaseInst->m_pSetting->g_qcs_nAudioDecVlm = 0;

	int nVideo = QC_ERR_NONE;
	if (m_pBoxSource->GetStreamCount (QC_MEDIA_Video) > 0)
	{
		if (m_pRndVideo == NULL)
		{
			QC_VIDEO_FORMAT * pVideoFormat = m_pBoxSource->GetMediaSource()->GetVideoFormat();
			if (pVideoFormat != NULL)
			{
				if (pVideoFormat->nCodecID != QC_CODEC_ID_H264 && pVideoFormat->nCodecID != QC_CODEC_ID_H265 && pVideoFormat->nCodecID != QC_CODEC_ID_NONE)
					nFlag = 0;
			}
#ifndef __QC_OS_IOS__
			if ((nFlag & QCPLAY_OPEN_VIDDEC_HW) == QCPLAY_OPEN_VIDDEC_HW)
			{
				m_pRndVideo = new CBoxVDecRnd(m_pBaseInst, m_hInst);
				if (m_pRndVideo == NULL)
				{
					nRC = QC_ERR_MEMORY;
					return nRC;
				}
				m_pRndVideo->SetExtRnd (m_pExtRndVideo);
                {
                    CAutoLock lock(&m_mtView);
                    m_pRndVideo->SetView (m_hView, &m_rcView);
                }
				m_pRndVideo->DisableVideo (m_nDisVideoLevel);
				nVideo = m_pRndVideo->SetSource(m_pBoxSource);
				if (nVideo != QC_ERR_NONE)
				{
					nVideo = m_pRndVideo->SetSource(NULL);
					QC_DEL_P(m_pRndVideo);
					m_fNotifyEvent (m_pUserData, QC_MSG_VIDEO_HWDEC_FAILED, NULL);
#ifdef __QC_OS_NDK__
					return QC_ERR_VIDEO_HWDEC;
#endif // __QC_OS_NDK__				
				}
				if (m_pRndVideo != NULL)
				{
					m_pRndVideo->SetView(m_hView, &m_rcView);
					m_lstBox.AddTail(m_pRndVideo);
				}
			}
#endif
			if (m_pRndVideo == NULL)
			{
				CBoxVideoDec * pVideoDec = NULL;
#ifdef __QC_OS_IOS__
                // It's workaround, may discuss later
				pVideoDec = new CBoxVideoDec (m_pBaseInst, (void*)&m_nOpenFlag);
#else
				pVideoDec = new CBoxVideoDec(m_pBaseInst, m_hInst);
#endif
				if (pVideoDec == NULL)
				{
					nRC = QC_ERR_MEMORY;
					return nRC;
				}
				nVideo = pVideoDec->SetSource (m_pBoxSource);
				if (nVideo == QC_ERR_NONE)
				{
					m_lstBox.AddTail (pVideoDec);
					CAutoLock lock(&m_mtView);
					m_pRndVideo = new CBoxVideoRnd(m_pBaseInst, m_hInst);
					if (m_pRndVideo == NULL)
					{
						nRC = QC_ERR_MEMORY;
						return nRC;
					}
					m_lstBox.AddTail(m_pRndVideo);
					m_pRndVideo->SetExtRnd (m_pExtRndVideo);
					m_pRndVideo->SetView (m_hView, &m_rcView);
					m_pRndVideo->DisableVideo (m_nDisVideoLevel);
                    m_pRndVideo->SetSeekMode (m_nSeekMode);
					nVideo = m_pRndVideo->SetSource (pVideoDec);
				}
				else
				{
					m_pBaseInst->m_bVideoDecErr = true;
					delete pVideoDec;
				}
			}
		}
	}

	int nAudio = QC_ERR_NONE;
	if (m_pBoxSource->GetStreamCount (QC_MEDIA_Audio) > 0)
	{
		CBoxAudioDec * pAudioDec = new CBoxAudioDec(m_pBaseInst, m_hInst);
		if (pAudioDec == NULL)
		{
			nRC = QC_ERR_MEMORY;
			return nRC;
		}
		nAudio = pAudioDec->SetSource (m_pBoxSource);
		if (nAudio == QC_ERR_NONE)
		{
			m_lstBox.AddTail(pAudioDec);
			m_pRndAudio = new CBoxAudioRnd(m_pBaseInst, m_hInst);
			if (m_pRndAudio == NULL)
			{
				nRC = QC_ERR_MEMORY;
				return nRC;
			}
			m_lstBox.AddTail(m_pRndAudio);
			m_pRndAudio->SetExtRnd (m_pExtRndAudio);
			m_pRndAudio->SetOtherRender (m_pRndVideo);
			if (m_pRndVideo != NULL)
				m_pRndVideo->SetOtherRender (m_pRndAudio);
            m_pRndAudio->SetSeekMode (m_nSeekMode);
			nAudio = m_pRndAudio->SetSource (pAudioDec);
		}
		else
		{
			m_pBaseInst->m_bAudioDecErr = true;
			delete pAudioDec;
		}
	}

	if (m_pRndAudio == NULL && m_pRndVideo == NULL)
	{
		QCLOGE ("it was error both audio and video dec!");
		nRC = QC_ERR_FORMAT;
		return nRC;
	}

	QC_DEL_P (m_pClockMng);
	if (m_pRndAudio != NULL)
		m_pClock = m_pRndAudio->GetClock ();
	else if (m_pRndVideo != NULL)
		m_pClock = m_pRndVideo->GetClock ();
	if (m_pClock == NULL)
	{
		m_pClockMng = new CBaseClock(m_pBaseInst);
		m_pClock = m_pClockMng;
	}
	m_pClock->SetTime (0);

	CBoxBase * pBox = NULL;
	NODEPOS pos = m_lstBox.GetHeadPosition ();
	while (pos != NULL)
	{
		pBox = m_lstBox.GetNext (pos);
		if (pBox != NULL)
			pBox->SetClock (m_pClock);
	}
	if (m_pBoxMonitor != NULL)
		m_pBoxMonitor->SetClock (m_pClock);
	m_llDur = m_pBoxSource->GetDuration ();
	if (m_llDur == 0)
		m_llDur = -1;
	m_stsPlay = QC_PLAY_Open;
	m_llStartTime = -1;
	m_pBaseInst->m_pSetting->g_qcs_nPerferFileForamt = QC_PARSER_NONE;

	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_DURATION, 0, m_llDur);
	nRC = QC_ERR_NONE;

	memset(m_szPreName, 0, sizeof(m_szPreName));
	strncpy(m_szPreName, pSource, 5);

	return nRC;
}

int	COMBoxMng::Start (void)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, 0);

	if (m_stsPlay <= QC_PLAY_Init)
    {
        nRC = QC_ERR_STATUS;
        return nRC;
    }
	if (m_bOpening || m_bSeeking || m_pBaseInst->m_bCheckReopn)
    {
        nRC = QC_ERR_STATUS;
        return nRC;
    }

	CAutoLock lock (&m_mtFunc);
    if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
        m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_RUN, 0, 0);
	if (m_pRndVideo != NULL)
		m_pRndVideo->Start ();	
	if (m_pRndAudio != NULL)
		m_pRndAudio->Start ();
	if (m_pClockMng != NULL)
		m_pClockMng->Start ();

	m_nTotalRndCount = 0;
	m_stsPlay = QC_PLAY_Run;

	return QC_ERR_NONE;
}

int COMBoxMng::Pause (void)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, 0);

    if (m_stsPlay <= QC_PLAY_Open || m_bSeeking || m_pBaseInst->m_bCheckReopn)
        return QC_ERR_STATUS;
    if (m_stsPlay == QC_PLAY_Pause)
        return QC_ERR_NONE;

	CAutoLock lock (&m_mtFunc);
	if (m_pRndVideo != NULL)
		m_pRndVideo->Pause ();
	if (m_pRndAudio != NULL)
		m_pRndAudio->Pause ();
	if (m_pClockMng != NULL)
		m_pClockMng->Pause ();

	m_stsPlay = QC_PLAY_Pause;
    if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
        m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_PAUSE, 0, GetPos());
	return QC_ERR_NONE;
}

int	COMBoxMng::Stop (void)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, m_stsPlay);

	if (/*m_stsPlay == QC_PLAY_Init || */m_stsPlay == QC_PLAY_Stop)
		return QC_ERR_NONE;

	if (m_pBaseInst->m_pMuxer != NULL)
		m_pBaseInst->m_pMuxer->Close();

	m_pBaseInst->SetForceClose(true);
	if (CheckOpenStatus(2000) != QC_ERR_NONE)
	{
		nRC = QC_ERR_STATUS;
		return nRC;
	}

	CAutoLock lock (&m_mtFunc);
    long long llPos = GetPos();
	if (m_stsPlay == QC_PLAY_Run || m_stsPlay == QC_PLAY_Pause)
    {
        if (m_pAnlMng)
            m_pAnlMng->OnStop(m_bEOS?GetDuration():GetPos());
    }
    
	if (m_pRndVideo != NULL)
		m_pRndVideo->Stop ();
	if (m_pRndAudio != NULL)
		m_pRndAudio->Stop ();

    QCPLAY_STATUS status = m_stsPlay;
    m_stsPlay = QC_PLAY_Stop;
    m_pBaseInst->SetForceClose(false);
	if (status == QC_PLAY_Run || status == QC_PLAY_Pause)
	{
        if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
            m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_STOP, 0, m_bEOS ? GetDuration() : llPos);
		if (m_pBoxMonitor != NULL)
			m_pBoxMonitor->ShowResult ();
	}

	return QC_ERR_NONE;
}

long long COMBoxMng::SetPos (long long llPos)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, (int)llPos);

	if (m_stsPlay <= QC_PLAY_Init)
		return QC_ERR_STATUS;
	if (m_bSeeking || m_bOpening || m_pBaseInst->m_bCheckReopn)
        return QC_ERR_STATUS;
    if (m_llDur <= 0)
        return QC_ERR_STATUS;
    if (m_pBoxSource->CanSeek () <= 0)
        return QC_ERR_UNSUPPORT;

	llPos = llPos + m_llStartTime;
	if (llPos + 1000 > GetDuration ())
		llPos = GetDuration () - 1000;
	if (llPos < 0)
		llPos = 0;
	if (m_llSeekPos != 0 && llPos != 0)
	{
		if (m_pClock != NULL && m_pClock->GetTime () > m_llSeekPos + 1000)
			m_llSeekPos = 0;
//		if (abs ((int)(m_llSeekPos - llPos)) < 1000)
//			return QC_ERR_IMPLEMENT;
	}
	if (qcGetSysTime() - m_nLastSeekTime < 200 && m_nLastSeekTime != 0)
	{
		nRC = QC_ERR_IMPLEMENT;
		return nRC;
	}
    m_pBaseInst->m_nOpenSysTime = qcGetSysTime();
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_SEEK_START, (int)GetPos(), llPos);
	m_nLastSeekTime = qcGetSysTime ();
    if (m_pRndVideo && m_pRndAudio && GetPos() > 0)
    {
        // It need consider BA From pure audio/video To audio/video
		/*
        if (m_pRndAudio->GetRndCount() <= 0)
            m_pRndVideo->SetOtherRender(NULL);
        else
            m_pRndVideo->SetOtherRender(m_pRndAudio);
        if (m_pRndVideo->GetRndCount() <= 0)
            m_pRndAudio->SetOtherRender(NULL);
        else
            m_pRndAudio->SetOtherRender(m_pRndVideo);
		*/
    }
    
    PostAsyncTask(QC_TASK_SEEK, 0, llPos, NULL);
	m_llSeekPos = llPos;
	m_bSeeking = true;
	if (m_stsPlay == QC_PLAY_Pause)
	{
		while (m_bSeeking)
			qcSleepEx (100000, &m_pBaseInst->m_bForceClose);
	}
	return QC_ERR_NONE;
}

int COMBoxMng::DoSeek (const long long llPos, bool bPause)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, (int)llPos);

	CAutoLock lock(&m_mtFunc);
	if (m_pBoxSource == NULL)
		return QC_ERR_STATUS;
	if (m_stsPlay < QC_PLAY_Open)
		return QC_ERR_STATUS;

	if (m_stsPlay == QC_PLAY_Open)
	{
		nRC = m_pBoxSource->SetPos(llPos);
        // it need pass llPos to render to handle QCPLAY_PID_Seek_Mode == 1
        if (m_pRndAudio != NULL)
            m_pRndAudio->SetPos(llPos);
        if (m_pRndVideo != NULL)
            m_pRndVideo->SetPos(llPos);
		if (nRC < 0)
			return QC_ERR_FAILED;
		else
			return QC_ERR_NONE;
	}
	if (m_stsPlay == QC_PLAY_Run)
	{
		if (m_pRndVideo != NULL)
			m_pRndVideo->Pause ();
		if (m_pRndAudio != NULL)
			m_pRndAudio->Pause ();
	}

	long long llPlayPos = 0;
	if (m_pRndAudio != NULL)
	{
		llPlayPos = m_pRndAudio->GetRndTime() - m_llStartTime;
		m_pRndAudio->SetPos(llPos);
	}
	if (m_pRndVideo != NULL)
	{
		llPlayPos = m_pRndVideo->GetRndTime() - m_llStartTime;
		m_pRndVideo->SetPos(llPos);
	}
	if (m_pClock != NULL)
		m_pClock->SetTime (llPos);

	if (m_pBoxSource->SetPos (llPos) >= 0)
		nRC = QC_ERR_NONE;
	else
		nRC = QC_ERR_FAILED;

	QCLOGI ("Set Pos % 8lld", llPos);

    m_bFirstFrameRendered = false;
	int nSourceType = m_pBoxSource->GetParam(QCIO_PID_SourceType, (void *)&llPos);
	if (m_fNotifyEvent != NULL)
		m_fNotifyEvent(m_pUserData, QC_MSG_IO_SEEK_SOURCE_TYPE, &nSourceType);

	int		nStartTime = qcGetSysTime ();
	if (m_stsPlay == QC_PLAY_Run && !bPause)
	{
		if (nRC != QC_ERR_NONE)
		{
			m_llSeekPos = llPlayPos;
			if (m_pRndAudio != NULL)
				m_pRndAudio->SetPos(m_llSeekPos);
			if (m_pRndVideo != NULL)
				m_pRndVideo->SetPos(m_llSeekPos);
			// m_pBoxSource->SetPos(llPlayPos);
		}

		if (m_pRndAudio != NULL)
			m_pRndAudio->Start ();
		if (m_pRndVideo != NULL)
			m_pRndVideo->Start ();
		if (nRC != QC_ERR_NONE)
			return nRC;

		bool bSeekDone = false;
		int nSeekStartTime = qcGetSysTime();
		while (!bSeekDone)
		{
			if (m_pRndAudio != NULL)
			{
				if (m_pRndAudio->GetRndCount() > 0 || m_pRndAudio->IsEOS())
					bSeekDone = true;
			}
			if (!bSeekDone && m_pRndVideo != NULL)
			{
				if (m_pRndVideo->GetRndCount() > 0 || m_pRndVideo->IsEOS())
					bSeekDone = true;
			}
			qcSleep(10000);
			if (m_pBaseInst->m_bForceClose)
				return QC_ERR_STATUS;
			if (qcGetSysTime() - nSeekStartTime > 10000)
			{
				//m_pBoxSource->SetPos(llPlayPos);
				return QC_ERR_FAILED;
			}
		}
	}
	else if (m_stsPlay == QC_PLAY_Pause)
	{
		if (m_pRndVideo != NULL && m_pRndVideo->GetType () == OMB_TYPE_RENDER)
		{
			m_pBoxSource->Start ();
			m_pBoxSource->Pause ();
		}
	}
	return nRC;
}

long long COMBoxMng::GetPos (void)
{
	if (m_bOpening)
		return 0;

	if (m_stsPlay == QC_PLAY_Run || m_stsPlay == QC_PLAY_Pause)
	{
		if (m_bSeeking)
			return m_llSeekPos;
		long long llPos = m_llSeekPos;
		if (m_pBaseInst->m_llFAudioTime != m_pBaseInst->m_llFVideoTime)
		{
			if (m_pClock != NULL)
				llPos = m_pClock->GetTime();
		}
		else
		{
			if (m_pRndAudio != NULL)
				llPos = m_pRndAudio->GetRndTime() - m_llStartTime;
			if (m_pRndVideo != NULL && (m_pRndVideo->GetRndTime() - m_llStartTime) > llPos)
				llPos = m_pRndVideo->GetRndTime() - m_llStartTime;
		}
		if (m_pBaseInst->m_pSetting->g_qcs_nPlaybackLoop > 0 && m_llDur > 0)
			llPos = llPos % m_llDur;
		if (m_llSeekPos == 0)
			return llPos;
//        if (llPos < 1000)
//            return m_llSeekPos;
		return llPos;
		// return QC_MAX (llPos, m_llSeekPos); 
	}

	return 0;
}

long long COMBoxMng::GetDuration (void)
{
	if (m_bOpening || m_bSeeking || m_pBaseInst->m_bCheckReopn)
		return m_llDur;

//	CAutoLock lock (&m_mtFunc);
	long long llDur = 0;
	if (m_pBoxSource != NULL)
		llDur = m_pBoxSource->GetDuration ();
	if (llDur <= 0 && m_llDur > 0)
		llDur = m_llDur;

	return llDur;
}

int COMBoxMng::SetVolume (int nVolume)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, nVolume);
	if (m_pBaseInst != NULL)
		m_pBaseInst->m_pSetting->g_qcs_nAudioVolume = nVolume;
	return QC_ERR_NONE;
}

int COMBoxMng::GetVolume (void)
{
	if (m_pBaseInst == NULL)
		return 100;
	return m_pBaseInst->m_pSetting->g_qcs_nAudioVolume;
}

int COMBoxMng::GetBoxCount (void)
{
	return m_lstBox.GetCount ();
}

CBoxBase * COMBoxMng::GetBox (int nIndex)
{
	CAutoLock lock (&m_mtFunc);
	if (nIndex < 0 || nIndex >= m_lstBox.GetCount ())
		return NULL;

	CBoxBase *	pBox = NULL;
	int			nListIndex = 0;
	NODEPOS pos = m_lstBox.GetHeadPosition ();
	while (pos != NULL)
	{
		pBox = m_lstBox.GetNext (pos);
		if (nIndex == nListIndex)
			break;
		nListIndex++;
	}

	return pBox;
}

int COMBoxMng::SetParam (int nID, void * pParam)
{
	if (nID == QCPLAY_PID_EXT_SOURCE_DATA || nID == QCPLAY_PID_EXT_VIDEO_CODEC || nID == QCPLAY_PID_EXT_AUDIO_CODEC)
	{
		if (nID == QCPLAY_PID_EXT_VIDEO_CODEC)
			m_pBaseInst->m_nVideoCodecID = *((QCCodecID *)pParam);
		else if (nID == QCPLAY_PID_EXT_AUDIO_CODEC)
			m_pBaseInst->m_nAudioCodecID = *((QCCodecID *)pParam);
		if (m_pBoxSource == NULL)
			return QC_ERR_RETRY;
		return m_pBoxSource->SetParam(nID, pParam);
	}

	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, nID);

	switch (nID)
	{
	case QCPLAY_PID_Log_Level:
		if (pParam == NULL)
			return QC_ERR_ARG;
		g_nLogOutLevel = *(int *)pParam;
        if (g_nLogOutLevel == 5)
            QCLOGI("SDK version %d.%d.%d.%d, %s %s", (GetSDKVersion()>>24) & 0xFF, (GetSDKVersion()>>16) & 0xFF, (GetSDKVersion()>>8) & 0xFF, GetSDKVersion()&0xFF, __TIME__,  __DATE__);
        QCLOGI("Log level %d", g_nLogOutLevel);
		return QC_ERR_NONE;

	case QCPLAY_PID_Socket_ConnectTimeout:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (*(int *)pParam <= 0)
			return QC_ERR_ARG;
		m_pBaseInst->m_pSetting->g_qcs_nTimeOutConnect = *(int *)pParam;
		return QC_ERR_NONE;

	case QCPLAY_PID_Socket_ReadTimeout:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (*(int *)pParam <= 0)
			return QC_ERR_ARG;
		m_pBaseInst->m_pSetting->g_qcs_nTimeOutRead = *(int *)pParam;
		return QC_ERR_NONE;

	case QCPLAY_PID_PlayBuff_MaxTime:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (*(int *)pParam <= 0)
			return QC_ERR_ARG;
		m_pBaseInst->m_pSetting->g_qcs_nMaxPlayBuffTime = *(int *)pParam;
		return QC_ERR_NONE;

	case QCPLAY_PID_PlayBuff_MinTime:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (*(int *)pParam <= 0)
			return QC_ERR_ARG;
		m_pBaseInst->m_pSetting->g_qcs_nMinPlayBuffTime = *(int *)pParam;
		return QC_ERR_NONE;

	case QCPLAY_PID_ADD_Cache:
		if (pParam == NULL)
			return QC_ERR_ARG;
		PostAsyncTask(QC_TASK_ADDCACHE, 0, 0, (const char *)pParam);
		return QC_ERR_NONE;

	case QCPLAY_PID_DEL_Cache:
		PostAsyncTask(QC_TASK_DELCACHE, 0, 0, (const char *)pParam);
		return QC_ERR_NONE;

	case QCPLAY_PID_ADD_IOCache:
		if (pParam == NULL)
			return QC_ERR_ARG;
		PostAsyncTask(QC_TASK_ADDIOCACHE, 0, 0, (const char *)pParam);
		return QC_ERR_NONE;

	case QCPLAY_PID_DEL_IOCache:
		PostAsyncTask(QC_TASK_DELIOCACHE, 0, 0, (const char *)pParam);
		return QC_ERR_NONE;

	case QCPLAY_PID_IOCache_Size:
		if (pParam != NULL)
			m_pBaseInst->m_pSetting->g_qcs_nIOCacheDownSize = *((int *)pParam);
		return QC_ERR_NONE;

	case QCPLAY_PID_HTTP_HeadReferer:
		if (pParam == NULL)
			strcpy(m_pBaseInst->m_pSetting->g_qcs_szHttpHeadReferer, "");
		else
			strcpy(m_pBaseInst->m_pSetting->g_qcs_szHttpHeadReferer, (char *)pParam);
		return QC_ERR_NONE;
    case QCPLAY_PID_HTTP_HeadUserAgent:
        if (pParam == NULL)
            strcpy(m_pBaseInst->m_pSetting->g_qcs_szHttpHeadUserAgent, "");
        else
            strcpy(m_pBaseInst->m_pSetting->g_qcs_szHttpHeadUserAgent, (char *)pParam);
        return QC_ERR_NONE;

	case QCPLAY_PID_DNS_SERVER:
		if (pParam == NULL)
			strcpy(m_pBaseInst->m_pSetting->g_qcs_szDNSServerName, "");
		else
			strcpy(m_pBaseInst->m_pSetting->g_qcs_szDNSServerName, (char *)pParam);
		if (m_pBaseInst->m_pDNSLookup != NULL)
			m_pBaseInst->m_pDNSLookup->UpdateDNSServer();
        if (m_pAnlMng)
            m_pAnlMng->UpdateDNSServer((char *)pParam);
		return QC_ERR_NONE;

	case QCPLAY_PID_Prefer_Format:
		if (pParam == NULL)
			return QC_ERR_ARG;
		m_pBaseInst->m_pSetting->g_qcs_nPerferFileForamt = *(int *)pParam;
		return QC_ERR_NONE;

	case QCPLAY_PID_Prefer_Protocol:
		if (pParam == NULL)
			return QC_ERR_ARG;
		m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol = *(int *)pParam;
		return QC_ERR_NONE;

	case QCPLAY_PID_PD_Save_Path:
		if (pParam == NULL)
			return QC_ERR_ARG;
		strcpy(m_pBaseInst->m_pSetting->g_qcs_szPDFileCachePath, (char *)pParam);
		nRC = qcCreateFolder((char *)pParam);
		return QC_ERR_NONE;

	case QCPLAY_PID_PD_Save_ExtName:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (strlen((char *)pParam) > 16)
			return QC_ERR_ARG;
		strcpy(m_pBaseInst->m_pSetting->g_qcs_szPDFileCacheExtName, (char *)pParam);
		return QC_ERR_NONE;
    
    case QCPLAY_PID_START_MUX_FILE:
        if (pParam == NULL)
            return QC_ERR_ARG;

		if (m_pBaseInst->m_pMuxer == NULL)
			m_pBaseInst->m_pMuxer = new CQCMuxer(m_pBaseInst, NULL);
		else
			m_pBaseInst->m_pMuxer->Uninit();
        m_pBaseInst->m_pMuxer->Init(QC_PARSER_MP4);
        m_pBaseInst->m_pMuxer->Open((char*)pParam);
        m_pBaseInst->m_bBeginMux = true;
        return QC_ERR_NONE;

	case QCPLAY_PID_STOP_MUX_FILE:
		if (m_pBaseInst->m_pMuxer)
		{
			int nParam = *(int *)pParam;
			if (nParam == 0)
				m_pBaseInst->m_pMuxer->Close();
			else if (nParam == 1)
				m_pBaseInst->m_pMuxer->Pause();
			else if (nParam == 2)
				m_pBaseInst->m_pMuxer->Restart();
			else
				m_pBaseInst->m_pMuxer->Close();
		}
        return QC_ERR_NONE;

	case QCPLAY_PID_DNS_DETECT:
		if (pParam == NULL)
			return QC_ERR_ARG;
		else
		{
			PostAsyncTask(QC_TASK_IPDETECT, 0, 0, (const char *)pParam);
			return QC_ERR_NONE;
		}

	case QCPLAY_PID_NET_CHANGED:
		PostAsyncTask(QC_TASK_NETCHANGE, 0, 0, NULL);
		return QC_ERR_NONE;
            
    case QCPLAY_PID_Disable_Video:
    {
        QCLOGI("Video enable flag: %d", *((int*)pParam));
        m_nDisVideoLevel = *((int*)pParam);
        if (m_pRndVideo != NULL)
            nRC = m_pRndVideo->DisableVideo (m_nDisVideoLevel);
        return QC_ERR_NONE;
    }

	case QCPLAY_PID_RTSP_UDPTCP_MODE:
		if (pParam == NULL)
			return QC_ERR_ARG;
		m_pBaseInst->m_pSetting->g_qcs_nRTSP_UDP_TCP_Mode = *(int*)pParam;
		return QC_ERR_NONE;

	case QCPLAY_PID_Playback_Loop:
		if (pParam == NULL)
			return QC_ERR_ARG;
		m_pBaseInst->m_pSetting->g_qcs_nPlaybackLoop = *(int*)pParam;
		return QC_ERR_NONE;

	case QCPLAY_PID_MP4_PRELOAD:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (*(int *)pParam <= 5000)
			return QC_ERR_ARG;
		//m_pBaseInst->m_pSetting->g_qcs_nMoovPreLoadItem = *(int*)pParam * 30 / 1000;
		m_nPreloadTimeSet = *(int *)pParam;
		return QC_ERR_NONE;

	case QCPLAY_PID_Clock_OffTime:
		if (m_pClock != NULL)
			nRC = m_pClock->SetOffset(*((int*)pParam));
		return QC_ERR_NONE;

	case QCPLAY_PID_Download_Pause:
		if (pParam != NULL)
			m_pBaseInst->m_nDownloadPause = *(int *)pParam;
		return QC_ERR_NONE;

	case QCPLAY_PID_START_POS:
		if (pParam != NULL)
			m_llStartPos = *(long long *)pParam;
		return QC_ERR_NONE;

#ifdef __QC_OS_WIN32__
	case QCPLAY_PID_EXT_AITracking:
		if (pParam == NULL)
			return QC_ERR_ARG;
		m_pBaseInst->m_pSetting->g_qcs_nExtAITracking = *(int*)pParam;
		if (m_pBaseInst->m_pSetting->g_qcs_nExtAITracking > 0)
		{
			if (m_pBaseInst->m_pTrackMng == NULL)
				m_pBaseInst->m_pTrackMng = new CTrackMng(m_pBaseInst);
		}
		else
		{
			QC_DEL_P(m_pBaseInst->m_pTrackMng);
		}
		return QC_ERR_NONE;
#endif // __QC_OS_WIN32__
	default:
		break;
	}

    if (m_bOpening || m_bSeeking || m_pBaseInst->m_bCheckReopn)
    {
        nRC = QC_ERR_STATUS;
        return nRC;
    }

	CAutoLock lock(&m_mtFunc);
	QCPLAY_STATUS stsOld = m_stsPlay;

	switch (nID)
	{
	case QCPLAY_PID_StreamPlay:
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		nRC = m_pBoxSource->SetStreamPlay (QC_MEDIA_Source, *((int*)pParam));
		return nRC;
	case QCPLAY_PID_AudioTrackPlay:
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		nRC = m_pBoxSource->SetStreamPlay (QC_MEDIA_Audio, *((int*)pParam));
		return nRC;
	case QCPLAY_PID_VideoTrackPlay:
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		nRC = m_pBoxSource->SetStreamPlay (QC_MEDIA_Video, *((int*)pParam));
		return nRC;
	case QCPLAY_PID_SubttTrackPlay:
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		nRC = m_pBoxSource->SetStreamPlay (QC_MEDIA_Subtt, *((int*)pParam));
		return nRC;
	case QCPLAY_PID_EXT_AudioRnd:
		m_pExtRndAudio = (CBaseAudioRnd * )pParam;
		nRC = m_pRndAudio == NULL ? QC_ERR_NONE : QC_ERR_STATUS;
		return nRC;
	case QCPLAY_PID_EXT_VideoRnd:
		m_pExtRndVideo = (CBaseVideoRnd * )pParam;
		nRC = m_pExtRndVideo == NULL ? QC_ERR_NONE : QC_ERR_STATUS;
		return nRC;
	case QCPLAY_PID_AspectRatio:
	{
		if (pParam == NULL)
			return QC_ERR_ARG;
		QCPLAY_ARInfo * pAR = (QCPLAY_ARInfo *)pParam;
		if (m_pRndVideo != NULL)
			nRC = m_pRndVideo->SetAspectRatio (pAR->nWidth, pAR->nHeight);
		return QC_ERR_NONE;
	}
	case QCPLAY_PID_Zoom_Video:
	{
		if (m_pBaseInst != NULL)
			nRC = m_pBaseInst->SetSettingParam(QC_BASEINST_EVENT_VIDEOZOOM, 0, pParam);
		return QC_ERR_NONE;
	}

	case QCPLAY_PID_SetWorkPath:
		if (pParam != NULL)
		{
			strcpy (g_szWorkPath, (char *)pParam);
			int nLen = strlen (g_szWorkPath);
			if (nLen > 1 && g_szWorkPath[nLen - 1] != '/' && g_szWorkPath[nLen - 1] != '\\' )
				strcat (g_szWorkPath, "/");
		}
		return QC_ERR_NONE;

	case QCPLAY_PID_Speed:
	{
        if (GetDuration() <= 0)
            return QC_ERR_UNSUPPORT;
		if (pParam == NULL)
			return QC_ERR_ARG;
		double fSpeed = *(double *)pParam;
		if (fSpeed < 0.05)
			fSpeed = 0.05;
		if (fSpeed > 64)
			fSpeed = 64;
		if (m_pRndAudio != NULL)
			nRC = m_pRndAudio->SetSpeed (fSpeed);
		if (m_pClock != NULL)
			m_pClock->SetSpeed (fSpeed);
		return QC_ERR_NONE;
	}

	case QCPLAY_PID_Seek_Mode:
		m_nSeekMode = *((int*)pParam);
		if (m_pRndAudio != NULL)
			m_pRndAudio->SetSeekMode (m_nSeekMode);
		if (m_pRndVideo != NULL)
			m_pRndVideo->SetSeekMode (m_nSeekMode);
		return QC_ERR_NONE;

	case QCPLAY_PID_Reconnect:
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		nRC = m_pBoxSource->SetParam (nID, NULL);
		return nRC;

	case QCPLAY_PID_Capture_Image:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pRndVideo == NULL)
			return QC_ERR_STATUS;
		nRC = m_pRndVideo->CaptureImage(*(long long *)pParam);
		return nRC;

	case QCPLAY_PID_DRM_KeyText:
		if (pParam == NULL)
			return QC_ERR_ARG;
		memcpy(m_szQiniuDrmKey, (char *)pParam, 16);
		if (m_pBoxSource != NULL)
			nRC = m_pBoxSource->SetParam(QC_PARSER_SET_QINIU_DRM_INFO, m_szQiniuDrmKey);
		return QC_ERR_NONE;	

	case QCPLAY_PID_FILE_KeyText:
		if (pParam == NULL)
			return QC_ERR_ARG;
		QCLOGI ("The file key is %s", (char *)pParam);
		strcpy(m_pBaseInst->m_pSetting->g_qcs_pFileKeyText, (char *)pParam);
		return QC_ERR_NONE;

	case QCPLAY_PID_COMP_KeyText:
		if (pParam == NULL)
			return QC_ERR_ARG;
		QCLOGI("The comp key is %s", (char *)pParam);
		strcpy(m_pBaseInst->m_pSetting->g_qcs_pCompKeyText, (char *)pParam);
		return QC_ERR_NONE;

    case QCPLAY_PID_BG_COLOR:
        {
            if (pParam == NULL)
                return QC_ERR_ARG;
            QC_COLOR* pColor = (QC_COLOR*)pParam;
            //m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor = {pColor->fRed, pColor->fGreen, pColor->fBlue, pColor->fAlpha};
			m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor.fRed = pColor->fRed;
			m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor.fGreen = pColor->fGreen;
			m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor.fBlue = pColor->fBlue;
			m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor.fAlpha = pColor->fAlpha;

        }
        return QC_ERR_NONE;

	case QCPLAY_PID_SendOut_VideoBuff:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pRndVideo != NULL)
			m_pRndVideo->SetSendOutData(m_pUserData, (QCPlayerOutAVData)pParam);
		return QC_ERR_NONE;

	case QCPLAY_PID_SendOut_AudioBuff:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pRndAudio != NULL)
			m_pRndAudio->SetSendOutData(m_pUserData, (QCPlayerOutAVData)pParam);
		return QC_ERR_NONE;

    case QCPLAY_PID_SDK_ID:
        {
            if (pParam == NULL)
                strcpy(m_pBaseInst->m_szSDK_ID, "");
            else
                strcpy(m_pBaseInst->m_szSDK_ID, (char*)pParam);
        }
        return QC_ERR_NONE;

	case QCPLAY_PID_Flush_Buffer:
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		m_pBoxSource->SetParam(QCPLAY_PID_Flush_Buffer, 0);
	return QC_ERR_NONE;
/*
	case QCPLAY_PID_SubTitle:
		m_nSubTTEnable = *((int*)pParam));
		if (m_nSubTTEnable > 0)
		{
			if (m_stsPlay < QC_PLAY_Open)
				return QC_ERR_NONE;

			if (m_pSubTT == NULL)
			{
				if (OpenSubTitle () != QC_ERR_NONE)
					return QC_ERR_IMPLEMENT;
				if (m_pSubTT != NULL)
				{
					if (m_stsPlay == QC_PLAY_Run)
						m_pSubTT->Start ();
				}
			}
		}
		if (m_pSubTT != NULL)
			m_pSubTT->Enable (m_nSubTTEnable);

		return QC_ERR_NONE;

	case QCPLAY_PID_SubTT_Size:
		m_nSubTTSize = *((int*)pParam));
		if (m_pSubTT != NULL)
			m_pSubTT->SetFontSize (m_nSubTTSize);
		return QC_ERR_NONE;

	case QCPLAY_PID_SubTT_Color:
		m_nSubTTColor = *((int*)pParam));
		if (m_pSubTT != NULL)
			m_pSubTT->SetFontColor (m_nSubTTColor);
		return QC_ERR_NONE;

	case QCPLAY_PID_SubTT_Font:
		m_hSubTTFont = (HFONT)pParam;
		if (m_pSubTT != NULL)
			m_pSubTT->SetFontHandle (m_hSubTTFont);
		return QC_ERR_NONE;

	case QCPLAY_PID_SubTT_View:
		m_hSubTTView = (HFONT)pParam;
		if (m_pSubTT != NULL)
			m_pSubTT->SetView (m_hSubTTView);
		if (m_pRndVideo != NULL)
		{
			if (m_hSubTTView != NULL)
				m_pRndVideo->SetSubTTEng (NULL);
			else
				m_pRndVideo->SetSubTTEng (m_pSubTT);
		}
		return QC_ERR_NONE;

*/
	default:
		break;
	}

	return QC_ERR_PARAMID;
}

int COMBoxMng::GetParam (int nID, void * pParam)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, nID);

	if (m_bOpening || m_bSeeking || m_pBaseInst->m_bCheckReopn)
		return QC_ERR_STATUS;

	CAutoLock lock(&m_mtFunc);
	switch (nID)
	{
	case QCPLAY_PID_Speed:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pClock != NULL)
			*(double *)pParam = m_pClock->GetSpeed ();
		return QC_ERR_NONE;

	case QCPLAY_PID_StreamNum:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		*(int *)pParam = m_pBoxSource->GetStreamCount (QC_MEDIA_Source);
		return QC_ERR_NONE;
	case QCPLAY_PID_StreamPlay:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		*(int *)pParam = m_pBoxSource->GetStreamPlay (QC_MEDIA_Source);
		return QC_ERR_NONE;
	case QCPLAY_PID_AudioTrackNum:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		*(int *)pParam = m_pBoxSource->GetStreamCount (QC_MEDIA_Audio);
		return QC_ERR_NONE;
	case QCPLAY_PID_AudioTrackPlay:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		*(int *)pParam = m_pBoxSource->GetStreamPlay (QC_MEDIA_Audio);
		return QC_ERR_NONE;
	case QCPLAY_PID_VideoTrackNum:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		*(int *)pParam = m_pBoxSource->GetStreamCount (QC_MEDIA_Video);
		return QC_ERR_NONE;
	case QCPLAY_PID_VideoTrackPlay:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		*(int *)pParam = m_pBoxSource->GetStreamPlay (QC_MEDIA_Video);
		return QC_ERR_NONE;
	case QCPLAY_PID_SubttTrackNum:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		*(int *)pParam = m_pBoxSource->GetStreamCount (QC_MEDIA_Subtt);
		return QC_ERR_NONE;
	case QCPLAY_PID_SubttTrackPlay:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		*(int *)pParam = m_pBoxSource->GetStreamPlay (QC_MEDIA_Subtt);
		return QC_ERR_NONE;
	case QCPLAY_PID_StreamInfo:
	{
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pBoxSource == NULL)
			return QC_ERR_STATUS;
		QC_STREAM_FORMAT * pStreamInfo = (QC_STREAM_FORMAT *)pParam;
		QC_STREAM_FORMAT * pStreamFormat = m_pBoxSource->GetStreamFormat (pStreamInfo->nID);
		if (pStreamFormat != NULL)
		{
			pStreamInfo->nBitrate = pStreamFormat->nBitrate;
			pStreamInfo->nWidth = pStreamFormat->nWidth;
			pStreamInfo->nHeight = pStreamFormat->nHeight;
			pStreamInfo->nLevel = pStreamFormat->nLevel;
		}
		return QC_ERR_NONE;
	}
    case QCPLAY_PID_RTMP_AUDIO_MSG_TIMESTAMP:
    {
        if (pParam == NULL)
            return QC_ERR_ARG;
        if (m_pBoxSource == NULL)
            return QC_ERR_STATUS;
        return m_pBoxSource->GetParam(QCPLAY_PID_RTMP_AUDIO_MSG_TIMESTAMP, pParam);
    }
    case QCPLAY_PID_RTMP_VIDEO_MSG_TIMESTAMP:
    {
        if (pParam == NULL)
            return QC_ERR_ARG;
        if (m_pBoxSource == NULL)
            return QC_ERR_STATUS;
        return m_pBoxSource->GetParam(QCPLAY_PID_RTMP_VIDEO_MSG_TIMESTAMP, pParam);
    }
	case QCPLAY_PID_Download_Pause:
		if (pParam != NULL)
			*(int *)pParam = m_pBaseInst->m_nDownloadPause;
		return m_pBaseInst->m_nDownloadPause;

/*
	case QCPLAY_PID_SubTitle:
		if (m_pSubTT == NULL)
			*(int *)pParam = 0;
		else
			*(int *)pParam = m_nSubTTEnable;
		return QC_ERR_NONE;

	case QCPLAY_PID_SubTT_Size:
		*(int *)pParam = m_nSubTTSize;
		return QC_ERR_NONE;

	case QCPLAY_PID_SubTT_Font:
		if (m_hSubTTFont != NULL)	
			*(void **)pParam = m_hSubTTFont;
		else if (m_pSubTT != NULL)
			*(void **)pParam = m_pSubTT->GetInFont ();
		return QC_ERR_NONE;

	case QCPLAY_PID_SubTT_View:
		*(void **)pParam = m_hSubTTView;
		return QC_ERR_NONE;

	case QCPLAY_PID_SubTT_Color:
		*(int *)pParam = m_nSubTTColor;
		return QC_ERR_NONE;
*/		
	default:
		break;
	}
	return QC_ERR_PARAMID;
}

int COMBoxMng::WaitAudioRender (int nWaitTime, bool bCheckStatus)
{
	if (m_pRndAudio == NULL)
		return QC_ERR_NONE;

	int nStartTime = qcGetSysTime ();
	while (m_pRndAudio->GetRndCount () <= 0)
	{
		if (bCheckStatus)
		{
			if (m_stsPlay != QC_PLAY_Run)
				break;
		}
		if (m_pRndAudio->IsEOS ())
			break;
		qcSleep (5000);
	}
	
	return QC_ERR_NONE;
}

int COMBoxMng::OnHandleEvent (CThreadEvent * pEvent)
{
	if (pEvent == NULL)
		return QC_ERR_ARG;	

	int nRC = QC_ERR_NONE;
	if (pEvent->m_nID == QC_TASK_CHECK)
	{
		nRC = DoCheckStatus();
		return nRC;
	}
	else if (pEvent->m_nID == QC_TASK_NETCHANGE)
	{
		if (m_pBaseInst != NULL)
			m_pBaseInst->NotifyNetChanged();
		return QC_ERR_NONE;
	}
	else if (pEvent->m_nID == QC_TASK_IPDETECT)
	{
		if (m_pBaseInst->m_pDNSCache == NULL)
			return QC_ERR_STATUS;
		return m_pBaseInst->m_pDNSCache->DetectHost((char *)pEvent->m_pName);
	}

	QCLOGI ("The Event ID = %X", pEvent->m_nID);
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, pEvent->m_nID);

	switch (pEvent->m_nID)
	{
	case QC_TASK_OPEN:
	{
		CAutoLock lock(&m_mtFunc);
        if(m_pBaseInst->m_bForceClose)
        {
            QCLOGW("Cancel open");
            m_bOpening = false;
            break;
        }

		if ((pEvent->m_nValue & QCPLAY_OPEN_SAME_SOURCE) == QCPLAY_OPEN_SAME_SOURCE)
			nRC = DoFastOpen(pEvent->m_pName, pEvent->m_nValue);
		else
			nRC = DoOpen(pEvent->m_pName, pEvent->m_nValue);
		if (nRC == QC_ERR_VIDEO_HWDEC)
		{
			m_bOpening = false;
			break;
		}

		if (m_llStartPos > 0 && nRC == QC_ERR_NONE)
		{
            if (m_pBoxSource)
            {
                if (m_llStartPos + 1000 > m_pBoxSource->GetDuration ())
                    m_llStartPos = m_pBoxSource->GetDuration () - 1000;
            }
            if (m_llStartPos < 0)
                m_llStartPos = 0;
			m_llStartTime = 0;
			DoSeek(m_llStartPos, true);
			m_llSeekPos = m_llStartPos;
		}
		m_llStartPos = 0;

		m_bOpening = false;

		if (nRC == QC_ERR_NONE && m_pBoxSource && m_pBoxSource->GetMediaSource())
			m_nParserFormat = m_pBoxSource->GetMediaSource()->GetParserFormat();
		else
			m_nParserFormat = QC_PARSER_NONE;
		m_llReopenPos = 0;
        
        if(m_pAnlMng)
            m_pAnlMng->OnOpenDone(nRC, m_llDur);

		if (m_fNotifyEvent != NULL && (m_pBaseInst && !m_pBaseInst->m_bForceClose))
		{
			m_fNotifyEvent(m_pUserData, nRC == QC_ERR_NONE ? QC_MSG_PLAY_OPEN_DONE : QC_MSG_PLAY_OPEN_FAILED, &nRC);
			QCLOGI("Open %s result = 0X%08X", pEvent->m_pName, nRC);
		}
		break;
	}

	case QC_TASK_SEEK:
	{
		CAutoLock lock(&m_mtFunc);
		nRC = DoSeek(pEvent->m_llValue, false);
		m_bSeeking = false;
		m_bEOS = false;
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(nRC == QC_ERR_NONE ? QC_MSG_PLAY_SEEK_DONE : QC_MSG_PLAY_SEEK_FAILED, 0, 0);
		break;
	}

	case QC_TASK_REOPEN:
		nRC = DoReopen();
		break;

	case QC_TASK_ADDCACHE:
	case QC_TASK_ADDIOCACHE:
		strcpy(m_szCacheURL, pEvent->m_pName);
		if (m_pBoxSource == NULL)
			m_pBoxSource = new CBoxSource(m_pBaseInst, m_hInst);
		nRC = m_pBoxSource->AddCache(pEvent->m_pName, pEvent->m_nID == QC_TASK_ADDIOCACHE);
		strcpy(m_szCacheURL, "");
		if (m_fNotifyEvent != NULL && (m_pBaseInst && !m_pBaseInst->m_bForceClose))
		{
			m_fNotifyEvent(m_pUserData, nRC == QC_ERR_NONE ? QC_MSG_PLAY_CACHE_DONE : QC_MSG_PLAY_CACHE_FAILED, pEvent->m_pName);
			QCLOGI("Add cache %s. Result = 0X%08X", pEvent->m_pName, nRC);
		}
		break;

	case QC_TASK_DELCACHE:
	case QC_TASK_DELIOCACHE:
		if (m_pBoxSource == NULL || pEvent == NULL)
			return QC_ERR_STATUS;
		if (pEvent->m_pName == NULL)
		{
			if (m_pThreadWork != NULL)
				m_pThreadWork->ResetEventByID(pEvent->m_nID);
			m_pBoxSource->CancelCache();
		}
		else if (m_pThreadWork != NULL)
		{
			CThreadEvent * pNewEvent = new CThreadEvent(QC_TASK_ADDCACHE, 0, 0, pEvent->m_pName);
			m_pThreadWork->RemoveEvent(pNewEvent);
			delete pNewEvent;
		}
		m_pBoxSource->DelCache(pEvent->m_pName);
		break;

	default:
		break;
	}

	return nRC;
}

int COMBoxMng::ReceiveMsg (CMsgItem * pItem)
{
	if (pItem == NULL)
		return QC_ERR_ARG;

	if (m_bExit)
		return QC_ERR_STATUS;

	if (pItem->m_nMsgID == QC_MSG_SNKA_RENDER || pItem->m_nMsgID == QC_MSG_SNKV_RENDER)
	{
		long long llTime = pItem->m_llValue;
		if (m_pBaseInst->m_pSetting->g_qcs_nPlaybackLoop > 0 && m_llDur > 0)
			llTime = llTime % m_llDur;
		if (m_fNotifyEvent != NULL)
			m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, &llTime);
		return QC_ERR_NONE;
	}
    else if (pItem->m_nMsgID == QC_MSG_BUFF_SEI_DATA)
    {
        if (m_fNotifyEvent != NULL)
            m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, pItem->m_pInfo);
        return QC_ERR_NONE;
	}
	else if (pItem->m_nMsgID == QC_MSG_SNKV_ROTATE)
	{
		if ((m_nOpenFlag & QCPLAY_OPEN_VIDDEC_HW) == QCPLAY_OPEN_VIDDEC_HW)
		{
			if (m_fNotifyEvent != NULL)
				m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, &pItem->m_nValue);
		}
		return QC_ERR_NONE;
	}
    else if (pItem->m_nMsgID == QC_MSG_HTTP_CONTENT_SIZE || pItem->m_nMsgID == QC_MSG_HTTP_BUFFER_SIZE)
    {
        if (m_fNotifyEvent != NULL)
            m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, &pItem->m_llValue);
        return QC_ERR_NONE;
	}
	else if (pItem->m_nMsgID == QC_MSG_BUFF_VBUFFTIME || pItem->m_nMsgID == QC_MSG_BUFF_ABUFFTIME)
	{
		if (m_fNotifyEvent != NULL)
			m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, &pItem->m_nValue);
		return QC_ERR_NONE;
	}
	else if (pItem->m_nMsgID == QC_MSG_RENDER_VIDEO_FPS || pItem->m_nMsgID == QC_MSG_RENDER_AUDIO_FPS)
	{
		if (m_fNotifyEvent != NULL)
			m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, &pItem->m_nValue);
		return QC_ERR_NONE;
	}
	else if (pItem->m_nMsgID == QC_MSG_RTMP_METADATA)
	{
		if (m_fNotifyEvent != NULL)
			m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, pItem->m_szValue);
		return QC_ERR_NONE;
	}
	else if (pItem->m_nMsgID == QC_MSG_SNKV_NEW_FORMAT)
	{
#ifdef __QC_OS_IOS__
        if(m_pExtRndVideo)
        {
            QC_VIDEO_FORMAT* pFmt = m_pExtRndVideo->GetFormat();
            if (m_fNotifyEvent != NULL && pFmt)
                m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, pFmt);
            return QC_ERR_NONE;
        }
#endif
		if (m_pRndVideo)
		{
			QC_VIDEO_FORMAT* pFmt = m_pRndVideo->GetVideoFormat();
			if (m_fNotifyEvent != NULL && pFmt)
				m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, pFmt);
		}
		return QC_ERR_NONE;
	}
	else if (pItem->m_nMsgID == QC_MSG_SNKA_NEW_FORMAT)
	{
		if (m_pRndAudio)
		{
			QC_AUDIO_FORMAT* pFmt = m_pRndAudio->GetAudioFormat();
			if (m_fNotifyEvent != NULL && pFmt)
				m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, pFmt);
		}
		return QC_ERR_NONE;
	}
	else if (pItem->m_nMsgID == QC_MSG_PLAY_DURATION)
	{
		m_llDur = pItem->m_llValue;
	}
    else if (pItem->m_nMsgID == QC_MSG_BUFF_START_BUFFERING)
    {
        if (!m_bFirstFrameRendered)
            return QC_ERR_NONE;
    }
    else if (pItem->m_nMsgID == QC_MSG_BUFF_END_BUFFERING)
    {
        if (!m_bFirstFrameRendered)
            return QC_ERR_NONE;
    }
	else if (pItem->m_nMsgID == QC_MSG_HTTP_DNS_GET_CACHE || pItem->m_nMsgID == QC_MSG_HTTP_DNS_GET_IPADDR)
    {
		if (pItem->m_szValue != NULL && !m_bSendIPAddr)
        {
			m_bSendIPAddr = true;
            QC_DATA_BUFF bufInfo;
            bufInfo.llTime = pItem->m_nTime;
            bufInfo.uBuffType = QC_BUFF_TYPE_Text;
            bufInfo.pBuffPtr = pItem->m_szValue;
            if (m_fNotifyEvent != NULL)
                m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, &bufInfo);
        }
        return QC_ERR_NONE;
    }
    else if (pItem->m_nMsgID == QC_MSG_HTTP_CONTENT_TYPE)
    {
        if (m_fNotifyEvent != NULL)
            m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, pItem->m_szValue);
        return QC_ERR_NONE;
    }
    else if (pItem->m_nMsgID == QC_MSG_HTTP_GET_HEADDATA
             || pItem->m_nMsgID == QC_MSG_HTTP_CONNECT_START
             || pItem->m_nMsgID == QC_MSG_HTTP_CONNECT_SUCESS
             || pItem->m_nMsgID == QC_MSG_IO_FIRST_BYTE_DONE
             || pItem->m_nMsgID == QC_MSG_IO_HANDSHAKE_START
             || pItem->m_nMsgID == QC_MSG_IO_HANDSHAKE_FAILED
             || pItem->m_nMsgID == QC_MSG_IO_HANDSHAKE_SUCESS
             || pItem->m_nMsgID == QC_MSG_HTTP_DNS_START)
    {
        if (m_fNotifyEvent != NULL)
            m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, &pItem->m_nTime);
        return QC_ERR_NONE;
    }
    else if (pItem->m_nMsgID == QC_MSG_RTMP_DISCONNECTED)
    {
        if (m_stsPlay == QC_PLAY_Run)
        {
            int nRndCount = 0;
            if (m_pRndVideo != NULL)
                nRndCount = m_pRndVideo->GetRndCount();
            if (m_pRndAudio != NULL)
                nRndCount += m_pRndAudio->GetRndCount();
            m_nTotalRndCount = nRndCount;
        }
    }


	if (pItem->m_nMsgID != QC_MSG_SNKA_EOS && pItem->m_nMsgID != QC_MSG_SNKV_EOS &&
		pItem->m_nMsgID != QC_MSG_SNKA_FIRST_FRAME && pItem->m_nMsgID != QC_MSG_SNKA_FIRST_FRAME && 
		pItem->m_nMsgID != QC_MSG_PLAY_CAPTURE_IMAGE && pItem->m_nMsgID != QC_MSG_HTTP_DISCONNECTED)
	{
		if (m_fNotifyEvent != NULL)
			m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, &pItem->m_nValue);
		return QC_ERR_NONE;
	}

	if (m_bOpening || m_bSeeking || m_pBaseInst->m_bCheckReopn)
		return QC_ERR_STATUS;
	CAutoLock lock(&m_mtFunc);
	int nRC = QC_ERR_NONE;
//	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, pItem->m_nMsgID);
	if (pItem->m_nMsgID == QC_MSG_SNKA_EOS || pItem->m_nMsgID == QC_MSG_SNKV_EOS)
	{
		bool bFinish = false;
		if (m_pRndAudio != NULL && m_pRndVideo != NULL)
		{
			if (m_pRndAudio->IsEOS () && m_pRndVideo->IsEOS ())
			{
				bFinish = true;
			}
		}
		else if (m_pRndAudio != NULL && m_pRndAudio->IsEOS ())
		{
				bFinish = true;
		}
		else if (m_pRndVideo != NULL && m_pRndVideo->IsEOS ())
		{
				bFinish = true;
		}
		if (bFinish)
		{
			QCLOGI("Play complete!");
            if (m_pAnlMng)
                m_pAnlMng->OnEOS();
			m_bEOS = true;
			if (m_pBaseInst->m_pSetting->g_qcs_nPlaybackLoop > 0)
			{
				DoSeek(0, false);
				return QC_ERR_NONE;
			}

			if (m_pURL != NULL && (strstr(m_pURL, "http") != m_pURL))
				DoSeek(0, true);

			if (m_fNotifyEvent != NULL)
			{
				int nStatus = 0;
				if (m_pBaseInst->m_pSetting->g_qcs_bIOReadError)
					nStatus = 1;
				m_fNotifyEvent(m_pUserData, QC_MSG_PLAY_COMPLETE, &nStatus);
			}
		}
		return QC_ERR_NONE;
	}
	else if (pItem->m_nMsgID == QC_MSG_SNKA_FIRST_FRAME || pItem->m_nMsgID == QC_MSG_SNKV_FIRST_FRAME)
	{
		if (m_pClock != NULL && m_llStartTime == -1 && pItem->m_llValue > 300 && pItem->m_llValue > m_llSeekPos)
			m_llStartTime = pItem->m_llValue - m_llSeekPos;
		if (m_pBaseInst->m_llFAudioTime != m_pBaseInst->m_llFVideoTime)
			m_llStartTime = 0;
        m_bFirstFrameRendered = true;
	}
	else if (pItem->m_nMsgID == QC_MSG_PLAY_CAPTURE_IMAGE)
	{
		if (m_fNotifyEvent != NULL)
			m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, pItem->m_pInfo);
		return QC_ERR_NONE;
	}
	else if (pItem->m_nMsgID == QC_MSG_HTTP_DISCONNECTED)
	{
//		if (m_nParserFormat == QC_PARSER_M3U8)
//		{
//			if (m_llDur > 0)
//				m_llReopenPos = GetPos();
//			PostAsyncTask(QC_TASK_REOPEN, 0, 0, NULL, 5000);
//		}
	}

	if (m_fNotifyEvent != NULL)
		m_fNotifyEvent(m_pUserData, pItem->m_nMsgID, &pItem->m_nValue);

	return QC_ERR_NONE;
}

void COMBoxMng::UpdateAnal()
{
    if(qcIsEnableAnalysis())
    {
        if(!m_pAnlMng)
        {
            m_pAnlMng = new CAnalysisMng(m_pBaseInst, this);
        }
    }
    else
    {
        if(m_pAnlMng)
        {
            QC_DEL_P(m_pAnlMng);
        }
    }
}

int COMBoxMng::GetSDKVersion()
{
    // 1.1.0.79
    return 0x0101004F;
}
