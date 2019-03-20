/*******************************************************************************
File:		CAdaptiveStreamHLS.cpp

Contains:	CAdaptiveStreamHLS implement file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-04		Qichao			Create file

*******************************************************************************/
#include "stdlib.h"
#include "CAdaptiveStreamHLS.h"
#include "CDataBox.h"
#include "qcErr.h"
#include "ULogFunc.h"
#include "USystemFunc.h"
#include "USourceFormat.h"
#include "HLSDRM.h"
#include "qcPlayer.h"
#include "CMsgMng.h"

#include "UUrlParser.h"

CAdaptiveStreamHLS::CAdaptiveStreamHLS(CBaseInst * pBaseInst) 
	: CBaseParser(pBaseInst)
	, m_pCurSegment(NULL)
	, m_pCurChunk(NULL)
	, m_nDownloadPercent(0)
{
	SetObjectName("CAdaptiveStreamHLS");
    m_llOffset = 0;
	m_pHLSParser = NULL;
	m_pAdaptiveBA = NULL;
	m_bHLSOpened = false;
//	InitContext();

	m_sSourceCallback.pUserData = this;
	m_sSourceCallback.SendEvent = OnEvent;
	m_pAdaptiveBA = NULL;
	m_pBitrateInfo = NULL;
	m_iBitrateCount = 0;
	m_iLastM3u8UpdateTime = 0;
	m_iLastSegmentDuration = 10000;
	m_llPreferBitrate = 0;
	m_bStopReadFlag = false;

	memset (m_szHostAddr, 0, sizeof(m_szHostAddr));

	m_llChunkTimeStart = -1;
	m_llChunkTimeVideo = -1;
	m_llChunkTimeAudio = -1;
	m_llVideoTimeNew = -1;
	m_llVideoTimeEnd = -1;
	m_llAudioTimeNew = -1;
	m_llAudioTimeEnd = -1;

	m_bLiveLive = false;
}

CAdaptiveStreamHLS::~CAdaptiveStreamHLS()
{
	Close();
}

int CAdaptiveStreamHLS::Open(QC_IO_Func * pIO, const char * pURL, int nFlag)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
		
	int iRet = 0;
	S_ADAPTIVESTREAM_PLAYLISTDATA  sPlaylistData = {0};
	int					iM3u8Size = 0;
	long long			illDownloadSpeed = 0;
	m_fIO = pIO;
	unsigned long long  ullDuration = 0;
	unsigned char*		pM3u8Data = NULL;

	memset(&sPlaylistData, 0, sizeof(S_ADAPTIVESTREAM_PLAYLISTDATA));

	char * pHostAddr = strstr((char *)pURL, "?domain=");
	if (pHostAddr != NULL)
		sprintf(m_szHostAddr, "Host:%s", pHostAddr + 8);

	m_ioProtocal = qcGetSourceProtocol (pURL);
	InitContext();
	iRet = CheckM3u8DataFromIO((char *)pURL, &m_pBufferForM3u8, &m_iBufferMaxSizeForM3u8, &iM3u8Size, illDownloadSpeed, nFlag);
	if (iRet != QC_ERR_NONE)
	{
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PARSER_M3U8_ERROR, 0, 0);
		return iRet;
	}

	sPlaylistData.pData = m_pBufferForM3u8;
	sPlaylistData.ulDataSize = (unsigned int)iM3u8Size;
	strcpy(sPlaylistData.strUrl, pURL);
	strcpy(sPlaylistData.strRootUrl, pURL);
	strcpy(sPlaylistData.strNewUrl, pURL);

	m_pHLSParser->Init_HLS(&sPlaylistData, &m_sSourceCallback);
	if (m_pHLSParser->Open_HLS() == QC_ERR_NONE)
	{
		m_bHLSOpened = true;
	}
	else
	{
		QCLOGI("url:%s, Open m3u8 fail!", pURL);
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PARSER_M3U8_ERROR, 0, 0);
		return QC_ERR_FAILED;
	}

	if (m_pHLSParser->GetProgramType() == E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_LIVE)
	{
		m_iLastM3u8UpdateTime = qcGetSysTime();
		m_bLive = true;
		m_bLiveLive = true;
	}

	m_pHLSParser->GetDuration_HLS(&ullDuration);
	FillBAInfo(pURL);
	m_llDuration = (long long)ullDuration;

	memset(&(m_sOutputCtx), 0, sizeof(S_Output_Ctrl_Ctx));
	SelectPreferBitrateInOpen();
	//Process first segment
	//
	m_nStrmVideoCount = 1;
	m_nStrmVideoPlay = 0;

	m_nStrmAudioCount = 1;
	m_nStrmAudioPlay = 0;

	m_nDownloadPercent = 0;
	m_pCurChunk = NULL;

	//only for qiniu drm test
	//SetParam(QC_PARSER_SET_QINIU_DRM_INFO, "kdnljjlcn2iu2384");
	//only for qiniu drm test


	CreateDefaultFmtInfo();

	return QC_ERR_NONE;
}

int CAdaptiveStreamHLS::Close(void)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);

	if (m_pHLSParser == NULL)
		return QC_ERR_NONE;

	StopAllRead();
	m_sStatus = QCWORK_Stop;
	CAutoLock    sAutolock(&m_mtFunc);
	m_pHLSParser->Uninit_HLS();
	QC_DEL_P(m_pHLSParser);
	QC_DEL_P(m_pAdaptiveBA);
	QC_DEL_A(m_pBufferForM3u8);
	QC_DEL_A(m_pBitrateInfo);
	
	for (int i = 0; i < MAX_SEGEMNT_WORKING_COUNT; i++)
	{
		QC_DEL_P(m_pTsParserWorking[i]);
	}

	for (int i = 0; i < MAX_SEGEMNT_WORKING_COUNT; i++)
	{
		if (m_pIOHandleWorking[i] != NULL)
		{
			if (m_pIOHandleWorking[i]->hIO != NULL)
				qcDestroyIO(m_pIOHandleWorking[i]);
		}
		QC_DEL_P(m_pIOHandleWorking[i]);
	}

	for (int i = 0; i < MAX_SEGEMNT_WORKING_COUNT; i++)
	{
		QC_DEL_P(m_pHLSNormalDrmWorking[i]);
	}

    for (int i = 0; i < MAX_SEGEMNT_WORKING_COUNT; i++)
	{
		QC_DEL_P(m_aSegmentBufferCtx[i].pSegmentBuffer);
	}

	return QC_ERR_NONE;
}

int CAdaptiveStreamHLS::Read(QC_DATA_BUFF * pBuff)
{
	int iRet = 0;
	if (m_bEOS == true)
	{
		return QC_ERR_FINISH;
	}
	else
	{
		CAutoLock    sAutolock(&m_mtFunc);
		DoM3u8UpdateIfNeed();
		iRet = ProcessSegment();
		if (iRet == QC_ERR_FINISH)
		{
			m_bEOS = true;
		}
		return iRet;
	}
}

int CAdaptiveStreamHLS::CanSeek (void)
{
	return 1;
}

bool CAdaptiveStreamHLS::IsLive(void)
{
	return m_bLiveLive;
}

long long CAdaptiveStreamHLS::GetPos (void)
{
	return 0;
}

