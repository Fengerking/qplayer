/*******************************************************************************
	File:		CQCVideoDec.cpp

	Contains:	the qc video dec implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcPlayer.h"

#include "CQCVideoDec.h"
#include "ULogFunc.h"

CQCVideoDec::CQCVideoDec(CBaseInst * pBaseInst, void * hInst)
	: CBaseVideoDec(pBaseInst, hInst)
	, m_hLib (NULL)
	, m_bFirstFrame(false)
{
	SetObjectName ("CQCVideoDec");
	memset (&m_fmtVideo, 0, sizeof (m_fmtVideo));
	memset (&m_fCodec, 0, sizeof (m_fCodec));
	m_fCodec.nAVType = 1;
}

CQCVideoDec::~CQCVideoDec(void)
{
	Uninit ();
}

int CQCVideoDec::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;

	Uninit ();
	int nRC = 0;
#ifdef __QC_LIB_ONE__
	nRC = qcCreateDecoder (&m_fCodec, pFmt);
#else
	if (m_pBaseInst->m_hLibCodec == NULL)
		m_hLib = (qcLibHandle)qcLibLoad("qcCodec", 0);
	else
		m_hLib = m_pBaseInst->m_hLibCodec;
	if (m_hLib == NULL)
		return QC_ERR_FAILED;
	QCCREATEDECODER pCreate = (QCCREATEDECODER)qcLibGetAddr (m_hLib, "qcCreateDecoder", 0);
	if (pCreate == NULL)
		return QC_ERR_FAILED;
	nRC = pCreate (&m_fCodec, pFmt);
#endif // __QC_LIB_ONE__
	if (nRC != QC_ERR_NONE)
	{
		QCLOGW ("Create QC video dec failed. err = 0X%08X", nRC);
		return nRC;
	}
	int nLogLevel = 0;
	m_fCodec.SetParam(m_fCodec.hCodec, QCPLAY_PID_Log_Level, &nLogLevel);

	if (pFmt->pHeadData != NULL && pFmt->nHeadSize > 0)
	{
		QC_DATA_BUFF buffData;
		memset (&buffData, 0, sizeof (buffData));
		buffData.pBuff = pFmt->pHeadData;
		buffData.uSize = pFmt->nHeadSize;
		buffData.uFlag = QCBUFF_HEADDATA;
		m_fCodec.SetBuff (m_fCodec.hCodec, &buffData);

		if (m_pDumpFile != NULL)
			m_pDumpFile->Write((unsigned char *)pFmt->pHeadData, pFmt->nHeadSize);
	}
	memcpy (&m_fmtVideo, pFmt, sizeof (m_fmtVideo));
	m_fmtVideo.pHeadData = NULL;
	m_fmtVideo.nHeadSize = 0;
	m_fmtVideo.pPrivateData = NULL;

	m_uBuffFlag = 0;
	m_nDecCount = 0;
	m_bFirstFrame = false;
	return QC_ERR_NONE;
}

int CQCVideoDec::Uninit (void)
{
#ifdef __QC_LIB_ONE__
	if (m_fCodec.hCodec != NULL)
		qcDestroyDecoder(&m_fCodec);
#else
	if (m_hLib != NULL)
	{
		QCDESTROYDECODER fDestroy = (QCDESTROYDECODER)qcLibGetAddr (m_hLib, "qcDestroyDecoder", 0);
		if (fDestroy != NULL)
			fDestroy (&m_fCodec);
		if (m_pBaseInst->m_hLibCodec == NULL)
			qcLibFree(m_hLib, 0);
		m_hLib = NULL;
	}
#endif // __QC_LIB_ONE__
	m_pBuffData = NULL;
	return QC_ERR_NONE;
}

int CQCVideoDec::Flush (void)
{
	CAutoLock lock (&m_mtBuffer);
	if (m_fCodec.hCodec != NULL)
		m_fCodec.Flush (m_fCodec.hCodec);
	return QC_ERR_NONE;
}

int CQCVideoDec::PushRestOut (void)
{
	QC_DATA_BUFF buffFlush;
	memset (&buffFlush, 0, sizeof (QC_DATA_BUFF));
	m_fCodec.SetBuff (m_fCodec.hCodec, &buffFlush);
	return QC_ERR_NONE;
}

int CQCVideoDec::SetBuff (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || m_fCodec.hCodec == NULL)
		return QC_ERR_ARG;

	CAutoLock lock (&m_mtBuffer);
	CBaseVideoDec::SetBuff (pBuff);

	if ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
	{
		if (m_nDecCount > 0)
			Flush ();
	}
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		if (pFmt != NULL && pFmt->pHeadData != NULL)
			InitNewFormat (pFmt);
	}
	if (m_pBuffData != NULL)
		m_uBuffFlag = pBuff->uFlag;
/*
	if (m_nDecCount == 0)
	{
		if ((pBuff->uFlag & QCBUFF_HEADDATA) == 0)
			m_bFirstFrame = true;
		else if (pBuff->uSize > 1024)
			m_bFirstFrame = true;
	}
*/
	if (m_pDumpFile != NULL)
		m_pDumpFile->Write((unsigned char *)pBuff->pBuff, pBuff->uSize);

	return m_fCodec.SetBuff (m_fCodec.hCodec, pBuff);
}

