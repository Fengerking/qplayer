/*******************************************************************************
File:		CRtspParser.cpp

Contains:	CRtspParser implement file.

Written by:	Qichao Shen

Change History (most recent first):
2018-05-03		Qichao			Create file

*******************************************************************************/

#include "qcErr.h"
#include "qcDef.h"
#include "CRtspParser.h"

#include "UIntReader.h"
#include "USystemFunc.h"
#include "ULogFunc.h"
#include "UUrlParser.h"
#include "UAVParser.h"

CRtspParser::CRtspParser(CBaseInst * pBaseInst)
	: CBaseParser(pBaseInst)
{
	SetObjectName("CRtspParser");
	m_pRtspWrapper = NULL;
	m_illFirstTimeStamp = QC_MAX_NUM64_S;
	m_iState = 0;

	m_pVideoTimeStampCheckCache = NULL;
	m_pAudioTimeStampCheckCache = NULL;
}

CRtspParser::~CRtspParser(void)
{
	UnInit();
}

int		CRtspParser::Open(QC_IO_Func * pIO, const char * pURL, int nFlag)
{
	int iRet = 0;
	int iWaitCount = 0;
	int  iVideoCodecId = 0;
	char*   pVideoHeaderData = NULL;
	int  iVideoHeaderSize = 0;
	int iRefNum = 0;

	int iSarWidth = 0;
	int iSarHeight = 0;

	int  iAudioCodecId = 0;
	char*   pAudioHeaderData = NULL;
	int  iAudioHeaderSize = 0;



	QCLOGI("Call RTSP Open!");
	if (m_pRtspWrapper != NULL)
	{
		UnInit();
	}

	Init();
	if (m_pRtspWrapper != NULL)
	{
		m_pRtspWrapper->Open(pURL);
	}

	//Wait a while, 500ms
	while (m_iState == RTSP_STATE_NONE && iWaitCount < 50)
	{
		QCLOGI("rtsp ins: %p", m_pRtspWrapper);
		qcSleep(10000);
		iWaitCount++;
	}


	m_pBaseInst->m_pSetting->g_qcs_nMaxPlayBuffTime = 1000;
	m_pBaseInst->m_pSetting->g_qcs_nMinPlayBuffTime = 200;

	if (m_iState == RTSP_STATE_NONE)
	{
		QCLOGI("TimeOut");
		return QC_ERR_FAILED;
	}
	else
	{
		if (m_iState == RTSP_STATE_PLAY_END)
		{
			QCLOGI("stream closed!");
			return QC_ERR_FAILED;
		}
	}

	m_bLive = true;

	iRet = m_pRtspWrapper->GetMediaTrackinfo(QC_MEDIA_Video, &iVideoCodecId, &pVideoHeaderData, &iVideoHeaderSize);
	if (iRet == 0)
	{
		if (iVideoCodecId == QC_CODEC_ID_H264)
		{
			m_nStrmVideoCount = 1;
			m_nStrmVideoPlay = 0;
			QC_DEL_P(m_pFmtVideo);
			m_pFmtVideo = new QC_VIDEO_FORMAT;
			memset(m_pFmtVideo, 0, sizeof(QC_VIDEO_FORMAT));
			m_pFmtVideo->nCodecID = iVideoCodecId;
			qcAV_FindAVCDimensions((unsigned char*)pVideoHeaderData, iVideoHeaderSize, &(m_pFmtVideo->nWidth), &(m_pFmtVideo->nHeight), &iRefNum, &iSarWidth, &iSarHeight);
			QCLOGI("set video track!");
			QC_DEL_P(m_pVideoTimeStampCheckCache);
			m_pVideoTimeStampCheckCache = new CheckTimestampCache;
		}
	}

	iRet = m_pRtspWrapper->GetMediaTrackinfo(QC_MEDIA_Audio, &iAudioCodecId, &pAudioHeaderData, &iAudioHeaderSize);
	if (iRet == 0)
	{
		if (iAudioCodecId == QC_CODEC_ID_AAC)
		{
			m_nStrmAudioCount = 1;
			m_nStrmAudioPlay = 0;
			QC_DEL_P(m_pFmtAudio);
			m_pFmtAudio = new QC_AUDIO_FORMAT;
			memset(m_pFmtAudio, 0, sizeof(QC_AUDIO_FORMAT));
			m_pFmtAudio->nCodecID = iAudioCodecId;

			qcAV_ParseAACConfig((unsigned char*)pAudioHeaderData, iAudioHeaderSize, &(m_pFmtAudio->nSampleRate), &(m_pFmtAudio->nChannels));
			QCLOGI("set audio track! sample rate:%d, channels:%d", m_pFmtAudio->nSampleRate, m_pFmtAudio->nChannels);
			QC_DEL_P(m_pAudioTimeStampCheckCache);
			m_pAudioTimeStampCheckCache = new CheckTimestampCache;
		}
	}

	QCLOGI("rtsp ins: %p open done!", m_pRtspWrapper);
	return QC_ERR_NONE;
}

