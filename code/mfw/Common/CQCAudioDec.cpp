/*******************************************************************************
	File:		CQCAudioDec.cpp

	Contains:	The qc audio dec implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-02		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcPlayer.h"

#include "CQCAudioDec.h"
#include "ULogFunc.h"

CQCAudioDec::CQCAudioDec(CBaseInst * pBaseInst, void * hInst)
	: CBaseAudioDec(pBaseInst, hInst)
	, m_hLib (NULL)
	, m_nScale(0X7FFF)
	, m_pPCMData (NULL)
	, m_nPCMSize (1024 * 64)
	, m_pFrmInfo (NULL)
	, m_nChannels (0)
	, m_llFirstFrameTime(-1)
{
	SetObjectName ("CQCAudioDec");
	memset (&m_fmtAudio, 0, sizeof (m_fmtAudio));
	memset (&m_fCodec, 0, sizeof (m_fCodec));
	m_fCodec.nAVType = 0;
}

CQCAudioDec::~CQCAudioDec(void)
{
	Uninit ();
	QC_DEL_A (m_pPCMData);
}

int CQCAudioDec::Init (QC_AUDIO_FORMAT * pFmt)
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
		QCLOGW ("Create QC audio dec failed. err = 0X%08X", nRC);
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
	}
	memcpy (&m_fmtAudio, pFmt, sizeof (m_fmtAudio));
	m_fmtAudio.pHeadData = NULL;
	m_fmtAudio.nHeadSize = 0;
	m_fmtAudio.pPrivateData = NULL;
	m_nChannels = m_fmtAudio.nChannels;
	if (m_fmtAudio.nChannels > 2)
		m_fmtAudio.nChannels = 2;

	m_uBuffFlag = 0;
	m_nDecCount = 0;

	m_pBaseInst->m_pSetting->g_qcs_nAudioDecVlm = 1;

	return QC_ERR_NONE;
}

int CQCAudioDec::Uninit(void)
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
			qcLibFree (m_hLib, 0);
		m_hLib = NULL;
	}
#endif // __QC_LIB_ONE__
	m_pBuffData = NULL;
	return QC_ERR_NONE;
}

int CQCAudioDec::Flush (void)
{
	CAutoLock lock (&m_mtBuffer);
	if (m_fCodec.hCodec != NULL)
		m_fCodec.Flush (m_fCodec.hCodec);
    m_llFirstFrameTime = -1;
	return QC_ERR_NONE;
}

int CQCAudioDec::PushRestOut (void)
{
	QC_DATA_BUFF buffFlush;
	memset (&buffFlush, 0, sizeof (QC_DATA_BUFF));
	m_fCodec.SetBuff (m_fCodec.hCodec, &buffFlush);
	return QC_ERR_NONE;
}

int CQCAudioDec::SetBuff (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || m_fCodec.hCodec == NULL)
		return QC_ERR_ARG;

	CAutoLock lock (&m_mtBuffer);
	CBaseAudioDec::SetBuff (pBuff);

	if ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
	{
		if (m_nDecCount > 0)
			Flush ();
	}
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_AUDIO_FORMAT * pFmt = (QC_AUDIO_FORMAT *)pBuff->pFormat;
		if (pFmt != NULL && ((m_fmtAudio.nSampleRate == 0 || m_fmtAudio.nChannels == 0) || (pFmt->nCodecID != m_fmtAudio.nCodecID)))
		{
			pFmt->nChannels = 0;
			pFmt->nSampleRate = 0;
			Init(pFmt);
		}
	}
	if (m_pBuffData != NULL)
		m_uBuffFlag = pBuff->uFlag;
	if (m_llFirstFrameTime == -1)
        m_llFirstFrameTime = pBuff->llTime;
    
	m_fCodec.SetBuff (m_fCodec.hCodec, pBuff);
	return QC_ERR_NONE;
}

int CQCAudioDec::GetBuff (QC_DATA_BUFF ** ppBuff)
{
	if (ppBuff == NULL || m_fCodec.hCodec == NULL)
		return QC_ERR_ARG;

	CAutoLock lock (&m_mtBuffer);
	if (m_pBuffData != NULL)
		m_pBuffData->uFlag = 0;
	int nRC = m_fCodec.GetBuff (m_fCodec.hCodec, &m_pBuffData);
	if (nRC != QC_ERR_NONE)
		return QC_ERR_FAILED;

	if ((m_pBuffData->uFlag & QCBUFF_NEW_FORMAT) && (m_pBuffData->pFormat != NULL))
	{
		QC_AUDIO_FORMAT * pFmt = (QC_AUDIO_FORMAT *)m_pBuffData->pFormat;
		m_nChannels = pFmt->nChannels;
		if (m_fmtAudio.nChannels != pFmt->nChannels || m_fmtAudio.nSampleRate != pFmt->nSampleRate)
		{
			m_fmtAudio.nChannels = pFmt->nChannels;
			if (m_fmtAudio.nChannels > 2)
				m_fmtAudio.nChannels = 2;
			m_fmtAudio.nSampleRate =pFmt->nSampleRate;
			m_pBuffData->pFormat = &m_fmtAudio;
		}
		else
		{
			m_pBuffData->uFlag = m_pBuffData->uFlag & ~QCBUFF_NEW_FORMAT;
		}
	}
	ConvertData ();
	CBaseAudioDec::GetBuff (&m_pBuffData);
	*ppBuff = m_pBuffData;
	m_nDecCount++;
    
    if(m_llFirstFrameTime >= 0)
    {
        m_pBuffData->llTime = m_llFirstFrameTime;
        m_llFirstFrameTime = -2;
    }

	return QC_ERR_NONE;
}

int CQCAudioDec::InitNewFormat (QC_AUDIO_FORMAT * pFmt)
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

int CQCAudioDec::SetVolume(int nVolume)
{ 
	m_nVolume = nVolume; 
	if (m_nVolume > 100)
		m_nVolume = 100;
	else if (m_nVolume < 0)
		m_nVolume = 0;
	m_nScale = 0X7FFF * m_nVolume / 100;
	return QC_ERR_NONE;
}

int CQCAudioDec::ConvertData (void)
{
	if (m_pBuffData == NULL)
		return QC_ERR_FAILED;

	m_nScale = 0X7FFF * m_pBaseInst->m_pSetting->g_qcs_nAudioVolume / 100;

	m_pFrmInfo = (QC_AUDIO_FRAME *)m_pBuffData->pData;
	if (m_pFrmInfo == NULL)
		return QC_ERR_FAILED;
	if (m_pFrmInfo->nFormat == QA_SAMPLE_FMT_S16)
		return QC_ERR_NONE;

	if (m_nPCMSize < m_pFrmInfo->nNBSamples * 16)
	{
		m_nPCMSize = m_pFrmInfo->nNBSamples * 16;
		QC_DEL_A(m_pPCMData);
	}
	if (m_pPCMData == NULL)
		m_pPCMData = new unsigned char[m_nPCMSize];

	int				i = 0;
	int				nBlocks = m_pFrmInfo->nNBSamples;
	int				nVolume = 0;
	if (m_pFrmInfo->nFormat == QA_SAMPLE_FMT_U8P)
	{
		unsigned char * pLeft = (unsigned char *)m_pFrmInfo->pDataBuff[0];
		unsigned char * pRight = (unsigned char *)m_pFrmInfo->pDataBuff[1];
		short *			pAudio = (short *)m_pPCMData;
		for (i = 0; i < nBlocks; i++)
		{
			*pAudio++ = (short)(*pLeft++ * 256) * m_nVolume / 100;
			if (m_fmtAudio.nChannels > 1)
				*pAudio++ = (short)(*pRight++ * 256) * m_nVolume / 100;
		}
	}
	else if (m_pFrmInfo->nFormat == QA_SAMPLE_FMT_S16P)
	{
		short * pLeft = (short *)m_pFrmInfo->pDataBuff[0];
		short * pRight = (short *)m_pFrmInfo->pDataBuff[1];
		short * pAudio = (short *)m_pPCMData;
		for (i = 0; i < nBlocks; i++)
		{
			*pAudio++ = (short)(*pLeft++) * m_nVolume / 100;
			if (m_fmtAudio.nChannels > 1)
				*pAudio++ = (short)(*pRight++) * m_nVolume / 100;
		}
	}
	else if (m_pFrmInfo->nFormat == QA_SAMPLE_FMT_S32P)
	{
		int *	pLeft = (int *)m_pFrmInfo->pDataBuff[0];
		int *	pRight = (int *)m_pFrmInfo->pDataBuff[1];
		short * pAudio = (short *)m_pPCMData;
		for (i = 0; i < nBlocks; i++)
		{
			*pAudio++ = (short)(*pLeft++) * m_nVolume / 100;
			if (m_fmtAudio.nChannels > 1)
				*pAudio++ = (short)(*pRight++) * m_nVolume / 100;
		}
	}	
	else if (m_pFrmInfo->nFormat == QA_SAMPLE_FMT_FLTP)
	{
		if (m_nChannels <= 2)
		{
			float * pLeft = (float *)m_pFrmInfo->pDataBuff[0];
			float * pRight = (float *)m_pFrmInfo->pDataBuff[1];
			short * pAudio = (short *)m_pPCMData;
			for (i = 0; i < nBlocks; i++)
			{
				nVolume = (*pLeft++ * m_nScale);
				if (nVolume > 0X7FFF)
					nVolume = 0X7FFF;
				else if (nVolume < -0X7FFF)
					nVolume = -0X7FFF;
				*pAudio++ = (short)nVolume;
				//*pAudio++ = (short)(*pLeft++ * m_nScale);
				if (m_fmtAudio.nChannels > 1)
				{
					nVolume = (*pRight++ * m_nScale);
					if (nVolume > 0X7FFF)
						nVolume = 0X7FFF;
					else if (nVolume < -0X7FFF)
						nVolume = -0X7FFF;
					*pAudio++ = (short)nVolume;
				//	*pAudio++ = (short)(*pRight++ * m_nScale);
				}
			}
		}
		else if (m_nChannels == 6 || m_nChannels == 5)
		{
			float * pLeft1 = (float *)m_pFrmInfo->pDataBuff[0];
			float * pRight1 = (float *)m_pFrmInfo->pDataBuff[1];
			float * pFront = (float *)m_pFrmInfo->pDataBuff[2];
			float * pLeft2 = (float *)m_pFrmInfo->pDataBuff[4];
			float * pRight2 = (float *)m_pFrmInfo->pDataBuff[5];
			if (m_nChannels == 5)
			{
				pLeft2 = (float *)m_pFrmInfo->pDataBuff[3];
				pRight2 = (float *)m_pFrmInfo->pDataBuff[4];
			}
			short * pAudio = (short *)m_pPCMData;
			for (i = 0; i < nBlocks; i++)
			{
				nVolume = ((*(pLeft1++) + *(pLeft2++) + *(pFront)) / 3 * m_nScale);
				if (nVolume > 0X7FFF)
					nVolume = 0X7FFF;
				else if (nVolume < -0X7FFF)
					nVolume = -0X7FFF;
				*pAudio++ = (short)nVolume;
				nVolume = ((*(pRight1++) + *(pRight2++) + *(pFront++)) / 3 * m_nScale);
				if (nVolume > 0X7FFF)
					nVolume = 0X7FFF;
				else if (nVolume < -0X7FFF)
					nVolume = -0X7FFF;
				*pAudio++ = (short)nVolume;
				//*pAudio++ = (short)((*(pLeft1++) + *(pLeft2++) + *(pFront)) / 3 * m_nScale);
				//*pAudio++ = (short)((*(pRight1++) + *(pRight2++) + *(pFront++)) / 3 * m_nScale);
			}
		}
	}
	else if (m_pFrmInfo->nFormat == QA_SAMPLE_FMT_DBLP)
	{
		double *	pLeft = (double *)m_pFrmInfo->pDataBuff[0];
		double *	pRight = (double *)m_pFrmInfo->pDataBuff[1];
		short *		pAudio = (short *)m_pPCMData;
		for (i = 0; i < nBlocks; i++)
		{
			nVolume = (*pLeft++ * m_nScale);
			if (nVolume > 0X7FFF)
				nVolume = 0X7FFF;
			else if (nVolume < -0X7FFF)
				nVolume = -0X7FFF;
			*pAudio++ = (short)nVolume;
			//*pAudio++ = (short)(*pLeft++ * m_nScale);
			if (m_fmtAudio.nChannels > 1)
			{
				nVolume = (*pRight++ * m_nScale);
				if (nVolume > 0X7FFF)
					nVolume = 0X7FFF;
				else if (nVolume < -0X7FFF)
					nVolume = -0X7FFF;
				*pAudio++ = (short)nVolume;
				//	*pAudio++ = (short)(*pRight++ * m_nScale);
			}
		}
	}
	else if (m_pFrmInfo->nFormat == QA_SAMPLE_FMT_U8)
	{
		unsigned char * pLeft = (unsigned char *)m_pFrmInfo->pDataBuff[0];
		short *			pAudio = (short *)m_pPCMData;
		for (i = 0; i < nBlocks * m_fmtAudio.nChannels; i++)
			*pAudio++ = (short)(*pLeft++) * 256 * m_nVolume / 100;
	}
	else if (m_pFrmInfo->nFormat == QA_SAMPLE_FMT_S32)
	{
		int *	pLeft = (int *)m_pFrmInfo->pDataBuff[0];
		short * pAudio = (short *)m_pPCMData;
		for (i = 0; i < nBlocks * m_fmtAudio.nChannels; i++)
			*pAudio++ = (short)(*pLeft++) * m_nVolume / 100;
	}
	else if (m_pFrmInfo->nFormat == QA_SAMPLE_FMT_FLT)
	{
		float * pLeft = (float *)m_pFrmInfo->pDataBuff[0];
		short * pAudio = (short *)m_pPCMData;
		for (i = 0; i < nBlocks * m_fmtAudio.nChannels; i++)
		{
			nVolume = (*pLeft++ * m_nScale);
			if (nVolume > 0X7FFF)
				nVolume = 0X7FFF;
			else if (nVolume < -0X7FFF)
				nVolume = -0X7FFF;
			*pAudio++ = (short)nVolume;
			//*pAudio++ = (short)(*pLeft++ * m_nScale);
		}
	}
	else if (m_pFrmInfo->nFormat == QA_SAMPLE_FMT_DBL)
	{
		double *	pLeft = (double *)m_pFrmInfo->pDataBuff[0];
		short *		pAudio = (short *)m_pPCMData;
		for (i = 0; i < nBlocks * m_fmtAudio.nChannels; i++)
		{
			nVolume = (*pLeft++ * m_nScale);
			if (nVolume > 0X7FFF)
				nVolume = 0X7FFF;
			else if (nVolume < -0X7FFF)
				nVolume = -0X7FFF;
			*pAudio++ = (short)nVolume;
			//*pAudio++ = (short)(*pLeft++ * m_nScale);
		}
	}

	m_pBuffData->uSize = nBlocks * 2 * m_fmtAudio.nChannels;
	m_pBuffData->pBuff = m_pPCMData;

	return QC_ERR_NONE;
}
