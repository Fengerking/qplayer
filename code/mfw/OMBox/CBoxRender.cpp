/*******************************************************************************
	File:		CBoxRender.cpp

	Contains:	The base render box implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-13		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "CBoxRender.h"
#include "CMsgMng.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

CBoxRender::CBoxRender(CBaseInst * pBaseInst, void * hInst)
	: CBoxBase (pBaseInst, hInst)
	, m_nMediaType (QC_MEDIA_Data)
	, m_pOtherRnd (NULL)
	, m_pDataClock (NULL)
	, m_nSeekMode (0)
	, m_pExtRnd (NULL)
	, m_nRndCount (0)
	, m_bInRender (false)
	, m_bBuffering(false)
	, m_pThreadWork (NULL)
	, m_llStartTime (0)
	, m_nSourceType (-1)
	, m_bNewSource (false)
	, m_llCaptureTime (-1)
	, m_pVideoEnc (NULL)
	, m_pVideoBuf (NULL)
	, m_pUserData (NULL)
	, m_fSendOutData (NULL)
	, m_llLastRndTime (0)
	, m_nPauseTime (0)
	, m_nLastRndCount (0)
	, m_nLastSysTime (0)
	, m_bDropFrame(false)
{
	SetObjectName ("CBoxRender");
	m_nBoxType = OMB_TYPE_RENDER;
	strcpy (m_szBoxName, "Base Render Box");
}

CBoxRender::~CBoxRender(void)
{
	Stop ();
	QC_DEL_P (m_pDataClock);
	QC_DEL_P (m_pVideoEnc);
	QC_DEL_P (m_pVideoBuf);
}

int CBoxRender::SetOtherRender (CBoxRender * pRnd)
{
	CAutoLock lock(&m_mtRnd);

	m_pOtherRnd = pRnd;
	return QC_ERR_NONE;
}

int	CBoxRender::Start (void)
{
	if (m_pBoxSource == NULL)
	{
		m_bEOS = true;
		return QC_ERR_STATUS;
	}

    UpdateStartTime();
	int nRC = CBoxBase::Start ();

	m_bEOS = false;
	if (m_pThreadWork == NULL)
	{
		m_pThreadWork = new CThreadWork(m_pBaseInst);
		m_pThreadWork->SetOwner (m_szObjName);
		m_pThreadWork->SetWorkProc (this, &CThreadFunc::OnWork);
		m_pThreadWork->SetStartStopFunc (&CThreadFunc::OnStart, &CThreadFunc::OnStop);
	}
	m_pThreadWork->Start ();

	return nRC;
}

int CBoxRender::Pause (void)
{
	if (m_pBoxSource == NULL)
		return QC_ERR_STATUS;

    if(m_nPauseTime == 0)
        m_nPauseTime = qcGetSysTime();
	if (m_pThreadWork != NULL)
		m_pThreadWork->Pause ();
	WaitForExitRender (500);

	if (m_pDataClock != NULL)
		m_pDataClock->Pause ();

	return CBoxBase::Pause ();
}

int	CBoxRender::Stop (void)
{
	if (m_pBoxSource == NULL)
		return QC_ERR_NONE;

    UpdateStartTime();
	int nRC = CBoxBase::Stop ();

	if (m_pThreadWork != NULL)
		m_pThreadWork->Stop ();
	WaitForExitRender (5000);
	QC_DEL_P (m_pThreadWork);
    m_bDropFrame = false;

	return nRC;
}

long long CBoxRender::SetPos (long long llPos)
{
	long long llRC = CBoxBase::SetPos (llPos);
	if (llRC == 0)
		m_nRndCount = 0;
	m_bNewSource = false;
	m_llLastRndTime = llPos;
	return llRC;
}

void CBoxRender::SetNewSource(bool bNewSource) 
{ 
	m_bNewSource = bNewSource; 
	m_llLastRndTime = 0; 
	m_llSeekPos = 0; 
}

void CBoxRender::SetSendOutData(void * pUserData, QCPlayerOutAVData fSendOutData)
{ 
	m_pUserData = pUserData; 
	m_fSendOutData = fSendOutData; 
}

CBaseClock * CBoxRender::GetClock (void)
{
	if (m_pBoxSource == NULL)
		return NULL;

	if (m_pDataClock == NULL)
		m_pDataClock = new CBaseClock (m_pBaseInst);

	return m_pDataClock;
}

int CBoxRender::WaitRenderTime (QC_DATA_BUFF * pBuff)
{
	return QC_ERR_NONE;
}

void CBoxRender::WaitForExitRender (unsigned int nMaxWaitTime)
{
	unsigned int	nStartTime = qcGetSysTime();
	while (m_bInRender)
	{
		qcSleep (1000);
		if (qcGetSysTime () - nStartTime >= nMaxWaitTime)
		{
			QCLOGW ("It exited without stop!");
			break;
		}
	}
}

int	CBoxRender::WaitOtherRndFirstFrame (void)
{
//	if (m_llSeekPos == 0)
//		return QC_ERR_NONE;

	if (m_nSourceType > 0)
		return QC_ERR_NONE;

	CAutoLock lock(&m_mtRnd);

	if (m_pOtherRnd != NULL)
	{
		int nStartTime = qcGetSysTime ();

		//QCLOGI ("Audio GetRndCount = %d", m_pOtherRnd->GetRndCount ());
				
		while (m_pOtherRnd->GetRndCount () <= 0)
		{
			if (m_pThreadWork->GetStatus () != QCWORK_Run)
				break;
			if (m_pOtherRnd->IsEOS())
				break;
			if (m_pBaseInst->m_bForceClose)
				break;
            // Audio's drop frame speed is faster than video's,
            // so sometimes it will take more time to wait
			if ((qcGetSysTime () - nStartTime > 1000) && !m_pOtherRnd->IsDropFrame())
			{
				QCLOGW ("It can not wait the other render first frame! %d", (qcGetSysTime () - nStartTime));
				if (m_pClock != NULL)
				{
					if (m_pClock->GetTime() == 0)
						m_pClock->SetTime(1);
				}
				return QC_ERR_TIMEOUT;
			}
			qcSleep (2000);
		}

		//QCLOGI ("Audio used time = %d", qcGetSysTime () - nStartTime);
	}
	return QC_ERR_NONE;
}

int	CBoxRender::CaptureVideoImage(QC_VIDEO_FORMAT * pFormat, QC_VIDEO_BUFF * pVideoBuff)
{
	if (m_pBuffData == NULL || m_llCaptureTime < 0 || pFormat == NULL)
		return QC_ERR_STATUS;
	if (m_pBuffData->llTime < m_llCaptureTime)
		return QC_ERR_STATUS;
	m_llCaptureTime = -1;
	if (m_pBuffData->uBuffType != QC_BUFF_TYPE_Video)
		return QC_ERR_UNSUPPORT;
	if (m_pVideoEnc == NULL)
		m_pVideoEnc = new CQCVideoEnc(m_pBaseInst, m_hInst);
	if (m_pVideoBuf == NULL)
	{
		m_pVideoBuf = new QC_DATA_BUFF();
		memset(m_pVideoBuf, 0, sizeof(QC_DATA_BUFF));
	}
	pFormat->nCodecID = QC_CODEC_ID_MJPEG;
	int nRC = m_pVideoEnc->Init(pFormat);
	if (nRC != QC_ERR_NONE)
		return nRC;
	nRC = m_pVideoEnc->EncodeImage(pVideoBuff, m_pVideoBuf);
	if (nRC != QC_ERR_NONE)
    {
        QCLOGW("Encode image fail. %X", nRC);
        return nRC;
    }

	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_CAPTURE_IMAGE, 0, 0, NULL, m_pVideoBuf);

	return QC_ERR_NONE;
}

int CBoxRender::OnStartFunc (void)
{
	if (m_nMediaType == QC_MEDIA_Audio)
		qcThreadSetPriority(qcThreadGetCurHandle(), QC_THREAD_PRIORITY_ABOVE_NORMAL);
	else if (m_nMediaType == QC_MEDIA_Video)
		qcThreadSetPriority(qcThreadGetCurHandle(), QC_THREAD_PRIORITY_NORMAL);
	return QC_ERR_NONE;
}

int CBoxRender::OnWorkItem (void)
{
	qcSleep (2000);
	return QC_ERR_NONE;
}

void CBoxRender::UpdateStartTime(void)
{
    if(m_nPauseTime > 0)
    {
        m_nPauseTime = qcGetSysTime() - m_nPauseTime;
        m_llStartTime += m_nPauseTime;
        m_nLastSysTime += m_nPauseTime;
        m_nPauseTime = 0;
    }
}

