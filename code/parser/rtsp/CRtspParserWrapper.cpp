/*******************************************************************************
File:		CRtspParserWrapper.cpp

Contains:	CRtspParserWrapper implement file.

Written by:	Qichao Shen

Change History (most recent first):
2018-05-03		Qichao			Create file

*******************************************************************************/


#include "CRtspParserWrapper.h"
#include "qcErr.h"
#include "stdio.h"
#include "CRtspParser.h"
#include "b64.h"
#include "hex_coding.h"
#include "UAVParser.h"

CRtspParserWrapper::CRtspParserWrapper(CBaseInst * pBaseInst)
	: CBaseObject(pBaseInst)
{
	SetObjectName("CRtspParserWrapper");
	m_fRTSPHandle = NULL;
	m_pFuncCallback = NULL;
	m_hLib = NULL;
	memset(&m_sRtspIns, 0, sizeof(CM_Rtsp_Ins));
	Init();
	InitFrameBufferCtx();
	InitDump();
}

CRtspParserWrapper::~CRtspParserWrapper(void)
{
	UnInit();
	UnInitDump();
	UnInitFrameBufferCtx();
}

int		CRtspParserWrapper::Open(const char * pURL)
{
	int iRet = 0;
	do 
	{
		if (m_sRtspIns.hRtspIns == NULL)
		{
			iRet = QC_ERR_EMPTYPOINTOR;
			break;
		}

		iRet = m_sRtspIns.RTSP_OpenStream(m_sRtspIns.hRtspIns, 0, (char *)pURL, 1, 1, this, 1);

	} while (false);
	return iRet;
}

int 	CRtspParserWrapper::Close(void)
{
	return QC_ERR_NONE;
}

int 	CRtspParserWrapper::GetParam(int nID, void * pParam)
{
	return QC_ERR_NONE;
}

int 	CRtspParserWrapper::SetParam(int nID, void * pParam)
{
	return QC_ERR_NONE;
}

int 	CRtspParserWrapper::GetCurState()
{
	if (m_sRtspIns.hRtspIns != NULL)
	{
		return RTSP_INS_INITED;
	}
	else
	{
		return RTSP_INS_UNINITED;
	}
}


int     CRtspParserWrapper::SetFrameHandle(void*   pSendFrameIns)
{
	m_pFrameHandle = pSendFrameIns;
	return 0;
}


int     CRtspParserWrapper::SendFrame(void*  pRtspFrame)
{
	CRtspParser*     pRtspIns = (CRtspParser*)m_pFrameHandle;

	if (pRtspIns != NULL && pRtspFrame != NULL)
	{
		DumpMedia(pRtspFrame);
		pRtspIns->SendRtspBuff((S_Rtsp_Media_Sample* )pRtspFrame);
	}

	return QC_ERR_NONE;
}


int   CRtspParserWrapper::ParseProc(int iChannelId, void *pUser, int iFrameType, void *pBuf, void* pFrameInfo)
{
	S_Media_Frame_Info*     pRtspFrameInfo = (S_Media_Frame_Info*)pFrameInfo;
	CRtspParserWrapper*     pRtspIns = (CRtspParserWrapper*)pUser;

	if (pRtspIns != NULL)
	{
		pRtspIns->TransactionFrames(iFrameType, pBuf, pFrameInfo);
	}

	return 0;
}

void   CRtspParserWrapper::Init()
{
	do 
	{
		if (m_hLib == NULL)
		{
			m_hLib = (qcLibHandle)qcLibLoad("qcRTSP", 0);
		}

		if (m_hLib == NULL)
		{
			QCLOGI("Load qcRTSP fail!");
			break;
		}

		// Create Instance
		QCCREATERTSPINS* pCreate = (QCCREATERTSPINS*)qcLibGetAddr(m_hLib, "qcCreateRtspIns", 0);

		if (pCreate == NULL)
		{
			QCLOGI("Get Function Pointer Error!");
			break;
		}

		pCreate(&m_sRtspIns, NULL);
		if (m_sRtspIns.hRtspIns == NULL)
		{
			QCLOGI("Create Ins Error!");
			break;
		}

		m_sRtspIns.RTSP_SetCallback(m_sRtspIns.hRtspIns, ParseProc);
		m_iMediaDataCome = 0;
	} while (0);

	return;
}