long long CAdaptiveStreamHLS::SetPos(long long llPos)
{
	unsigned long long ullPos = llPos;
	if (llPos >= m_llDuration || llPos < 0)
	{
		return QC_ERR_FAILED;
	}

	QCLOGI("input Pos:%lld in SetPos", llPos);
	StopAllRead();
	CAutoLock    sAutolock(&m_mtFunc);
	InitAllSegmentHandleCtx();
	m_pHLSParser->Seek_HLS(&ullPos, E_ADAPTIVESTREAMPARSER_SEEK_MODE_NORMAL);
	ResetAllParser(true);
	m_pAdaptiveBA->ResetContext();
	m_bEOS = false;

	m_llChunkTimeStart = -1;
	m_llChunkTimeVideo = -1;
	m_llChunkTimeAudio = -1;
	m_llVideoTimeNew = -1;
	m_llVideoTimeEnd = -1;
	m_llAudioTimeNew = -1;
	m_llAudioTimeEnd = -1;

	return 0;
}

int CAdaptiveStreamHLS::GetParam(int nID, void * pParam)
{
	return CBaseParser::GetParam(nID, pParam);
}

int CAdaptiveStreamHLS::SetParam(int nID, void * pParam)
{
	switch (nID)
	{
		case QCPLAY_PID_Reconnect:
		{
			ReconnectAllIO();
			return QC_ERR_NONE;
		}

		case QCPARSER_PID_LastBitrate:
		{
			if (pParam != NULL)
			{
				m_llPreferBitrate = (long long)(*(int *)pParam);
			}
			return QC_ERR_NONE;
		}

		case QCPARSER_PID_ExitRead:
		{
			m_bStopReadFlag = true;
			return QC_ERR_NONE;
		}

		case QC_PARSER_SET_QINIU_DRM_INFO:
		{
			if (pParam != NULL)
			{
				m_iQiniuDrmInfoSize = 16;
				memcpy(m_aQiniuDrmInfo, (void *)pParam, 16);
			}
			return QC_ERR_NONE;
		}

		default:
			break;
	}

	return CBaseParser::SetParam(nID, pParam);
}

int CAdaptiveStreamHLS::SetStreamPlay(QCMediaType nType, int nStream)
{
	int						iRet = QC_ERR_FAILED;
	unsigned long long		ullOffset = 0;

	ullOffset = (unsigned long long)m_pBuffMng->GetPlayTime(QC_MEDIA_Video);
	QCLOGI("Media Type:%d, cur Pos:%d, stream id:%d", nType, (int)ullOffset, nStream);
	switch (nType)
	{
		case QC_MEDIA_Source:
		{
			if (nStream == -1)
			{
				CAutoLock    sAutolock(&m_mtFunc);
				iRet = m_pAdaptiveBA->SelectStream(ADAPTIVE_AUTO_STREAM_ID);
			}
			else
			{
				StopAllRead();
				CAutoLock    sAutolock(&m_mtFunc);
				iRet = m_pAdaptiveBA->SelectStream(nStream);
				m_pHLSParser->SelectStream_HLS(nStream, E_ADAPTIVESTREAMING_CHUNKPOS_PRESENT);
				m_pHLSParser->Seek_HLS(&ullOffset, E_ADAPTIVESTREAMPARSER_SEEK_MODE_NORMAL);
				InitAllSegmentHandleCtx();
				ResetAllParser(false);
			}

			break;
		}

		default:
		{
			break;
		}
	}
	return iRet;
}

int CAdaptiveStreamHLS::GetStreamFormat (int nID, QC_STREAM_FORMAT ** ppStreamFmt)
{
	if (m_pHLSParser == NULL || ppStreamFmt == NULL)
		return QC_ERR_STATUS;

	QC_STREAM_FORMAT**	ppStreamInfo = NULL;
	int nCount = 0;
	int nRC = m_pHLSParser->GetProgramInfo_HLS(ppStreamInfo, &nCount);
	if (nRC != QC_ERR_NONE)
		return nRC;
	if (nID < 0 || nID >= nCount)
		return QC_ERR_ARG;

	*ppStreamFmt = ppStreamInfo[nID];

	return QC_ERR_NONE;
}

int CAdaptiveStreamHLS::OnEvent(void* pUserData, unsigned int ulID, void * ulParam1, void * ulParam2)
{
	int iRet = 0;
	CAdaptiveStreamHLS * pIns = (CAdaptiveStreamHLS *)pUserData;
	if (!pIns)
	{
		return QC_ERR_FAILED;
	}

	switch (ulID)
	{
		case QC_EVENTID_ADAPTIVESTREAMING_NEEDPARSEITEM:
		{
			pIns->DownloadM3u8ForCallback(ulParam1);
			break;
		}
		default:
		{
			iRet = QC_ERR_IMPLEMENT;
			break;
		}
	}

	return iRet;
}

int CAdaptiveStreamHLS::DownloadM3u8ForCallback(void * nM3u8CallbackReq)
{
	int iMaxSize = 0;
	int iDataSize = 0;
	long long illDownloadSpeed = 0;
	int iRet = QC_ERR_NONE;
	S_ADAPTIVESTREAM_PLAYLISTDATA * pData = (S_ADAPTIVESTREAM_PLAYLISTDATA *)nM3u8CallbackReq;
	char  strAbsoluteURL[4096] = { 0 };
	if (pData == NULL)
	{
		return QC_ERR_FAILED;
	}

	if (pData->pReserve == NULL)
	{
		return QC_ERR_FAILED;
	}

	GetAbsoluteURL(strAbsoluteURL, pData->strUrl, pData->strRootUrl);
	S_DATABOX_CALLBACK* pCallBack = (S_DATABOX_CALLBACK*)pData->pReserve;
	memset(pData->strNewUrl, 0, sizeof(pData->strNewUrl));
	memcpy(pData->strNewUrl, strAbsoluteURL, strlen(strAbsoluteURL));
	pData->ulDataSize = 256 * 1024;
	pCallBack->MallocData(pCallBack->pUserData, &pData->pData, pData->ulDataSize);
	iMaxSize = pData->ulDataSize;
	CDataBox * pDataBox = (CDataBox *)pCallBack->pUserData;
	iRet = CheckM3u8DataFromIO(strAbsoluteURL, &pData->pData, &iMaxSize, &iDataSize, illDownloadSpeed, QCIO_OPEN_CONTENT);
	if (iRet != QC_ERR_NONE)
	{
		QCLOGI("GET URL:%s data fail!", strAbsoluteURL);
		return QC_ERR_FAILED;
	}
	pDataBox->m_pData = pData->pData;
	pDataBox->m_uDataSize = iDataSize;
	pData->ulDataSize = iDataSize;
	return 0;
}

void CAdaptiveStreamHLS::InitContext()
{
	m_pHLSParser = new C_HLS_Entity (m_pBaseInst);
	m_pAdaptiveBA = new CAdaptiveStreamBA (m_pBaseInst);

	InitAllIOContext();
	InitAllParserContext();
	InitAllIOContext();
	InitAllDrmContext();
	InitAllM3u8SegmentBuffer();
	InitAllSegmentHandleCtx();
}

void CAdaptiveStreamHLS::InitAllDrmContext()
{
	for (int i = 0; i < MAX_SEGEMNT_WORKING_COUNT; i++)
	{
		m_pHLSNormalDrmWorking[i] = NULL;
	}

    memset(m_aQiniuDrmInfo, 0, 128);
	m_iQiniuDrmInfoSize = 0;
}

void CAdaptiveStreamHLS::InitAllParserContext()
{
	for (int i = 0; i < MAX_SEGEMNT_WORKING_COUNT; i++)
	{
		m_pTsParserWorking[i] = NULL;
	}
}

