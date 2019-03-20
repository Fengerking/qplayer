/*******************************************************************************
	File:		CBaseAudioRnd.cpp

	Contains:	The base audio render implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CBaseAudioRnd.h"

#include "ULogFunc.h"

CBaseAudioRnd::CBaseAudioRnd(CBaseInst * pBaseInst, void * hInst)
	: CBaseObject (pBaseInst)
	, m_hInst (hInst)
	, m_bAudioOnly (false)
	, m_pClock (NULL)
	, m_pPCMData (NULL)
	, m_nPCMSize (0)
	, m_pPCMBuff (NULL)
	, m_nPCMLen (0)
	, m_fSpeed (1.0)
	, m_llPrevTime (0)
	, m_nSizeBySec (0)
	, m_nRndCount (0)
{
	SetObjectName ("CBaseAudioRnd");

	memset (&m_fmtAudio, 0, sizeof (m_fmtAudio));
}

CBaseAudioRnd::~CBaseAudioRnd(void)
{
	QC_DEL_A (m_pPCMData);
	QC_DEL_P (m_pClock);
}

int CBaseAudioRnd::Init (QC_AUDIO_FORMAT * pFmt, bool bAudioOnly)
{
	if (m_pClock == NULL)
		m_pClock = new CBaseClock (m_pBaseInst);
	m_pClock->SetTime (0);
	m_bAudioOnly = bAudioOnly;
	m_nRndCount = 0;
	return QC_ERR_IMPLEMENT;
}

int CBaseAudioRnd::Uninit (void)
{
	QC_DEL_A (m_fmtAudio.pHeadData);
	memset (&m_fmtAudio, 0, sizeof (m_fmtAudio));
	return QC_ERR_IMPLEMENT;
}

int	CBaseAudioRnd::Start (void)
{
	m_nRndCount = 0;
	return QC_ERR_NONE;
}

int CBaseAudioRnd::Pause (void)
{
	if (m_pClock != NULL)
		m_pClock->Pause ();
	return QC_ERR_NONE;
}

int	CBaseAudioRnd::Stop (void)
{
	return QC_ERR_NONE;
}

int CBaseAudioRnd::OnStart (void)
{
	return QC_ERR_NONE;
}

int CBaseAudioRnd::OnStop (void)
{
	return QC_ERR_NONE;
}

int CBaseAudioRnd::Flush (void)
{
	m_nRndCount = 0;
	return QC_ERR_NONE;
}

int CBaseAudioRnd::SetSpeed (double fSpeed)
{
	CAutoLock lock (&m_mtRnd);

	if (m_fSpeed == fSpeed)
		return QC_ERR_NONE;

	m_fSpeed = fSpeed;

	return QC_ERR_NONE;
}

int CBaseAudioRnd::Render (QC_DATA_BUFF * pBuff)
{
	CAutoLock lock (&m_mtRnd);
	if ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
		m_nRndCount = 0;
	return QC_ERR_NONE;
}

int CBaseAudioRnd::SetVolume (int nVolume)
{
	return QC_ERR_FAILED;
}

int CBaseAudioRnd::GetVolume (void)
{
	return 0;
}

