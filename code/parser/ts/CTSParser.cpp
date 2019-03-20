/*******************************************************************************
File:		CTSParser.cpp

Contains:	CTSParser implement file.

Written by:	Qichao Shen

Change History (most recent first):
2016-12-22		Qichao			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcDef.h"
#include "qcData.h"
#include "CTSParser.h"
#include "UAVParser.h"
#include "CADTSFrameSpliter.h"
#include "ULogFunc.h"
#include "qcData.h"
#include "CMsgMng.h"
#include "CH2645FrameSpliter.h"

#include "../m3u8/CAdaptiveStreamHLS.h"

CTSParser::CTSParser(CBaseInst *pBaseInst)
	: CBaseParser(pBaseInst), m_pSendBuff(NULL)
{
	SetObjectName("CTSParser");

	m_bInitedFlag = false;
	m_nMaxAudioSize = 0;
	m_nMaxVideoSize = 0;
	SetObjectName("CTSParser");
	memset(&m_sResInfo, 0, sizeof(QC_RESOURCE_INFO));
	InitWithoutMem();

	m_pReadBuff = NULL;
	m_nReadSize = 188 * 64;
	m_bTSFile = false;
}

CTSParser::~CTSParser(void)
{
	UnInit();
	QC_DEL_A(m_pReadBuff);
}

int CTSParser::Open(QC_IO_Func *pIO, const char *pURL, int nFlag)
{
	m_bTSFile = true;
	m_fIO = pIO;
	if (m_fIO->GetSize(m_fIO->hIO) <= 0)
	{
		if (m_fIO->Open(m_fIO->hIO, pURL, 0, QCIO_FLAG_READ) != QC_ERR_NONE)
			return QC_ERR_IO_FAILED;
	}
	else
	{
		m_fIO->SetPos(m_fIO->hIO, 0, QCIO_SEEK_BEGIN);
	}

	m_nStrmVideoCount = 1;
	m_nStrmVideoPlay = 0;
	m_pFmtVideo = new QC_VIDEO_FORMAT();
	memset(m_pFmtVideo, 0, sizeof(QC_VIDEO_FORMAT));
	m_pFmtVideo->nSourceType = QC_SOURCE_QC;

	m_nStrmAudioCount = 1;
	m_nStrmAudioPlay = 0;
	m_pFmtAudio = new QC_AUDIO_FORMAT();
	memset(m_pFmtAudio, 0, sizeof(QC_AUDIO_FORMAT));
	m_pFmtAudio->nSourceType = QC_SOURCE_QC;
	m_pFmtAudio->nCodecID = QC_CODEC_ID_AAC;

	int nDLNotify = 1;
	m_fIO->SetParam(m_fIO->hIO, QCIO_PID_HTTP_NOTIFYDL_PERCENT, &nDLNotify);

	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_CONTENT_SIZE, 0, m_llFileSize);

	OnOpenDone(pURL);

	if (m_pReadBuff == NULL)
		m_pReadBuff = new unsigned char[m_nReadSize];

	return QC_ERR_NONE;
}

int CTSParser::Close(void)
{
	return QC_ERR_NONE;
}

int CTSParser::Read(QC_DATA_BUFF *pBuff)
{
	if (m_fIO == NULL || m_pReadBuff == NULL)
		return QC_ERR_STATUS;

	int nReadSize = m_nReadSize;
	int nRC = m_fIO->Read(m_fIO->hIO, m_pReadBuff, nReadSize, false, QCIO_READ_DATA);
	if (nReadSize > 0)
		Process(m_pReadBuff, nReadSize);

	if (nRC == QC_ERR_FINISH)
	{
		if (m_pBuffMng != NULL)
			m_pBuffMng->SetEOS(true);
	}

	return nRC;
}

long long CTSParser::GetPos(void)
{
	return 0;
}

long long CTSParser::SetPos(long long llPos)
{
	if (llPos == 0 && m_fIO != NULL)
		m_fIO->SetPos(m_fIO->hIO, 0, QCIO_SEEK_BEGIN);

	return 0;
}

int CTSParser::GetParam(int nID, void *pParam)
{
	return CBaseParser::GetParam (nID, pParam);
}

int CTSParser::SetParam(int nID, void *pParam)
{
	unsigned int ulValue = 0;
	QC_RESOURCE_INFO *pResInfo = NULL;

	switch (nID)
	{
	case QC_EVENTID_RESET_TS_PARSER_FOR_NEW_STREAM:
		UnInit();
		Init();
		m_ulHeaderDataFlag = QC_PARSER_HEADER_FLAG_NEW_STREAM;
		return QC_ERR_NONE;

	case QC_EVENTID_RESET_TS_PARSER_FOR_SEEK:
		UnInit();
		Init();
		m_ulHeaderDataFlag = QC_PARSER_HEADER_FLAG_SEEK;
		return QC_ERR_NONE;

	case QC_EVENTID_SET_TS_PARSER_TIME_OFFSET:
		ulValue = *((unsigned int *)pParam);
		m_ulOffsetInM3u8 = ulValue;
		return QC_ERR_NONE;

	case QC_EVENTID_SET_TS_PARSER_BA_MODE:
		m_iBAMode = (int)(*((unsigned char *)pParam));
		break;

	case QC_EVENTID_SET_TS_PARSER_RES_INFO:
		pResInfo = (QC_RESOURCE_INFO *)pParam;
		m_sResInfo.llDuration = pResInfo->llDuration;
		m_sResInfo.pszURL = pResInfo->pszURL;
		m_sResInfo.pszDomain = pResInfo->pszDomain;
		m_sResInfo.pszFormat = pResInfo->pszFormat;
		m_sResInfo.nBitrate = pResInfo->nBitrate;
		return QC_ERR_NONE;

	case QC_EVENTID_SET_TS_PARSER_FLUSH:
		FlushData();
		return QC_ERR_NONE;

	default:
		break;
	}

	return CBaseParser::SetParam(nID, pParam);
}

int CTSParser::Process(unsigned char *pBuff, int nSize)
{
	int iParsedCount = 0;
	int iPerProcess = 188 * 200;
	int iOffset = 0;
	int iProcessed = 0;
	if (m_bInitedFlag == false)
	{
		Init();
		InitDump();
		m_bInitedFlag = true;
	}

	m_iCurParsedCount = 0;
	while (iProcessed < nSize)
	{
		if ((nSize - iProcessed) > iPerProcess)
		{
			ProcessTs(pBuff + iProcessed, iPerProcess, &m_sTsParserContext);
			iProcessed += iPerProcess;
		}
		else
		{
			ProcessTs(pBuff + iProcessed, nSize - iProcessed, &m_sTsParserContext);
			iProcessed += (nSize - iProcessed);
		}
	}

	return m_iCurParsedCount;
}

void CTSParser::ParsePorc(void *pDataCallback)
{
	S_Ts_Media_Sample *pSampleCallback = (S_Ts_Media_Sample *)pDataCallback;
	CTSParser *pTsParser = NULL;

	if (pSampleCallback != NULL && pSampleCallback->pUserData != NULL)
	{
		pTsParser = (CTSParser *)pSampleCallback->pUserData;
		pTsParser->FrameParsedTrans(pDataCallback);
	}
}

int CTSParser::GetForamtInfo(QC_AUDIO_FORMAT *pAudioFmt, QC_VIDEO_FORMAT *pVideoFmt, QC_SUBTT_FORMAT *pSubFmt)
{
	int iIndex = 0;
	for (iIndex = 0; iIndex < m_iCurTrackCount; iIndex++)
	{
		if (m_pTsTrackInfo[iIndex] != NULL && m_pTsTrackInfo[iIndex]->pFmtTrack != NULL)
		{
			switch (m_pTsTrackInfo[iIndex]->eMediaType)
			{
			case QC_MEDIA_Video:
				pVideoFmt->nCodecID = ((QC_VIDEO_FORMAT *)(m_pTsTrackInfo[iIndex]->pFmtTrack))->nCodecID;
				pVideoFmt->nHeight = ((QC_VIDEO_FORMAT *)(m_pTsTrackInfo[iIndex]->pFmtTrack))->nHeight;
				pVideoFmt->nWidth = ((QC_VIDEO_FORMAT *)(m_pTsTrackInfo[iIndex]->pFmtTrack))->nWidth;
				pVideoFmt->nDen = ((QC_VIDEO_FORMAT *)(m_pTsTrackInfo[iIndex]->pFmtTrack))->nDen;
				pVideoFmt->nNum = ((QC_VIDEO_FORMAT *)(m_pTsTrackInfo[iIndex]->pFmtTrack))->nNum;
				break;

			case QC_MEDIA_Audio:
				pAudioFmt->nCodecID = ((QC_AUDIO_FORMAT *)(m_pTsTrackInfo[iIndex]->pFmtTrack))->nCodecID;
				pAudioFmt->nBits = ((QC_AUDIO_FORMAT *)(m_pTsTrackInfo[iIndex]->pFmtTrack))->nBits;
				pAudioFmt->nChannels = ((QC_AUDIO_FORMAT *)(m_pTsTrackInfo[iIndex]->pFmtTrack))->nChannels;
				pAudioFmt->nSampleRate = ((QC_AUDIO_FORMAT *)(m_pTsTrackInfo[iIndex]->pFmtTrack))->nSampleRate;
				break;

			case QC_MEDIA_Subtt:
				break;

			default:
				break;
			}
		}
	}

	return 0;
}

void CTSParser::FrameParsedTrans(void *pMediaSample)
{
	S_Ts_Media_Sample *pFrameFromTs = (S_Ts_Media_Sample *)pMediaSample;
	S_TS_Track_Info *pTsTrackInfo = NULL;
	unsigned int ulFrameSize = 0;
	S_Ts_Media_Sample aFrames[256] = { 0 };
	int iFrameCount = 0;
	int iTrackIndex = -1;
	int iIndex = 0;
	bool bCanCommit = false;
	int iRet = 0;

	iTrackIndex = FindTrackIndexByPID(pFrameFromTs->usTrackId);
	if (iTrackIndex == -1)
		iTrackIndex = GetAvailableTrackIndex();

	if (iTrackIndex == -1)
		return;

	if (m_iAudioCurPID != 0 && pFrameFromTs->usTrackId != m_iAudioCurPID && pFrameFromTs->usMediaType == MEDIA_AUDIO_IN_TS)
	{
		//multi audio, not the cur audio pid
		pFrameFromTs->usTrackId = pFrameFromTs->usTrackId;
		return;
	}

	if (m_pTsTrackInfo[iTrackIndex] == NULL)
		m_pTsTrackInfo[iTrackIndex] = CreateTSTrackInfo(pFrameFromTs->usTrackId, pFrameFromTs->ulMediaCodecId, pFrameFromTs->usMediaType);

	if (m_pTsTrackInfo[iTrackIndex] == NULL)
		return;

	pTsTrackInfo = m_pTsTrackInfo[iTrackIndex];
	if (pTsTrackInfo->iGetHeaderInfo == 0)
	{
		iRet = ParseMediaHeader(pTsTrackInfo, pFrameFromTs->pSampleBuffer, pFrameFromTs->ulSampleBufferSize, pFrameFromTs->ulMediaCodecId);
		if (iRet == 0)
		{
			SetCommitFlag(pTsTrackInfo);
		}
	}

	//issue the frame only header data is ready
	if (pTsTrackInfo->iGetHeaderInfo == 0)
		return;

	iRet = SplitMediaFrame(pTsTrackInfo, pFrameFromTs, 256, aFrames, iFrameCount);
	if (iRet != QC_ERR_NONE)
		return;

	//Add for test
	/*
	int  i = 0;
	int  iBaseStep = 100;
	int  iStepSize = 0;
	while (i < pFrameFromTs->ulSampleBufferSize)
	{
	memset(aSFrame, 0, sizeof(S_Frame_Base) * 64);
	iFrameCount = 0;
	iStepSize = ((pFrameFromTs->ulSampleBufferSize - i)>iBaseStep) ? iBaseStep : (pFrameFromTs->ulSampleBufferSize - i);

	m_pAudioFrameSpliter->CommitInputAndSplitting(pFrameFromTs->pSampleBuffer + i, iStepSize, pFrameFromTs->ullTimeStamp,
	64, aSFrame, iFrameCount);
	i += iStepSize;
	}
	*/
	//Add for test

	//m_pAudioFrameSpliter->CommitInputAndSplitting(pFrameFromTs->pSampleBuffer, pFrameFromTs->ulSampleBufferSize, pFrameFromTs->ullTimeStamp,
	//	64, aSFrame, iFrameCount);
	//}

	//pBuff = m_pBuffMng->GetEmpty(QC_MEDIA_Audio);

	for (iIndex = 0; iIndex < iFrameCount; iIndex++)
	{
		PreProcessTimestamp(pTsTrackInfo, &(aFrames[iIndex]), bCanCommit);
		if (bCanCommit == false)
		{
			continue;
		}
		else
		{
			CommitMediaFrameToBuffer(pTsTrackInfo->iCommitEnabled, &(aFrames[iIndex]));
		}
	}
}