int 	CRtspParser::Close(void)
{
	UnInit();
	return QC_ERR_NONE;
}

int		CRtspParser::Read(QC_DATA_BUFF * pBuff)
{
	//Free CPU loading
	qcSleep(4000);
	return QC_ERR_NONE;
}

int		CRtspParser::CanSeek()
{
	return 0;
}

long long 	CRtspParser::GetPos(void)
{
	return QC_ERR_NONE;
}

long long 	CRtspParser::SetPos(long long llPos)
{
	return QC_ERR_NONE;
}

int 	CRtspParser::GetParam(int nID, void * pParam)
{
	return QC_ERR_NONE;
}

int 	CRtspParser::SetParam(int nID, void * pParam)
{
	return QC_ERR_NONE;
}

void   CRtspParser::Init()
{
	m_pRtspWrapper = new CRtspParserWrapper(m_pBaseInst);
	m_nMaxAudioSize = 0;
	m_nMaxVideoSize = 0;
	m_iState = 0;

	do 
	{
		if (m_pRtspWrapper == NULL)
		{
			break;
		}

		if (m_pRtspWrapper->GetCurState() == RTSP_INS_INITED)
		{
			m_pRtspWrapper->SetFrameHandle(this);
		}
	} while (0);

	return;
}


void   CRtspParser::UnInit()
{
	QC_DEL_P(m_pRtspWrapper);
	QC_DEL_P(m_pFmtAudio);
	QC_DEL_P(m_pFmtVideo);
	QC_DEL_P(m_pFmtSubtt);
	QC_DEL_P(m_pVideoTimeStampCheckCache);
	QC_DEL_P(m_pAudioTimeStampCheckCache);
}

void CRtspParser::CreateDefaultFmtInfo()
{
	QC_DEL_P(m_pFmtAudio);
	QC_DEL_P(m_pFmtVideo);
	QC_DEL_P(m_pFmtSubtt);

	//Fill Default Audio Format Info
	m_pFmtAudio = new QC_AUDIO_FORMAT;
	memset(m_pFmtAudio, 0, sizeof(QC_AUDIO_FORMAT));
	m_pFmtAudio->nCodecID = QC_CODEC_ID_AAC;


	//Fill Default Video Format Info
	m_pFmtVideo = new QC_VIDEO_FORMAT;
	memset(m_pFmtVideo, 0, sizeof(QC_VIDEO_FORMAT));
	m_pFmtVideo->nCodecID = QC_CODEC_ID_H264;
	m_pFmtVideo->nWidth = 640;
	m_pFmtVideo->nHeight = 480;

	//Fill Default SubTitle Format Info
	m_pFmtSubtt = new QC_SUBTT_FORMAT;
	memset(m_pFmtSubtt, 0, sizeof(QC_SUBTT_FORMAT));
}

