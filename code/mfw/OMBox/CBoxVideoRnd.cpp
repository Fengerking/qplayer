/*******************************************************************************
	File:		CBoxVideoRnd.cpp

	Contains:	The video render box implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-13		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"
#include "qcMsg.h"
#include "qcPlayer.h"

#ifdef __QC_OS_WIN32__
#include "CDDrawRnd.h"
#include "CGDIRnd.h"
#include "CTrackMng.h"
#elif defined __QC_OS_IOS__
#include "CVideoRndFactory.h"
#endif // __QC_OS_WIN32__

#include "CBoxVideoRnd.h"
#include "CBoxMonitor.h"
#include "CMsgMng.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

CBoxVideoRnd::CBoxVideoRnd(CBaseInst * pBaseInst, void * hInst)
	: CBoxRender (pBaseInst, hInst)
	, m_hView (NULL)
	, m_nARW (1)
	, m_nARH (1)
	, m_nVideoWidth(0)
	, m_nVideoHeight(0)
	, m_bZoom(false)
	, m_bRotate(false)
	, m_nZoomLeft(0)
	, m_nZoomTop(0)
	, m_nZoomWidth(0)
	, m_nZoomHeight(0)
	, m_nRotateAngle(0)
	, m_pRnd (NULL)
	, m_nDisableRnd (0)
	, m_llDelayTime (0)
	, m_llVideoTime (0)
	, m_llLastFTime (0)
	, m_nDroppedFrames (0)
{
	SetObjectName ("CBoxVideoRnd");
	m_nMediaType = QC_MEDIA_Video;
	m_nBoxType = OMB_TYPE_RENDER;
	strcpy (m_szBoxName, "Video Render Box");

	memset (&m_rcView, 0, sizeof (m_rcView));
	memset (&m_fmtVideo, 0, sizeof (m_fmtVideo));

	if (m_pBaseInst != NULL)
		m_pBaseInst->AddListener(this);
}

CBoxVideoRnd::~CBoxVideoRnd(void)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	if (m_pBaseInst != NULL)
		m_pBaseInst->RemListener(this);

	Stop();
	if (m_pRnd != NULL)
		m_pRnd->Uninit ();
	if (m_pExtRnd == NULL)
		QC_DEL_P (m_pRnd);

	QC_DEL_A (m_fmtVideo.pHeadData);
}

int CBoxVideoRnd::SetView (void * hView, RECT * pRect)
{
    int nRet = QC_ERR_NONE;
    if (hView != m_hView)
    {
        CAutoLock lock (&m_mtRnd);
        nRet = SetViewInterval(hView, pRect);
    }
    else
        nRet = SetViewInterval(hView, pRect);
    
    return nRet;
}

int CBoxVideoRnd::SetViewInterval (void * hView, RECT * pRect)
{
    m_hView = hView;
    if (pRect != NULL)
        memcpy (&m_rcView, pRect, sizeof (m_rcView));
    else
    {
#ifdef __QC_OS_WIN32__
        GetClientRect ((HWND)hView, &m_rcView);
#endif // __QC_OS_WIN32__
    }
    if (m_pRnd != NULL)
        m_pRnd->SetView (m_hView, &m_rcView);

    return QC_ERR_NONE;
}

int CBoxVideoRnd::SetAspectRatio (int w, int h)
{
	CAutoLock lock (&m_mtRnd);
	if (m_nARW == w && m_nARH == h)
		return QC_ERR_NONE;
	m_nARW = w;
	m_nARH = h;
	if (m_pRnd != NULL)
		m_pRnd->SetAspectRatio (m_nARW, m_nARH);
	return QC_ERR_NONE;
}

int CBoxVideoRnd::DisableVideo (int nFlag)
{
	if (m_nDisableRnd == nFlag)
		return QC_ERR_NONE;
	m_nDisableRnd = nFlag;
    if (m_pRnd)
        m_pRnd->SetParam(QCPLAY_PID_Disable_Video, &nFlag);
	return QC_ERR_NONE;
}

int CBoxVideoRnd::SetSource (CBoxBase * pSource)
{
	CAutoLock lock (&m_mtRnd);
	if (pSource == NULL)
	{
		m_pBoxSource = NULL;
		ResetMembers ();
		return QC_ERR_ARG;
	}
	CBoxBase::SetSource (pSource);
	QC_VIDEO_FORMAT * pFmt = pSource->GetVideoFormat ();
	if (pFmt == NULL)
		return QC_ERR_FORMAT;
	m_nVideoWidth = pFmt->nWidth;
	m_nVideoHeight = pFmt->nHeight;
	m_fmtVideo.nWidth = pFmt->nWidth;
	m_fmtVideo.nHeight = pFmt->nHeight;
	m_fmtVideo.nNum = pFmt->nNum;
	m_fmtVideo.nDen = pFmt->nDen;
	UpdateVideoFormat();
	m_bZoom = false;
	m_bRotate = false;

	if (m_pExtRnd == NULL)
	{
		QC_DEL_P (m_pRnd);
#ifdef __QC_OS_WIN32__
		if (m_pBaseInst->m_pSetting->g_qcs_bRndTypeIsGDI)
			m_pRnd = new CGDIRnd(m_pBaseInst, m_hInst);
		else
			m_pRnd = new CDDrawRnd(m_pBaseInst, m_hInst);
#elif defined __QC_OS_IOS__
        m_pRnd = CVideoRndFactory::Create(m_pBaseInst);
#endif // __QC_OS_WIN32__
	}
	else
	{
		m_pRnd = (CBaseVideoRnd *)m_pExtRnd;
	}
	if (m_pRnd == NULL)
		return QC_ERR_MEMORY;

	m_pRnd->SetView (m_hView, &m_rcView);
	m_pRnd->SetAspectRatio (m_nARW, m_nARH);
	int nRC = m_pRnd->Init (&m_fmtVideo);
	if (nRC != QC_ERR_NONE)
		nRC = CreateGDIRnd();

	//m_nSourceType = GetParam (BOX_GET_SourceType, NULL);

	return nRC;
}

int	CBoxVideoRnd::Start (void)
{
    if (m_pRnd != NULL)
        m_pRnd->Start ();
	int nRC = CBoxRender::Start ();
	return nRC;
}

int CBoxVideoRnd::Pause (void)
{
	int nRC = CBoxRender::Pause ();
	if (m_pRnd != NULL)
		m_pRnd->Pause ();
	return nRC;
}

int	CBoxVideoRnd::Stop (void)
{
	if (m_pRnd != NULL)
		m_pRnd->Stop ();
	int nRC = CBoxRender::Stop ();
	if (m_llStartTime > 0)
	{
		int nUsedTime = (int)(qcGetSysTime () - m_llStartTime) / 1000;
		if (nUsedTime == 0)
			nUsedTime = 1;
		QCLOGI ("Video Render num: % 8d - % 8d", m_nRndCount, (m_nRndCount * 100) / nUsedTime);
		m_llStartTime = 0;
        m_nLastSysTime = 0;
	}
	return nRC;
}

CBaseClock * CBoxVideoRnd::GetClock (void)
{
	CAutoLock lock (&m_mtRnd);
	if (m_pRnd != NULL)
		return m_pRnd->GetClock ();
	return NULL;
}

RECT * CBoxVideoRnd::GetRenderRect (void )
{
	if (m_pRnd != NULL)
		return m_pRnd->GetRenderRect ();
	return NULL;
}

long long CBoxVideoRnd::SetPos (long long llPos)
{
	CAutoLock lock (&m_mtRnd);

	m_nDroppedFrames = 0;
	m_llVideoTime = 0;
	m_llDelayTime = 0;

	return CBoxRender::SetPos (llPos);
}

void CBoxVideoRnd::SetNewSource(bool bNewSource)
{
	CBoxRender::SetNewSource(bNewSource);
	m_llVideoTime = 0;
}

int CBoxVideoRnd::GetRndCount (void)
{
	if (m_pRnd == NULL || m_nRndCount == 0)
		return m_nRndCount;
	return m_pRnd->GetRndCount ();
}

int CBoxVideoRnd::OnWorkItem (void)
{
	int nRC  = QC_ERR_NONE;
	if (m_pBoxSource == NULL || m_bEOS || m_pBaseInst->m_bForceClose)
	{
		qcSleep (5000);
		return QC_ERR_STATUS;
	}

	if (m_llStartTime == 0)
		m_llStartTime = qcGetSysTime ();
    if (m_nLastSysTime == 0)
        m_nLastSysTime = qcGetSysTime ();

	// unlock the rnd locker.
    if (m_pClock != NULL && m_llVideoTime > m_pClock->GetTime())
		qcSleep(2000);

	CAutoLock lock (&m_mtRnd);
	m_pBuffInfo->nMediaType = QC_MEDIA_Video;
	m_pBuffInfo->uFlag = 0;
	if (m_nDroppedFrames > 0)
	{
		m_pBuffInfo->uFlag = QCBUFF_DEC_DISA_DEBLOCK;
		if (m_llDelayTime > 0 && m_pClock->GetTime () - m_llVideoTime < m_llDelayTime)
			m_pBuffInfo->uFlag += QCBUFF_DEC_SKIP_BFRAME;
	}
	m_llDelayTime = m_pClock->GetTime () - m_llVideoTime;
	if (m_llVideoTime == 0 || m_nRndCount <= 1)
		m_llDelayTime = 0;
	m_pBuffInfo->llDelay = m_llDelayTime;
	m_pBuffInfo->llTime = 0;	
	if (m_pClock != NULL && GetRndCount() > 50 && !m_bNewSource)
		m_pBuffInfo->llTime = m_pClock->GetTime ();

	m_pBuffData = NULL;

	nRC = m_pBoxSource->ReadBuff (m_pBuffInfo, &m_pBuffData, false);
	if (nRC == QC_ERR_BUFFERING)
		m_bBuffering = true;
	if (m_pBuffData == NULL && nRC != QC_ERR_FINISH)
	{
		qcSleep(2000);
		return nRC;
	}
	if (m_bBuffering)
	{
		m_bBuffering = false;
		if (m_pBuffData != NULL)
			m_pClock->SetTime(m_pBuffData->llTime);
	}

	if (m_pBuffData != NULL && ((m_pBuffData->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS))
	{
		m_nRndCount = 0;
		m_bNewSource = false;
		m_llVideoTime = m_pBuffData->llTime;
	}

	if (m_nSeekMode > 0)
	{
		if (m_pBuffData != NULL && m_pBuffData->llTime < m_llSeekPos)
			m_llVideoTime = 0;
	}
    if (m_pBuffData && m_pThreadWork->GetStatus() == QCWORK_Run && nRC == QC_ERR_NONE)
    {
        if (m_pClock != NULL && m_pClock->IsPaused())
        {
            m_pClock->Start();
            m_pClock->SetTime(m_pBuffData->llTime);
        }
    }

	if (m_nRndCount > 0 && m_pBuffData)
		WaitRenderTime (m_pBuffData->llTime);
	//QCLOGI ("Video Time % 8d, clock % 8d, System % 8d AVOffset: % 8d", (int)m_llVideoTime, (int)m_pClock->GetTime (), (int)(qcGetSysTime () - m_llStartTime), (int)(m_llVideoTime - m_pClock->GetTime ()));
	// QCLOGI ("Video Time: % 8lld, Step % 8d  Clock % 8d============", m_pBuffData->llTime, (int)(m_pBuffData->llTime - m_llVideoTime), m_pClock->GetTime ());

	BOX_READ_BUFFER_REC_VIDEORND
	
	if (nRC == QC_ERR_FINISH || (m_pBuffData != NULL && (m_pBuffData->uFlag & QCBUFF_EOS) == QCBUFF_EOS))
	{		
		m_bEOS = true;
		// nofity message eos
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKV_EOS, 0, 0);
	}
	if (nRC == QC_ERR_RETRY)
	{
		if (m_nRndCount == 0)
			qcSleep (1000);
		return nRC;	
	}
	if (nRC != QC_ERR_NONE || m_pBuffData == NULL)
		return nRC;
    // source type maybe changed on same soure mode
//    if (m_nSourceType == -1)
        m_nSourceType = GetParam (BOX_GET_SourceType, NULL);

	// for live source 
	if (m_pBuffData->llTime == 0 && m_nRndCount >= 1 && m_pBuffData->uFlag == 0)
		return QC_ERR_RETRY;
	
	m_pBuffInfo->llTime = m_pBuffData->llTime;
	if ((m_pBuffData->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)m_pBuffData->pFormat;
		if (pFmt == NULL)
			pFmt = m_pBoxSource->GetVideoFormat ();
		if (pFmt != NULL)
		{
			m_fmtVideo.nWidth = pFmt->nWidth;
			m_fmtVideo.nHeight = pFmt->nHeight;
			m_nVideoWidth = pFmt->nWidth;
			m_nVideoHeight = pFmt->nHeight;
			UpdateVideoFormat();
			if (m_nRndCount > 0 && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKV_NEW_FORMAT, m_nVideoWidth, m_nVideoHeight);
		}
	}
    if (m_pBuffData->pFormat == NULL)
        m_pBuffData->pFormat = &m_fmtVideo;
    
    if (m_nSeekMode > 0 && m_pBuffData->llTime < m_llSeekPos)
    {
        m_bDropFrame = true;
        if ((m_pBuffData->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
        {
            QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)m_pBuffData->pFormat;
            if (pFmt == NULL)
                pFmt = m_pBoxSource->GetVideoFormat();
            if (pFmt != NULL && m_pRnd != NULL)
                m_pRnd->Init(pFmt);
        }
        //QCLOGI("Skip video %lld, pos %lld", m_pBuffData->llTime, m_llSeekPos);
        return QC_ERR_NONE;
    }
    m_bDropFrame = false;

	if (m_nRndCount == 0 && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKV_NEW_FORMAT, m_nVideoWidth, m_nVideoHeight);

	QC_DATA_BUFF * pRndBuff = UpdateVideoData(m_pBuffData);

	m_pBuffData->nValue = 0;
	if (m_fSendOutData != NULL)
	{
		pRndBuff->nMediaType = QC_MEDIA_Video;
		pRndBuff->pData = NULL;
		pRndBuff->nDataType = 0;
		pRndBuff->nValue = 0;
		m_fSendOutData(m_pUserData, pRndBuff);
		if (pRndBuff->pData != NULL && pRndBuff->nDataType == QC_BUFF_TYPE_Video)
		{
			memcpy(&m_bufOutData, pRndBuff, sizeof(QC_DATA_BUFF));
			m_bufOutData.pBuffPtr = pRndBuff->pData;
			pRndBuff = &m_bufOutData;
		}
	}
#ifdef __QC_OS_WIN32__
	if (m_pBaseInst->m_pTrackMng != NULL)
	{
		m_pBaseInst->m_pTrackMng->SendVideoBuff(pRndBuff);
		if (pRndBuff->pData != NULL && pRndBuff->nDataType == QC_BUFF_TYPE_Video)
		{
			memcpy(&m_bufOutData, pRndBuff, sizeof(QC_DATA_BUFF));
			m_bufOutData.pBuffPtr = pRndBuff->pData;
			pRndBuff = &m_bufOutData;
		}
	}
#endif // __QC_OS_WIN32__

	if (!m_bEOS && m_nDisableRnd == QC_PLAY_VideoEnable && !m_bNewSource)
	{
		if (m_pRnd != NULL && m_pBuffData->nValue != 11) // 11 is render out
		{
			if (m_bZoom || m_bRotate)
			{
				m_bZoom = false;
				m_bRotate = false;
				pRndBuff->uFlag |= QCBUFF_NEW_FORMAT;
			}
			nRC = m_pRnd->Render(pRndBuff);
			if (nRC != QC_ERR_NONE)
			{
				nRC = CreateGDIRnd();
				if (nRC == QC_ERR_NONE)
					nRC = m_pRnd->Render(pRndBuff);
			}
		}
	}
	if (m_pOtherRnd == NULL && 	m_nSourceType > 0)
	{
		if (m_nRndCount == 0 || abs ((int)(m_pBuffData->llTime - m_llVideoTime)) > 1000)
			m_pClock->SetTime (m_pBuffData->llTime == 0 ? 1 : m_pBuffData->llTime);

		int nMinTime = m_pBaseInst->m_pSetting->g_qcs_nMinPlayBuffTime;
		int nMaxTime = m_pBaseInst->m_pSetting->g_qcs_nMaxPlayBuffTime;
		int nBuffTime = GetParam(BOX_GET_VideoBuffTime, NULL);
		if (nBuffTime < nMinTime)
			m_pClock->SetSpeed(0.9);
		else if (nBuffTime > nMaxTime)
			m_pClock->SetSpeed(1.1);
		else
			m_pClock->SetSpeed(1.0);
	}
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKV_RENDER, 0, m_pBuffData->llTime);
	//QCLOGI ("Video Time: % 8lld, Step %d ============", m_pBuffData->llTime, (int)(m_pBuffData->llTime - m_llVideoTime));
	if (m_nRndCount == 0)
	{
		QCLOGI("The first video render time is % 8d", qcGetSysTime() - m_pBaseInst->m_nOpenSysTime);
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKV_FIRST_FRAME, qcGetSysTime() - m_pBaseInst->m_nOpenSysTime, m_pBuffData->llTime);
	}
	m_nRndCount++;
	m_pBaseInst->m_nVideoRndCount = m_nRndCount;
	m_llLastRndTime = m_pBuffData->llTime;
	if (m_llCaptureTime >= 0)
	{
		if (m_nRotateAngle == 0)
			CaptureVideoImage(&m_fmtVideo, (QC_VIDEO_BUFF *)m_pBuffData->pBuffPtr);
		else if (m_pRnd != NULL)
			CaptureVideoImage(&m_fmtVideo, m_pRnd->GetRotateData ());
	}
	if (m_nRndCount <= 1)
	{
		if (m_pBaseInst->m_llFAudioTime <= m_pBaseInst->m_llFVideoTime && m_pOtherRnd != NULL)
			WaitOtherRndFirstFrame();
        // Clock time need be updated by video on both seek and startup status, especially for pure video
		if (m_pOtherRnd != NULL /*&& m_pOtherRnd->IsEOS()*/ && m_pOtherRnd->GetRndCount () == 0)
		{
			if (/*m_llSeekPos > 0 && */m_pClock != NULL)
				m_pClock->SetTime(m_pBuffData->llTime);
		}
	}
	if (abs ((int)(m_pBuffData->llTime - m_pClock->GetTime())) > 500)
		QCLOGW ("The av sync Time is % 8d at time: % 8lld clock = % 8lld count: % 8d", (int)(m_pBuffData->llTime - m_pClock->GetTime()), m_pBuffData->llTime, m_pClock->GetTime(), m_nRndCount);
	if (m_llLastFTime >0 && qcGetSysTime() - m_llLastFTime > 200)
		QCLOGI ("The delay Time is % 8d at time: %lld  count: %d", (int)(qcGetSysTime() - m_llLastFTime), m_pBuffData->llTime, m_nRndCount);
    int interval = (int)(m_pBuffData->llTime-m_llVideoTime)/2;
    if (qcGetSysTime () - m_llLastFTime < interval && interval < 50)
        qcSleep (interval);
	m_llLastFTime = qcGetSysTime ();
    m_llVideoTime = m_pBuffData->llTime;
    
    if(qcGetSysTime()-m_nLastSysTime > 10000)
    {
        int nTime = (qcGetSysTime()-m_nLastSysTime) / 1000;
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RENDER_VIDEO_FPS, (m_nRndCount - m_nLastRndCount) / nTime, 0);
        m_nLastRndCount = m_nRndCount;
        m_nLastSysTime = qcGetSysTime();
    }

	return QC_ERR_NONE;
}