void CTSParser::Init()
{
	CM_PARSER_INIT_INFO sParserInit = { 0 };
	sParserInit.pProc = ParsePorc;
	sParserInit.pUserData = this;
	InitTsParser(&sParserInit, &m_sTsParserContext);

	for (int i = 0; i < MAX_TRACK_COUNT_IN_TS; i++)
	{
		m_pTsTrackInfo[i] = NULL;
	}

	m_iCurTrackCount = 0;
	m_iCurAudioTrackCount = 0;
	m_nMaxAudioSize = 0;
	m_nMaxVideoSize = 0;
	m_ulHeaderDataFlag = QC_PARSER_HEADER_FLAG_NEW_STREAM;
	m_iBAMode = 0;
	m_iAudioCurPID = 0;
}

void CTSParser::InitWithoutMem()
{
	memset(&m_sTsParserContext, 0, sizeof(S_Ts_Parser_Context));
	for (int i = 0; i < MAX_TRACK_COUNT_IN_TS; i++)
	{
		m_pTsTrackInfo[i] = NULL;
	}

	m_iCurTrackCount = 0;
	m_iCurAudioTrackCount = 0;
	m_nMaxAudioSize = 0;
	m_nMaxVideoSize = 0;
	m_ulHeaderDataFlag = QC_PARSER_HEADER_FLAG_NEW_STREAM;
	m_iBAMode = 0;
	m_iAudioCurPID = 0;
}

