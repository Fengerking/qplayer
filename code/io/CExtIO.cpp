/*******************************************************************************
	File:		CExtIO.cpp

	Contains:	ffmpeg io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-08		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CExtIO.h"

#include "ULogFunc.h"
#include "USystemFunc.h"

CExtIO::CExtIO(CBaseInst * pBaseInst)
	: CBaseIO (pBaseInst)
	, m_pMemData(NULL)
{
	SetObjectName ("CExtIO");
	m_llFileSize = 0X7FFFFFFFFFFFFFFF;
}

CExtIO::~CExtIO(void)
{
	Close ();
}

int CExtIO::Open (const char * pURL, long long llOffset, int nFlag)
{
	int nRC = QC_ERR_NONE;
	if (m_pMemData == NULL)
		m_pMemData = new CMemFile(m_pBaseInst);

	m_llDownPos = 0;
	m_llReadPos = 0;
	return nRC;
}

int CExtIO::Close (void)
{
	QC_DEL_P(m_pMemData);
	return QC_ERR_NONE;
}

int CExtIO::Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	int nRC = QC_ERR_NONE;
	if (m_llReadPos >= m_llFileSize)
		return QC_ERR_FINISH;
	if (m_llReadPos + nSize > m_llFileSize)
		nSize = (int)(m_llFileSize - m_llReadPos);

	nRC = QC_ERR_NONE;
	int nRead = nSize;
	if (!bFull)
	{
		CAutoLock lock(&m_mtLock);
		nSize = m_pMemData->ReadBuff(m_llReadPos, (char *)pBuff, nRead, false, nFlag);
		m_llReadPos += nSize;
		if (m_sStatus != QCIO_Run && m_sStatus != QCIO_Pause)
			return QC_ERR_STATUS;
		if (nSize == 0)
		{
			qcSleep(5000);
			return QC_ERR_RETRY;
		}
	}
	else
	{
		int nMemBuffSize = m_pMemData->GetBuffSize(m_llReadPos);
		while (nMemBuffSize < nSize)
		{
			qcSleep(1000);
			if (m_llReadPos + nSize > m_llFileSize)
			{
				nSize = (int)(m_llFileSize - m_llReadPos);
				nRead = nSize;
			}
			if (m_pBaseInst->m_bForceClose == true || m_llReadPos + nMemBuffSize > m_llFileSize)
				return QC_ERR_FINISH;

			nMemBuffSize = m_pMemData->GetBuffSize(m_llReadPos);
		}

		CAutoLock lock(&m_mtLock);
		nSize = m_pMemData->ReadBuff(m_llReadPos, (char *)pBuff, nRead, true, nFlag);
		m_llReadPos += nSize;
		if (nSize != nRead)
		{
			if (nFlag == QCIO_READ_AUDIO || nFlag == QCIO_READ_VIDEO)
				return QC_ERR_RETRY;
		}
	}
	return nRC;
}

int	CExtIO::ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	m_llReadPos = llPos;
	return Read(pBuff, nSize, bFull, nFlag);
}

long long CExtIO::SetPos (long long llPos, int nFlag)
{
	int nRC = QC_ERR_NONE;
	CAutoLock lock(&m_mtLock);
	m_llSeekPos = llPos;
	if (m_llReadPos == llPos)
		return llPos;

	m_pMemData->SetPos(llPos);
	m_llReadPos = llPos;
	return nRC;
}

int CExtIO::Write(unsigned char * pBuff, int & nSize, long long llPos)
{
	CAutoLock lock(&m_mtLock);
	if (m_pMemData == NULL)
		return QC_ERR_RETRY;
	if (llPos >= 0)
		m_llDownPos = llPos;
	m_pMemData->FillBuff(m_llDownPos, (char *)pBuff, nSize);
	m_llDownPos += nSize;
	return QC_ERR_NONE;
}

QCIOType CExtIO::GetType (void)
{
	return QC_IOTYPE_EXTIO;
}

int CExtIO::GetParam (int nID, void * pParam)
{
	int nRC = QC_ERR_IMPLEMENT;

	return nRC;
}

int CExtIO::SetParam (int nID, void * pParam)
{
	int nRC = QC_ERR_IMPLEMENT;

	return nRC;
}