int CBoxVideoRnd::OnStopFunc(void)
{
	if (m_pRnd != NULL)
		m_pRnd->OnStop();
	return QC_ERR_NONE;
}

int CBoxVideoRnd::WaitRenderTime (long long llTime)
{
	if (m_bEOS || m_pClock == NULL)
		return QC_ERR_NONE;

	while (m_pClock->GetTime () < llTime)
	{
		int nOffset = (int)(llTime - m_pClock->GetTime ());
//		QCLOGI("Video Time: % 8lld,  Clock: % 8lld    Offset:  % 8d  m_bNewSource = %d", llTime, m_pClock->GetTime(), nOffset, m_bNewSource);
		if (m_pThreadWork->GetStatus () != QCWORK_Run)
			break;
		if (m_bNewSource)
			break;
		if (m_pBaseInst->m_bForceClose || m_pBaseInst->m_bCheckReopn)
			break;
//        if (abs (nOffset) >= 3000 && m_pClock->GetTime () != 0)
//        {
//            qcSleep (30000);
//            return QC_ERR_NONE;
//        }
		qcSleep (2000);

		if (m_llCaptureTime >= 0)
		{
			if (m_nRotateAngle == 0)
				CaptureVideoImage(&m_fmtVideo, (QC_VIDEO_BUFF *)m_pBuffData->pBuffPtr);
			else if (m_pRnd != NULL)
				CaptureVideoImage(&m_fmtVideo, m_pRnd->GetRotateData());
		}
	}

	return QC_ERR_NONE;
}