void CTSParser::UnInit()
{
	QC_AUDIO_FORMAT *pFmtAudio = NULL;
	QC_VIDEO_FORMAT *pFmtVideo = NULL;
	QC_SUBTT_FORMAT *pFmtSubtt = NULL;

	for (int i = 0; i < MAX_TRACK_COUNT_IN_TS; i++)
	{
		if (m_pTsTrackInfo[i] != NULL)
		{

			switch (m_pTsTrackInfo[i]->eMediaType)
			{
			case QC_MEDIA_Audio:
			{
				pFmtAudio = (QC_AUDIO_FORMAT *)m_pTsTrackInfo[i]->pFmtTrack;
				QC_DEL_P(pFmtAudio);
				break;
			}
			case QC_MEDIA_Video:
			{
				pFmtVideo = (QC_VIDEO_FORMAT *)m_pTsTrackInfo[i]->pFmtTrack;
				QC_DEL_P(pFmtVideo);
				break;
			}
			case QC_MEDIA_Subtt:
			{
				pFmtSubtt = (QC_SUBTT_FORMAT *)m_pTsTrackInfo[i]->pFmtTrack;
				QC_DEL_P(pFmtSubtt);
				break;
			}
			}

			QC_DEL_P(m_pTsTrackInfo[i]->pFrameSpliter);
			QC_DEL_P(m_pTsTrackInfo[i]->pTimeStampCheckCache);
			QC_DEL_A(m_pTsTrackInfo[i]->pHeaderData);
			QC_DEL_P(m_pTsTrackInfo[i]);
		}
	}

	UnInitDump();
	UnInitTsParser(&m_sTsParserContext);
}

void CTSParser::PreProcessTimestamp(S_TS_Track_Info *pTrackInfo, void *pSampleFromTs, bool &bCanCommit)
{
	S_Ts_Media_Sample *pSample = (S_Ts_Media_Sample *)pSampleFromTs;
	CheckTimestampCache *pCache = NULL;
	S_Ts_Media_Sample *pFrameArray = NULL;
	uint64 ullLastTimeStampInCache = 0;

	bCanCommit = false;
	if (pSample == NULL || pTrackInfo == NULL)
	{
		return;
	}

	pCache = pTrackInfo->pTimeStampCheckCache;
	if (pCache == NULL)
	{
		return;
	}

	ullLastTimeStampInCache = pCache->GetLastTimeStamp();
	if (pSample->ullTimeStamp != ullLastTimeStampInCache)
	{
		if (pCache->GetBufferCount() == 0)
		{
			pCache->SetLastTimeStamp(pSample->ullTimeStamp);
			bCanCommit = true;
			return;
		}

		pFrameArray = pCache->GetCacheFrameArray();
		if (pFrameArray == NULL)
		{
			return;
		}

		if (pSample->ullTimeStamp > ullLastTimeStampInCache)
		{
			// commit cache frames
			pCache->CalculateAvgTS(pSample->ullTimeStamp); //(pBuf->nStartTime - pCache->nLastTimestamp)/(pCache->nBufCount+1);
			for (int i = 0; i < pCache->GetBufferCount(); i++)
			{
				CommitMediaFrameToBuffer(pTrackInfo->iCommitEnabled, &(pFrameArray[i]));
			}
		}
		else
		{
			//timestamp rollback, commit cache frames
			for (int i = 0; i < pCache->GetBufferCount(); i++)
			{
				CommitMediaFrameToBuffer(pTrackInfo->iCommitEnabled, &(pFrameArray[i]));
			}
		}

		//pCache->nBufCount		= 0;
		pCache->Reset();
		pCache->SetLastTimeStamp(pSample->ullTimeStamp);
		bCanCommit = true;
		return;
	}
	else
	{
		if (pCache->InsertFrame(pSample) == false)
		{
		}
	}

	return;
}

int CTSParser::FindTrackIndexByPID(uint32 ulPIDValue)
{
	int iRet = -1;
	for (int i = 0; i < m_iCurTrackCount; i++)
	{
		if (m_pTsTrackInfo[i] != NULL && m_pTsTrackInfo[i]->ulPID == ulPIDValue)
		{
			iRet = i;
			break;
		}
	}

	return iRet;
}

int CTSParser::GetAvailableTrackIndex()
{
	int iRet = -1;
	for (int i = 0; i < MAX_TRACK_COUNT_IN_TS; i++)
	{
		if (m_pTsTrackInfo[i] == NULL)
		{
			iRet = i;
			break;
		}
	}

	return iRet;
}

