/*******************************************************************************
	File:		CBaseVideoDec.cpp

	Contains:	The base video dec implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CBaseVideoDec.h"

//#define QC_DUMP_DECFILE

CBaseVideoDec::CBaseVideoDec(CBaseInst * pBaseInst, void * hInst)
	: CBaseObject (pBaseInst)
	, m_hInst (hInst)
	, m_uBuffFlag (0)
	, m_pBuffData (NULL)
	, m_nDecCount (0)
	, m_pDumpFile(NULL)
{
	SetObjectName ("CBaseVideoDec");

	memset (&m_fmtVideo, 0, sizeof (m_fmtVideo));
	memset (&m_buffVideo, 0, sizeof (m_buffVideo));

#ifdef QC_DUMP_DECFILE
	char	szDumpFile[1024];
	strcpy(szDumpFile, "c:/temp/0000.264");
	m_pDumpFile = new CFileIO(m_pBaseInst);
	m_pDumpFile->Open(szDumpFile, 0, QCIO_FLAG_WRITE);
#endif // QC_DUMP_DECFILE
}

CBaseVideoDec::~CBaseVideoDec(void)
{
	Uninit();
	QC_DEL_P(m_pDumpFile);
}

int CBaseVideoDec::Init (QC_VIDEO_FORMAT * pFmt)
{
	memcpy (&m_fmtVideo, pFmt, sizeof (m_fmtVideo));
	m_fmtVideo.pHeadData = NULL;
	m_fmtVideo.nHeadSize = 0;
	m_fmtVideo.pPrivateData = NULL;

	m_uBuffFlag = 0;

	return QC_ERR_NONE;
}

int CBaseVideoDec::Uninit (void)
{
	return QC_ERR_NONE;
}

int CBaseVideoDec::SetBuff (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL)
		return QC_ERR_ARG;
	if ((pBuff->uFlag & QCBUFF_EOS) == QCBUFF_EOS)
		m_uBuffFlag |= QCBUFF_EOS;
	if ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
		m_uBuffFlag |= QCBUFF_NEW_POS;
	if ((pBuff->uFlag & QCBUFF_DISCONNECT) == QCBUFF_DISCONNECT)
		m_uBuffFlag |= QCBUFF_DISCONNECT;

	return QC_ERR_NONE;
}

int CBaseVideoDec::GetBuff (QC_DATA_BUFF ** ppBuff)
{
	if (ppBuff == NULL || *ppBuff == NULL)
		return QC_ERR_ARG;
	if ((m_uBuffFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
		(*ppBuff)->uFlag |= QCBUFF_NEW_POS;
	if ((m_uBuffFlag & QCBUFF_EOS) == QCBUFF_EOS)
		(*ppBuff)->uFlag |= QCBUFF_EOS;
	if ((m_uBuffFlag & QCBUFF_DISCONNECT) == QCBUFF_DISCONNECT)
		(*ppBuff)->uFlag |= QCBUFF_DISCONNECT;
	m_uBuffFlag = 0;

	return QC_ERR_NONE;
}

int	CBaseVideoDec::Start (void)
{
	return QC_ERR_NONE;
}

int CBaseVideoDec::Pause (void)
{
	return QC_ERR_NONE;
}

int	CBaseVideoDec::Stop (void)
{
	return QC_ERR_NONE;
}

int CBaseVideoDec::Flush (void)
{
	return QC_ERR_NONE;
}

int CBaseVideoDec::PushRestOut (void)
{
	return QC_ERR_NONE;
}