void CAdaptiveStreamHLS::InitAllM3u8SegmentBuffer()
{
	m_pBufferForM3u8 = NULL;
	m_iBufferMaxSizeForM3u8 = 0;

	for (int i = 0; i < MAX_SEGEMNT_WORKING_COUNT; i++)
	{
		m_aSegmentBufferCtx[i].pSegmentBuffer = NULL;
		m_aSegmentBufferCtx[i].iSegmentMaxSize = 0;
	}
}

void CAdaptiveStreamHLS::InitAllIOContext()
{
	for (int i = 0; i < MAX_SEGEMNT_WORKING_COUNT; i++)
	{
		m_pIOHandleWorking[i] = NULL;
	}
}

void CAdaptiveStreamHLS::InitAllSegmentHandleCtx()
{
	memset(m_aSegmentHandleCtx, 0, sizeof(S_Segment_Handle_Ctx)*MAX_SEGEMNT_WORKING_COUNT);
}


void CAdaptiveStreamHLS::StopAllRead()
{
	//unit ms
	int  iMaxWaitTime = 1000;
	int  iStartSysTime = qcGetSysTime();
	int  iCurSysTime = 0;
	bool bWaitNext = false;
	for (int i = 0; i < MAX_SEGEMNT_WORKING_COUNT; i++)
		m_aSegmentHandleCtx[i].ulForceStopFlag = 1;

	iCurSysTime = qcGetSysTime();
	while (((iCurSysTime - iStartSysTime) < iMaxWaitTime))
	{
		bWaitNext = false;
		for (int i = 0; i < MAX_SEGEMNT_WORKING_COUNT; i++)
		{
			if (m_aSegmentHandleCtx[i].ulReadStateFlag == WORKING_STATE_FOR_READ)
			{
				bWaitNext = true;
			}
		}

		if (bWaitNext == false)
		{
			break;
		}
		iCurSysTime = qcGetSysTime();
	}

	QCLOGI("Wait time:%d", iCurSysTime - iStartSysTime);
}

int CAdaptiveStreamHLS::ProcessSegment()
{
	S_Segment_Handle_Ctx*	pSegmentHandleCtx = NULL;
	char					strAbsoluteURL[4096] = { 0 };
	int						iRet = 0;
	unsigned int			ulRet = 0;
	bool					bResetParser = false;
	int						iNewStreamId = 0;
	unsigned char*			pSegmentBuffer = NULL;

	S_ADAPTIVESTREAMPARSER_CHUNK*   pChunk = NULL;
	long long				illDownloadSpeed = 0;
	long long				ullBufferTime = 0;
	long long				ullTranactTime = 0;
	unsigned int			ulM3u8StartOffset = 0;
	unsigned long long		ullCurPos = 0;
	void*					pDrmInfo = NULL;
	bool					bVideoBetter = false;
	int                     iBAMode = 0;
	QC_RESOURCE_INFO        sResInfo = {0};

	if (m_bHLSOpened == false)
	{
		return QC_ERR_FAILED;
	}

	if (m_bLive == false)
	{
		ullCurPos = (unsigned long long)m_pBuffMng->GetPlayTime(QC_MEDIA_Video);
	}
	else
	{
		ullCurPos = (unsigned long long)m_pBuffMng->GetBuffM3u8Pos();
	}

	ullBufferTime = m_pBuffMng->GetBuffTime(QC_MEDIA_Video);

	pSegmentHandleCtx = GetWorkingSegmentHandleCtx();
	if (pSegmentHandleCtx == NULL)
	{
		//no working segment
		if (m_pAdaptiveBA && m_pAdaptiveBA->GetStreamIDForNext(iNewStreamId, ullBufferTime, bVideoBetter, iBAMode) == QC_ERR_NONE)
		{
			QCLOGI("cur play pos:%d, cur buffer time:%d", (int)ullCurPos, (int)ullBufferTime);
			bResetParser = true;
			QCLOGI("do bitrate change to stream id:%d", iNewStreamId);
			m_pHLSParser->SelectStream_HLS(iNewStreamId, E_ADAPTIVESTREAMING_CHUNKPOS_PRESENT);

			//only vod seek
			if (m_bLive == false)
			{
				if (bVideoBetter == true)
				{
					QCLOGI("Seek Pos:%lld, for better", ullCurPos);
					m_pHLSParser->Seek_HLS(&ullCurPos, E_ADAPTIVESTREAMPARSER_SEEK_MODE_FOR_SWITCH_STREAM);
				}
				else
				{
					ullCurPos = ullCurPos + ullBufferTime - DEFAULT_ROLL_BACK_LENGTH_BITRATE_DROP;
					QCLOGI("Seek Pos:%lld, for bader", ullCurPos);
					m_pHLSParser->Seek_HLS(&ullCurPos, E_ADAPTIVESTREAMPARSER_SEEK_MODE_NORMAL);
				}
			}

			m_nStrmSourcePlay = iNewStreamId;
		}

		ulRet = m_pHLSParser->GetChunk_HLS(E_ADAPTIVESTREAMPARSER_CHUNKTYPE_AUDIOVIDEO, &pChunk);
		if (ulRet == 0)
		{	
			m_iLastSegmentDuration = pChunk->ullDuration;
			pSegmentHandleCtx = GetIdleSegmentHandleCtx(pChunk->eType);
			if (pSegmentHandleCtx == NULL)
			{
				return QC_ERR_FAILED;
			}

			pSegmentHandleCtx->pTsParser = GetTsParserByChunkType(pChunk->eType, true);
			pSegmentHandleCtx->pIOHandle = GetIOHandleByChunkType(pChunk->eType, true);
			pSegmentHandleCtx->pBufferCtx = GetSegmentBufferByChunkType(pChunk->eType, true);
			pSegmentHandleCtx->pDrmIns = GetDrmHandleByChunkType(pChunk->eType, true);
			pSegmentHandleCtx->iStartTransTime = qcGetSysTime();
			pSegmentHandleCtx->pDrmInfo = pChunk->pChunkDRMInfo;

			if (m_bLive == true)
			{
				pSegmentHandleCtx->ullM3u8Offset = pChunk->ullChunkLiveTime;
			}
			else
			{
				pSegmentHandleCtx->ullM3u8Offset = pChunk->ullStartTime;
			}

			pSegmentHandleCtx->ulPlaylistId = pChunk->ulPlaylistId;
			pSegmentHandleCtx->ulChunkID = pChunk->ulChunkID;

			illDownloadSpeed = 0;
			GetAbsoluteURL(pSegmentHandleCtx->strURL, pChunk->strUrl, pChunk->strRootUrl);
			if (bResetParser == true)
			{
				pSegmentHandleCtx->pTsParser->SetParam(QC_EVENTID_RESET_TS_PARSER_FOR_NEW_STREAM, NULL);
			}

			if (m_bLive == false && m_llDuration <= (pChunk->ullStartTime + pChunk->ullDuration))
			{
				pSegmentHandleCtx->bVodEnd = true;
			}

			FillResInfo(&sResInfo, pChunk->ulPlaylistId);
			pSegmentHandleCtx->pTsParser->SetParam(QC_EVENTID_SET_TS_PARSER_RES_INFO, &sResInfo);
			pSegmentHandleCtx->pTsParser->SetParam(QC_EVENTID_SET_TS_PARSER_BA_MODE, &iBAMode);

			m_llChunkTimeStart = pChunk->ullStartTime;
		}
		else
		{
			return ulRet;
		}
	}

	//QCLOGI("handle segment! url:%s, handle offset:%d, segment size:%d", pSegmentHandleCtx->strURL, (int)pSegmentHandleCtx->illHandleOffset, (int)pSegmentHandleCtx->illSegmentSize);
	if (pSegmentHandleCtx != NULL)
	{
		m_pCurSegment = pSegmentHandleCtx;
		if (pChunk != NULL)
			m_pCurChunk = pChunk;

		pSegmentHandleCtx->ulReadStateFlag = WORKING_STATE_FOR_READ;
		iRet = HandleSegmentFromIO(pSegmentHandleCtx, illDownloadSpeed);
		pSegmentHandleCtx->ulReadStateFlag = IDLE_STATE_FOR_READ;
		illDownloadSpeed *= 8;
		if (iRet == QC_ERR_NONE && pSegmentHandleCtx->illSegmentSize > 0 && pSegmentHandleCtx->illSegmentSize == pSegmentHandleCtx->illHandleOffset)
		{
			m_pAdaptiveBA->SetCurBitrate(pSegmentHandleCtx->ulPlaylistId, illDownloadSpeed, qcGetSysTime() - pSegmentHandleCtx->iStartTransTime, pSegmentHandleCtx->ullM3u8Offset);
			if (pSegmentHandleCtx->bVodEnd)
			{
				pSegmentHandleCtx->pTsParser->SetParam(QC_EVENTID_SET_TS_PARSER_FLUSH, NULL);
			}
			DumpSegment(pSegmentHandleCtx->ulPlaylistId, pSegmentHandleCtx->ulChunkID, pSegmentHandleCtx->pBufferCtx->pSegmentBuffer, (int)pSegmentHandleCtx->illSegmentSize);
			memset(pSegmentHandleCtx, 0, sizeof(S_Segment_Handle_Ctx));
		}
		else
		{
			// QCLOGI("handle segment fail! url:%s, handle size:%d, segment size:%d", pSegmentHandleCtx->strURL, (int)pSegmentHandleCtx->illHandleOffset, (int)pSegmentHandleCtx->illSegmentSize);
			if (iRet != QC_ERR_STATUS)
			{
				ulRet = QC_ERR_RETRY;
			}
			else
			{
				ulRet = QC_ERR_STATUS;
			}
		}
		m_pCurSegment = NULL;
	}

	//restore the read flag
	m_bStopReadFlag = false;

	return ulRet;
}

