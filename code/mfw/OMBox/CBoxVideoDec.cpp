/*******************************************************************************
	File:		CBoxVideoDec.cpp

	Contains:	The video dec box implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-13		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcPlayer.h"
#include "CBoxVideoDec.h"

#include "CBoxMonitor.h"

#include "CQCVideoDec.h"
#include "CGKVideoDec.h"
#ifdef __QC_USE_FFMPEG__
#include "CFFMpegVideoDec.h"
#endif //__QC_USE_FFMPEG__

#ifdef __QC_OS_IOS__
#include "CVTBVideoDec.h"
#endif

#include "USystemFunc.h"
#include "ULogFunc.h"

CBoxVideoDec::CBoxVideoDec(CBaseInst * pBaseInst, void * hInst)
	: CBoxBase (pBaseInst, hInst)
	, m_pDec (NULL)
	, m_nOutCount (0)
	, m_pNewBuff (NULL)
{
	SetObjectName ("CBoxVideoDec");
	m_nBoxType = OMB_TYPE_FILTER;
	strcpy (m_szBoxName, "Video Dec Box");
	memset (&m_fmtVideo, 0, sizeof (QC_VIDEO_FORMAT));
}

CBoxVideoDec::~CBoxVideoDec(void)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	QC_DEL_P(m_pDec);
}

int CBoxVideoDec::SetSource (CBoxBase * pSource)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, 0);

	if (pSource == NULL)
		return QC_ERR_ARG;

	Stop ();

	QC_DEL_P (m_pDec);

	CBoxBase::SetSource (pSource);

	QC_VIDEO_FORMAT * pFmt = pSource->GetVideoFormat ();
	if (pFmt == NULL)
		return QC_ERR_VIDEO_DEC;

	if (pFmt->nCodecID == QC_CODEC_ID_NONE && pFmt->nPrivateFlag == 0)
	{
		m_fmtVideo.nWidth = pFmt->nWidth;
		m_fmtVideo.nHeight = pFmt->nHeight;
		return QC_ERR_NONE;
	}
	
	nRC = CreateDec (pFmt);
	return nRC;
}

long long CBoxVideoDec::SetPos (long long llPos)
{
    if (m_pDec != NULL)
        m_pDec->Flush ();
	m_llSeekPos = llPos;
	m_bEOS = false;
	return llPos;//CBoxBase::SetPos (llPos);
}

void CBoxVideoDec::Flush(void)
{
	if (m_pDec != NULL)
		m_pDec->Flush();
	return CBoxBase::Flush();
}

int CBoxVideoDec::ReadBuff (QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait)
{
	if (pBuffInfo == NULL|| ppBuffData == NULL)
		return QC_ERR_FAILED;

	m_pBuffInfo->uFlag |= pBuffInfo->uFlag;

	int nRC = QC_ERR_NONE;
	*ppBuffData = NULL;
	if (m_bEOS && m_pDec != NULL)
	{
		m_pDec->PushRestOut();
		nRC = m_pDec->GetBuff(ppBuffData);
		if (nRC != QC_ERR_NONE)
		{
			pBuffInfo->uFlag |= QCBUFF_EOS;
			return QC_ERR_FINISH;
		}
		return nRC;
	}

	if (m_pNewBuff == NULL)
	{
		m_pBuffInfo->nMediaType = QC_MEDIA_Video;
		m_pBuffInfo->llTime = pBuffInfo->llTime;
		nRC = m_pBoxSource->ReadBuff (m_pBuffInfo, &m_pBuffData, bWait);
		if (nRC != QC_ERR_NONE || m_pBuffData == NULL)
		{
			if (nRC == QC_ERR_FINISH)
			{
				m_bEOS = true;
				nRC = QC_ERR_RETRY;
			}
			return nRC;
		}

		if (m_pDec == NULL)
		{
			if ((m_pBuffData->uFlag & QCBUFF_NEW_FORMAT) == 0)
				return QC_ERR_FAILED;
			QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)m_pBuffData->pFormat;
			m_fmtVideo.nCodecID = pFmt->nCodecID;
			if (CreateDec (&m_fmtVideo) != QC_ERR_NONE)
				return QC_ERR_FAILED;
		}

		m_pBuffData->llDelay = pBuffInfo->llDelay;
		if ((m_pBuffData->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT && m_nOutCount > 0)
		{
			m_pNewBuff = m_pBuffData;
			m_pDec->PushRestOut ();
		}
		else
		{
			BOX_READ_BUFFER_REC_VIDEODEC
			m_pBuffInfo->llTime = m_pBuffData->llTime;

			nRC = m_pDec->SetBuff (m_pBuffData);
			if (nRC != QC_ERR_NONE)
				return nRC;
		}
	}

	if (m_pDec == NULL)
		return QC_ERR_NONE;
	nRC = m_pDec->GetBuff (ppBuffData);
	if (nRC == QC_ERR_NONE)
	{
		m_pBuffInfo->uFlag = 0;
		m_nOutCount++;
	}
	else if (m_pNewBuff != NULL)
	{
		nRC = m_pDec->SetBuff (m_pNewBuff);
		m_pNewBuff = NULL;
		if (nRC != QC_ERR_NONE)
			return nRC;
		nRC = m_pDec->GetBuff (ppBuffData);
	}

	return nRC;
}

QC_VIDEO_FORMAT * CBoxVideoDec::GetVideoFormat (int nID)
{
	if (m_pDec != NULL)
		return m_pDec->GetVideoFormat ();
	return CBoxBase::GetVideoFormat ();
}

int	CBoxVideoDec::CreateDec (QC_VIDEO_FORMAT * pFmt)
{

#ifdef __QC_USE_FFMPEG__
//	if (pFmt->nCodecID == QC_CODEC_ID_H265)
//		m_pDec = new COpenHEVCDec (m_hInst);
//	else
		m_pDec = new CFFMpegVideoDec (m_hInst);
#else
#ifdef __QC_OS_IOS__
    int nFlag = *(int*)m_hInst;
    if((nFlag & QCPLAY_OPEN_VIDDEC_HW) == QCPLAY_OPEN_VIDDEC_HW)
		m_pDec = new CVTBVideoDec (m_pBaseInst, m_hInst);
    else
		m_pDec = new CQCVideoDec (m_pBaseInst, m_hInst);
#else
	m_pDec = new CQCVideoDec(m_pBaseInst, m_hInst);
//	m_pDec = new CGKVideoDec(m_hInst);
#endif // __QC_OS_IOS__
#endif // __QC_USE_FFMPEG__

	if (m_pDec == NULL)
		return QC_ERR_MEMORY;

	int nRC = m_pDec->Init (pFmt);
    
#ifdef __QC_OS_IOS__
    if (nRC != QC_ERR_NONE && (nFlag & QCPLAY_OPEN_VIDDEC_HW) == QCPLAY_OPEN_VIDDEC_HW)
    {
        QC_DEL_P(m_pDec);
        m_pDec = new CQCVideoDec (m_pBaseInst, m_hInst);
        nRC = m_pDec->Init (pFmt);
    }
#endif
    
	if (nRC != QC_ERR_NONE)
		return nRC;

	m_nOutCount = 0;
	return QC_ERR_NONE;
}
