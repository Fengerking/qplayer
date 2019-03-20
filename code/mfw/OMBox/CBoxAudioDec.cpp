/*******************************************************************************
	File:		CBoxAudioDec.cpp

	Contains:	The audio dec box implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-13		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "CBoxAudioDec.h"
#include "CBoxMonitor.h"

#include "CGKAudioDec.h"
#include "CQCAudioDec.h"
#include "CQCSpeexDec.h"
#include "CQCAdpcmDec.h"

#ifdef __QC_USE_FFMPEG__
#include "CFFMpegAudioDec.h"
#endif //__QC_USE_FFMPEG__

#include "ULogFunc.h"

CBoxAudioDec::CBoxAudioDec(CBaseInst * pBaseInst, void * hInst)
	: CBoxBase (pBaseInst, hInst)
	, m_pDec (NULL)
	, m_pCurrBuff (NULL)
	, m_pFile(NULL)
{
	SetObjectName ("CBoxAudioDec");
	m_nBoxType = OMB_TYPE_FILTER;
	strcpy (m_szBoxName, "Audio Dec Box");

//	m_pFile = new CFileIO(pBaseInst);
	if (m_pFile != NULL)
		m_pFile->Open("c:\\temp\\test.pcm", 0, QCIO_FLAG_READ);
}

CBoxAudioDec::~CBoxAudioDec(void)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	QC_DEL_P(m_pDec);
	QC_DEL_P (m_pFile);
}

int CBoxAudioDec::SetSource (CBoxBase * pSource)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, 0);

	if (pSource == NULL)
		return QC_ERR_ARG;

	Stop ();

	QC_DEL_P (m_pDec);

	CBoxBase::SetSource (pSource);

	QC_AUDIO_FORMAT * pFmt = pSource->GetAudioFormat ();
	if (pFmt == NULL)
		return QC_ERR_AUDIO_DEC;

#ifdef __QC_USE_FFMPEG__
//	m_pDec = new CFFMpegAudioDec (m_hInst);
	m_pDec = new CGKAudioDec (m_hInst);
#else
	if (pFmt->nCodecID == QC_CODEC_ID_SPEEX)
		m_pDec = new CQCSpeexDec(m_pBaseInst, m_hInst);
	else if (pFmt->nCodecID == QC_CODEC_ID_PCM || 
			 pFmt->nCodecID == QC_CODEC_ID_LPCM ||
			 pFmt->nCodecID == QC_CODEC_ID_G711A ||
			 pFmt->nCodecID == QC_CODEC_ID_G711U)
		m_pDec = new CQCAdpcmDec(m_pBaseInst, m_hInst);
	else
		m_pDec = new CQCAudioDec(m_pBaseInst, m_hInst);
//		m_pDec = new CGKAudioDec(m_pBaseInst, m_hInst);
#endif //__QC_USE_FFMPEG__
	if (m_pDec == NULL)
		return QC_ERR_MEMORY;

	nRC = m_pDec->Init (pFmt);
	if (nRC != QC_ERR_NONE)
		return nRC;

	return QC_ERR_NONE;
}

long long CBoxAudioDec::SetPos (long long llPos)
{
	if (m_pDec != NULL)
		m_pDec->Flush ();
	m_llSeekPos = llPos;
	m_bEOS = false;
	return llPos;//CBoxBase::SetPos (llPos);
}

void CBoxAudioDec::Flush(void)
{
	if (m_pDec != NULL)
		m_pDec->Flush();
	return CBoxBase::Flush();
}

int CBoxAudioDec::ReadBuff (QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait)
{
	int nRC = QC_ERR_NEEDMORE;
	if (m_pDec == NULL)
		return QC_ERR_FAILED;
	
	if (m_pDec->GetBuff(ppBuffData) == QC_ERR_NONE)
	{
		if (m_pFile != NULL)
		{
			int nSize = (*ppBuffData)->uSize;
			m_pFile->Read((*ppBuffData)->pBuff, nSize, true, 0);
		}
		return QC_ERR_NONE;
	}

	if (m_pCurrBuff != NULL)
	{
		nRC = m_pDec->SetBuff (m_pCurrBuff);
		if (nRC == QC_ERR_RETRY || nRC == QC_ERR_NONE)
		{
			if (nRC == QC_ERR_NONE)
				m_pCurrBuff = NULL;	
			nRC = m_pDec->GetBuff (ppBuffData);
			if (m_pFile != NULL && nRC == QC_ERR_NONE)
			{
				int nSize = (*ppBuffData)->uSize;
				m_pFile->Read((*ppBuffData)->pBuff, nSize, true, 0);
			}
			if (m_bEOS)
			{
				if (*ppBuffData != NULL)
					(*ppBuffData)->uFlag |= QCBUFF_EOS;
				return QC_ERR_FINISH;
			}
			return nRC;
		}
		m_pCurrBuff = NULL;
		if (nRC < 0)
			return nRC;
	}

	m_pBuffInfo->nMediaType = QC_MEDIA_Audio;
	m_pBuffInfo->uFlag = 0;
	m_pBuffInfo->llTime = 0;
	while (m_nStatus == OMB_STATUS_RUN || m_nStatus == OMB_STATUS_PAUSE)
	{
		nRC = m_pBoxSource->ReadBuff (m_pBuffInfo, &m_pBuffData, bWait);
		if (nRC != QC_ERR_NONE || m_pBuffData == NULL)
		{
			if (nRC == QC_ERR_FINISH)
				m_bEOS = true;
			return nRC;
		}

		BOX_READ_BUFFER_REC_AUDIODEC
		m_pBuffInfo->llTime = m_pBuffData->llTime;

		if ((m_pBuffData->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
		{
			m_pBaseInst->m_pSetting->g_qcs_nAudioDecVlm = 0;
			QC_AUDIO_FORMAT * pFmt = (QC_AUDIO_FORMAT*) m_pBuffData->pFormat;
			if (pFmt && pFmt->nCodecID == QC_CODEC_ID_SPEEX)
			{
				QC_DEL_P(m_pDec);
				m_pDec = new CQCSpeexDec(m_pBaseInst, m_hInst);
			}
			else if (pFmt && (pFmt->nCodecID == QC_CODEC_ID_PCM ||
				pFmt->nCodecID == QC_CODEC_ID_LPCM ||
				pFmt->nCodecID == QC_CODEC_ID_G711A ||
				pFmt->nCodecID == QC_CODEC_ID_G711U))
			{
				QC_DEL_P(m_pDec);
				m_pDec = new CQCAdpcmDec(m_pBaseInst, m_hInst);
			}
			nRC = m_pDec->Init (pFmt);
			if (nRC != QC_ERR_NONE)
				return nRC;
		}

		nRC = m_pDec->SetBuff (m_pBuffData);
		if (nRC == QC_ERR_NONE)
			break;
		else if (nRC == QC_ERR_RETRY)
		{
			m_pCurrBuff = m_pBuffData;
			break;
		}
		else if (nRC == QC_ERR_NEEDMORE)
			continue;
		else
			return nRC;
	}

	nRC = m_pDec->GetBuff (ppBuffData);
	if ((m_pBuffData->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT && nRC == QC_ERR_NONE)
	{
		QC_AUDIO_FORMAT * pFmt = m_pDec->GetAudioFormat();
		(*ppBuffData)->uFlag |= QCBUFF_NEW_FORMAT;
		(*ppBuffData)->pFormat = pFmt;
	}

	if (m_pFile != NULL && nRC == QC_ERR_NONE)
	{
		int nSize = (*ppBuffData)->uSize;
		m_pFile->Read((*ppBuffData)->pBuff, nSize, true, 0);
	}
	return nRC;
}

int CBoxAudioDec::SetParam(int nID, void * pParam)
{
	if (nID == BOX_SET_AudioVolume && m_pDec != NULL)
		return m_pDec->SetVolume(*(int *)pParam);
	return CBoxBase::SetParam(nID, pParam);
}

QC_AUDIO_FORMAT * CBoxAudioDec::GetAudioFormat (int nID)
{
	if (m_pDec != NULL)
		return m_pDec->GetAudioFormat ();
	return CBoxBase::GetAudioFormat ();
}