S_TS_Track_Info *CTSParser::CreateTSTrackInfo(uint32 ulPID, int iCodecId, int iMediaType)
{
	S_TS_Track_Info *pTsTrackInfo = NULL;
	do
	{
		pTsTrackInfo = new S_TS_Track_Info;
		if (pTsTrackInfo == NULL)
		{
			break;
		}

		m_iCurTrackCount++;
		memset(pTsTrackInfo, 0, sizeof(S_TS_Track_Info));
		pTsTrackInfo->ulPID = ulPID;
		pTsTrackInfo->iCodecId = iCodecId;
		switch (iCodecId)
		{
		case STREAM_TYPE_AUDIO_AAC:
		{
			pTsTrackInfo->eMediaType = QC_MEDIA_Audio;
			pTsTrackInfo->pFrameSpliter = new CADTSFrameSpliter(m_pBaseInst);
			pTsTrackInfo->pFrameSpliter->SetCodecID(iCodecId);
			pTsTrackInfo->pTimeStampCheckCache = new CheckTimestampCache;
			pTsTrackInfo->pFmtTrack = new QC_AUDIO_FORMAT;
			if (pTsTrackInfo->pFmtTrack != NULL)
			{
				memset(pTsTrackInfo->pFmtTrack, 0, sizeof(QC_AUDIO_FORMAT));
			}
			m_iCurAudioTrackCount++;
			break;
		}

		case STREAM_TYPE_AUDIO_MPEG1:
		{
			pTsTrackInfo->eMediaType = QC_MEDIA_Audio;
			pTsTrackInfo->pFmtTrack = new QC_AUDIO_FORMAT;
			pTsTrackInfo->pTimeStampCheckCache = new CheckTimestampCache;
			if (pTsTrackInfo->pFmtTrack != NULL)
			{
				memset(pTsTrackInfo->pFmtTrack, 0, sizeof(QC_AUDIO_FORMAT));
			}
			m_iCurAudioTrackCount++;
			break;
		}

		case STREAM_TYPE_VIDEO_H264:
		case STREAM_TYPE_VIDEO_HEVC:
		{
			pTsTrackInfo->eMediaType = QC_MEDIA_Video;
			pTsTrackInfo->pFrameSpliter = new CH2645FrameSpliter(m_pBaseInst);
			pTsTrackInfo->pFrameSpliter->SetCodecID(iCodecId);
			pTsTrackInfo->pTimeStampCheckCache = new CheckTimestampCache;
			pTsTrackInfo->pFmtTrack = new QC_VIDEO_FORMAT;
			if (pTsTrackInfo->pFmtTrack != NULL)
			{
				memset(pTsTrackInfo->pFmtTrack, 0, sizeof(QC_VIDEO_FORMAT));
			}
			break;
		}

		case STREAM_TYPE_AUDIO_G711_A:
		case STREAM_TYPE_AUDIO_G711_U:
		{
			pTsTrackInfo->eMediaType = QC_MEDIA_Audio;
			pTsTrackInfo->pFmtTrack = new QC_AUDIO_FORMAT;
			pTsTrackInfo->pTimeStampCheckCache = new CheckTimestampCache;
			if (pTsTrackInfo->pFmtTrack != NULL)
			{
				memset(pTsTrackInfo->pFmtTrack, 0, sizeof(QC_AUDIO_FORMAT));
			}
			m_iCurAudioTrackCount++;
			break;
		}

		default:
		{
			break;
		}
		}
	} while (false);

	return pTsTrackInfo;
}

int CTSParser::ParseMediaHeader(S_TS_Track_Info *pTsTrackInfo, uint8 *pData, int iDataSize, int iCodecId)
{
	int iRet = 0;
	switch (iCodecId)
	{
	case STREAM_TYPE_AUDIO_AAC:
	{
		pTsTrackInfo->iCodecId = STREAM_TYPE_AUDIO_AAC;
		iRet = ParseAACHeader(pTsTrackInfo, pData, iDataSize);
		break;
	}

	case STREAM_TYPE_VIDEO_H264:
	{
		pTsTrackInfo->iCodecId = STREAM_TYPE_VIDEO_H264;
		iRet = ParseH264Header(pTsTrackInfo, pData, iDataSize);
		break;
	}

	case STREAM_TYPE_VIDEO_HEVC:
	{
		pTsTrackInfo->iCodecId = STREAM_TYPE_VIDEO_HEVC;
		iRet = ParseHEVCHeader(pTsTrackInfo, pData, iDataSize);
		break;
	}

	case STREAM_TYPE_AUDIO_MPEG1:
	{
		pTsTrackInfo->iCodecId = STREAM_TYPE_AUDIO_MPEG1;
		iRet = ParseMp3Header(pTsTrackInfo, pData, iDataSize);
		break;
	}

	case STREAM_TYPE_AUDIO_G711_A:
	case STREAM_TYPE_AUDIO_G711_U:
	{
		pTsTrackInfo->iCodecId = iCodecId;
		iRet = ParseG711Header(pTsTrackInfo, pData, iDataSize);
		break;
	}
	}
	return iRet;
}

void CTSParser::SetCommitFlag(S_TS_Track_Info *pTsTrackInfo)
{
	int iIndex = 0;
	switch (pTsTrackInfo->eMediaType)
	{
	case QC_MEDIA_Video:
	{
		pTsTrackInfo->iCommitEnabled = 1;
		break;
	}
	case QC_MEDIA_Audio:
	{
		if (m_iCurAudioTrackCount == 1)
		{
			pTsTrackInfo->iCommitEnabled = 1;
		}
		else
		{
			//AdjustAuidoCommitFlag();
		}
		break;
	}
	}

	return;
}

void CTSParser::AdjustAuidoCommitFlag()
{
	S_TS_Track_Info *pTsTrack = NULL;
	QC_AUDIO_FORMAT *pAudioTrackInfo = NULL;
	int iIndex = 0;
	int iCurPreferValue = 0;
	int iCurPreferIndex = -1;
	int iPreferValue = 0;

	for (iIndex = 0; iIndex < m_iCurTrackCount; iIndex++)
	{
		pTsTrack = m_pTsTrackInfo[iIndex];
		if (pTsTrack->eMediaType == QC_MEDIA_Audio)
		{
			iPreferValue = 0;
			pAudioTrackInfo = (QC_AUDIO_FORMAT *)pTsTrack->pFmtTrack;
			if (pAudioTrackInfo != NULL && pAudioTrackInfo->nChannels <= 2)
			{
				iPreferValue = 1;
			}
			else
			{
				iPreferValue = 0;
			}

			if (iPreferValue > iCurPreferValue)
			{
				iCurPreferIndex = iIndex;
			}
		}
	}

	for (iIndex = 0; iIndex < m_iCurTrackCount; iIndex++)
	{
		pTsTrack = m_pTsTrackInfo[iIndex];
		if (pTsTrack->eMediaType == QC_MEDIA_Audio)
		{
			if (iIndex == iCurPreferIndex)
			{
				pTsTrack->iCommitEnabled = 1;
			}
			else
			{
				pTsTrack->iCommitEnabled = 0;
			}
		}
	}
}