int   CRtspParser::SendRtspBuffInner(S_Ts_Media_Sample * pSample)
{
	char   strOutput[8192] = { 0 };
	unsigned int*     pMediaMaxSize = NULL;
	unsigned int      ulFrameSize = 0;
	QC_DATA_BUFF * pBuff = NULL;
	QCMediaType     eMediaType = QC_MEDIA_MAX;


	if (pSample == NULL)
	{
		return QC_ERR_NONE;
	}

	if (m_illFirstTimeStamp == QC_MAX_NUM64_S)
	{
		m_illFirstTimeStamp = pSample->ullTimeStamp;
	}

	switch (pSample->usMediaType)
	{
	case QC_MEDIA_Audio:
	{
		pMediaMaxSize = &m_nMaxAudioSize;
		eMediaType = QC_MEDIA_Audio;
		ulFrameSize = pSample->ulSampleBufferSize;
		pBuff = m_pBuffMng->GetEmpty(QC_MEDIA_Audio, ulFrameSize + 128);
		break;
	}

	case QC_MEDIA_Video:
	{
		pMediaMaxSize = &m_nMaxVideoSize;
		eMediaType = QC_MEDIA_Video;
		ulFrameSize = pSample->ulSampleBufferSize;
		pBuff = m_pBuffMng->GetEmpty(QC_MEDIA_Video, ulFrameSize + 128);
		break;
	}
	default:
	{
		break;
	}
	}

	if (pBuff == NULL)
	{
		return QC_ERR_MEMORY;
	}


	pBuff->uBuffType = QC_BUFF_TYPE_Data;
	pBuff->nMediaType = eMediaType;
	pBuff->llTime = (pSample->ullTimeStamp > m_illFirstTimeStamp) ? (pSample->ullTimeStamp - m_illFirstTimeStamp) : 0;
	pBuff->uFlag = pSample->ulSampleFlag ? QCBUFF_KEY_FRAME : 0;

	if ((*pMediaMaxSize) < ulFrameSize - 2 + 128)
	{
		(*pMediaMaxSize) = ulFrameSize - 2 + 128;
	}

	if (pBuff->pBuff == NULL || pBuff->uBuffSize < (*pMediaMaxSize))
	{
		if (pBuff->pBuff != NULL)
		{
			delete[]pBuff->pBuff;
		}
		pBuff->pBuff = new unsigned char[(*pMediaMaxSize)];
		if (pBuff->pBuff == NULL)
		{
			m_pBuffMng->Return(pBuff);
			return QC_ERR_MEMORY;
		}
		pBuff->uBuffSize = (*pMediaMaxSize);
	}

	if (pSample->ulMediaCodecId == QC_CODEC_ID_AAC && m_pFmtAudio != NULL)
	{
		qcAV_ConstructAACHeader(pBuff->pBuff, pBuff->uBuffSize, m_pFmtAudio->nSampleRate, m_pFmtAudio->nChannels, pSample->ulSampleBufferSize);
		memcpy(pBuff->pBuff + 7, pSample->pSampleBuffer, pSample->ulSampleBufferSize);
		pBuff->uSize = pSample->ulSampleBufferSize + 7;
	}
	else
	{
		memcpy(pBuff->pBuff, pSample->pSampleBuffer, ulFrameSize);
		pBuff->uSize = ulFrameSize;
	}


	pBuff->nUsed--;

	m_pBuffMng->Send(pBuff);
	return QC_ERR_NONE;
}


int   CRtspParser::SendRtspBuff(S_Rtsp_Media_Sample * pRtspBuff)
{
	S_Ts_Media_Sample  sMediaSample;
	int iRet = 0;
	bool  bCanCommited = false;
	CheckTimestampCache*   pCheckTimeStampCache = NULL;

	do 
	{
		if (pRtspBuff == NULL)
		{
			break;
		}

		switch (pRtspBuff->eMediaType)
		{
			case QC_MEDIA_Audio:
			{
				pCheckTimeStampCache = m_pAudioTimeStampCheckCache;
				break;
			}

			case QC_MEDIA_Video:
			{
				pCheckTimeStampCache = m_pVideoTimeStampCheckCache;
				break;
			}
		}

		if (pCheckTimeStampCache == NULL)
		{
			break;
		}

		PreProcessTimestamp(pCheckTimeStampCache, pRtspBuff, bCanCommited);
		if (bCanCommited == true)
		{
			//
			memset(&sMediaSample, 0, sizeof(S_Ts_Media_Sample));
			sMediaSample.pSampleBuffer = pRtspBuff->pSampleBuffer;
			sMediaSample.ullTimeStamp = pRtspBuff->ullTimeStamp;
			sMediaSample.ulMediaCodecId = pRtspBuff->ulMediaCodecId;
			sMediaSample.ulSampleBufferSize = pRtspBuff->iSampleBufferSize;
			sMediaSample.ulSampleFlag = pRtspBuff->ulSampleFlag;
			sMediaSample.usMediaType = pRtspBuff->eMediaType;

			SendRtspBuffInner(&sMediaSample);
		}
	} while (0);

	return iRet;
}