void   CRtspParserWrapper::UnInit()
{
	do 
	{
		if (m_sRtspIns.hRtspIns != NULL && m_hLib != NULL)
		{
			m_sRtspIns.RTSP_CloseStream(m_sRtspIns.hRtspIns);
			QCDESTROYRTSPINS* pDestroy = (QCDESTROYRTSPINS*)qcLibGetAddr(m_hLib, "qcDestroyRtspIns", 0);
			if (pDestroy == NULL)
			{
				QCLOGI("Get Function Pointer Error!");
				break;
			}

			pDestroy(&m_sRtspIns);
			m_sRtspIns.hRtspIns = NULL;

			if (m_hLib != NULL)
			{
				qcLibFree(m_hLib, 0);
				m_hLib = NULL;
			}
		}
	} while (false);
}

void   CRtspParserWrapper::DumpMedia(void* pFrame)
{
#ifdef _DUMP_FRAME_
	S_Rtsp_Media_Sample*     pRtspFrameInfo = (S_Rtsp_Media_Sample*)pFrame;
	char   strDump[256] = {0};
	switch(pRtspFrameInfo->eMediaType)
	{
	case QC_MEDIA_Video:
	{
		if (m_aFMediaData[RTSP_VIDEO_STREAM_INDEX] == NULL || m_aFMediaInfo[RTSP_VIDEO_STREAM_INDEX] == NULL)
		{
			m_aFMediaData[RTSP_VIDEO_STREAM_INDEX] = fopen("rtsp_video.h264", "wb");
			m_aFMediaInfo[RTSP_VIDEO_STREAM_INDEX] = fopen("rtsp_video.dat", "wb");
		}

		if (m_aFMediaData[RTSP_VIDEO_STREAM_INDEX] != NULL && m_aFMediaInfo[RTSP_VIDEO_STREAM_INDEX] != NULL)
		{
			fwrite(pRtspFrameInfo->pSampleBuffer, 1, pRtspFrameInfo->iSampleBufferSize, m_aFMediaData[RTSP_VIDEO_STREAM_INDEX]);
			sprintf(strDump, "data size:%d, time stamp:%lld\n", pRtspFrameInfo->iSampleBufferSize, pRtspFrameInfo->ullTimeStamp);
			fwrite(strDump, 1, strlen(strDump), m_aFMediaInfo[RTSP_VIDEO_STREAM_INDEX]);
			fflush(m_aFMediaData[RTSP_VIDEO_STREAM_INDEX]);
			fflush(m_aFMediaInfo[RTSP_VIDEO_STREAM_INDEX]);
		}

		break;
	}
	}


#endif
}

void   CRtspParserWrapper::InitDump()
{
#ifdef _DUMP_FRAME_
	for(int i=0; i<RTSP_MAX_STREAM_COUNT; i++)
	{
		m_aFMediaData[i] = NULL;
		m_aFMediaInfo[i] = NULL;
	}
#endif
}

void   CRtspParserWrapper::UnInitDump()
{
#ifdef _DUMP_FRAME_
	for (int i = 0; i < RTSP_MAX_STREAM_COUNT; i++)
	{
		if(m_aFMediaData[i] != NULL)
		{
			fclose(m_aFMediaData[i]);
			m_aFMediaData[i] = NULL;
		}

		if(m_aFMediaInfo[i] != NULL)
		{
			fclose(m_aFMediaInfo[i]);
			m_aFMediaInfo[i] = NULL;
		}
	}
#endif
}