void CAdaptiveStreamHLS::ReconnectAllIO()
{
	QC_IO_Func*   pRet = NULL;
	int iIndex = -1;
	for (iIndex = 0; iIndex < MAX_SEGEMNT_WORKING_COUNT; iIndex++)
	{
		if (m_pIOHandleWorking[iIndex] != NULL)
		{
			m_pIOHandleWorking[iIndex]->Reconnect(m_pIOHandleWorking[iIndex]->hIO, NULL, -1);
		}
	}
}

int CAdaptiveStreamHLS::ReallocBufferAsNeed(unsigned char*&  pCurBuffer, int& iCurMaxSize, int iNewSize, int iCurDataSize, int  iExtraAppendSize)
{
	unsigned char* pBufferNew = NULL;
	int     iRet = 0;
	if (iCurMaxSize < iNewSize)
	{
		pBufferNew = new unsigned char[(unsigned int)iNewSize + iExtraAppendSize];
		if (pBufferNew == NULL)
			return QC_ERR_MEMORY;		

		memset(pBufferNew, 0, iNewSize + iExtraAppendSize);
		if (iCurDataSize > 0)
			memcpy(pBufferNew, pCurBuffer, iCurDataSize);
		iCurMaxSize = iNewSize + iExtraAppendSize;
		QC_DEL_A(pCurBuffer);
		pCurBuffer = pBufferNew;
	}

	return 0;
}

void CAdaptiveStreamHLS::DoM3u8UpdateIfNeed()
{
	int illLastUpdateTime = 0;
	if (m_bLive == true)
	{
		illLastUpdateTime = qcGetSysTime();
		if ((illLastUpdateTime - m_iLastM3u8UpdateTime) > (m_iLastSegmentDuration * 2 / 3))
		{
			m_iLastM3u8UpdateTime = qcGetSysTime();
			m_pHLSParser->UpdateThePlayListForLive();
		}
	}
}

void CAdaptiveStreamHLS::AdjustFrameFlags(QC_DATA_BUFF * pBuff)
{
	if ((pBuff->uFlag & QCBUFF_HEADDATA) != 0)
	{
		if (m_sOutputCtx.iMediaStreamCount == 1)
		{
			switch (pBuff->nMediaType)
			{
			case QC_MEDIA_Video:
			{
				if (m_sOutputCtx.iVideoTrackInfoDone == 0)
				{
					pBuff->uFlag |= QCBUFF_NEW_FORMAT;
					m_sOutputCtx.iVideoTrackInfoDone = 1;
				}
				break;
			}

			case QC_MEDIA_Audio:
			{
				if (m_sOutputCtx.iAudioTrackInfoDone == 0)
				{
					pBuff->uFlag |= QCBUFF_NEW_FORMAT;
					m_sOutputCtx.iAudioTrackInfoDone = 1;
				}
				break;
			}

			case QC_MEDIA_Subtt:
			{
				if (m_sOutputCtx.iSubTrackInfoDone == 0)
				{
					pBuff->uFlag |= QCBUFF_NEW_FORMAT;
					m_sOutputCtx.iSubTrackInfoDone = 1;
				}
				break;
			}

			default:
			{
				break;
			}
			}
		}
		else
		{
			//Do nothing to multi bitrate
		}
	}
}

