/*******************************************************************************
	File:		CFFMpegInIO.cpp

	Contains:	ffmpeg io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-07		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcDef.h"
#include "CFFMpegInIO.h"

CFFMpegInIO::CFFMpegInIO(void)
	: m_pAVIO(NULL)
	, m_pQCIO(NULL)
	, m_pBuffer(NULL)
	, m_nBuffSize(32768)
{
}

CFFMpegInIO::~CFFMpegInIO(void)
{
	Close ();
}

int	CFFMpegInIO::Open (QC_IO_Func * pQCIO, const char * pSource)
{
	if (pQCIO == NULL || pQCIO->hIO == NULL)
		return QC_ERR_ARG;

	if (m_pAVIO != NULL)
		av_free(m_pAVIO);
	m_pQCIO = pQCIO;
	if (m_pQCIO->GetSize(m_pQCIO->hIO) <= 0)
	{
		if (m_pQCIO->Open(m_pQCIO->hIO, pSource, 0, QCIO_FLAG_READ) != QC_ERR_NONE)
			return QC_ERR_FAILED;
	}

	if (m_pBuffer == NULL)
		m_pBuffer = (unsigned char *)av_malloc (m_nBuffSize);
//	m_pAVIO = avio_alloc_context(m_pBuffer, m_nBuffSize, AVIO_FLAG_READ, this, QCFF_Read, QCFF_Write, QCFF_Seek);
	m_pAVIO = avio_alloc_context(m_pBuffer, m_nBuffSize, 0, this, QCFF_Read, QCFF_Write, QCFF_Seek);
	if (m_pAVIO == NULL)
		return QC_ERR_FAILED;

	return QC_ERR_NONE;
}

int	CFFMpegInIO::Close (void)
{
	if (m_pAVIO != NULL)
	{
		av_freep (&m_pAVIO->buffer);
		m_pBuffer = NULL;
		av_freep (&m_pAVIO);
	}
	m_pAVIO = NULL;

	if (m_pQCIO == NULL || m_pQCIO->hIO == NULL)
		return QC_ERR_NONE;
	m_pQCIO->Close (m_pQCIO->hIO);

	return QC_ERR_NONE;
}

int	CFFMpegInIO::Read(uint8_t *buf, int buf_size)
{
	if (m_pQCIO == NULL || m_pQCIO->hIO == NULL)
		return -1;
	int nRead = buf_size;
	int nRC = m_pQCIO->Read(m_pQCIO->hIO, buf, nRead, true, 0);
	if (nRC != QC_ERR_NONE)
		return 0;
	return nRead;
}

int	CFFMpegInIO::Write(uint8_t *buf, int buf_size, long long llPos)
{
	if (m_pQCIO == NULL || m_pQCIO->hIO == NULL)
		return -1;
	int nRC = m_pQCIO->Write (m_pQCIO->hIO, buf, buf_size, llPos);
	return nRC;
}

int64_t	CFFMpegInIO::Seek(int64_t offset, int whence)
{
	if (whence == AVSEEK_SIZE)
		return m_pQCIO->GetSize(m_pQCIO->hIO);

	int nSeekFlag = QCIO_SEEK_BEGIN;
	if (whence == SEEK_SET)
		nSeekFlag = QCIO_SEEK_BEGIN;
	else if (whence == SEEK_CUR)
		nSeekFlag = QCIO_SEEK_CUR;
	else if (whence == SEEK_END)
		nSeekFlag = QCIO_SEEK_END;
	long long llPos = m_pQCIO->SetPos(m_pQCIO->hIO, offset, nSeekFlag);
	if (llPos >= 0)
		return 0;
	else
		return -1;
}

int	CFFMpegInIO::QCFF_Read (void *opaque, uint8_t *buf, int buf_size)
{
	CFFMpegInIO * pIO = (CFFMpegInIO *)opaque;
	return pIO->Read (buf, buf_size);
}

int	CFFMpegInIO::QCFF_Write (void *opaque, uint8_t *buf, int buf_size)
{
	CFFMpegInIO * pIO = (CFFMpegInIO *)opaque;
	return pIO->Write (buf, buf_size, 0);
}

int64_t	CFFMpegInIO::QCFF_Seek (void *opaque, int64_t offset, int whence)
{
	CFFMpegInIO * pIO = (CFFMpegInIO *)opaque;
	return pIO->Seek (offset, whence);
}

int	CFFMpegInIO::QCFF_Pasue (void *opaque, int pause)
{
	return 0;
}
