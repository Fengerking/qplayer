/*******************************************************************************
	File:		CBoxBase.cpp

	Contains:	base box implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "CBoxBase.h"

CBoxBase::CBoxBase(CBaseInst * pBaseInst, void * hInst)
	: CBaseObject (pBaseInst)
	, m_hInst (hInst)
	, m_nBoxType (OMB_TYPE_BASE)
	, m_nStatus (OMB_STATUS_INIT)
	, m_pClock (NULL)
	, m_pBoxSource (NULL)
	, m_pBuffInfo (NULL)
	, m_pBuffData (NULL)
	, m_llSeekPos (0)
	, m_bEOS (false)
{
	SetObjectName ("CBoxBase");
	strcpy (m_szBoxName, "Base Box");

	m_pBuffInfo = new QC_DATA_BUFF ();
	memset (m_pBuffInfo, 0, sizeof (QC_DATA_BUFF));
}

CBoxBase::~CBoxBase(void)
{
	QC_DEL_P (m_pBuffInfo);
}

int CBoxBase::SetSource (CBoxBase * pSource)
{
	m_pBoxSource = pSource;
	m_llSeekPos = 0;
	m_bEOS = false;
	return QC_ERR_NONE;
}

int	CBoxBase::Start (void)
{
	m_nStatus = OMB_STATUS_RUN;
	if (m_pBoxSource != NULL)
		return m_pBoxSource->Start ();
	return QC_ERR_NONE;
}

int CBoxBase::Pause (void)
{
	m_nStatus = OMB_STATUS_PAUSE;
	if (m_pBoxSource != NULL)
		return m_pBoxSource->Pause ();
	return QC_ERR_NONE;
}

int	CBoxBase::Stop (void)
{
	m_nStatus = OMB_STATUS_STOP;
	if (m_pBoxSource != NULL)
		return m_pBoxSource->Stop ();
	return QC_ERR_NONE;
}

int CBoxBase::ReadBuff (QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait)
{
	return QC_ERR_FAILED;
}

int CBoxBase::RendBuff (QC_DATA_BUFF * pBuffer, bool bRnd)
{
	return QC_ERR_FAILED;
}

long long CBoxBase::SetPos (long long llPos)
{
	m_llSeekPos = llPos;
	if (m_pBoxSource != NULL)
		m_pBoxSource->SetPos (llPos);
	m_bEOS = false;
	return QC_ERR_NONE;
}

void CBoxBase::Flush(void)
{
	if (m_pBoxSource != NULL)
		m_pBoxSource->Flush();
}

bool CBoxBase::IsEOS (void)
{
	if (m_pBoxSource == NULL)
		return true;
	return m_bEOS;
}

void CBoxBase::SetClock (CBaseClock * pClock)
{
	m_pClock = pClock;
}

CBaseClock * CBoxBase::GetClock (void)
{
	return m_pClock;
}

int CBoxBase::SetParam (int nID, void * pParam)
{
	if (m_pBoxSource != NULL)
		return m_pBoxSource->SetParam (nID, pParam);
	return QC_ERR_PARAMID;
}

int CBoxBase::GetParam (int nID, void * pParam)
{
	if (m_pBoxSource != NULL)
		return m_pBoxSource->GetParam (nID, pParam);
	return QC_ERR_PARAMID;
}

QC_STREAM_FORMAT * CBoxBase::GetStreamFormat (int nID) 
{
	if (m_pBoxSource != NULL)
		return m_pBoxSource->GetStreamFormat (nID);
	return NULL;
}

QC_AUDIO_FORMAT * CBoxBase::GetAudioFormat (int nID) 
{
	if (m_pBoxSource != NULL)
		return m_pBoxSource->GetAudioFormat (nID);
	return NULL;
}

QC_VIDEO_FORMAT * CBoxBase::GetVideoFormat (int nID) 
{
	if (m_pBoxSource != NULL)
		return m_pBoxSource->GetVideoFormat (nID);
	return NULL;
}

QC_SUBTT_FORMAT * CBoxBase::GetSubttFormat (int nID) 
{
	if (m_pBoxSource != NULL)
		return m_pBoxSource->GetSubttFormat (nID);
	return NULL;
}
