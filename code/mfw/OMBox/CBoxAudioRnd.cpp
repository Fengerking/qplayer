/*******************************************************************************
	File:		CBoxAudioRnd.cpp

	Contains:	The audio render box implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-13		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"
#include "qcMsg.h"

#ifdef __QC_OS_WIN32__
#include "CWaveOutRnd.h"
#endif // __QC_OS_WIN32__

#ifdef __QC_OS_IOS__
#include "CVideoRndFactory.h"
#endif // __QC_OS_WIN32__

#include "CBoxAudioRnd.h"
#include "CBoxMonitor.h"
#include "CMsgMng.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

CBoxAudioRnd::CBoxAudioRnd(CBaseInst * pBaseInst, void * hInst)
	: CBoxRender (pBaseInst, hInst)
	, m_bNewFormat (false)
	, m_pRnd (NULL)
	, m_nVolume (-1)
	, m_fSpeed (1.0)
	, m_pBuffSpeed (NULL)
	, m_pBuffStretch (NULL)
	, m_pTDStretch (NULL)
	, m_nBlockSize (4)
	, m_fStretchSpeed(1.0)
	, m_bSendFormat(false)
	, m_pResample(NULL)
{
	SetObjectName ("CBoxAudioRnd");
	m_nBoxType = OMB_TYPE_RENDER;
	strcpy (m_szBoxName, "Audio Render Box");

	m_nMediaType = QC_MEDIA_Audio;
	memset (&m_fmtAudio, 0, sizeof (m_fmtAudio));
}

CBoxAudioRnd::~CBoxAudioRnd(void)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);

	Stop();

	if (m_pExtRnd == NULL)
		QC_DEL_P (m_pRnd);
	QC_DEL_A (m_fmtAudio.pHeadData);
	if (m_pBuffSpeed != NULL)
	{
		QC_DEL_A (m_pBuffSpeed->pBuff);
		QC_DEL_P (m_pBuffSpeed);
	}
	if (m_pBuffStretch != NULL)
	{
		QC_DEL_A (m_pBuffStretch->pBuff);
		QC_DEL_P (m_pBuffStretch);
	}
	QC_DEL_P (m_pTDStretch);
	QC_DEL_P (m_pResample);
}

int CBoxAudioRnd::SetSource (CBoxBase * pSource)
{
	if (pSource == NULL)
	{
		m_pBoxSource = NULL;
		m_llSeekPos = 0;
		m_bEOS = true;
		m_nRndCount = 0;
        m_bSendFormat = false;
		return QC_ERR_ARG;
	}

	Stop ();

	CBoxBase::SetSource (pSource);

	//m_nSourceType = GetParam(BOX_GET_SourceType, NULL);
	if (m_pTDStretch != NULL)
		m_pTDStretch->clear();

	QC_AUDIO_FORMAT * pFmt = pSource->GetAudioFormat ();
	if (pFmt == NULL)
		return QC_ERR_STATUS;
	m_fmtAudio.nChannels = pFmt->nChannels;
	m_fmtAudio.nSampleRate = pFmt->nSampleRate;
	m_fmtAudio.nBits = pFmt->nBits;
	if (m_fmtAudio.nBits == 0)
		m_fmtAudio.nBits = 16;

	if (m_pExtRnd == NULL)
	{
		QC_DEL_P (m_pRnd);
#ifdef __QC_OS_WIN32__
		m_pRnd = new CWaveOutRnd(m_pBaseInst, m_hInst);
#elif defined __QC_OS_IOS__
        m_pRnd = CAudioRndFactory::Create(m_pBaseInst);
#endif // __QC_OS_WIN32__
	}
	else
	{
		m_pRnd = (CBaseAudioRnd *)m_pExtRnd;
	}
	if (m_pRnd == NULL)
		return QC_ERR_MEMORY;

	int nRC = m_pRnd->Init (pFmt, m_pOtherRnd == NULL);
	if (nRC != QC_ERR_NONE)
		return nRC;

//	if (m_nVolume >= 0)
//		m_pRnd->SetVolume (m_nVolume);
	if (m_fSpeed != 1.0)
		m_pRnd->SetSpeed (m_fSpeed);
	m_bNewFormat = false;

	m_nBlockSize = m_fmtAudio.nChannels * m_fmtAudio.nBits / 8;

	return QC_ERR_NONE;
}

int	CBoxAudioRnd::Start (void)
{
	int nRC = QC_ERR_NONE;
	if (m_pRnd != NULL)
		nRC = m_pRnd->Start ();
	nRC = CBoxRender::Start ();
	return nRC;
}

int CBoxAudioRnd::Pause (void)
{
	int nRC = CBoxRender::Pause ();
	if (m_pRnd != NULL)
		nRC = m_pRnd->Pause ();
	return nRC;
}

int	CBoxAudioRnd::Stop (void)
{
	if (m_pRnd != NULL)
		m_pRnd->Stop ();
	int nRC = CBoxRender::Stop ();
	return nRC;
}

long long CBoxAudioRnd::SetPos (long long llPos)
{
	if (m_pRnd != NULL)
		m_pRnd->Flush ();

	long long llRC = CBoxRender::SetPos (llPos);
	m_llLastRndTime = 0;
	return llRC;
}

int CBoxAudioRnd::SetSpeed (double fSpeed)
{
	CAutoLock lock(&m_mtRsmp);
	m_fSpeed = fSpeed;
	if (m_pResample != NULL && m_fmtAudio.nChannels > 0)
		m_pResample->initialize(1 / m_fSpeed, m_fmtAudio.nChannels, 1.0);
	if (m_pRnd != NULL)
		return m_pRnd->SetSpeed (m_fSpeed);
	return QC_ERR_STATUS;
}

int CBoxAudioRnd::SetVolume (int nVolume)
{
	m_nVolume = nVolume;
	if (m_pBoxSource != NULL)
		m_pBoxSource->SetParam(BOX_SET_AudioVolume, &m_nVolume);
//	if (m_pRnd != NULL)
//		return m_pRnd->SetVolume (nVolume);
	return QC_ERR_NONE;
}

int CBoxAudioRnd::GetVolume (void)
{
/*
	if (m_pRnd != NULL)
	{
		int nVolume = m_pRnd->GetVolume ();
		if (nVolume < 0)
			return m_nVolume;
		if (abs (m_nVolume - nVolume) <= 1)
			return m_nVolume;
		else
			return nVolume;
	}
*/
	return m_nVolume;
}