int CAdaptiveStreamHLS::CheckM3u8DataFromIO(char*  pURL, unsigned char**  ppBufferOutput, int* piBufferMaxSize, int* piDataSize, long long&  illDownloadBitrate, int nFlag)
{
	long  long  illDataSize = 0;
	unsigned char*    pBufferNew = NULL;
	int   iPerReadSize = 0;
	int   iTotalReadSize = 0;
	int   iRet = QC_ERR_NONE;
	int   nRestSize = 0;
	unsigned  char   aHeaderContent[4096] = { 0 };
	int   iHeaderSizeForCheck = 1024;
	int   iDoneLoadHeaderSize = 0;
	char*   pFind = NULL;
	int   iChunckModeSize = 64*1024;
	bool  bChunkMode = false;
	bool  bGotFinish = false;
	int   iPerRead = 1024;

	do 
	{
		if (m_fIO->Open(m_fIO->hIO, pURL, 0, QCIO_FLAG_READ | nFlag) != QC_ERR_NONE)
		{
			QCLOGE("Can't open the url:%s", pURL);
			iRet = QC_ERR_FAILED;
			break;
		}

		illDataSize = m_fIO->GetSize(m_fIO->hIO);
		if (illDataSize == -1)
		{
			QCLOGE("Can't get the ");
			iRet = QC_ERR_FAILED;
			break;
		}

		if (illDataSize == QCIO_MAX_CONTENT_LEN)
		{
			illDataSize = iChunckModeSize;
			bChunkMode = true;
		}

		if (illDataSize > DEFAULT_M3U8_MAX_DATA_SIZE || bChunkMode == true)
		{
			//m3u8 size is too large, should check the content
			iRet = m_fIO->Read(m_fIO->hIO, aHeaderContent, iHeaderSizeForCheck, true, QCIO_READ_DATA);
			if (iRet != QC_ERR_NONE)
			{
				iRet = QC_ERR_FAILED;
				break;
			}

			pFind = strstr((char*)aHeaderContent, "#EXT");
			if (pFind == NULL)
			{
				iRet = QC_ERR_FAILED;
				break;
			}
			else
			{
				if (memcmp(pFind, "#EXTM3U", strlen("#EXTM3U")) != 0)
				{
					QCLOGI("can't find the M3U Begin!");
					iRet = QC_ERR_FAILED;
					break;
				}
			}

			iDoneLoadHeaderSize = iHeaderSizeForCheck;
		}

		nRestSize = (int)illDataSize;

		iRet = ReallocBufferAsNeed(*ppBufferOutput, *piBufferMaxSize, illDataSize, 0, 32*1024);
		if (iRet == QC_ERR_MEMORY)
		{
			break;
		}

		if (iDoneLoadHeaderSize > 0)
		{
			memcpy(*ppBufferOutput + iTotalReadSize, aHeaderContent, iDoneLoadHeaderSize);
			iTotalReadSize += iDoneLoadHeaderSize;
		}

		while(((bChunkMode == false && iTotalReadSize < illDataSize) ||
			(bChunkMode == true && bGotFinish == false)) &&
			m_sStatus != QCWORK_Stop && 
			((m_pBaseInst == NULL) || 
			(m_pBaseInst != NULL && (m_pBaseInst->m_bForceClose != true))))
		{
			nRestSize = iPerRead;

			iRet = ReallocBufferAsNeed(*ppBufferOutput, *piBufferMaxSize, iTotalReadSize + nRestSize, iTotalReadSize, 32 * 1024);
			if (iRet == QC_ERR_MEMORY)
			{
				break;
			}

			iRet = m_fIO->Read(m_fIO->hIO, (*ppBufferOutput) + iTotalReadSize, nRestSize, false, QCIO_READ_DATA);
			if (iRet == QC_ERR_STATUS)
			{
				QCLOGI("Download Err!");
				break;
			}

			iTotalReadSize += nRestSize;
			if (iRet == QC_ERR_FINISH)
			{
				iRet = QC_ERR_NONE;
				bGotFinish = true;
			}
			qcSleep(1000);
		}

	} while (false);
	

	if (iRet != QC_ERR_NONE)
	{
		illDownloadBitrate = 0;
	}
	else
	{
		illDownloadBitrate = m_fIO->GetSpeed(m_fIO->hIO, 5);
	}
	*piDataSize = iTotalReadSize;
	m_fIO->Close(m_fIO->hIO);
	return iRet;
}

int CAdaptiveStreamHLS::HandleSegmentFromIO(S_Segment_Handle_Ctx*  pSegmentHandleCtx, long long&  illDownloadBitrate)
{
	long  long  illDataSize = 0;
	unsigned char*    pBufferNew = NULL;
	int   iPerReadSize = 0;
	int   iTotalReadSize = 0;
	int   iRet = QC_ERR_NONE;
	int   nRestSize = 0;
	int   iStartTime = qcGetSysTime();
	S_DRM_HSL_PROCESS_INFO*   pDRMHls = (S_DRM_HSL_PROCESS_INFO*)pSegmentHandleCtx->pDrmInfo;
	unsigned char     abufferDrm[188 * 256] = {0};
	int                iPreferSize = PERFER_READ_FILE_FOR_TS;
	unsigned int       ulDataSize = 0;
	unsigned int       ulM3u8Offset = 0;
	bool               bEnd = false;
	long long          illLeftSize = 0;
	int                iParsedCount = 0;
	long long          illDefaultChunkModeSize = 2 * 1024 * 1024;
	CAutoLock		   sAutolock(&m_mtFunc);
	int                iCurTime = 0;
	QC_IO_Func* pIOHandle = NULL;

	do
	{
		pIOHandle = pSegmentHandleCtx->pIOHandle;
		if (pIOHandle == NULL)
		{
			break;
		}

		if (pSegmentHandleCtx->pTsParser != NULL && pSegmentHandleCtx->illSegmentSize == 0 && pSegmentHandleCtx->illHandleOffset == 0)
		{
			ulM3u8Offset = (unsigned int)pSegmentHandleCtx->ullM3u8Offset;
			pSegmentHandleCtx->pTsParser->SetParam(QC_EVENTID_SET_TS_PARSER_TIME_OFFSET, &ulM3u8Offset);
			if (pSegmentHandleCtx->pDrmIns != NULL && pSegmentHandleCtx->pDrmInfo != NULL)
			{
				memset(pDRMHls->strCurURL, 0, 1024);
				strcpy(pDRMHls->strCurURL, pSegmentHandleCtx->strURL);
				pDRMHls->pReserved = pSegmentHandleCtx->pIOHandle;
				pSegmentHandleCtx->pDrmIns->Init(pSegmentHandleCtx->pDrmInfo, m_iQiniuDrmInfoSize, m_aQiniuDrmInfo);
			}

			if (pSegmentHandleCtx->ulLastConnectFailTime != 0 )
			{
				if ((int)(qcGetSysTime() - pSegmentHandleCtx->ulLastConnectFailTime) < m_pBaseInst->m_pSetting->g_qcs_nReconnectInterval)
				{
					//QCLOGI("within Connect fail interval, try next time!");
					iRet = QC_ERR_RETRY;
					break;
				}
			}

			iRet = pSegmentHandleCtx->pIOHandle->Open(pIOHandle->hIO, pSegmentHandleCtx->strURL, 0, QCIO_FLAG_READ | QCIO_OPEN_CONTENT);
			if (iRet != QC_ERR_NONE)
			{
				if (QC_ERR_CANNOT_CONNECT == iRet)
				{
					pSegmentHandleCtx->ulLastConnectFailTime = qcGetSysTime();
				}

				QCLOGE("Can't open the url:%s", pSegmentHandleCtx->strURL);
				break;
			}
			if (m_bLive == true)
			{
				if (!pSegmentHandleCtx->pIOHandle->IsStreaming(pIOHandle->hIO))
					m_bLiveLive = false;
			}

			illDataSize = pSegmentHandleCtx->pIOHandle->GetSize(pIOHandle->hIO);
			illLeftSize = illDataSize;
			if (illDataSize == -1)
			{
				QCLOGE("Can't get the size info");
				break;
			}

			if (illDataSize == QCIO_MAX_CONTENT_LEN)
			{
				pSegmentHandleCtx->bChunkMode = true;
				pSegmentHandleCtx->illSegmentSize = illDefaultChunkModeSize;
				pSegmentHandleCtx->bGotFinished = false;
			}
			else
			{
				pSegmentHandleCtx->bChunkMode = false;
				pSegmentHandleCtx->bGotFinished = false;
				pSegmentHandleCtx->illSegmentSize = illLeftSize;
			}

			QCLOGI("segment size:%d", (int)pSegmentHandleCtx->illSegmentSize);

			iRet = ReallocBufferAsNeed(pSegmentHandleCtx->pBufferCtx->pSegmentBuffer, pSegmentHandleCtx->pBufferCtx->iSegmentMaxSize, pSegmentHandleCtx->illSegmentSize, 0, 256*1024);
			if (iRet == QC_ERR_MEMORY)
			{
				break;
			}
		}

		illLeftSize = pSegmentHandleCtx->illSegmentSize - pSegmentHandleCtx->illHandleOffset;
		while (((illLeftSize > 0 && pSegmentHandleCtx->bChunkMode == false) || (pSegmentHandleCtx->bChunkMode == true && pSegmentHandleCtx->bGotFinished == false)) &&
			m_sStatus != QCWORK_Stop &&
			pSegmentHandleCtx->ulForceStopFlag == 0 && m_bStopReadFlag == false &&
			((m_pBaseInst == NULL) ||
			(m_pBaseInst != NULL && (m_pBaseInst->m_bForceClose != true)))
			)
		{
			iPreferSize = PERFER_READ_FILE_FOR_TS;

			iRet = ReallocBufferAsNeed(pSegmentHandleCtx->pBufferCtx->pSegmentBuffer, pSegmentHandleCtx->pBufferCtx->iSegmentMaxSize, pSegmentHandleCtx->illHandleOffset + iPreferSize, 
									pSegmentHandleCtx->illHandleOffset, 256 * 1024);
			if (iRet == QC_ERR_MEMORY)
			{
				break;
			}

			if (pSegmentHandleCtx->pDrmIns->NeedDecrypt() && pSegmentHandleCtx->ulForceStopFlag == 0)
			{
				ulDataSize = 0;
				iRet = pSegmentHandleCtx->pIOHandle->Read(pIOHandle->hIO, abufferDrm, iPreferSize, true, QCIO_READ_DATA);
				if (iRet != QC_ERR_NONE && iRet != QC_ERR_FINISH )
				{
					//QCLOGI("Download Err!");
					break;
				}

				bEnd = (iPreferSize >= illLeftSize) ? true : false;
				pSegmentHandleCtx->pDrmIns->DecryptData(abufferDrm, iPreferSize, pSegmentHandleCtx->pBufferCtx->pSegmentBuffer + pSegmentHandleCtx->illHandleOffset, &ulDataSize, bEnd);
			}
			else
			{
				iRet = pSegmentHandleCtx->pIOHandle->Read(pIOHandle->hIO, pSegmentHandleCtx->pBufferCtx->pSegmentBuffer + pSegmentHandleCtx->illHandleOffset, iPreferSize, false, QCIO_READ_DATA);
				if (iRet != QC_ERR_NONE && iRet != QC_ERR_FINISH)
				{
					//QCLOGI("Download Err!");
					break;
				}


				ulDataSize = (unsigned int)iPreferSize;
			}

			if (pSegmentHandleCtx->pTsParser != NULL && ulDataSize >= 0 && pSegmentHandleCtx->ulForceStopFlag == 0)
			{
				iParsedCount = pSegmentHandleCtx->pTsParser->Process(pSegmentHandleCtx->pBufferCtx->pSegmentBuffer + pSegmentHandleCtx->illHandleOffset, (int)ulDataSize);
				illLeftSize -= (int)iPreferSize;
				pSegmentHandleCtx->illHandleOffset += ((long long)iPreferSize);
			}

			if (iRet == QC_ERR_FINISH)
			{
				pSegmentHandleCtx->bGotFinished = true;
			}

			qcSleep(5000);
		}
	} while (false);

	if (pSegmentHandleCtx->bChunkMode == true && pSegmentHandleCtx->bGotFinished == true)
	{
		pSegmentHandleCtx->illSegmentSize = pSegmentHandleCtx->illHandleOffset;
	}

	if ((iRet == QC_ERR_NONE || iRet == QC_ERR_FINISH) && pIOHandle != NULL)
	{
		illDownloadBitrate = pIOHandle->GetSpeed(pIOHandle->hIO, 5);
		iRet = QC_ERR_NONE;
	}

	if (pSegmentHandleCtx->illHandleOffset == pSegmentHandleCtx->illSegmentSize && pSegmentHandleCtx->illSegmentSize > 0 && pIOHandle != NULL)
	{
		pIOHandle->Close(pIOHandle->hIO);
	}

	return iRet;
}