int CQCVideoDec::GetBuff (QC_DATA_BUFF ** ppBuff)
{
	if (ppBuff == NULL || m_fCodec.hCodec == NULL)
		return QC_ERR_ARG;

	CAutoLock lock (&m_mtBuffer);

	if (m_pBuffData != NULL)
		m_pBuffData->uFlag = 0;
	int nRC = m_fCodec.GetBuff (m_fCodec.hCodec, &m_pBuffData);
	if (nRC != QC_ERR_NONE)
	{
		if (m_bFirstFrame)
		{
			PushRestOut();
			nRC = m_fCodec.GetBuff(m_fCodec.hCodec, &m_pBuffData);
			m_bFirstFrame = false;
		}
		if (nRC != QC_ERR_NONE)
			return QC_ERR_FAILED;
	}

	QC_VIDEO_BUFF * pBufVideo = (QC_VIDEO_BUFF *)m_pBuffData->pBuffPtr;
	bool bNewFormat = false;
	if (m_fmtVideo.nWidth != pBufVideo->nWidth || m_fmtVideo.nHeight != pBufVideo->nHeight)
		bNewFormat = true;
	if (!bNewFormat)
	{
		if (m_fmtVideo.nNum != pBufVideo->nRatioNum)
		{
			if (m_fmtVideo.nNum > 1 || pBufVideo->nRatioNum > 1)
				bNewFormat = true;
		}
		if (m_fmtVideo.nDen != pBufVideo->nRatioDen)
		{
			if (m_fmtVideo.nDen > 1 || pBufVideo->nRatioDen > 1)
				bNewFormat = true;
		}
	}
	if (bNewFormat)
	{
		m_fmtVideo.nWidth = pBufVideo->nWidth;
		m_fmtVideo.nHeight =pBufVideo->nHeight;
		m_fmtVideo.nNum = pBufVideo->nRatioNum;
		m_fmtVideo.nDen = pBufVideo->nRatioDen;
		m_pBuffData->uFlag |= QCBUFF_NEW_FORMAT;
		m_pBuffData->pFormat = &m_fmtVideo;
	}

	CBaseVideoDec::GetBuff (&m_pBuffData);
	*ppBuff = m_pBuffData;
	m_nDecCount++;

	return QC_ERR_NONE;
}

int CQCVideoDec::InitNewFormat (QC_VIDEO_FORMAT * pFmt)
{
	if (m_hLib == NULL)
		return Init (pFmt);

	if (pFmt->pHeadData != NULL && pFmt->nHeadSize > 0)
	{
		QC_DATA_BUFF buffData;
		memset (&buffData, 0, sizeof (buffData));
		buffData.pBuff = pFmt->pHeadData;
		buffData.uSize = pFmt->nHeadSize;
		buffData.uFlag = QCBUFF_HEADDATA;
		m_fCodec.SetBuff (m_fCodec.hCodec, &buffData);
	}

	return QC_ERR_NONE;
}