CBaseClock * CBoxAudioRnd::GetClock (void)
{
	if (m_pRnd != NULL)
		return m_pRnd->GetClock ();
	return CBoxRender::GetClock ();
}

int CBoxAudioRnd::GetRndCount (void)
{
	if (m_pRnd == NULL)
		return m_nRndCount;
	return m_pRnd->GetRndCount ();
}

int CBoxAudioRnd::OnWorkItem (void)
{
	m_bInRender = true;
	int nRC = RenderFrame ();
	m_bInRender = false;
	return nRC;
}

int CBoxAudioRnd::RenderFrame (void)
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

	CAutoLock lock (&m_mtRnd);
	m_pBuffInfo->nMediaType = QC_MEDIA_Audio;
	m_pBuffInfo->uFlag = 0;
	m_pBuffInfo->llTime = 0;
	m_pBuffData = NULL;
	nRC = m_pBoxSource->ReadBuff (m_pBuffInfo, &m_pBuffData, true);
	if (nRC == QC_ERR_BUFFERING)
	{
		if (m_pClock != NULL && !m_pClock->IsPaused () && m_nRndCount>0)
			m_pClock->Pause ();
		qcSleep (2000);
		return QC_ERR_RETRY;
	}
	else if (nRC == QC_ERR_RETRY)
	{
		qcSleep (2000);
		return nRC;
	}
	else if (nRC != QC_ERR_NONE && nRC != QC_ERR_FINISH)
	{
		qcSleep(2000);
		return nRC;
	}

    // source type maybe changed on same soure mode