int	CAdaptiveStreamHLS::SendBuff(QC_DATA_BUFF * pBuff)
{
	int nRC = QC_ERR_FAILED;
	AdjustFrameFlags(pBuff);
	if (m_pBuffMng != NULL && m_llDuration != 0)
	{
		if ((pBuff->uFlag & QCBUFF_HEADDATA) == 0 && m_pCurSegment != NULL && m_llDuration != 0)
		{
			int nOffsetTime = 0;
			if (pBuff->nMediaType == QC_MEDIA_Video)
			{
				if (m_llChunkTimeVideo == -1)
					m_llChunkTimeVideo = m_llChunkTimeStart;
				if (m_llVideoTimeNew == -1)
					m_llVideoTimeNew = pBuff->llTime;
				if (m_llVideoTimeEnd == -1)
					m_llVideoTimeEnd = pBuff->llTime;
				nOffsetTime = abs((int)(m_llVideoTimeEnd - pBuff->llTime));
				if (nOffsetTime > 2000)
				{
					m_llChunkTimeVideo = m_llChunkTimeStart;
					m_llVideoTimeNew = pBuff->llTime;
				}
				m_llVideoTimeEnd = pBuff->llTime;

				pBuff->llTime = m_llChunkTimeVideo + (pBuff->llTime - m_llVideoTimeNew);
				//QCLOGI("SendBuffTimeVideo  Seg = % 8lld   Buff = % 8lld", m_llChunkTimeAudio, pBuff->llTime);
			}
			else
			{
				if (m_llChunkTimeAudio == -1)
					m_llChunkTimeAudio = m_llChunkTimeStart;
				if (m_llAudioTimeNew == -1)
					m_llAudioTimeNew = pBuff->llTime;
				if (m_llAudioTimeEnd == -1)
					m_llAudioTimeEnd = pBuff->llTime;
				nOffsetTime = abs((int)(m_llAudioTimeEnd - pBuff->llTime));
				if (nOffsetTime > 2000)
				{
					m_llChunkTimeAudio = m_llChunkTimeStart;
					m_llAudioTimeNew = pBuff->llTime;
				}
				m_llAudioTimeEnd = pBuff->llTime;

				pBuff->llTime = m_llChunkTimeAudio + (pBuff->llTime - m_llAudioTimeNew);
				//QCLOGI("SendBuffTimeAudio  Seg = % 8lld   Buff = % 8lld", m_llChunkTimeAudio, pBuff->llTime);
			}
		}
		nRC = m_pBuffMng->Send(pBuff);
	}
	else
	{
		nRC = m_pBuffMng->Send(pBuff);
	}

	// for notify download percent
	if (!m_bLive && m_pCurChunk != NULL && m_pCurSegment != NULL && m_llDuration > 0)
	{
		int nDLPercent = m_pCurChunk->ullStartTime * 100 / m_llDuration;
		int nChunkPerc = m_pCurChunk->ullDuration * 100 / m_llDuration;
		long long llSize = m_pCurSegment->pIOHandle->GetSize(m_pCurSegment->pIOHandle->hIO);
		long long llDown = m_pCurSegment->pIOHandle->GetDownPos(m_pCurSegment->pIOHandle->hIO);
        
        // Once IO was closed, llSize will be reset to QCIO_MAX_CONTENT_LEN
        if (llDown == m_pCurSegment->illSegmentSize && m_pCurSegment->illSegmentSize > 0 && llDown > 0)
            llSize = llDown;
		if (llSize > 0)
			nDLPercent = nDLPercent + (nChunkPerc * llDown / llSize);
		if (nDLPercent >= 99 && llDown == llSize)
			nDLPercent = 100;
		if (nDLPercent != m_nDownloadPercent)
		{
			if (m_nDownloadPercent > nDLPercent)
			{
				long long llPlayTime = m_pBuffMng->GetPlayTime(QC_MEDIA_Audio);
				if (llPlayTime <= 0)
					llPlayTime = m_pBuffMng->GetPlayTime(QC_MEDIA_Video);
				if (llPlayTime < pBuff->llTime)
				{
					m_nDownloadPercent = nDLPercent;
					if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
						m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_PERCENT, m_nDownloadPercent, pBuff->llTime);
				}
			}
			else
			{
				m_nDownloadPercent = nDLPercent;
				if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_PERCENT, m_nDownloadPercent, pBuff->llTime);
			}
		}
	}

	return nRC;
}

