/*******************************************************************************
	File:		CBaseIO.cpp

	Contains:	base io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CBaseIO.h"

#include "USystemFunc.h"

CBaseIO::CBaseIO(CBaseInst * pBaseInst)
	: CBaseObject (pBaseInst)
	, m_sStatus (QCIO_Init)
	, m_pURL(NULL)
	, m_llFileSize (0)
	, m_llReadPos (-1)
	, m_llDownPos (-1)
	, m_llSeekPos (-1)
	, m_llStopPos (-1)
	, m_nNeedSleep (1000)
	, m_bIsStreaming (false)
	, m_nExitRead(0)
	, m_nCheckBitrate(0)
	, m_nOpenTime(0)
	, m_llReadSize(0)
	, m_nNotifyDLPercent(0)
	, m_llHadReadSize(0)
{
	SetObjectName ("CBaseIO");
	memset(m_szHostAddr, 0, sizeof(m_szHostAddr));
}

CBaseIO::~CBaseIO(void)
{
	Close ();
}

int CBaseIO::Open (const char * pURL, long long llOffset, int nFlag)
{
	return QC_ERR_IMPLEMENT;
}

int CBaseIO::Reconnect (const char * pNewURL, long long llOffset)
{
	return QC_ERR_IMPLEMENT;
}

int CBaseIO::Close (void)
{
	m_llFileSize = 0;
	m_llReadPos = -1;
	m_llDownPos = -1;
	m_llHadReadSize = 0;
	QC_DEL_A (m_pURL);
	return QC_ERR_IMPLEMENT;
}

int CBaseIO::Run (void)
{
	m_sStatus = QCIO_Run;
	return QC_ERR_NONE;
}

int CBaseIO::Pause (void)
{
	m_sStatus = QCIO_Pause;
	return QC_ERR_NONE;
}

int CBaseIO::Stop (void)
{
	m_sStatus = QCIO_Stop;
	return QC_ERR_NONE;
}

long long CBaseIO::GetSize (void)
{
	return m_llFileSize;
}

int CBaseIO::Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	return QC_ERR_IMPLEMENT;
}

int	CBaseIO::ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	return QC_ERR_IMPLEMENT;
}

int	CBaseIO::ReadSync (long long llPos, unsigned char * pBuff, int nSize, int nFlag)
{
	int nRead = nSize;
	int nRC = ReadAt (llPos, pBuff, nRead, true, nFlag);
	m_llHadReadSize += nRead;
	if (nRC == QC_ERR_NONE)
		return nRead;
	else if (nRC == QC_ERR_Disconnected)
		return QC_ERR_Disconnected;
	else if (nRC == QC_ERR_RETRY)
		return QC_ERR_Disconnected;
	else
    {
	//	if (nRC != QC_ERR_FINISH)
			QCLOGI("[E]IO read return %X", nRC);
        return -1;
    }
}

int	CBaseIO::Write (unsigned char * pBuff, int & nSize, long long llPos)
{
	return QC_ERR_IMPLEMENT;
}

long long CBaseIO::SetPos (long long llPos, int nFlag)
{
	return -1;
}

long long CBaseIO::GetDownPos (void)
{
	return m_llDownPos;
}

long long CBaseIO::GetReadPos (void)
{
	return m_llReadPos;
}

int CBaseIO::GetSpeed (int nLastSecs)
{
	return 1024 * 1024 * 8;
}

QCIOType CBaseIO::GetType (void)
{
	return QC_IOTYPE_NONE;
}

bool CBaseIO::IsStreaming (void)
{
	return m_bIsStreaming;
}

int CBaseIO::GetParam (int nID, void * pParam)
{
	switch (nID)
	{
	case QCIO_PID_SourceType:
		return GetType ();

	default:
		break;
	}
	return QC_ERR_IMPLEMENT;
}

int CBaseIO::SetParam (int nID, void * pParam)
{
	if (nID == QCIO_PID_HTTP_HeadHost)
	{
		strcpy(m_szHostAddr, (char *)pParam);
		return QC_ERR_NONE;
	}
	else if (nID == QCIO_PID_EXIT_READ)
	{
		m_nExitRead = *(int *)pParam;
		return QC_ERR_NONE;
	}
	else if (nID == QCIO_PID_HTTP_NOTIFYDL_PERCENT)
	{
		m_nNotifyDLPercent = *(int *)pParam;
		return QC_ERR_NONE;
	}
	else if (nID == QCIO_PID_HTTP_STOP_POS)
	{
		m_llStopPos = *(long long *)pParam;
		return QC_ERR_NONE;
	}
	else if (nID == QCIO_PID_HTTP_NEED_SLEEP)
	{
		m_nNeedSleep = *(int *)pParam;
		return QC_ERR_NONE;
	}
	return QC_ERR_IMPLEMENT;
}

int	CBaseIO::CheckBitrate(int nReadSize)
{
	if (m_nOpenTime == 0 || m_nCheckBitrate == 0)
		return QC_ERR_NONE;

	int nUsedTime = qcGetSysTime() - m_nOpenTime;

	return QC_ERR_NONE;
}