int   CRtspParserWrapper::InitFrameBufferCtx()
{
	m_pVideoBuffer = new unsigned char[RTSP_MAX_VIDEO_FRAME_SIZE];
	m_iVideoBufferMaxSize = RTSP_MAX_VIDEO_FRAME_SIZE;

    m_iFirstIFrameArrived = false;
	m_illCurVideoFrameTime = 0;
	m_iIFrameFlag = 0;
	m_iCurVideoFrameSize = 0;
	m_iVideoCodecId = 0;
	m_iFirstISent = 0;

	m_pAudioBuffer = new unsigned char[RTSP_MAX_AUDIO_FRAME_SIZE];
	m_iAudioBufferMaxSize = RTSP_MAX_AUDIO_FRAME_SIZE;
	m_iCurAudioFrameSize = 0;
	m_illCurAudioFrameTime = 0;

	memset(m_aVideoConfigData, 0, 256);
	m_iVideoConfigSize = 0;

	memset(m_aAudioConfigData, 0, 256);
	m_iAudioConfigSize = 0;
	m_iAudioCodecId = 0;


	m_iVideoTrackCount = 0;
	m_iAudioTrackCount = 0;
	return 0;
}

int   CRtspParserWrapper::UnInitFrameBufferCtx()
{
	if (m_pVideoBuffer != NULL)
	{
		delete[]  m_pVideoBuffer;
		m_pVideoBuffer = NULL;
	}

	if (m_pAudioBuffer != NULL)
	{
		delete[] m_pAudioBuffer;
		m_pAudioBuffer = NULL;
	}

	m_iVideoBufferMaxSize = 0;
	m_iAudioBufferMaxSize = 0;
	m_iFirstIFrameArrived = false;
	m_illCurVideoFrameTime = 0;
	m_iIFrameFlag = 0;
	m_iCurVideoFrameSize = 0;
	m_iCurAudioFrameSize = 0;
	m_iVideoCodecId = 0;
	return 0;
}

int  CRtspParserWrapper::TransactionFrames(int iFrameType, void *pBuf, void* pFrameInfo)
{
	int iRet = 0;
	S_Media_Frame_Info*     pRtspFrameInfo = (S_Media_Frame_Info*)pFrameInfo;
	S_Media_Track_Info*     pRtspTrackInfo = (S_Media_Track_Info*)pBuf;

	S_Rtsp_Media_Sample     sMediaSample;
	unsigned char    uNalType = 0;
	unsigned char    aSync[4] = { 0, 0, 0, 1 };
	CRtspParser*     pRtspIns = (CRtspParser*)m_pFrameHandle;

	memset(&sMediaSample, 0, sizeof(S_Rtsp_Media_Sample));

	switch (iFrameType)
	{
		case FRAME_TYPE_INFO:
		{
			if (pRtspIns != NULL)
			{
				if (pRtspFrameInfo->iInfoData == RTSP_STATE_TRACK_INFO_READY && pRtspTrackInfo != NULL)
				{
					switch (pRtspTrackInfo->iMediaType)
					{
						case MEDIA_TYPE_VIDEO:
						{
							iRet = GetVideoTrackInfoFromRtsp(pRtspTrackInfo);
							if (iRet == 0)
							{
								m_iVideoTrackCount++;
								QCLOGI("Add video track!");
							}
							break;
						}

						case MEDIA_TYPE_AUDIO:
						{
							iRet = GetAudioTrackInfoFromRtsp(pRtspTrackInfo);
							if (iRet == 0)
							{
								m_iAudioTrackCount++;
								QCLOGI("Add audio track!");
							}
							break;
						}
					}
				}
			}
			break;
		}

		case FRAME_TYPE_MEDIA_DATA:
		{
			if (m_iMediaDataCome == 0)
			{
				m_iMediaDataCome = 1;
				pRtspIns->SetRtspState(m_iMediaDataCome);
			}

			MediaTransaction((char*)pBuf, pFrameInfo);

			break;
		}
	}

	return 0;
}