void CBoxVideoRnd::ResetMembers (void)
{
	m_llSeekPos = 0;
	m_nRndCount = 0;
	m_llDelayTime = 0;
}

int	CBoxVideoRnd::CreateGDIRnd(void)
{
#ifdef __QC_OS_WIN32__
	int nRC = QC_ERR_NONE;
	if (m_pBaseInst->m_pSetting->g_qcs_bRndTypeIsGDI)
		return QC_ERR_FAILED;

	if (m_pRnd != NULL)
		m_pRnd->Uninit ();
	if (m_pExtRnd == NULL)
		QC_DEL_P(m_pRnd);

	m_pRnd = new CGDIRnd(m_pBaseInst, m_hInst);
	if (m_pRnd == NULL)
		return QC_ERR_MEMORY;

	m_pRnd->SetView(m_hView, &m_rcView);
	m_pRnd->SetAspectRatio(m_nARW, m_nARH);
	nRC = m_pRnd->Init(&m_fmtVideo);
	m_pBaseInst->m_pSetting->g_qcs_bRndTypeIsGDI = true;
	return nRC;
#else
	return QC_ERR_FAILED;
#endif // __QC_OS_WIN32__
}

int	CBoxVideoRnd::UpdateVideoFormat(void)
{
	CAutoLock lock(&m_mtRnd);
	m_bufVideo.nWidth = m_fmtVideo.nWidth;
	m_bufVideo.nHeight = m_fmtVideo.nHeight;

    m_nZoomLeft = m_pBaseInst->m_pSetting->g_qcs_nVideoZoomLeft;
    m_nZoomTop = m_pBaseInst->m_pSetting->g_qcs_nVideoZoomTop;
	m_nZoomWidth = m_pBaseInst->m_pSetting->g_qcs_nVideoZoomWidth;
	m_nZoomHeight = m_pBaseInst->m_pSetting->g_qcs_nVideoZoomHeight;

	if (m_pBaseInst->m_pSetting->g_qcs_nVideoZoomWidth != 0 && m_pBaseInst->m_pSetting->g_qcs_nVideoZoomHeight != 0)
	{
		if (m_pBaseInst->m_pSetting->g_qcs_nVideoZoomWidth + m_pBaseInst->m_pSetting->g_qcs_nVideoZoomLeft < m_nVideoWidth)
			m_fmtVideo.nWidth = m_pBaseInst->m_pSetting->g_qcs_nVideoZoomWidth;
		else
			m_fmtVideo.nWidth = m_nVideoWidth - m_pBaseInst->m_pSetting->g_qcs_nVideoZoomLeft;

		if (m_pBaseInst->m_pSetting->g_qcs_nVideoZoomHeight + m_pBaseInst->m_pSetting->g_qcs_nVideoZoomTop < m_nVideoHeight)
			m_fmtVideo.nHeight = m_pBaseInst->m_pSetting->g_qcs_nVideoZoomHeight;
		else
			m_fmtVideo.nHeight = m_nVideoHeight - m_pBaseInst->m_pSetting->g_qcs_nVideoZoomTop;

		m_bufVideo.nWidth = m_fmtVideo.nWidth;
		m_bufVideo.nHeight = m_fmtVideo.nHeight;

		m_bZoom = true;
	}
    else
    {
        m_fmtVideo.nWidth = m_nVideoWidth;
        m_fmtVideo.nHeight = m_nVideoHeight;
        m_bufVideo.nWidth = m_nVideoWidth;
        m_bufVideo.nHeight = m_nVideoHeight;
    }

	m_nRotateAngle = m_pBaseInst->m_pSetting->g_qcs_nVideoRotateAngle;
	if (m_pBaseInst->m_pSetting->g_qcs_nVideoRotateAngle == 90 ||
		m_pBaseInst->m_pSetting->g_qcs_nVideoRotateAngle == 270)
	{
		int nWidth = m_fmtVideo.nWidth;
		m_fmtVideo.nWidth = m_fmtVideo.nHeight;
		m_fmtVideo.nHeight = nWidth;

		m_bRotate = true;
	}
	else if (m_pBaseInst->m_pSetting->g_qcs_nVideoRotateAngle == 180)
	{
		m_bRotate = true;
	}

	return QC_ERR_NONE;
}

