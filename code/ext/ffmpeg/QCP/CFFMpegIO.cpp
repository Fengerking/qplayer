/*******************************************************************************
	File:		CFFMpegIO.cpp

	Contains:	local file io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-08		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "CFFMpegIO.h"

CFFMpegIO::CFFMpegIO(void)
	: CFFBaseIO ()
	, m_pFFIO (NULL)
{
}

CFFMpegIO::~CFFMpegIO(void)
{
	Close ();
}

int CFFMpegIO::Open (const char * pURL, long long llOffset, int nFlag)
{
	Close();

	int nFFFlag = AVIO_FLAG_READ;
	if (nFlag & QCIO_FLAG_READ)
		nFFFlag = AVIO_FLAG_READ;
	else if (nFlag & QCIO_FLAG_WRITE)
		nFFFlag = AVIO_FLAG_WRITE;
	else
		nFFFlag = AVIO_FLAG_WRITE | AVIO_FLAG_READ;

	int nRC = avio_open(&m_pFFIO, pURL, nFFFlag);
	if (nRC >= 0 && llOffset > 0)
		SetPos (llOffset, QCIO_SEEK_BEGIN);

	if (nRC < 0)
		Close ();

	m_llFileSize = avio_size (m_pFFIO);
	m_llReadPos = 0;

	return nRC >= 0 ? QC_ERR_NONE : QC_ERR_FAILED;
}

int CFFMpegIO::Reconnect (const char * pNewURL, long long llOffset)
{
	m_llReadPos = llOffset;
	return QC_ERR_NONE;
}

int CFFMpegIO::Close (void)
{
	if (m_pFFIO != NULL)
		avio_close (m_pFFIO);
	m_pFFIO = NULL;

	return QC_ERR_NONE;
}

int CFFMpegIO::Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	if (m_pFFIO == NULL)
		return QC_ERR_STATUS;

	int nRead = avio_read(m_pFFIO, pBuff, nSize);
	if (nRead == nSize)
	{
		nSize = nRead;
		return QC_ERR_NONE;
	}
	else if (nRead > 0)
	{
		nSize = nRead;
		return QC_ERR_FINISH;
	}
	else if (nRead == 0)
	{
		nSize = 0;
		return QC_ERR_RETRY;
	}
	return QC_ERR_FAILED;
}

int	CFFMpegIO::ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	if (llPos != m_llReadPos)
		SetPos (llPos, QCIO_SEEK_BEGIN);
	return Read (pBuff, nSize, bFull, nFlag);
}

int	CFFMpegIO::Write (unsigned char * pBuff, int nSize)
{
	if (m_pFFIO == NULL)
		return QC_ERR_STATUS;
	avio_write (m_pFFIO, (const unsigned char *)pBuff, nSize);
	return nSize;
}

long long CFFMpegIO::SetPos (long long llPos, int nFlag)
{
	int whence = SEEK_SET;
	if (nFlag == QCIO_SEEK_BEGIN)
	{
		m_llReadPos = llPos;
		whence = SEEK_SET;
	}
	else if (nFlag == QCIO_SEEK_CUR)
	{
		m_llReadPos += llPos;
		whence = SEEK_CUR;
	}
	else if (nFlag == QCIO_SEEK_END)
	{
		m_llReadPos = m_llFileSize - llPos;
		whence = SEEK_END;
	}
	if (m_llReadPos > m_llFileSize)
		return QC_ERR_STATUS;

	m_llReadPos = avio_seek(m_pFFIO, llPos, whence);

	return m_llReadPos;
}

QCIOType CFFMpegIO::GetType (void)
{
	return QC_IOTYPE_FILE;
}

int CFFMpegIO::GetParam (int nID, void * pParam)
{
	return QC_ERR_IMPLEMENT;
}

int CFFMpegIO::SetParam (int nID, void * pParam)
{
	return QC_ERR_IMPLEMENT;
}