int  CRtspParserWrapper::MediaTransaction(char *pBuf, void* pFrameInfo)
{
	S_Media_Frame_Info*     pRtspFrameInfo = (S_Media_Frame_Info*)pFrameInfo;
	S_Rtsp_Media_Sample     sMediaSample;
	unsigned char    uNalType = 0;
	unsigned char    aSync[4] = { 0, 0, 0, 1 };
	
	memset(&sMediaSample, 0, sizeof(S_Rtsp_Media_Sample));

	if (pRtspFrameInfo->iMediaType == MEDIA_TYPE_VIDEO)
	{
		if (m_iCurVideoFrameSize > 0 && m_illCurVideoFrameTime != pRtspFrameInfo->illTimeStamp)
		{
			if (m_iFirstISent == 0)
			{
				if (m_iIFrameFlag == 1)
				{
					sMediaSample.iSampleBufferSize = m_iCurVideoFrameSize;
					sMediaSample.pSampleBuffer = m_pVideoBuffer;
					sMediaSample.eMediaType = QC_MEDIA_Video;
					sMediaSample.ullTimeStamp = m_illCurVideoFrameTime;
					sMediaSample.ulMediaCodecId = m_iVideoCodecId;
					sMediaSample.ulSampleFlag = (m_iIFrameFlag == 1) ? 1 : 0;
					SendFrame(&sMediaSample);
					m_iFirstISent = 1;
				}
			}
			else
			{
				sMediaSample.iSampleBufferSize = m_iCurVideoFrameSize;
				sMediaSample.pSampleBuffer = m_pVideoBuffer;
				sMediaSample.eMediaType = QC_MEDIA_Video;
				sMediaSample.ullTimeStamp = m_illCurVideoFrameTime;
				sMediaSample.ulMediaCodecId = m_iVideoCodecId;
				sMediaSample.ulSampleFlag = (m_iIFrameFlag == 1) ? 1 : 0;
				SendFrame(&sMediaSample);
			}

			m_iCurVideoFrameSize = 0;
			m_illCurVideoFrameTime = 0;
			m_iIFrameFlag = 0;
		}
	}

	if (pRtspFrameInfo->iMediaType == MEDIA_TYPE_AUDIO)
	{
		if (m_iCurAudioFrameSize > 0 )
		{
			sMediaSample.iSampleBufferSize = m_iCurAudioFrameSize;
			sMediaSample.pSampleBuffer = m_pAudioBuffer;
			sMediaSample.eMediaType = QC_MEDIA_Audio;
			sMediaSample.ullTimeStamp = m_illCurAudioFrameTime;
			sMediaSample.ulMediaCodecId = m_iAudioCodecId;
			SendFrame(&sMediaSample);
			m_iCurAudioFrameSize = 0;
			m_illCurAudioFrameTime = 0;
		}
	}



	switch (pRtspFrameInfo->iMediaType)
	{
		case MEDIA_TYPE_VIDEO:
		{
			if (pRtspFrameInfo->iMediaCodec == MEDIA_VIDEO_CODEC_H264)
			{
				m_iVideoCodecId = QC_CODEC_ID_H264;
				uNalType = 0x1f & pBuf[0];

				switch (uNalType)
				{
					case 5:
					{
						m_iIFrameFlag = 1;
						break;
					}

					default:
					{
						break;
					}
				}

				if ((m_iCurVideoFrameSize + 4 + pRtspFrameInfo->iFrameDataSize)< m_iVideoBufferMaxSize)
				{
					m_illCurVideoFrameTime = pRtspFrameInfo->illTimeStamp;
					memcpy(m_pVideoBuffer + m_iCurVideoFrameSize, aSync, 4);
					m_iCurVideoFrameSize += 4;
					memcpy(m_pVideoBuffer + m_iCurVideoFrameSize, pBuf, pRtspFrameInfo->iFrameDataSize);
					m_iCurVideoFrameSize += pRtspFrameInfo->iFrameDataSize;
				}
				else
				{
					QCLOGI("something error! timestamp invalid or frame size too large!");
				}
			}

			break;
		}

		case MEDIA_TYPE_AUDIO:
		{
			if (pRtspFrameInfo->iMediaCodec == MEDIA_AUDIO_CODEC_AAC)
			{
				//For construct the adts header
				m_illCurAudioFrameTime = pRtspFrameInfo->illTimeStamp;
				memcpy(m_pAudioBuffer + m_iCurAudioFrameSize, pBuf, pRtspFrameInfo->iFrameDataSize);
				m_iCurAudioFrameSize += pRtspFrameInfo->iFrameDataSize;
			}
			break;
		}

		default:
		{
			break;
		}
	}

	return 0;
}

