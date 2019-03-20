/*******************************************************************************
	File:		CBoxVDecRnd.cpp

	Contains:	The video render box implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-04		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"
#include "qcMsg.h"
#include "qcPlayer.h"

#include "CBoxVDecRnd.h"
#include "CBoxMonitor.h"

#include "CQCVDecRnd.h"
#include "CMsgMng.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

CBoxVDecRnd::CBoxVDecRnd(CBaseInst * pBaseInst, void * hInst)
	: CBoxRender (pBaseInst, hInst)
	, m_nARW (1)
	, m_nARH (1)
	, m_nDisableRnd (0)
	, m_pRnd (NULL)
	, m_bNewPos (false)
	, m_llNewTime (0)
	, m_nLastClock (0)	
{
	SetObjectName ("CBoxVDecRnd");
	m_nMediaType = QC_MEDIA_Video;
	m_nBoxType = OMB_TYPE_RENDER;
	strcpy (m_szBoxName, "Video DecRnd Box");

	memset (&m_fmtVideo, 0, sizeof (m_fmtVideo));
}

CBoxVDecRnd::~CBoxVDecRnd(void)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	Stop();
	if (m_pExtRnd == NULL)
	{
		if (m_pRnd != NULL)
			m_pRnd->Uninit();
		QC_DEL_P(m_pRnd);
	}
	QC_DEL_A (m_fmtVideo.pHeadData);
}

int	CBoxVDecRnd::SetView(void * hView, RECT * pRect)
{
	if (m_pRnd != NULL && m_pExtRnd == NULL)
		m_pRnd->SetView(hView, pRect);
	return QC_ERR_NONE;
}

int CBoxVDecRnd::SetAspectRatio (int w, int h)
{
	CAutoLock lock (&m_mtRnd);
	if (m_nARW == w && m_nARH == h)
		return QC_ERR_NONE;
	m_nARW = w;
	m_nARH = h;
	return QC_ERR_NONE;
}

int CBoxVDecRnd::DisableVideo (int nFlag)
{
	if (m_nDisableRnd == nFlag)
		return QC_ERR_NONE;
	m_nDisableRnd = nFlag;
	return QC_ERR_NONE;
}

int CBoxVDecRnd::SetSource (CBoxBase * pSource)
{
	CAutoLock lock (&m_mtRnd);
	if (pSource == NULL)
	{
		m_pBoxSource = NULL;
		return QC_ERR_ARG;
	}

#ifdef __QC_OS_NDK__
    m_pRnd = (CBaseVideoRnd *)m_pExtRnd;
#else
	m_pRnd = new CQCVDecRnd(m_pBaseInst, NULL);
#endif // __QC_OS_NDK__
    if (m_pRnd == NULL)
        return QC_ERR_MEMORY;
    
    CBoxBase::SetSource (pSource);
	QC_VIDEO_FORMAT * pFmt = pSource->GetVideoFormat ();
	if (pFmt == NULL)
		return QC_ERR_FORMAT;
	m_fmtVideo.nWidth = pFmt->nWidth;
	m_fmtVideo.nHeight = pFmt->nHeight;

	m_pRnd->SetAspectRatio (m_nARW, m_nARH);
	return m_pRnd->Init (pFmt);
}

long long CBoxVDecRnd::SetPos (long long llPos)
{
	m_llSeekPos = llPos;
	m_bEOS = false;
	m_nRndCount = 0;
	return llPos;
}

void CBoxVDecRnd::SetNewSource (bool bNewSource)
{
	m_llSeekPos = 0;
	m_bNewSource = bNewSource;
	if (m_pRnd != NULL)
		m_pRnd->SetNewSource (m_bNewSource);
}

int CBoxVDecRnd::GetRndCount (void)
{
	if (m_pRnd == NULL)
		return m_nRndCount;
	return m_pRnd->GetRndCount ();
}

int CBoxVDecRnd::Start (void)
{
	if (m_pRnd != NULL)
		m_pRnd->Start ();
	return CBoxRender::Start ();
}

int CBoxVDecRnd::Pause (void)
{
	int nRC = CBoxRender::Pause ();
	if (m_pRnd != NULL)
		m_pRnd->Pause ();
	return nRC;
}

int CBoxVDecRnd::Stop (void)
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

int CBoxVDecRnd::OnWorkItem (void)
{
	int nRC  = QC_ERR_NONE;
	if (m_pBoxSource == NULL || m_bEOS)
	{
		qcSleep (5000);
		return QC_ERR_STATUS;
	}
	if (m_bNewPos && m_pOtherRnd != NULL && m_pClock != NULL)
	{
		//QCLOGI ("Last Clock: % 8d,   Clock: % 8lld    NewTime: % 8lld", m_nLastClock, m_pClock->GetTime (), m_llNewTime);
		if (m_pClock->GetTime () >= m_nLastClock)
		{
			m_nLastClock = m_pClock->GetTime ();
			if (m_pClock->GetTime () > (int)m_llNewTime + 5000)
			{
				qcSleep (5000);
				return QC_ERR_RETRY;
			}
		}
		m_bNewPos = false;
	}

	CAutoLock lock (&m_mtRnd);
	m_pBuffInfo->nMediaType = QC_MEDIA_Video;
	m_pBuffInfo->uFlag = 0;
	m_pBuffInfo->llDelay = 0;
	m_pBuffInfo->llTime = 0;
	if (m_pClock != NULL && GetRndCount ()  > 1)
		m_pBuffInfo->llTime = m_pClock->GetTime ();
	m_pBuffData = NULL;

	nRC = m_pBoxSource->ReadBuff (m_pBuffInfo, &m_pBuffData, true);
	if (m_pBuffData == NULL && nRC != QC_ERR_FINISH)
		return nRC;

	if (m_pBuffData && ((m_pBuffData->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS))
	{
		m_bNewPos = true;
		m_llNewTime = m_pBuffData->llTime;
		m_nLastClock = m_pClock->GetTime ();	
		m_nRndCount = 0;
		m_bNewSource = false;
	}
	BOX_READ_BUFFER_REC_VIDEORND
	
	if (nRC == QC_ERR_FINISH || (m_pBuffData != NULL && (m_pBuffData->uFlag & QCBUFF_EOS) == QCBUFF_EOS))
	{		
		// nofity message eos
		if (!m_bEOS	&& m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKV_EOS, 0, 0);
		m_bEOS = true;
	}
	if (nRC == QC_ERR_RETRY)
	{
		if (m_nRndCount == 0)
			qcSleep (2000);
		return nRC;	
	}
	if (nRC != QC_ERR_NONE || m_pBuffData == NULL)
		return nRC;
	
	m_pBuffInfo->llTime = m_pBuffData->llTime;

	if (m_nDisableRnd == QC_PLAY_VideoEnable)
		nRC = m_pRnd->Render (m_pBuffData);
	if (m_pRnd->GetRndCount () <= 0)
		return QC_ERR_RETRY;
	if (m_pOtherRnd == NULL)
	{
		if (m_nRndCount == 0)
		{
			m_pClock->Start();
			m_pClock->SetTime(m_pBuffData->llTime == 0 ? 1 : m_pBuffData->llTime);
		}
	}
	if (m_nRndCount == 0 && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKV_FIRST_FRAME, 0, m_pBuffData->llTime);
	m_nRndCount++;
	m_pBaseInst->m_nVideoRndCount = m_nRndCount;
	m_llLastRndTime = m_pBuffData->llTime;
	if (m_nRndCount <= 1)
		WaitOtherRndFirstFrame ();

	return QC_ERR_NONE;
}

int CBoxVDecRnd::OnStartFunc (void)
{
	m_llStartTime = qcGetSysTime ();
	if (m_pRnd != NULL)
	{
		m_pRnd->OnStart ();
		m_pRnd->SetClock (m_pClock);
	}
	return QC_ERR_NONE;
}

int CBoxVDecRnd::OnStopFunc (void)
{
	if (m_pRnd != NULL)
		m_pRnd->OnStop ();
	return QC_ERR_NONE;
}