QC_DATA_BUFF * CBoxVideoRnd::UpdateVideoData(QC_DATA_BUFF * pSourceData)
{
	if (pSourceData == NULL)
		return NULL;
	if (pSourceData->uBuffType != QC_BUFF_TYPE_Video)
		return pSourceData;

	QC_DATA_BUFF *  pDataBuff = pSourceData;
	QC_VIDEO_BUFF * pVideoBuff = (QC_VIDEO_BUFF *)pSourceData->pBuffPtr;
	if (m_nZoomWidth != 0 && m_nZoomHeight != 0)
	{
		if (m_pRnd != NULL)
			pVideoBuff = m_pRnd->ConvertYUVData(pSourceData);
		if (pVideoBuff == NULL)
			return pSourceData;
		if (pVideoBuff->nType != QC_VDT_YUV420_P)
			return pSourceData;

		memcpy(&m_bufData, pSourceData, sizeof(QC_DATA_BUFF));
		m_bufData.pFormat = &m_fmtVideo;
		m_bufData.pBuffPtr = &m_bufVideo;
		m_bufVideo.nType = QC_VDT_YUV420_P;

		m_bufVideo.nStride[0] = pVideoBuff->nStride[0];
		m_bufVideo.nStride[1] = pVideoBuff->nStride[1];
		m_bufVideo.nStride[2] = pVideoBuff->nStride[2];

		int nZoomLeft = m_nZoomLeft;
		int nZoomTop = m_nZoomTop;
		m_bufVideo.pBuff[0] = pVideoBuff->pBuff[0] + m_bufVideo.nStride[0] * nZoomTop + nZoomLeft;
		m_bufVideo.pBuff[1] = pVideoBuff->pBuff[1] + m_bufVideo.nStride[1] * nZoomTop / 2 + nZoomLeft / 2;
		m_bufVideo.pBuff[2] = pVideoBuff->pBuff[2] + m_bufVideo.nStride[2] * nZoomTop / 2 + nZoomLeft / 2;

		pVideoBuff = &m_bufVideo;
		pDataBuff = &m_bufData;
	}

	if (m_pRnd != NULL && m_nRotateAngle != 0)
	{
		if (pVideoBuff != &m_bufVideo)
		{
			memcpy(&m_bufData, pSourceData, sizeof(QC_DATA_BUFF));
			m_bufData.pFormat = &m_fmtVideo;
		}
		m_bufData.pBuffPtr = m_pRnd->RotateYUVData(pVideoBuff, m_nRotateAngle);
		pDataBuff = &m_bufData;
	}

	return pDataBuff;
}

int	CBoxVideoRnd::RecvEvent(int nEventID)
{
	if (nEventID == QC_BASEINST_EVENT_VIDEOZOOM)
	{
		CAutoLock lock(&m_mtRnd);
		UpdateVideoFormat();
		m_bZoom = true;
	}

	return QC_ERR_NONE;
}

int CBoxVideoRnd::CaptureImage(long long llTime)
{
    int nRet = CBoxRender::CaptureImage(llTime);
    
    if (m_nStatus == OMB_STATUS_PAUSE)
    {
        CAutoLock lock(&m_mtRnd);
		if (m_nRotateAngle == 0)
			nRet = CaptureVideoImage(&m_fmtVideo, (QC_VIDEO_BUFF *)m_pBuffData->pBuffPtr);
		else if (m_pRnd != NULL)
			nRet = CaptureVideoImage(&m_fmtVideo, m_pRnd->GetRotateData());
	}
    
    return nRet;
}