void CAdaptiveStreamHLS::GetAbsoluteURL(char*  pURL, char* pSrc, char* pRefer)
{
	//and more case later
	char*  pFind = NULL;
	char*  pLastSep = NULL;
	char*  pFirstSep = NULL;
	int    iIndex = 0;


	//pSrc is http:// or https://
	pFind = strstr(pSrc, "://");
	if (pFind != NULL)
	{
		strcpy(pURL, pSrc);
	}
	else
	{
		pFind = strstr(pRefer, "://");
		if (pFind != NULL)
		{
			pFirstSep = strchr(pFind+strlen("://"), '/');
			pLastSep = strrchr(pRefer, '/');
			if (*pSrc == '/')
			{
				memcpy(pURL, pRefer, pFirstSep-pRefer);
				strcat(pURL, pSrc);
			}
			else
			{
				if (pLastSep != NULL)
				{
					memcpy(pURL, pRefer, pLastSep - pRefer + 1);
					strcat(pURL, pSrc);
					PurgeURL(pURL);
				}
			}
		}
		else
		{
			//local path
			//Linux
			pLastSep = strrchr(pRefer, '/');
			if (pLastSep != NULL)
			{
				memcpy(pURL, pRefer, pLastSep - pRefer + 1);
				strcat(pURL, pSrc);
				return;
			}

			//Windows
			pLastSep = strrchr(pRefer, '\\');
			if (pLastSep != NULL)
			{
				memcpy(pURL, pRefer, pLastSep - pRefer + 1);
				strcat(pURL, pSrc);
				return;
			}
		}
	}

	return;
}

void CAdaptiveStreamHLS::PurgeURL(char*  pURL)
{
	int  iLen = 0;
	int  iIndex = 0;
	char  strForSplit[4096] = { 0 };
	char*  pCur = NULL;
	char*  pFind = NULL;
	char*  pPathList[1024] = {0 };
	int    iPathListCount = 0;
	if (strstr(pURL, "http") != NULL && strstr(pURL, "/../"))
	{
		pFind = strstr(pURL, "://");
		strcpy(strForSplit, pFind + 3);
		pCur = strtok(strForSplit, "/");
		pPathList[iPathListCount] = pCur;
		iPathListCount++;

		while ((pCur = strtok(NULL, "/")))
		{
			if (strcmp(pCur, "..") == 0)
			{
				iPathListCount--;
			}
			else
			{
				pPathList[iPathListCount] = pCur;
				iPathListCount++;
			}
		}

		memset(pFind + 2, 0, strlen(pFind) - 2);
		for (iIndex = 0; iIndex < iPathListCount; iIndex++)
		{
			strcat(pURL, "/");
			strcat(pURL, pPathList[iIndex]);
		}
	}
}



int CAdaptiveStreamHLS::GetIndexByChunkType(E_ADAPTIVESTREAMPARSER_CHUNKTYPE  eType, int&  iIndex)
{
	int iRet = QC_ERR_FAILED;
	switch (eType)
	{
		case E_ADAPTIVESTREAMPARSER_CHUNKTYPE_AUDIO:
		{
			iIndex = HLS_SEGEMNT_ALTER_AUDIO_INDEX;
			iRet = 0;
			break;
		}

		case E_ADAPTIVESTREAMPARSER_CHUNKTYPE_AUDIOVIDEO:
		{
			iIndex = HLS_SEGEMNT_MAIN_STREAM_INDEX;
			iRet = 0;
			break;
		}

		case E_ADAPTIVESTREAMPARSER_CHUNKTYPE_VIDEO:
		{
			iIndex = HLS_SEGEMNT_ALTER_VIDEO_INDEX;
			iRet = 0;
			break;
		}

		case E_ADAPTIVESTREAMPARSER_CHUNKTYPE_SUBTITLE:
		{
			iIndex = HLS_SEGEMNT_ALTER_SUB_INDEX;
			iRet = 0;
			break;
		}
		default:
		{
			break;
		}
	}

	return iRet;
}

CTSParser* CAdaptiveStreamHLS::GetTsParserByChunkType(E_ADAPTIVESTREAMPARSER_CHUNKTYPE  eType, bool bCreate)
{
	CTSParser*   pRet = NULL;
	int iIndex = -1;
	if (GetIndexByChunkType(eType, iIndex) == QC_ERR_NONE)
	{
		if (m_pTsParserWorking[iIndex] == NULL)
		{
			m_pTsParserWorking[iIndex] = new CTSParser (m_pBaseInst);
			if (m_pTsParserWorking[iIndex] != NULL)
			{
				m_pTsParserWorking[iIndex]->SetBuffMng(m_pBuffMng);
				m_pTsParserWorking[iIndex]->SetSendBuffFunc(this);
			}
		}

		pRet = m_pTsParserWorking[iIndex];
	}

	return pRet;
}

QC_IO_Func* CAdaptiveStreamHLS::GetIOHandleByChunkType(E_ADAPTIVESTREAMPARSER_CHUNKTYPE  eType, bool bCreate)
{
	QC_IO_Func*   pRet = NULL;
	int iIndex = -1;
	if (GetIndexByChunkType(eType, iIndex) == QC_ERR_NONE)
	{
		if (m_pIOHandleWorking[iIndex] == NULL)
		{
			m_pIOHandleWorking[iIndex] = new QC_IO_Func;
			if (m_pIOHandleWorking[iIndex] != NULL)
			{
				memset(m_pIOHandleWorking[iIndex], 0, sizeof(QC_IO_Func));
				m_pIOHandleWorking[iIndex]->pBaseInst = m_pBaseInst;
				qcCreateIO(m_pIOHandleWorking[iIndex], m_ioProtocal);
				m_pIOHandleWorking[iIndex]->SetParam(m_pIOHandleWorking[iIndex]->hIO, QCIO_PID_HTTP_HeadHost, m_szHostAddr);
			}
		}

		pRet = m_pIOHandleWorking[iIndex];
	}

	return pRet;
}

CNormalHLSDrm * CAdaptiveStreamHLS::GetDrmHandleByChunkType(E_ADAPTIVESTREAMPARSER_CHUNKTYPE  eType, bool bCreate)
{
	CNormalHLSDrm*   pRet = NULL;
	int iIndex = -1;
	if (GetIndexByChunkType(eType, iIndex) == QC_ERR_NONE)
	{
		if (m_pHLSNormalDrmWorking[iIndex] == NULL)
		{
			m_pHLSNormalDrmWorking[iIndex] = new CNormalHLSDrm;
		}

		pRet = m_pHLSNormalDrmWorking[iIndex];
	}

	return pRet;
}

S_Segment_Buffer_Ctx * CAdaptiveStreamHLS::GetSegmentBufferByChunkType(E_ADAPTIVESTREAMPARSER_CHUNKTYPE  eType, bool bCreate)
{
	S_Segment_Buffer_Ctx*   pRet = NULL;
	int iIndex = -1;
	if (GetIndexByChunkType(eType, iIndex) == QC_ERR_NONE)
	{
		pRet = &m_aSegmentBufferCtx[iIndex];
	}

	return pRet;
}

