/*******************************************************************************
	File:		CBaseClock.cpp

	Contains:	The base clock implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"

#include "CBaseClock.h"

#include "USystemFunc.h"

CBaseClock::CBaseClock(CBaseInst * pBaseInst)
	: CBaseObject (pBaseInst)
	, m_bRun (false)
	, m_nErrAdj (50)
	, m_nOldAdj (50)
	, m_llStart (0)
	, m_llSystem (0)
	, m_llLast (0)
	, m_llNow (0)
	, m_fSpeed (1.0)
	, m_nOffset (0)
{
	SetObjectName ("CBaseClock");
	QCLOGI ("Create CBaseClock");
}

CBaseClock::~CBaseClock(void)
{
	QCLOGI ("Destroy CBaseClock");	
}

long long CBaseClock::GetTime (void)
{
	CAutoLock lock (&m_mtTime);
	if (m_bRun)
	{
		if (m_llStart <= 0)
			return 0;

		m_llNow = m_llStart + (int)((qcGetSysTime () - m_llSystem) * m_fSpeed) - m_nOffset;
		if (m_llNow <= 0)
			return 1;
			
		return m_llNow;
	}
	else
	{
		return m_llLast;
	}
}

int CBaseClock::SetTime (long long llTime)
{
	CAutoLock lock (&m_mtTime);
	if (m_llStart > 0 && abs ((int)(GetTime () - llTime)) < (m_nErrAdj * m_fSpeed))
		return QC_ERR_NONE;

	m_llStart = llTime;
	if (m_llStart <= 0)
		m_llStart = 1;

	m_llSystem = qcGetSysTime ();

	m_llLast = m_llStart;

	return QC_ERR_NONE;
}

int CBaseClock::SetErrAdj (int nError)
{
	CAutoLock lock (&m_mtTime);

	m_nErrAdj = nError;

	return QC_ERR_NONE;
}

int CBaseClock::GetErrAdj (void)
{
	return m_nErrAdj;
}

int CBaseClock::SetSpeed (double fSpeed)
{
	CAutoLock lock (&m_mtTime);
	if (fSpeed <= 0)
		return QC_ERR_ARG;

	m_llStart = GetTime ();
	m_llSystem = qcGetSysTime ();

	m_llLast = m_llStart;

	m_fSpeed = fSpeed;

	if (m_fSpeed < 0.5)
	{
		if (m_nOldAdj != 5000)
			m_nOldAdj = m_nErrAdj;
		m_nErrAdj = 5000;
	}
	else
	{
		m_nErrAdj = m_nOldAdj;
	}

	return QC_ERR_NONE;
}

double CBaseClock::GetSpeed (void)
{
	CAutoLock lock (&m_mtTime);

	return m_fSpeed;
}

int CBaseClock::SetOffset (int nOffset)
{
	CAutoLock lock (&m_mtTime);

	m_nOffset = nOffset;
	
	return QC_ERR_NONE;	
}
	
int CBaseClock::GetOffset (void)
{
	CAutoLock lock (&m_mtTime);
	
	return m_nOffset;
}

int CBaseClock::Start (void)
{
	CAutoLock lock (&m_mtTime);
	if (m_bRun)
		return QC_ERR_NONE;

	m_bRun = true;

	m_llStart = m_llLast;
	m_llSystem = qcGetSysTime ();

	return QC_ERR_NONE;
}

int CBaseClock::Pause (void)
{
	CAutoLock lock (&m_mtTime);

	m_llLast = GetTime ();
	if (m_llLast == 0)
		m_llLast = 1;
	m_bRun = false;

	return QC_ERR_NONE;
}

bool CBaseClock::IsPaused (void)
{
	CAutoLock lock (&m_mtTime);

	if (m_bRun)
		return false;

	return true;
}