int  CRtspParser::ExtractSeiPrivate(unsigned char*  pFrameData, int iFrameSize, char*  pOutput)
{
	int      iNalCount = 0;
	unsigned char nalhead[3] = { 0, 0, 1 };
	int ioffset = 0;
	unsigned char * pScan = pFrameData;
	unsigned char * pScanEnd = pFrameData + iFrameSize - 4;
	unsigned char  naluType = 0;
	unsigned char * pSeiStart = NULL;
	int             iSeiPayloadSize = 0;
	int             iSeiType = 0;
	int             iSeiLen = 0;
	while (pScan < pScanEnd)
	{
		if (memcmp(pScan, nalhead, 3) == 0)
		{
			naluType = (*(pScan + 3)) & 0x0f;
			if (naluType == 6)
			{
				pSeiStart = pScan + 4;
				
				while (*pSeiStart == 0xff && pSeiStart < pScanEnd)
				{
					iSeiType += 255;
					pSeiStart++;
				}

				iSeiType += *(pSeiStart);
				pSeiStart++;

				//Get the SEI Size
				while (*pSeiStart == 0xff && pSeiStart < pScanEnd)
				{
					iSeiLen += 255;
					pSeiStart++;
				}

				if (iSeiType == 5)
				{
					iSeiPayloadSize += *(pSeiStart);
					pSeiStart++;
					pSeiStart += 16;
					//QCLOGI("Output:%s", pSeiStart);
					break;
				}
			}
		}

		pScan++;
	}

	return 0;
}


void   CRtspParser::SetRtspState(int iState)
{
	m_iState = iState;
}



void   CRtspParser::DoVideoHeaderNotify()
{
}

void   CRtspParser::PreProcessTimestamp(CheckTimestampCache* pCheckTimeStampCache, S_Rtsp_Media_Sample * pRtspBuff, bool& bCanCommit)
{
	S_Ts_Media_Sample   sTsSample;
	CheckTimestampCache*  pCache = NULL;
	S_Ts_Media_Sample*   pFrameArray = NULL;
	uint64    ullLastTimeStampInCache = 0;

	bCanCommit = false;
	if (pRtspBuff == NULL || pCheckTimeStampCache == NULL)
	{
		return;
	}

	pCache = pCheckTimeStampCache;

	//struct convert
	memset(&sTsSample, 0, sizeof(S_Ts_Media_Sample));
	sTsSample.ullTimeStamp = pRtspBuff->ullTimeStamp;
	sTsSample.usMediaType = pRtspBuff->eMediaType;
	sTsSample.pSampleBuffer = pRtspBuff->pSampleBuffer;
	sTsSample.ulSampleFlag = pRtspBuff->ulSampleFlag;
	sTsSample.ulMediaCodecId = pRtspBuff->ulMediaCodecId;
	sTsSample.ulSampleBufferSize = pRtspBuff->iSampleBufferSize;


	ullLastTimeStampInCache = pCache->GetLastTimeStamp();
	if (sTsSample.ullTimeStamp != ullLastTimeStampInCache)
	{
		if (pCache->GetBufferCount() == 0)
		{
			pCache->SetLastTimeStamp(sTsSample.ullTimeStamp);
			bCanCommit = true;
			return;
		}

		pFrameArray = pCache->GetCacheFrameArray();
		if (pFrameArray == NULL)
		{
			return;
		}

		if (pRtspBuff->ullTimeStamp > ullLastTimeStampInCache)
		{
			// commit cache frames
			pCache->CalculateAvgTS(pRtspBuff->ullTimeStamp);//(pBuf->nStartTime - pCache->nLastTimestamp)/(pCache->nBufCount+1);
			for (int i = 0; i < pCache->GetBufferCount(); i++)
			{
				SendRtspBuffInner(&(pFrameArray[i]));
			}
		}
		else
		{
			//timestamp rollback, commit cache frames 
			for (int i = 0; i < pCache->GetBufferCount(); i++)
			{
				SendRtspBuffInner(&(pFrameArray[i]));
			}
		}

		//pCache->nBufCount		= 0;
		pCache->Reset();
		pCache->SetLastTimeStamp(sTsSample.ullTimeStamp);
		bCanCommit = true;
		return;
	}
	else
	{
		if (pCache->InsertFrame(&sTsSample) == false)
		{

		}
	}

	return;
}