int  CRtspParserWrapper::GetVideoTrackInfoFromRtsp(S_Media_Track_Info*   pTrackInfo)
{
	int iRet = 0;
	char*  pFind = NULL;
	char*  pStart = NULL;
	char   aDataForBase64Decode[256];
	int    iBase64DecodeLen = 0;
	unsigned char    aSync[] = { 0, 0, 0, 1 };

	do 
	{
		switch (pTrackInfo->iCodecId)
		{
			case MEDIA_VIDEO_CODEC_H264:
			{
				m_iVideoCodecId = QC_CODEC_ID_H264;
				pFind = strstr(pTrackInfo->sVideoInfo.aCodecConfig, ",");
				if (pFind != NULL)
				{
					iBase64DecodeLen = 0;
					iBase64DecodeLen = b64_decode(pTrackInfo->sVideoInfo.aCodecConfig, pFind - pTrackInfo->sVideoInfo.aCodecConfig, aDataForBase64Decode, 256);
					memcpy(m_aVideoConfigData, aSync, 4);
					m_iVideoConfigSize += 4;
					memcpy(m_aVideoConfigData+m_iVideoConfigSize, aDataForBase64Decode, iBase64DecodeLen);
					m_iVideoConfigSize += iBase64DecodeLen;

					memcpy(m_aVideoConfigData + m_iVideoConfigSize, aSync, 4);
					m_iVideoConfigSize += 4;
					iBase64DecodeLen = b64_decode(pFind + 1, pTrackInfo->sVideoInfo.iCodecConfigSize - (pFind - pTrackInfo->sVideoInfo.aCodecConfig+1), aDataForBase64Decode, 256);
					memcpy(m_aVideoConfigData + m_iVideoConfigSize, aDataForBase64Decode, iBase64DecodeLen);
					m_iVideoConfigSize += iBase64DecodeLen;
				}

				break;
			}
		}
	} while (0);

	return iRet;
}

int  CRtspParserWrapper::GetAudioTrackInfoFromRtsp(S_Media_Track_Info*   pTrackInfo)
{
	int iRet = 0;
	char   aDataForHexDecode[256];
	int    iHexDecodeLen = 0;

	do
	{
		switch (pTrackInfo->iCodecId)
		{
			case MEDIA_AUDIO_CODEC_AAC:
			{
				iRet = HexDecode(pTrackInfo->sAudioInfo.aCodecConfig, pTrackInfo->sAudioInfo.iCodecConfigSize, aDataForHexDecode, &iHexDecodeLen, 256);
				if (iRet != 0)
				{
					break;
				}

				memcpy(m_aAudioConfigData, aDataForHexDecode, iHexDecodeLen);
				m_iAudioConfigSize = iHexDecodeLen;
				m_iAudioCodecId = QC_CODEC_ID_AAC;
				break;
			}
		}

		if (iRet != 0)
		{
			break;
		}
	} while (0);

	return iRet;
}


int  CRtspParserWrapper::GetMediaTrackinfo(int iMediaType, int*  piCodecId, char**  ppCodecHeader, int* piCodecHeaderSize)
{
	int iRet = -1;

	do 
	{
		switch (iMediaType)
		{
			case QC_MEDIA_Video:
			{
				if (m_iVideoTrackCount > 0)
				{
					*piCodecId = m_iVideoCodecId;
					*ppCodecHeader = (char*)(m_aVideoConfigData);
					*piCodecHeaderSize = m_iVideoConfigSize;
					iRet = 0;
				}

				break;
			}

			case QC_MEDIA_Audio:
			{
				if (m_iAudioTrackCount > 0)
				{
					*piCodecId = m_iAudioCodecId;
					*ppCodecHeader = (char*)(m_aAudioConfigData);
					*piCodecHeaderSize = m_iAudioConfigSize;
					iRet = 0;
				}
				break;
			}
		}
	} while (0);

	return iRet;
}