int CTSParser::CommitMediaFrameToBuffer(int iCommitFlag, void *pMediaFrame)
{
	uint32 *pMediaMaxSize = NULL;
	uint32 ulFrameSize = 0;
	QC_DATA_BUFF *pBuff = NULL;
	S_Ts_Media_Sample *pFrame = (S_Ts_Media_Sample *)pMediaFrame;
	QCMediaType eMediaType = QC_MEDIA_MAX;

	if (iCommitFlag == 0)
	{
		return 0;
	}

	if (pFrame == NULL)
	{
		return false;
	}

	switch (pFrame->usMediaType)
	{
	case MEDIA_AUDIO_IN_TS:
	{
		eMediaType = QC_MEDIA_Audio;
		pMediaMaxSize = &m_nMaxAudioSize;
		ulFrameSize = pFrame->ulSampleBufferSize;
		pBuff = m_pBuffMng->GetEmpty(QC_MEDIA_Audio, ulFrameSize + 128);
		break;
	}

	case MEDIA_VIDEO_IN_TS:
	{
		eMediaType = QC_MEDIA_Video;
		pMediaMaxSize = &m_nMaxVideoSize;
		ulFrameSize = pFrame->ulSampleBufferSize;
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
	pBuff->llTime = pFrame->ullTimeStamp;
	pBuff->uFlag = pFrame->ulSampleFlag ? QCBUFF_KEY_FRAME : 0;
	pBuff->nValue = m_ulOffsetInM3u8;

	if ((*pMediaMaxSize) < ulFrameSize - 2 + 128)
	{
		(*pMediaMaxSize) = ulFrameSize - 2 + 128;
	}

	if (pBuff->pBuff == NULL || pBuff->uBuffSize < (*pMediaMaxSize))
	{
		if (pBuff->pBuff != NULL)
		{
			delete[] pBuff->pBuff;
		}
		pBuff->pBuff = new unsigned char[(*pMediaMaxSize)];
		if (pBuff->pBuff == NULL)
		{
			m_pBuffMng->Return(pBuff);
			return QC_ERR_MEMORY;
		}
		pBuff->uBuffSize = (*pMediaMaxSize);
	}

	//
	DumpMedia(pFrame);
	//
	memcpy(pBuff->pBuff, pFrame->pSampleBuffer, ulFrameSize);
	pBuff->uSize = ulFrameSize;
	m_iCurParsedCount++;

	pBuff->nUsed--;
	if (m_pSendBuff != NULL)
	{
		((CAdaptiveStreamHLS *)m_pSendBuff)->SendBuff(pBuff);
	}
	else
	{
		m_pBuffMng->Send(pBuff);
	}
	return QC_ERR_NONE;
}

int CTSParser::ParseH264Header(S_TS_Track_Info *pTsTrackInfo, uint8 *pData, int iDataSize)
{
	int iRet = 0;
	unsigned char aSpsBuf[256] = { 0 };
	int iSpsBufSize = 0;
	unsigned char aPpsBuf[256] = { 0 };
	int iPpsBufSize = 0;
	int iWidth = 0;
	int iHeight = 0;
	int iNumRef = 0;
	int iSarWidth = 0;
	int iSarHeight = 0;
	QC_VIDEO_FORMAT *pVideoFmt = NULL;
	pVideoFmt = (QC_VIDEO_FORMAT *)pTsTrackInfo->pFmtTrack;

	iRet = qcAV_FindH264SpsPps(pData, iDataSize, aSpsBuf, 256, iSpsBufSize, aPpsBuf, 256, iPpsBufSize);
	if (iRet == 0 && iPpsBufSize > 0 && iSpsBufSize > 0)
	{
		pTsTrackInfo->pHeaderData = new unsigned char[iSpsBufSize + iPpsBufSize + 64];
		if (pTsTrackInfo->pHeaderData != NULL)
		{
			memset(pTsTrackInfo->pHeaderData, 0, iSpsBufSize + iPpsBufSize + 64);
			memcpy(pTsTrackInfo->pHeaderData + pTsTrackInfo->ulHeaderDataSize, aSpsBuf, iSpsBufSize);
			pTsTrackInfo->ulHeaderDataSize += iSpsBufSize;
			memcpy(pTsTrackInfo->pHeaderData + pTsTrackInfo->ulHeaderDataSize, aPpsBuf, iPpsBufSize);
			pTsTrackInfo->ulHeaderDataSize += iPpsBufSize;
			qcAV_FindAVCDimensions(pTsTrackInfo->pHeaderData, pTsTrackInfo->ulHeaderDataSize, &iWidth, &iHeight, &iNumRef, &iSarWidth, &iSarHeight);
			pVideoFmt->nCodecID = QC_CODEC_ID_H264;
			pVideoFmt->nWidth = iWidth;
			pVideoFmt->nHeight = iHeight;
			pVideoFmt->nNum = iSarWidth;
			pVideoFmt->nDen = iSarHeight;
			pVideoFmt->nHeadSize = pTsTrackInfo->ulHeaderDataSize;
			pVideoFmt->pHeadData = pTsTrackInfo->pHeaderData;
			pTsTrackInfo->iGetHeaderInfo = 1;
			CommitMediaHeader(pTsTrackInfo->pHeaderData, pTsTrackInfo->ulHeaderDataSize, (void *)pVideoFmt, MEDIA_VIDEO_IN_TS);
		}
	}
	else
	{
		iRet = QC_ERR_MEMORY;
	}

	return iRet;
}

int CTSParser::ParseHEVCHeader(S_TS_Track_Info *pTsTrackInfo, uint8 *pData, int iDataSize)
{
	int iRet = 0;
	unsigned char aVpsBuf[256] = { 0 };
	int iVpsBufSize = 0;
	unsigned char aSpsBuf[256] = { 0 };
	int iSpsBufSize = 0;
	unsigned char aPpsBuf[256] = { 0 };
	int iPpsBufSize = 0;
	int iWidth = 0;
	int iHeight = 0;
	int iNumRef = 0;
	int iSarWidth = 0;
	int iSarHeight = 0;
	QC_VIDEO_FORMAT *pVideoFmt = NULL;
	pVideoFmt = (QC_VIDEO_FORMAT *)pTsTrackInfo->pFmtTrack;

	iRet = qcAV_FindHEVCVpsSpsPps(pData, iDataSize, aVpsBuf, 256, iVpsBufSize, aSpsBuf, 256, iSpsBufSize, aPpsBuf, 256, iPpsBufSize);
	if (iRet == 0 && iPpsBufSize > 0 && iSpsBufSize > 0 && iVpsBufSize > 0)
	{
		pTsTrackInfo->pHeaderData = new unsigned char[iVpsBufSize + iSpsBufSize + iPpsBufSize + 64];
		if (pTsTrackInfo->pHeaderData != NULL)
		{
			memset(pTsTrackInfo->pHeaderData, 0, iVpsBufSize + iSpsBufSize + iPpsBufSize + 64);
			memcpy(pTsTrackInfo->pHeaderData + pTsTrackInfo->ulHeaderDataSize, aVpsBuf, iVpsBufSize);
			pTsTrackInfo->ulHeaderDataSize += iVpsBufSize;
			memcpy(pTsTrackInfo->pHeaderData + pTsTrackInfo->ulHeaderDataSize, aSpsBuf, iSpsBufSize);
			pTsTrackInfo->ulHeaderDataSize += iSpsBufSize;
			memcpy(pTsTrackInfo->pHeaderData + pTsTrackInfo->ulHeaderDataSize, aPpsBuf, iPpsBufSize);
			pTsTrackInfo->ulHeaderDataSize += iPpsBufSize;
			qcAV_FindHEVCDimensions(pTsTrackInfo->pHeaderData + iVpsBufSize, pTsTrackInfo->ulHeaderDataSize - iVpsBufSize, &iWidth, &iHeight);
			pVideoFmt->nCodecID = QC_CODEC_ID_H265;
			pVideoFmt->nWidth = iWidth;
			pVideoFmt->nHeight = iHeight;
			pVideoFmt->nNum = iSarWidth;
			pVideoFmt->nDen = iSarHeight;
			pVideoFmt->nHeadSize = pTsTrackInfo->ulHeaderDataSize;
			pVideoFmt->pHeadData = pTsTrackInfo->pHeaderData;
			pTsTrackInfo->iGetHeaderInfo = 1;
			CommitMediaHeader(pTsTrackInfo->pHeaderData, pTsTrackInfo->ulHeaderDataSize, (void *)pVideoFmt, MEDIA_VIDEO_IN_TS);
		}
	}
	else
	{
		iRet = QC_ERR_MEMORY;
	}

	return iRet;
}

int CTSParser::ParseAACHeader(S_TS_Track_Info *pTsTrackInfo, uint8 *pData, int iDataSize)
{
	int iSampleRate = 0;
	int iChannelCount = 0;
	int iSampleBits = 0;
	QC_AUDIO_FORMAT *pAudioFmt = NULL;

	if (qcAV_ParseADTSAACHeaderInfo(pData, iDataSize, &iSampleRate, &iChannelCount, &iSampleBits) == 0)
	{
		pAudioFmt = (QC_AUDIO_FORMAT *)pTsTrackInfo->pFmtTrack;
		pAudioFmt->nCodecID = QC_CODEC_ID_AAC;
		pAudioFmt->nSampleRate = iSampleRate;
		pAudioFmt->nChannels = iChannelCount;
		pAudioFmt->nBits = iSampleBits;
		pTsTrackInfo->iGetHeaderInfo = 1;
		if (m_iAudioCurPID == 0)
		{
			CommitMediaHeader(pTsTrackInfo->pHeaderData, pTsTrackInfo->ulHeaderDataSize, (void *)pAudioFmt, MEDIA_AUDIO_IN_TS);
			m_iAudioCurPID = pTsTrackInfo->ulPID;
		}
	}

	return 0;
}

int CTSParser::ParseMp3Header(S_TS_Track_Info *pTsTrackInfo, uint8 *pData, int iDataSize)
{
	int iSampleRate = 0;
	int iChannelCount = 0;
	int iSampleBits = 0;
	QC_AUDIO_FORMAT *pAudioFmt = NULL;

	pAudioFmt = (QC_AUDIO_FORMAT *)pTsTrackInfo->pFmtTrack;
	pAudioFmt->nCodecID = QC_CODEC_ID_MP3;
	pTsTrackInfo->iGetHeaderInfo = 1;
	CommitMediaHeader(pTsTrackInfo->pHeaderData, pTsTrackInfo->ulHeaderDataSize, (void *)pAudioFmt, MEDIA_AUDIO_IN_TS);
	return 0;
}

int CTSParser::ParseG711Header(S_TS_Track_Info *pTsTrackInfo, uint8 *pData, int iDataSize)
{
	int iSampleRate = 0;
	int iChannelCount = 0;
	int iSampleBits = 0;
	QC_AUDIO_FORMAT *pAudioFmt = NULL;
	S_Track_Info_From_Desc sTrackInfoDesc = { 0 };
	int iRet = 0;

	pAudioFmt = (QC_AUDIO_FORMAT *)pTsTrackInfo->pFmtTrack;

	if (pTsTrackInfo->iCodecId == STREAM_TYPE_AUDIO_G711_A)
	{
		pAudioFmt->nCodecID = QC_CODEC_ID_G711A;
	}

	if (pTsTrackInfo->iCodecId == STREAM_TYPE_AUDIO_G711_U)
	{
		pAudioFmt->nCodecID = QC_CODEC_ID_G711U;
	}

	iRet = GetEsTrackInfoByPID(&m_sTsParserContext, pTsTrackInfo->ulPID, &sTrackInfoDesc);
	if (iRet == 1)
	{
		pAudioFmt->nSampleRate = sTrackInfoDesc.iSampleRate;
		pAudioFmt->nChannels = sTrackInfoDesc.iChannelCount;
	}

	pTsTrackInfo->iGetHeaderInfo = 1;
	CommitMediaHeader(pTsTrackInfo->pHeaderData, pTsTrackInfo->ulHeaderDataSize, (void *)pAudioFmt, MEDIA_AUDIO_IN_TS);
	return 0;
}

int CTSParser::FlushData()
{
	int iIndex = 0;
	int iFrameIndex = 0;
	S_TS_Track_Info *pTsTrackInfo = NULL;
	S_Ts_Media_Sample aFrames[256] = { 0 };
	int iFrameCount = 0;
	CheckTimestampCache *pCache = NULL;
	S_Ts_Media_Sample *pFrameArray = NULL;
	long long ullLastTime = 0;

	do
	{
		FlushAllCacheData(&m_sTsParserContext);
		for (iIndex = 0; iIndex < m_iCurTrackCount; iIndex++)
		{
			memset(aFrames, 0, sizeof(S_Ts_Media_Sample) * 256);
			pTsTrackInfo = m_pTsTrackInfo[iIndex];
			if (pTsTrackInfo != NULL)
			{
				iFrameCount = 0;
				iFrameIndex = 0;
				if (pTsTrackInfo->pFrameSpliter != NULL)
				{
					pTsTrackInfo->pFrameSpliter->FlushAllData(256, aFrames, iFrameCount);
				}

				pCache = pTsTrackInfo->pTimeStampCheckCache;
				if (pCache != NULL && pCache->GetBufferCount() > 1)
				{
					ullLastTime = pCache->GetLastTimeStamp();
					pFrameArray = pCache->GetCacheFrameArray();
					for (iFrameIndex = 0; iFrameIndex < pCache->GetBufferCount(); iFrameIndex++)
					{
						pFrameArray[iFrameIndex].ulMediaCodecId = pTsTrackInfo->iCodecId;
						ullLastTime = pFrameArray[iFrameIndex].ullTimeStamp = pFrameArray[iFrameIndex].ullTimeStamp + GetFrameDuration(&(pFrameArray[iFrameIndex])) * (iFrameIndex + 1);
						switch (pTsTrackInfo->eMediaType)
						{
						case QC_MEDIA_Video:
						{
							pFrameArray[iFrameIndex].usMediaType = MEDIA_VIDEO_IN_TS;
							break;
						}

						case QC_MEDIA_Audio:
						{
							pFrameArray[iFrameIndex].usMediaType = MEDIA_AUDIO_IN_TS;
							break;
						}
						}

						pFrameArray[iFrameIndex].usTrackId = pTsTrackInfo->ulPID;
						CommitMediaFrameToBuffer(pTsTrackInfo->iCommitEnabled, &(pFrameArray[iFrameIndex]));
					}

					pCache->Reset();
				}

				for (iFrameIndex = 0; iFrameIndex < iFrameCount; iFrameIndex++)
				{
					aFrames[iFrameIndex].ulMediaCodecId = pTsTrackInfo->iCodecId;
					switch (pTsTrackInfo->eMediaType)
					{
					case QC_MEDIA_Video:
					{
						aFrames[iFrameIndex].usMediaType = MEDIA_VIDEO_IN_TS;
						break;
					}

					case QC_MEDIA_Audio:
					{
						if (ullLastTime != 0)
						{
							aFrames[iFrameIndex].ullTimeStamp = ullLastTime + GetFrameDuration(&(aFrames[iFrameIndex])) * (iFrameIndex + 1);
						}
						aFrames[iFrameIndex].usMediaType = MEDIA_AUDIO_IN_TS;
						break;
					}
					}

					aFrames[iFrameIndex].usTrackId = pTsTrackInfo->ulPID;
					CommitMediaFrameToBuffer(pTsTrackInfo->iCommitEnabled, &(aFrames[iFrameIndex]));
				}
			}
		}
	} while (false);

	return 0;
}

int CTSParser::GetFrameDuration(S_Ts_Media_Sample *pTsMediaSample)
{
	unsigned int s_dwSamplingRates[16] = { 96000, 88200, 64000, 48000,
		44100, 32000, 24000, 22050,
		16000, 12000, 11025, 8000,
		0, 0, 0, 0 };

	unsigned int btSampleRateIndex = 0;
	unsigned int dSampleTime = 0;
	unsigned char *pFrameStart = NULL;

	if (pTsMediaSample->ulMediaCodecId == STREAM_TYPE_AUDIO_AAC)
	{
		if (pTsMediaSample->pSampleBuffer == NULL || pTsMediaSample->ulSampleBufferSize < 7)
		{
			return 0;
		}

		pFrameStart = pTsMediaSample->pSampleBuffer;
		btSampleRateIndex = (pFrameStart[2] >> 2) & 0xF;
		dSampleTime = double(1024) * 1000 / s_dwSamplingRates[btSampleRateIndex];
		return dSampleTime;
	}

	return 0;
}

int CTSParser::SplitMediaFrame(S_TS_Track_Info *pTsTrackInfo, void *pInputMediaFrame, int iArrayMaxSize, S_Ts_Media_Sample *pMediaFrameArray, int &iSplitCount)
{
	S_Ts_Media_Sample *pTsFrame = (S_Ts_Media_Sample *)pInputMediaFrame;
	S_Ts_Media_Sample *pOutputTsFrames = (S_Ts_Media_Sample *)pMediaFrameArray;

	iSplitCount = 0;
	if (pTsTrackInfo == NULL || pInputMediaFrame == NULL || pMediaFrameArray == NULL)
	{
		return QC_ERR_NEEDMORE;
	}

	switch (pTsTrackInfo->iCodecId)
	{
	case STREAM_TYPE_AUDIO_AAC:
	case STREAM_TYPE_VIDEO_H264:
	case STREAM_TYPE_VIDEO_HEVC:
	{
		if (pTsTrackInfo->pFrameSpliter != NULL)
		{
			pTsTrackInfo->pFrameSpliter->CommitInputAndSplitting(pTsFrame->pSampleBuffer, pTsFrame->ulSampleBufferSize, pTsFrame->ullTimeStamp,
				iArrayMaxSize, pMediaFrameArray, iSplitCount);
			if (iSplitCount > 0)
			{
				for (int i = 0; i < iSplitCount; i++)
				{
					pMediaFrameArray[i].ulMediaCodecId = pTsFrame->ulMediaCodecId;
					pMediaFrameArray[i].usMediaType = pTsFrame->usMediaType;
					pMediaFrameArray[i].usTrackId = pTsFrame->usTrackId;
				}
			}
		}
		break;
	}

	default:
	{
		pMediaFrameArray[0].ullTimeStamp = pTsFrame->ullTimeStamp;
		pMediaFrameArray[0].pSampleBuffer = pTsFrame->pSampleBuffer;
		pMediaFrameArray[0].ulMediaCodecId = pTsFrame->ulMediaCodecId;
		pMediaFrameArray[0].ulSampleBufferSize = pTsFrame->ulSampleBufferSize;
		pMediaFrameArray[0].ulSampleFlag = pTsFrame->ulSampleFlag;
		pMediaFrameArray[0].usMediaType = pTsFrame->usMediaType;
		pMediaFrameArray[0].usTrackId = pTsFrame->usTrackId;
		iSplitCount = 1;
		break;
	}
	}

	return QC_ERR_NONE;
}

int CTSParser::CommitMediaHeader(unsigned char *pData, int iDataSize, void *pTrackInfo, uint16 usMediaType)
{
	QCMediaType eMediaType = QC_MEDIA_MAX;
	QC_DATA_BUFF *pBuff = NULL;
	QC_AUDIO_FORMAT *pAudioTk = NULL;
	QC_VIDEO_FORMAT *pVideoTk = NULL;

	switch (usMediaType)
	{
	case MEDIA_AUDIO_IN_TS:
	{
		eMediaType = QC_MEDIA_Audio;
		pAudioTk = (QC_AUDIO_FORMAT *)pTrackInfo;
		break;
	}
	case MEDIA_VIDEO_IN_TS:
	{
		eMediaType = QC_MEDIA_Video;
		pVideoTk = (QC_VIDEO_FORMAT *)pTrackInfo;
		if (pVideoTk->nHeadSize > 0 && memcmp(pVideoTk->pHeadData, pData, iDataSize) != 0)
		{
			QCLOGI("Header data abnormal!");
		}
		m_sResInfo.nVideoCodec = pVideoTk->nCodecID;
		m_sResInfo.nAudioCodec = QC_CODEC_ID_AAC;
		m_sResInfo.nHeight = pVideoTk->nHeight;
		m_sResInfo.nWidth = pVideoTk->nWidth;

		DoVideoHeaderNotify();
		break;
	}
	case MEDIA_SUBTITLE_IN_TS:
	{
		eMediaType = QC_MEDIA_Subtt;
		break;
	}
	default:
	{
		return QC_ERR_FAILED;
	}
	}

	pBuff = m_pBuffMng->GetEmpty(eMediaType, iDataSize + 128);
	if (pBuff == NULL)
	{
		return QC_ERR_MEMORY;
	}

	pBuff->uBuffType = QC_BUFF_TYPE_Data;
	pBuff->nMediaType = eMediaType;
	pBuff->llTime = -1;

	switch (m_ulHeaderDataFlag)
	{
	case QC_PARSER_HEADER_FLAG_SEEK:
	{
		pBuff->uFlag = QCBUFF_HEADDATA;
		break;
	}

	default:
	{
		if (m_bTSFile)
			pBuff->uFlag = QCBUFF_NEW_FORMAT | QCBUFF_HEADDATA;
		else
			pBuff->uFlag = QCBUFF_NEWSTREAM | QCBUFF_NEW_FORMAT | QCBUFF_HEADDATA;
		break;
	}
	}
	pBuff->pFormat = pTrackInfo;

	if ((int)pBuff->uBuffSize < iDataSize + 128)
	{
		QC_DEL_A(pBuff->pBuff);
		pBuff->uBuffSize = iDataSize + 128;
	}
	if (pBuff->pBuff == NULL && pBuff->uBuffSize != 0)
	{
		pBuff->pBuff = new unsigned char[pBuff->uBuffSize];
	}

	memset(pBuff->pBuff, 0, pBuff->uBuffSize);
	memcpy((char *)(pBuff->pBuff), pData, iDataSize);
	pBuff->nValue = m_iBAMode;

	QCLOGI("Send header data, media type:%d, url:%s, BA mode:%d", pBuff->nMediaType, pBuff->pBuff, pBuff->nValue);
	pBuff->uSize = iDataSize;
	pBuff->nUsed--;
	if (m_pSendBuff != NULL)
	{
		((CAdaptiveStreamHLS *)m_pSendBuff)->SendBuff(pBuff);
	}
	else
	{
		m_pBuffMng->Send(pBuff);
	}
	return 0;
}

void CTSParser::DoVideoHeaderNotify()
{
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PARSER_NEW_STREAM, m_iBAMode, (long long)m_ulOffsetInM3u8, NULL, (void *)(&m_sResInfo));
}

void CTSParser::DumpMedia(void *pMediaFrame)
{
#ifdef _DUMP_FRAME_
	char strFrameInfo[256] = { 0 };
	char strDumpDataPath[256] = { 0 };
	char strDumpInfoPath[256] = { 0 };

	int iIndex = -1;
	int i = 0;
	S_Ts_Media_Sample *pDumpFrame = (S_Ts_Media_Sample *)pMediaFrame;
	if (pDumpFrame == NULL)
	{
		return;
	}

	for (i = 0; i < m_iPIDCount; i++)
	{
		if (m_aPIDArray[i] == pDumpFrame->usTrackId)
		{
			iIndex = i;
			break;
		}
	}

	if (iIndex == -1)
	{
		m_aPIDArray[m_iPIDCount] = pDumpFrame->usTrackId;
		iIndex = m_iPIDCount;
		m_iPIDCount++;
	}

	sprintf(strFrameInfo, "datasize:%d, time:%d, flag:%d\n", pDumpFrame->ulSampleBufferSize, (int)pDumpFrame->ullTimeStamp, pDumpFrame->ulSampleFlag);

	if (m_aFMediaData[iIndex] == NULL)
	{
		sprintf(strDumpDataPath, "Media_data_%d.dat", pDumpFrame->usMediaType);
		m_aFMediaData[iIndex] = fopen(strDumpDataPath, "ab");
	}

	if (m_aFMediaData[iIndex] == NULL)
	{
		return;
	}

	if (m_aFMediaInfo[iIndex] == NULL)
	{
		sprintf(strDumpInfoPath, "Media_Info_%d.dat", pDumpFrame->usMediaType);
		m_aFMediaInfo[iIndex] = fopen(strDumpInfoPath, "ab");
	}

	if (m_aFMediaInfo[iIndex] == NULL)
	{
		return;
	}

	fwrite(strFrameInfo, 1, strlen(strFrameInfo), m_aFMediaInfo[iIndex]);
	fwrite(pDumpFrame->pSampleBuffer, 1, pDumpFrame->ulSampleBufferSize, m_aFMediaData[iIndex]);

	fflush(m_aFMediaInfo[iIndex]);
	fflush(m_aFMediaData[iIndex]);
#endif
}

void CTSParser::InitDump()
{
#ifdef _DUMP_FRAME_
	m_iPIDCount = 0;
	for (int i = 0; i < MAX_TRACK_COUNT_IN_TS; i++)
	{
		m_aFMediaData[i] = NULL;
		m_aFMediaInfo[i] = NULL;
		m_aPIDArray[i] = 0;
	}
#endif
}

void CTSParser::UnInitDump()
{
#ifdef _DUMP_FRAME_
	for (int i = 0; i < MAX_TRACK_COUNT_IN_TS; i++)
	{
		if (m_aFMediaData[i] != NULL)
		{
			fclose(m_aFMediaData[i]);
			m_aFMediaData[i] = NULL;
		}

		if (m_aFMediaInfo[i] != NULL)
		{
			fclose(m_aFMediaInfo[i]);
			m_aFMediaInfo[i] = NULL;
		}
	}
#endif
}