//    if (m_nSourceType == -1)
        m_nSourceType = GetParam (BOX_GET_SourceType, NULL);

	BOX_READ_BUFFER_REC_AUDIORND

	if (nRC == QC_ERR_FINISH || (m_pBuffData != NULL  && (m_pBuffData->uFlag & QCBUFF_EOS) == QCBUFF_EOS))
	{
		m_bEOS = true;
		if (m_nRndCount == 0)
		{
			if (m_pClock != NULL && m_llSeekPos > 0)
				m_pClock->SetTime (m_llSeekPos);
		}
		// Notify message audio is eos
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKA_EOS, 0, 0);
	}
	if (m_pBuffData == NULL)
		return QC_ERR_RETRY;

	//QCLOGI("Audio Time: % 8lld, clock % 8lld  Last = % 8lld, Step = % 8lld**********", m_pBuffData->llTime, m_pClock->GetTime(), m_llLastRndTime, m_pBuffData->llTime - m_llLastRndTime);

	if (m_pBaseInst->m_llFAudioTime > m_pBaseInst->m_llFVideoTime && m_pOtherRnd != NULL)
	{
		if (m_nRndCount == 0 && m_pClock != NULL && m_pClock->IsPaused ())
		{
			m_pClock->Start();
			if (m_llSeekPos > 0)
				m_pClock->SetTime(m_llSeekPos);
			else
				m_pClock->SetTime(m_pBaseInst->m_llFVideoTime);
		}
		bool bWait = false;
		if (m_llLastRndTime > 0 && m_pBuffData->llTime - m_llLastRndTime > m_pBaseInst->m_llFAudioTime / 2)
			bWait = true;

		if (m_nRndCount == 0 || bWait)
		{
			while (m_pClock->GetTime() < m_pBuffData->llTime)
			{
				qcSleep(2000);
				if (m_pBaseInst->m_bForceClose)
					break;
				if (m_pThreadWork->GetStatus() != QCWORK_Run)
					break;
			}
		}
	}

	m_pBuffInfo->llTime = m_pBuffData->llTime;
	if (m_pThreadWork->GetStatus () == QCWORK_Run)
	{
		if (m_pClock != NULL && m_pClock->IsPaused())
		{
			m_pClock->Start();
			m_pClock->SetTime(m_pBuffData->llTime);
		}
	}
	if ((m_pBuffData->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT && m_pBuffData->pFormat != NULL)
	{
		QC_AUDIO_FORMAT *	pFmt = (QC_AUDIO_FORMAT *)m_pBuffData->pFormat;
		m_fmtAudio.nBits = pFmt->nBits;
		if (m_fmtAudio.nBits == 0)
			m_fmtAudio.nBits = 16;
		m_fmtAudio.nSampleRate = pFmt->nSampleRate;
		m_fmtAudio.nChannels = pFmt->nChannels;
		if (m_pTDStretch != NULL)
		{
			m_pTDStretch->clearInput ();
			m_pTDStretch->setParameters (m_fmtAudio.nSampleRate, 40, 15, 8);
			m_pTDStretch->setChannels (m_fmtAudio.nChannels);
		}
		if (m_pResample != NULL)
		{
			CAutoLock lockFormat(&m_mtRsmp);
			m_pResample->initialize (1 / m_fSpeed, m_fmtAudio.nChannels, 1.0);
		}
		m_nBlockSize = m_fmtAudio.nChannels * m_fmtAudio.nBits / 8;
		m_bNewFormat = true;
		if (m_nRndCount > 0 && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKA_NEW_FORMAT, m_fmtAudio.nSampleRate, m_fmtAudio.nChannels);
	}
	if (m_fmtAudio.nChannels == 0 || m_fmtAudio.nSampleRate == 0) {
		QC_AUDIO_FORMAT *	pFmt = m_pBoxSource->GetAudioFormat();
		if (pFmt != NULL)
		{
			m_fmtAudio.nBits = pFmt->nBits;
			if (m_fmtAudio.nBits == 0)
				m_fmtAudio.nBits = 16;
			m_fmtAudio.nSampleRate = pFmt->nSampleRate;
			m_fmtAudio.nChannels = pFmt->nChannels;
		}
		m_nBlockSize = m_fmtAudio.nChannels * m_fmtAudio.nBits / 8;
		m_bNewFormat = true;
	}
	if (m_nSourceType == 0 && m_nRndCount == 0 && m_pBuffData->llTime > 1000 && m_pOtherRnd != NULL && m_llSeekPos == 0)
	{
		if (m_pClock != NULL)
		{
			if (m_pClock->GetTime() == 0)
				m_pClock->SetTime(1);
		}
		while (m_pOtherRnd->GetRndTime() < m_pBuffData->llTime)
		{
			qcSleep(10000);
			if (m_pBaseInst->m_bForceClose || m_nStatus != OMB_STATUS_RUN || m_pThreadWork->GetStatus () != QCWORK_Run)
				return QC_ERR_STATUS;
		}
	}
    
    if (m_nSeekMode > 0 && m_pBuffData->llTime < m_llSeekPos)
    {
        m_bDropFrame = true;
        if ((m_pBuffData->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
        {
            QC_AUDIO_FORMAT *    pFmt = (QC_AUDIO_FORMAT *)m_pBuffData->pFormat;
            if (m_pRnd != NULL && pFmt != NULL)
                m_pRnd->Init(pFmt, m_pOtherRnd == NULL);
        }
        return QC_ERR_NONE;
    }
    m_bDropFrame = false;

	if (m_nRndCount < 1)
	{
		WaitOtherRndFirstFrame ();
		if (m_pOtherRnd != NULL && m_pOtherRnd->GetRndCount () <= 0)
			QCLOGW ("The other m_nRndCount is %d!", m_pOtherRnd->GetRndCount());
	}
    
	m_pBuffData->nValue = 0;
    if (!m_bSendFormat)
    {
        m_bSendFormat = true;
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKA_NEW_FORMAT, m_fmtAudio.nSampleRate, m_fmtAudio.nChannels);
    }
	if (m_fSendOutData != NULL)
	{
		m_pBuffData->nMediaType = QC_MEDIA_Audio;
		m_fSendOutData(m_pUserData, m_pBuffData);
	}
	if (m_pRnd != NULL && m_pBuffData->nValue != 11)
	{
		if ((m_pBuffData->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
		{
            m_nRndCount = 0;
			m_pRnd->Flush ();
			if (m_pClock != NULL)
				m_pClock->SetTime (m_pBuffData->llTime);
		}

		QC_DATA_BUFF *	pBuffData = m_pBuffData;
		if (m_fSpeed != 1.0)
		{
			CAutoLock lock(&m_mtRsmp);
			if (m_fSpeed <= 2.5 && m_fSpeed >= 0.4)
			{
				QC_DATA_BUFF *	pBuffStretch = NULL;
				StretchData(m_pBuffData, &pBuffStretch, m_fSpeed);
				if (pBuffStretch != NULL)
					pBuffData = pBuffStretch;
			}
			else
			{
				if (m_pBuffSpeed == NULL)
				{
					m_pBuffSpeed = new QC_DATA_BUFF();
					memset(m_pBuffSpeed, 0, sizeof(QC_DATA_BUFF));
					m_pBuffSpeed->uSize = m_pBuffData->uSize * 20;
					m_pBuffSpeed->uBuffSize = m_pBuffSpeed->uSize;
					m_pBuffSpeed->pBuff = new unsigned char[m_pBuffSpeed->uBuffSize];
					memset(m_pBuffSpeed->pBuff, 0, m_pBuffSpeed->uBuffSize);
				}
				if (m_pResample == NULL)
				{
					m_pResample = new aflibConverter(false, true, false);
					m_pResample->initialize(1 / m_fSpeed, m_fmtAudio.nChannels, 1.0);
				}
				int inSamples = m_pBuffData->uSize / (m_fmtAudio.nChannels * 2);
				int outSamples = (int)((inSamples / m_fSpeed) / 4 * 4);
				m_pResample->resample(inSamples, outSamples, (short *)m_pBuffData->pBuff, (short *)m_pBuffSpeed->pBuff);

				m_pBuffSpeed->uSize = (unsigned int)((int)(m_pBuffData->uSize / m_fSpeed) / 4 * 4);
				m_pBuffSpeed->uFlag = m_pBuffData->uFlag;
				m_pBuffSpeed->llTime = m_pBuffData->llTime;
				pBuffData = m_pBuffSpeed;
			}
		}
		else if (m_nSourceType > 0 && m_fmtAudio.nSampleRate > 16000)
		{
			QC_DATA_BUFF *	pBuffStretch = NULL;
			StretchData (m_pBuffData, &pBuffStretch, 1.0);
			if (pBuffStretch != NULL)
				pBuffData = pBuffStretch;
		}

		if (pBuffData->uSize <= 0)
			return QC_ERR_RETRY;

		if (m_bNewFormat)
		{
			pBuffData->uFlag |= QCBUFF_NEW_FORMAT;
			pBuffData->pFormat = &m_fmtAudio;
			m_bNewFormat = false;
		}
  
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKA_RENDER, 0, pBuffData->llTime);

		if (m_pBaseInst->m_pSetting->g_qcs_nAudioDecVlm > 0)
		{
			if (m_pBaseInst->m_pSetting->g_qcs_nAudioVolume > 100)
				m_nVolume = m_pBaseInst->m_pSetting->g_qcs_nAudioVolume;
			else
				m_nVolume = 100;
		}
		else
		{
			m_nVolume = m_pBaseInst->m_pSetting->g_qcs_nAudioVolume;
		}
		if (m_nVolume != 100 && pBuffData != NULL && pBuffData->pBuff != NULL)
		{
			int nBuffSize = pBuffData->uSize;
			int nVolume = 0;
			if (m_fmtAudio.nBits == 8)
			{
				char * pAudio = (char *)pBuffData->pBuff;
				for (int i = 0; i < nBuffSize; i++)
				{
					nVolume = (m_nVolume * (*pAudio)) / 100;
					if (nVolume >= 128)
						*pAudio++ = 127;
					else if (nVolume <= -128)
						*pAudio++ = (char)0XFF;
					else
						*pAudio++ = (char)nVolume;
				}
			}
			else
			{
				short * pAudio = (short *)pBuffData->pBuff;
				for (int i = 0; i < nBuffSize / 2; i++)
				{
					nVolume = (m_nVolume * (*pAudio)) / 100;
					if (nVolume >= 0X7FFF)
						*pAudio++ = 0X7FFF;
					else if (nVolume <= -0X7FFF)
						*pAudio++ = (short)-0X7FFF;
					else
						*pAudio++ = (short)nVolume;
				}
			}
		}

		unsigned int	nStepSize = 1024 * 8;
		unsigned char * pBuffSrc = pBuffData->pBuff;
		unsigned int	nSrcSize = pBuffData->uSize;
		unsigned int	nRndSize = 0;
		unsigned int	nOneSize = 0;
		long long		llRndTime = pBuffData->llTime;
		if (m_pTDStretch != NULL && m_pBuffData != NULL)
			nStepSize = m_pBuffData->uSize * 2;
		while (nRndSize < nSrcSize)
		{
			if (nSrcSize - nRndSize > nStepSize)
				nOneSize = nStepSize;
			else
				nOneSize = nSrcSize - nRndSize;
			pBuffData->uSize = nOneSize;
			pBuffData->pBuff = pBuffSrc + nRndSize;
			pBuffData->llTime = llRndTime + nRndSize * 1000 / (m_fmtAudio.nChannels * m_fmtAudio.nSampleRate * m_fmtAudio.nBits / 8);
			nRC = m_pRnd->Render(pBuffData);
			while (nRC == QC_ERR_RETRY)
			{
				qcSleep(5000);
				nRC = m_pRnd->Render(pBuffData);
				if (m_pThreadWork->GetStatus() != QCWORK_Run)
				{
					pBuffData->pBuff = pBuffSrc;
					pBuffData->uSize = nSrcSize;
					return QC_ERR_STATUS;
				}
			}
			nRndSize += nOneSize;
			pBuffData->uFlag = 0;
		}
		pBuffData->pBuff = pBuffSrc;
		pBuffData->uSize = nSrcSize;
		m_llLastRndTime = pBuffData->llTime;
	}

    m_nRndCount++;
	m_pBaseInst->m_nAudioRndCount = m_nRndCount;
	if (m_nRndCount == 1 && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKA_FIRST_FRAME, qcGetSysTime() - m_pBaseInst->m_nOpenSysTime, m_pBuffData->llTime);
    if(qcGetSysTime()-m_nLastSysTime > 10000)
    {
        int nTime = (qcGetSysTime()-m_nLastSysTime) / 1000;
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RENDER_AUDIO_FPS, (m_nRndCount - m_nLastRndCount) / nTime, 0);
        m_nLastRndCount = m_nRndCount;
        m_nLastSysTime = qcGetSysTime();
    }

	return QC_ERR_NONE;
}

int CBoxAudioRnd::OnStopFunc (void)
{
	if (m_pRnd != NULL)
		m_pRnd->OnStop ();
	return QC_ERR_NONE;
}

int CBoxAudioRnd::StretchData(QC_DATA_BUFF * pBuffData, QC_DATA_BUFF ** ppBuffStretch, double dSpeed)
{
	if (ppBuffStretch == NULL)
		return QC_ERR_ARG;
	int nRC = 0;
	int nMinTime = m_pBaseInst->m_pSetting->g_qcs_nMinPlayBuffTime;
	int nMaxTime = m_pBaseInst->m_pSetting->g_qcs_nMaxPlayBuffTime;
	int nBuffTime = 0;
	*ppBuffStretch = NULL;
	
	nBuffTime = GetParam(BOX_GET_AudioBuffTime, NULL);
	if (m_pBuffStretch == NULL)
	{
		m_pBuffStretch = new QC_DATA_BUFF ();
		memset (m_pBuffStretch, 0, sizeof (QC_DATA_BUFF));
		m_pBuffStretch->uSize = pBuffData->uSize * 8;
		m_pBuffStretch->uBuffSize = m_pBuffStretch->uSize;
		m_pBuffStretch->pBuff = new unsigned char[m_pBuffStretch->uBuffSize];
		memset (m_pBuffStretch->pBuff, 0, m_pBuffStretch->uBuffSize);				
	}
	m_pBuffStretch->uFlag = pBuffData->uFlag;
	m_pBuffStretch->llTime = pBuffData->llTime;
	if ((pBuffData->uFlag & QCBUFF_NEW_FORMAT) != 0)
		m_pBuffStretch->pFormat = &m_fmtAudio;
	else
		m_pBuffStretch->pFormat = NULL;

	if (m_pTDStretch == NULL)
	{
		m_pTDStretch = new TDStretch ();
		m_pTDStretch->setParameters (m_fmtAudio.nSampleRate, 40, 15, 8);
		m_pTDStretch->setChannels (m_fmtAudio.nChannels);
	}

	float fStretchSpeed = 1.0;
	if (dSpeed != 1.0)
	{
		fStretchSpeed = (float)dSpeed;
	}
	else
	{
		if (nBuffTime < nMinTime)
			fStretchSpeed = (float)0.90;
		else if (nBuffTime > nMaxTime)
			fStretchSpeed = (float)1.10;
		else
			fStretchSpeed = (float)1.0;
	}

	//fStretchSpeed = 1.1 - (rand() % 3) * 0.1;
	if (m_fStretchSpeed != fStretchSpeed)
	{
		m_fStretchSpeed = fStretchSpeed;
		m_pTDStretch->setTempo(fStretchSpeed);
		QCLOGI("The strech audio speed is %.2f. The buffer is % 8d", m_fStretchSpeed, nBuffTime);
	}
	nRC = m_pTDStretch->process ((SAMPLETYPE *)pBuffData->pBuff, pBuffData->uSize / m_nBlockSize, 
								 (SAMPLETYPE *)m_pBuffStretch->pBuff, m_pBuffStretch->uBuffSize / m_nBlockSize);
	m_pBuffStretch->uSize = nRC * m_nBlockSize;
	*ppBuffStretch = m_pBuffStretch;

	return QC_ERR_NONE;
}