S_Segment_Handle_Ctx * CAdaptiveStreamHLS::GetWorkingSegmentHandleCtx()
{
	S_Segment_Handle_Ctx*   pRet = NULL;
	for (int i = 0; i < MAX_SEGEMNT_WORKING_COUNT; i++)
	{
		if (strlen(m_aSegmentHandleCtx[i].strURL) > 0  &&  (
			m_aSegmentHandleCtx[i].illSegmentSize == 0 ||
		    ( m_aSegmentHandleCtx[i].illSegmentSize > 0 && 
			  m_aSegmentHandleCtx[i].illSegmentSize > m_aSegmentHandleCtx[i].illHandleOffset)))
		{
			pRet = &m_aSegmentHandleCtx[i];
			break;
		}
	}

	return pRet;
}

S_Segment_Handle_Ctx * CAdaptiveStreamHLS::GetIdleSegmentHandleCtx(E_ADAPTIVESTREAMPARSER_CHUNKTYPE  eType)
{
	S_Segment_Handle_Ctx*   pRet = NULL;
	int   iIndex = 0;

	if (GetIndexByChunkType(eType, iIndex) == QC_ERR_NONE && strlen(m_aSegmentHandleCtx[iIndex].strURL) == 0)
	{
		pRet = &m_aSegmentHandleCtx[iIndex];
	}

	return pRet;
}

void CAdaptiveStreamHLS::SelectPreferBitrateInOpen()
{
	int iIndex = 0;
	int iStreamIdx = -1;
	if (m_llPreferBitrate == 0 || m_iBitrateCount <= 1)
	{
		m_sOutputCtx.iMediaStreamCount = 1;
		return;
	}
	else
	{
		m_sOutputCtx.iMediaStreamCount = m_iBitrateCount;
		for (iIndex = 0; iIndex < m_iBitrateCount; iIndex++)
		{
			if (m_pBitrateInfo[iIndex].illBitrateInManifest >= m_llPreferBitrate)
			{
				iStreamIdx = iIndex;
				break;
			}
		}

		if (iStreamIdx != -1)
		{
			m_pHLSParser->SelectStream_HLS(iStreamIdx, E_ADAPTIVESTREAMING_CHUNKPOS_PRESENT);
		}
	}
}

void CAdaptiveStreamHLS::CreateFmtInfoFromTs()
{
	CTSParser*   pTsParser = NULL;
	QC_DEL_P(m_pFmtAudio);
	QC_DEL_P(m_pFmtVideo);
	QC_DEL_P(m_pFmtSubtt);

	m_pFmtAudio = new QC_AUDIO_FORMAT;
	memset(m_pFmtAudio, 0, sizeof(QC_AUDIO_FORMAT));

	m_pFmtVideo = new QC_VIDEO_FORMAT;
	memset(m_pFmtVideo, 0, sizeof(QC_VIDEO_FORMAT));

	m_pFmtSubtt = new QC_SUBTT_FORMAT;
	memset(m_pFmtSubtt, 0, sizeof(QC_SUBTT_FORMAT));

	pTsParser = GetTsParserByChunkType(E_ADAPTIVESTREAMPARSER_CHUNKTYPE_AUDIOVIDEO, true);
	if (pTsParser != NULL)
	{
		pTsParser->GetForamtInfo(m_pFmtAudio, m_pFmtVideo, m_pFmtSubtt);
	}
}

void CAdaptiveStreamHLS::CreateDefaultFmtInfo()
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
	m_pFmtVideo->nCodecID = QC_CODEC_ID_NONE;
	m_pFmtVideo->nWidth = 0;
	m_pFmtVideo->nHeight = 0;

	//Fill Default SubTitle Format Info
	m_pFmtSubtt = new QC_SUBTT_FORMAT;
	memset(m_pFmtSubtt, 0, sizeof(QC_SUBTT_FORMAT));
}

void CAdaptiveStreamHLS::ResetAllParser(bool bSeek)
{
	int iIndex = 0;
	for (iIndex = 0; iIndex < MAX_SEGEMNT_WORKING_COUNT; iIndex++)
	{
		if (m_pTsParserWorking[iIndex] != NULL)
		{
			if (bSeek)
				m_pTsParserWorking[iIndex]->SetParam(QC_EVENTID_RESET_TS_PARSER_FOR_SEEK, NULL);
			else
				m_pTsParserWorking[iIndex]->SetParam(QC_EVENTID_RESET_TS_PARSER_FOR_NEW_STREAM, NULL);
		}
	}
}

void CAdaptiveStreamHLS::FillBAInfo(const char* pURL)
{
	S_ADAPTIVESTREAM_BITRATE*	pBAInfo = NULL;
	QC_STREAM_FORMAT**			ppStreamInfo = NULL;
	char    strAbsoluteURL[4096] = { 0 };
	int iCount = 0;
	int iRet = 0;
	if (m_bHLSOpened == false)
	{
		return;
	}

	iRet = m_pHLSParser->GetProgramInfo_HLS(ppStreamInfo, &iCount);
	if (iRet != 0)
	{
		return;
	}

	pBAInfo = new S_ADAPTIVESTREAM_BITRATE[iCount];
	if (pBAInfo == NULL)
	{
		return;
	}

	memset(pBAInfo, 0, sizeof(S_ADAPTIVESTREAM_BITRATE)*iCount);
	m_iBitrateCount = iCount;
	for (int i = 0; i < iCount; i++)
	{
		memset(strAbsoluteURL, 0, 1024);
		pBAInfo[i].ulStreamID = ppStreamInfo[i]->nID;
		pBAInfo[i].illBitrateInManifest = ppStreamInfo[i]->nBitrate;
		GetAbsoluteURL(strAbsoluteURL, (char*)ppStreamInfo[i]->pPrivateData, (char* )pURL);
		strcpy(pBAInfo[i].strAbsoluteURL, strAbsoluteURL);
		qcUrlParseUrl(pBAInfo[i].strAbsoluteURL, pBAInfo[i].strDomainURL, pBAInfo[i].strPathURL, pBAInfo[i].iPort, NULL);
	}

	m_pBitrateInfo = pBAInfo;
	m_pAdaptiveBA->SetBitrateInfo(m_pBitrateInfo, m_iBitrateCount);
	m_nStrmSourceCount = m_iBitrateCount;
}

void CAdaptiveStreamHLS::FillResInfo(QC_RESOURCE_INFO*  pResInfo, unsigned int ulStreamId)
{
	int  iIndex = 0;
	if (pResInfo == NULL)
	{
		return;
	}

	for (iIndex = 0; iIndex < m_iBitrateCount; iIndex++)
	{
		if (ulStreamId == m_pBitrateInfo[iIndex].ulStreamID)
		{
			pResInfo->nBitrate = (int)m_pBitrateInfo[iIndex].illBitrateInManifest;
			pResInfo->pszURL = m_pBitrateInfo[iIndex].strAbsoluteURL;
			pResInfo->pszDomain = m_pBitrateInfo[iIndex].strDomainURL;
			pResInfo->llDuration = m_llDuration;
			pResInfo->pszFormat = FORATM_DESC_STRING;

			break;
		}
	}
}

void CAdaptiveStreamHLS::DumpSegment(int iPlaylistID, int iSequenceId, unsigned char*  pData, int iDataSize)
{
#ifdef _DUMP_FRAME_
	char  strDumpDataPath[256] = { 0 };
	FILE*   pFile = NULL;
	sprintf(strDumpDataPath, "playlist_%d_seq_%d.ts", iPlaylistID, iSequenceId);

	pFile = fopen(strDumpDataPath, "wb");
	if (pFile == NULL)
	{
		return;
	}

	fwrite(pData, 1, iDataSize, pFile);
	fclose(pFile);
#endif
}
