/*******************************************************************************
	File:		CFFBaseIO.cpp

	Contains:	base io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-08		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CFFBaseIO.h"

CFFBaseIO::CFFBaseIO(void)
	: m_sStatus (QCIO_Init)
	, m_llFileSize (0)
	, m_llReadPos (-1)
	, m_llDownPos (-1)
	, m_bIsStreaming (false)
{
	memset (m_szURL, 0, sizeof (m_szURL));
	memset(m_szHostAddr, 0, sizeof(m_szHostAddr));
}

CFFBaseIO::~CFFBaseIO(void)
{
	Close ();
}

int CFFBaseIO::Open (const char * pURL, long long llOffset, int nFlag)
{
	return QC_ERR_IMPLEMENT;
}

int CFFBaseIO::Reconnect (const char * pNewURL, long long llOffset)
{
	return QC_ERR_IMPLEMENT;
}

int CFFBaseIO::Close (void)
{
	m_llFileSize = 0;
	m_llReadPos = -1;
	m_llDownPos = -1;
	return QC_ERR_IMPLEMENT;
}

int CFFBaseIO::Run (void)
{
	m_sStatus = QCIO_Run;
	return QC_ERR_NONE;
}

int CFFBaseIO::Pause (void)
{
	m_sStatus = QCIO_Pause;
	return QC_ERR_NONE;
}

int CFFBaseIO::Stop (void)
{
	m_sStatus = QCIO_Stop;
	return QC_ERR_NONE;
}

long long CFFBaseIO::GetSize (void)
{
	return m_llFileSize;
}

int CFFBaseIO::Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	return QC_ERR_IMPLEMENT;
}

int	CFFBaseIO::ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	return QC_ERR_IMPLEMENT;
}

int	CFFBaseIO::ReadSync (long long llPos, unsigned char * pBuff, int nSize, int nFlag)
{
	int nRead = nSize;
	int nRC = ReadAt (llPos, pBuff, nRead, true, nFlag);
	if (nRC == QC_ERR_NONE)
		return nRead;
	else if (nRC == QC_ERR_Disconnected)
		return QC_ERR_Disconnected;
	else if (nRC == QC_ERR_RETRY)
		return QC_ERR_Disconnected;
	else
		return -1;
}

int	CFFBaseIO::Write (unsigned char * pBuff, int & nSize, long long llPos)
{
	return QC_ERR_IMPLEMENT;
}

long long CFFBaseIO::SetPos (long long llPos, int nFlag)
{
	return -1;
}

long long CFFBaseIO::GetDownPos (void)
{
	return m_llDownPos;
}

long long CFFBaseIO::GetReadPos (void)
{
	return m_llReadPos;
}

int CFFBaseIO::GetSpeed (int nLastSecs)
{
	return 1024 * 1024 * 8;
}

QCIOType CFFBaseIO::GetType (void)
{
	return QC_IOTYPE_NONE;
}

bool CFFBaseIO::IsStreaming (void)
{
	return m_bIsStreaming;
}

int CFFBaseIO::GetParam (int nID, void * pParam)
{
	return QC_ERR_IMPLEMENT;
}

int CFFBaseIO::SetParam (int nID, void * pParam)
{
	if (nID == QCIO_PID_HTTP_HeadHost)
	{
		strcpy(m_szHostAddr, (char *)pParam);
		return QC_ERR_NONE;
	}
	return QC_ERR_IMPLEMENT;
}

