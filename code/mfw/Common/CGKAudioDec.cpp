/*******************************************************************************
	File:		CGKAudioDec.cpp

	Contains:	gk audio dec implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "CGKAudioDec.h"
#include "ULogFunc.h"

CGKAudioDec::CGKAudioDec(CBaseInst * pBaseInst, void * hInst)
	: CBaseAudioDec(pBaseInst, hInst)
	, m_hLib (NULL)
	, m_hDec (NULL)
{
	SetObjectName ("CGKAudioDec");
	memset (&m_fmtAudio, 0, sizeof (m_fmtAudio));
	memset (&m_fAPI, 0, sizeof (m_fAPI));
	memset (&m_Input, 0, sizeof (m_Input));
}

CGKAudioDec::~CGKAudioDec(void)
{
	Uninit ();
}

int CGKAudioDec::Init (QC_AUDIO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;

	Uninit ();

	m_ttFmt.Channels = pFmt->nChannels;
	m_ttFmt.SampleRate = pFmt->nSampleRate;
	m_ttFmt.SampleBits = pFmt->nBits;

	if (pFmt->nCodecID == QC_CODEC_ID_AAC)
	{
		m_hLib = (qcLibHandle)qcLibLoad ("qcAACDec", 0);
		if (m_hLib == NULL)
			return QC_ERR_FAILED;
		__GetAudioDECAPI  pGetAPI = (__GetAudioDECAPI)qcLibGetAddr (m_hLib, "ttGetAACDecAPI", 0);
		if (pGetAPI == NULL)
			return QC_ERR_FAILED;
		pGetAPI (&m_fAPI);
	}
	if (m_fAPI.Open == NULL)
		return QC_ERR_FAILED;

	int nRC = m_fAPI.Open (&m_hDec);
	if (m_hDec == NULL)
		return QC_ERR_FAILED;

	if(pFmt->nCodecID == QC_CODEC_ID_AAC)
	{
		nRC = m_fAPI.SetParam (m_hDec, TT_PID_AUDIO_FORMAT, &m_ttFmt);
		TTAACFRAMETYPE nFrameType = TTAAC_ADTS;
		if (pFmt->pPrivateData != NULL && pFmt->nPrivateFlag == 1)
		{
			nFrameType = TTAAC_RAWDATA;
			TTMP4DecoderSpecificInfo * pDecInfo = (TTMP4DecoderSpecificInfo *)pFmt->pPrivateData;
			nRC = m_fAPI.SetParam (m_hDec, TT_PID_AUDIO_DECODER_INFO, pDecInfo);
		}
		else if (pFmt->pHeadData != NULL && pFmt->nHeadSize > 0)
		{
			nFrameType = TTAAC_RAWDATA;
			TTMP4DecoderSpecificInfo DecInfo;
			memset (&DecInfo, 0, sizeof (DecInfo));
			DecInfo.iData = pFmt->pHeadData;
			DecInfo.iSize = pFmt->nHeadSize;
			nRC = m_fAPI.SetParam (m_hDec, TT_PID_AUDIO_DECODER_INFO, &DecInfo);
		}
		nRC = m_fAPI.SetParam (m_hDec, TT_AACDEC_PID_FRAMETYPE, &nFrameType);

		TTAudioFormat fmtAudio;
		fmtAudio.Channels = pFmt->nChannels;
		fmtAudio.SampleRate = pFmt->nSampleRate;
		fmtAudio.SampleBits = pFmt->nBits;
		nRC = m_fAPI.SetParam (m_hDec, TT_PID_AUDIO_FORMAT, &fmtAudio);
	}
	memcpy (&m_fmtAudio, pFmt, sizeof (m_fmtAudio));
	m_fmtAudio.pHeadData = NULL;
	m_fmtAudio.nHeadSize = 0;
	m_fmtAudio.pPrivateData = NULL;

	memset (&m_Input, 0, sizeof (m_Input));

	if (m_pBuffData == NULL)
	{
		m_pBuffData = new QC_DATA_BUFF ();
		memset (m_pBuffData, 0, sizeof (QC_DATA_BUFF));
		m_pBuffData->uBuffSize = 1024 * 64;
		m_pBuffData->pBuff = new unsigned char[m_pBuffData->uBuffSize];
	}
	m_Output.pBuffer = m_pBuffData->pBuff;
	m_Output.nSize = m_pBuffData->uBuffSize;

	m_uBuffFlag = 0;
	
	return QC_ERR_NONE;
}

int CGKAudioDec::Uninit (void)
{
	if (m_hDec != NULL)
		m_fAPI.Close (m_hDec);
	m_hDec = NULL;

	if (m_hLib != NULL)
	{
		qcLibFree (m_hLib, 0);
		m_hLib = NULL;
	}
	return CBaseAudioDec::Uninit ();
}

int CGKAudioDec::Flush (void)
{
	CAutoLock lock (&m_mtBuffer);

	if (m_hDec != NULL)
	{
		unsigned int	nFlush = 1;	
		m_fAPI.SetParam (m_hDec, TT_PID_AUDIO_FLUSH, &nFlush);
	}
	memset (&m_Input, 0, sizeof (m_Input));

	return QC_ERR_NONE;
}

int CGKAudioDec::SetBuff (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || m_hDec == NULL)
		return QC_ERR_ARG;

	CAutoLock lock (&m_mtBuffer);
	CBaseAudioDec::SetBuff (pBuff);

	if ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
		Flush ();
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_AUDIO_FORMAT * pFmt = (QC_AUDIO_FORMAT *)pBuff->pFormat;
		if (pFmt != NULL && pFmt->pHeadData != NULL && pFmt->nHeadSize > 0)
			Init (pFmt);
	}
	
	if (pBuff->uBuffType == QC_BUFF_TYPE_Data)
	{
		m_Input.pBuffer = pBuff->pBuff;
		m_Input.nSize = pBuff->uSize;
		m_Input.llTime = pBuff->llTime;
	}
	else
	{
		return QC_ERR_UNSUPPORT;
	}

	int nRC = m_fAPI.SetInput (m_hDec, &m_Input);
	if (nRC != TTKErrNone)
		return QC_ERR_RETRY;

	return QC_ERR_NONE;
}

int CGKAudioDec::GetBuff (QC_DATA_BUFF ** ppBuff)
{
	if (ppBuff == NULL || m_hDec == NULL)
		return QC_ERR_ARG;
	*ppBuff = NULL;

	CAutoLock lock (&m_mtBuffer);
	if (m_Input.nSize <= 0)
		return QC_ERR_RETRY;

	int nRC = 0;
	int nBuffSize = 0;
	memset (&m_Output, 0, sizeof (m_Output));
	while (true)
	{
		m_Output.nSize = m_pBuffData->uBuffSize - nBuffSize;
		if (m_Output.nSize < 4096)
			break;
		m_Output.pBuffer = m_pBuffData->pBuff + nBuffSize;
		nRC = m_fAPI.Process (m_hDec, &m_Output, &m_OutputInfo);
		if (nRC != TTKErrNone)
		{
			m_Input.nSize = 0;
			break;
		}
		nBuffSize += m_Output.nSize;
	}
	if (nBuffSize <= 0)
		return QC_ERR_RETRY;

	m_pBuffData->uFlag = QC_BUFF_TYPE_Data;
	m_pBuffData->uSize = nBuffSize;
	m_pBuffData->llTime = m_Input.llTime;
	if (m_fmtAudio.nChannels != m_OutputInfo.Channels || m_fmtAudio.nSampleRate !=m_OutputInfo.SampleRate)
	{
		m_fmtAudio.nChannels = m_OutputInfo.Channels;
		m_fmtAudio.nSampleRate =m_OutputInfo.SampleRate;
		m_pBuffData->uFlag |= QCBUFF_NEW_FORMAT;
		m_pBuffData->pFormat = &m_fmtAudio;
	}

	CBaseAudioDec::GetBuff (&m_pBuffData);
	*ppBuff = m_pBuffData;
	return QC_ERR_NONE;
}