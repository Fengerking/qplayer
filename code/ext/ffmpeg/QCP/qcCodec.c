/*******************************************************************************
	File:		qcCodec.cpp

	Contains:	Create the Codec with codec id

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-24		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcCodec.h"
#include "qcPlayer.h"

#ifdef __QC_OS_NDK__
#include <unistd.h>
#include <android/log.h>
#endif // __QC_OS_NDK__

#if defined(__QC_OS_IOS__) || defined(__QC_OS_MACOS__)
#include <sys/sysctl.h>
#endif

#ifndef INT64_C
#define UINT64_C
#endif

#include "libavcodec/avcodec.h"

#include "speex/speex.h"
#include "speex/speex_stereo.h"
#include "speex/speex_header.h"
#include "speex/speex_callbacks.h"

#include "qcFFLog.h"

#define SPEEX_FRAME_SIZE	160
int Speex_pkt_size[] = { 5, 10, 15, 20, 20, 28, 28, 38, 38, 46, 62 };

typedef struct
{
	void *				m_pState;
	SpeexBits			m_bits;
	SpeexStereoState	m_stereo;
	int					m_nFrameSize;
	int					m_nPacketSize;
}QCCodec_Speex;

typedef struct
{
	AVCodecContext *		pDecCtx;
	AVCodecContext *		pDecCtxNew;
	AVCodec *				pAVDec;
	AVFrame *				pAVFrm;
	AVPacket				pktData;
	AVPacket *				pPacket;
	QC_DATA_BUFF *			pBuffData;
	QC_VIDEO_BUFF *			pBufVideo;
	long long				llDelay;
	int						nAVType;
	int						nCodecID;
	QC_AUDIO_FORMAT *		pFmtAudio;
	QC_AUDIO_FRAME *		pFrmAudio;
	QCCodec_Speex *			pSpeexDec;
	QCSourceType			nSourceType;
}QCCodec_Context;

int		qcCodec_SetBuff_V1 (void * hCodec, QC_DATA_BUFF * pBuff);
int		qcCodec_GetBuff_V1 (void * hCodec, QC_DATA_BUFF ** ppBuff);

int		qcCodec_SetBuff_V2 (void * hCodec, QC_DATA_BUFF * pBuff);
int		qcCodec_GetBuff_V2 (void * hCodec, QC_DATA_BUFF ** ppBuff);

int		qcCodec_InitSpeex (QCCodec_Context * pContext, QC_AUDIO_FORMAT * pFmt);
int		qcCodec_UninitSpeex (QCCodec_Context * pContext);

int		qcCodec_Release (QCCodec_Context * pContext);

int qcCodec_SetBuff (void * hCodec, QC_DATA_BUFF * pBuff)
{
	QCCodec_Context *	pContext = (QCCodec_Context *)hCodec;
	if (pContext == NULL)
		return QC_ERR_ARG;

	if (pContext->nSourceType == QC_SOURCE_FF && (pBuff->uFlag & QCBUFF_HEADDATA) == QCBUFF_HEADDATA)
		return QC_ERR_NEEDMORE;

	if (pContext->nCodecID == QC_CODEC_ID_SPEEX)
	{
		if (pContext->pSpeexDec == NULL)
			return QC_ERR_ARG;
		if ((pBuff->uFlag & QCBUFF_HEADDATA) == QCBUFF_HEADDATA)
			return QC_ERR_NEEDMORE;
		speex_bits_read_from(&pContext->pSpeexDec->m_bits, (char *)pBuff->pBuff, pBuff->uSize);
		return QC_ERR_NONE;
	}

	return qcCodec_SetBuff_V1(hCodec, pBuff);
}

int qcCodec_GetBuff (void * hCodec, QC_DATA_BUFF ** ppBuff)
{
	QCCodec_Context *	pContext = (QCCodec_Context *)hCodec;
	if (pContext == NULL)
		return QC_ERR_ARG;

	if (pContext->nCodecID == QC_CODEC_ID_SPEEX)
	{
		if (pContext->pSpeexDec == NULL)
			return QC_ERR_ARG;

		QCCodec_Speex * pSpeexDec = pContext->pSpeexDec;
		if (speex_bits_remaining(&pSpeexDec->m_bits) < 5 || speex_bits_peek_unsigned(&pSpeexDec->m_bits, 5) == 0xF)
			return QC_ERR_NEEDMORE;

		if (pContext->pBuffData != NULL)
			pContext->pBuffData->uFlag = 0;

		short * pOutBuff = (short *)pContext->pBuffData->pBuff;
		int nRC = speex_decode_int(pSpeexDec->m_pState, &pSpeexDec->m_bits, pOutBuff);
		if (nRC <= -2)
			return QC_ERR_FAILED;
		else if (nRC == -1)
			return QC_ERR_NEEDMORE;

		if (pContext->pFmtAudio->nChannels == 2)
			speex_decode_stereo_int(pOutBuff, pSpeexDec->m_nFrameSize, &pSpeexDec->m_stereo);
		pContext->pBuffData->uSize = pSpeexDec->m_nFrameSize * 2;

		*ppBuff = pContext->pBuffData;

		return QC_ERR_NONE;
	}

	return qcCodec_GetBuff_V1 (hCodec, ppBuff);
}

int qcCodec_Flush(void * hCodec)
{
	if (hCodec == NULL)
		return QC_ERR_ARG;

	QCCodec_Context *	pContext = (QCCodec_Context *)hCodec;
	if (pContext->nCodecID == QC_CODEC_ID_SPEEX)
	{
		if (pContext->pSpeexDec != NULL)
			speex_bits_reset(&pContext->pSpeexDec->m_bits);
		return QC_ERR_NONE;
	}

	if (pContext->pDecCtx == NULL)
		return QC_ERR_STATUS;

	avcodec_flush_buffers(pContext->pDecCtx);

	return QC_ERR_NONE;
}

int qcCodec_Run (void * hCodec)
{
	return QC_ERR_NONE;
}

int qcCodec_Pause (void * hCodec)
{
	return QC_ERR_NONE;
}

int qcCodec_Stop (void * hCodec)
{
	return QC_ERR_NONE;
}

int qcCodec_GetParam (void * hCodec, int nID, void * pParam)
{
	return QC_ERR_NONE;
}

int qcCodec_SetParam (void * hCodec, int nID, void * pParam)
{
	switch (nID)
	{
	case QCPLAY_PID_Log_Level:
		g_nQcCodecLogLevel = *(int *)pParam;
		return QC_ERR_NONE;

	default:
		break;
	}

	return QC_ERR_NONE;
}

int qcCreateDecoder (QC_Codec_Func * pCodec, void * pFormat)
{
	int					nRC = 0;
	enum AVCodecID		nID = AV_CODEC_ID_H264;
	QCCodec_Context *	pContext = NULL;

	if (pCodec == NULL)
		return QC_ERR_ARG;

	pContext = (QCCodec_Context *) malloc (sizeof (QCCodec_Context));
	memset(pContext, 0, sizeof(QCCodec_Context));

	pCodec->nVer = 1;
	pCodec->hCodec = NULL;
	pCodec->SetBuff = qcCodec_SetBuff;
	pCodec->GetBuff = qcCodec_GetBuff;
	pCodec->Flush = qcCodec_Flush;
	pCodec->Run = qcCodec_Run;
	pCodec->Pause = qcCodec_Pause;
	pCodec->Stop = qcCodec_Stop;
	pCodec->GetParam = qcCodec_GetParam;
	pCodec->SetParam = qcCodec_SetParam;

	pContext->nSourceType = QC_SOURCE_QC;
	pContext->pBuffData = (QC_DATA_BUFF *)malloc(sizeof(QC_DATA_BUFF));
	memset(pContext->pBuffData, 0, sizeof(QC_DATA_BUFF));
	if (pCodec->nAVType == 1)
	{
		pContext->pBufVideo = (QC_VIDEO_BUFF *)malloc(sizeof(QC_VIDEO_BUFF));
		memset(pContext->pBufVideo, 0, sizeof(QC_VIDEO_BUFF));
		pContext->pBuffData->uBuffType = QC_BUFF_TYPE_Video;
		pContext->pBuffData->pBuffPtr = pContext->pBufVideo;
	}
	else
	{
		pContext->pBuffData->uBuffType = QC_BUFF_TYPE_Data;
		pContext->pFmtAudio = (QC_AUDIO_FORMAT *)malloc(sizeof(QC_AUDIO_FORMAT));
		memcpy(pContext->pFmtAudio, pFormat, sizeof(QC_AUDIO_FORMAT));
		pContext->pFrmAudio = (QC_AUDIO_FRAME *)malloc(sizeof(QC_AUDIO_FRAME));
	}
	if (pCodec->nAVType == 1)
	{
		QC_VIDEO_FORMAT * pFmtVideo = (QC_VIDEO_FORMAT *)pFormat;
		if (pFmtVideo->nSourceType == QC_SOURCE_FF)
			pContext->pDecCtx = (AVCodecContext *)pFmtVideo->pPrivateData;
		else if (pFmtVideo->nPrivateFlag == QC_SOURCE_FF && pFmtVideo->pPrivateData != NULL)
		{
			pContext->pDecCtx = (AVCodecContext *)pFmtVideo->pPrivateData;
			pContext->nSourceType = QC_SOURCE_FF;
		}
	}
	else
	{
		QC_AUDIO_FORMAT * pFmtAudio = (QC_AUDIO_FORMAT *)pFormat;
		if (pFmtAudio->nSourceType == QC_SOURCE_FF)
			pContext->pDecCtx = (AVCodecContext *)pFmtAudio->pPrivateData;
		else if (pFmtAudio->nPrivateFlag == QC_SOURCE_FF && pFmtAudio->pPrivateData != NULL)
		{
			pContext->pDecCtx = (AVCodecContext *)pFmtAudio->pPrivateData;
			pContext->nSourceType = QC_SOURCE_FF;
		}
	}

	qclog_init();
	pContext->nAVType = pCodec->nAVType;
	if (pContext->nAVType == 1)
		pContext->nCodecID = ((QC_VIDEO_FORMAT *)pFormat)->nCodecID;
	else
		pContext->nCodecID = ((QC_AUDIO_FORMAT *)pFormat)->nCodecID;

	if (pContext->nCodecID == QC_CODEC_ID_H265)
		nID = AV_CODEC_ID_HEVC;
	else if (pContext->nCodecID == QC_CODEC_ID_H264)
		nID = AV_CODEC_ID_H264;
	else if (pContext->nCodecID == QC_CODEC_ID_MPEG4)
		nID = AV_CODEC_ID_MPEG4;
	else if (pContext->nCodecID == QC_CODEC_ID_AAC)
		nID = AV_CODEC_ID_AAC;
	else if (pContext->nCodecID == QC_CODEC_ID_MP3)
		nID = AV_CODEC_ID_MP3;
	else if (pContext->nCodecID == QC_CODEC_ID_MP2)
		nID = AV_CODEC_ID_MP2;
	else if (pContext->nCodecID == QC_CODEC_ID_G722)
		nID = AV_CODEC_ID_ADPCM_G722;
	else if (pContext->nCodecID == QC_CODEC_ID_G723)
		nID = AV_CODEC_ID_G723_1;
	else if (pContext->nCodecID == QC_CODEC_ID_G726)
		nID = AV_CODEC_ID_ADPCM_G726;
	else if (pContext->nCodecID == QC_CODEC_ID_SPEEX)
	{
		pContext->pSpeexDec = (QCCodec_Speex*)malloc(sizeof(QCCodec_Speex));
		memset(pContext->pSpeexDec, 0, sizeof(QCCodec_Speex));
		nRC = qcCodec_InitSpeex(pContext, (QC_AUDIO_FORMAT *)pFormat);
		if (nRC != QC_ERR_NONE)
		{
			qcCodec_Release(pContext);
			return nRC;
		}
		pCodec->hCodec = pContext;
		return QC_ERR_NONE;
	}
	else
	{
		if (pContext->pDecCtx == NULL)
			return QC_ERR_UNSUPPORT;
		nID = pContext->pDecCtx->codec_id;
	}

	avcodec_register_all();
	pContext->pAVDec = avcodec_find_decoder(nID);
	if (pContext->pAVDec == NULL)
	{
		qcCodec_Release(pContext);
		return QC_ERR_FAILED;
	}
	if (pContext->pDecCtx == NULL)
	{
		pContext->pDecCtxNew = avcodec_alloc_context3(pContext->pAVDec);
		pContext->pDecCtx = pContext->pDecCtxNew;
		if (pCodec->nAVType == 1 && nID == AV_CODEC_ID_MPEG4)
		{
			QC_VIDEO_FORMAT * pFmtVideo = (QC_VIDEO_FORMAT *)pFormat;
			if (pFmtVideo->pHeadData != NULL && pFmtVideo->nHeadSize > 0)
			{
				pContext->pDecCtx->extradata = av_malloc(pFmtVideo->nHeadSize);
				memcpy(pContext->pDecCtx->extradata, pFmtVideo->pHeadData, pFmtVideo->nHeadSize);
				pContext->pDecCtx->extradata_size = pFmtVideo->nHeadSize;
			}
		}
	}
	if (pContext->pDecCtx == NULL)
	{
		qcCodec_Release(pContext);
		return QC_ERR_FAILED;
	}
	if (pCodec->nAVType == 1)
	{
		pContext->pDecCtx->thread_count = 1;
		pContext->pDecCtx->thread_type = FF_THREAD_FRAME;
#ifdef __QC_OS_WIN32__
		SYSTEM_INFO si;
		GetSystemInfo(&si);
//		pContext->pDecCtx->thread_count = si.dwNumberOfProcessors;
//		pContext->pDecCtx->thread_type = FF_THREAD_FRAME;
#elif defined __QC_OS_NDK__
		int nTemps[] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21};
		char cCpuName[512];
		memset(cCpuName, 0, sizeof(cCpuName));
		int nCount = (sizeof(nTemps)/sizeof(nTemps[0])) - 1;
		int i = 0;
		for(i = nCount; i >= 0; i--)
		{
			sprintf(cCpuName, "/sys/devices/system/cpu/cpu%d", nTemps[i]);
			int nOk = access(cCpuName, F_OK);
			if( nOk == 0)
			{
				pContext->pDecCtx->thread_count = nTemps[i]+1;
				break;
			}
		}
#elif defined __QC_OS_IOS__
		int nValue = 1;
		size_t nSize = sizeof(nValue);
		const char* name = "hw.physicalcpu";
		sysctlbyname(name, (void*)&nValue, &nSize, NULL, 0);
		pContext->pDecCtx->thread_count = nValue;
#endif // __QC_OS_WIN32__
	}
	nRC = avcodec_open2(pContext->pDecCtx, pContext->pAVDec, NULL);
	if (nRC < 0)
	{
		qcCodec_Release(pContext);
		return QC_ERR_FAILED;
	}
	pContext->pAVFrm = av_frame_alloc();
	av_init_packet(&pContext->pktData);
	pContext->pktData.data = NULL;
	pContext->pktData.size = 0;
    
#ifdef __QC_OS_IOS__
    // just a workaround to resolve conflict with other FFMPEG lib from third-party
    // av_init_packet(&pContext->pktData) would cause pContext->pPacket = 0xFFFFFFFF, and then crash happens
    pContext->pPacket = NULL;
#endif
	pCodec->hCodec = pContext;

	return QC_ERR_NONE;
}

int	qcDestroyDecoder (QC_Codec_Func * pCodec)
{
	qclog_uninit();

	if (pCodec == NULL || pCodec->hCodec == NULL)
		return QC_ERR_ARG;

	qcCodec_Release ((QCCodec_Context *)pCodec->hCodec);

	pCodec->hCodec = NULL;

	return QC_ERR_NONE;
}

int qcCodec_SetBuff_V1(void * hCodec, QC_DATA_BUFF * pBuff)
{
	QCCodec_Context *	pContext = (QCCodec_Context *)hCodec;
	if (pBuff->uBuffType == QC_BUFF_TYPE_Packet)
	{
		memcpy(&pContext->pktData, pBuff->pBuffPtr, sizeof(AVPacket));
	}
	else
	{
		pContext->pktData.data = pBuff->pBuff;
		pContext->pktData.size = pBuff->uSize;
		pContext->pktData.pts = pBuff->llTime;
		if ((pBuff->uFlag & QCBUFF_KEY_FRAME) == QCBUFF_KEY_FRAME)
			pContext->pktData.flags = AV_PKT_FLAG_KEY;
		else
			pContext->pktData.flags = 0;
	}
	pContext->pPacket = &pContext->pktData;
	pContext->pPacket->dts = abs((int)pBuff->llDelay);
	pContext->llDelay = pBuff->llDelay;
	return QC_ERR_NONE;
}

int qcCodec_GetBuff_V1(void * hCodec, QC_DATA_BUFF ** ppBuff)
{
	int					nRC = QC_ERR_NONE;
	int					nGotFrame = 0;
	QCCodec_Context *	pContext = (QCCodec_Context *)hCodec;

	if (ppBuff == NULL)
		return QC_ERR_ARG;
	*ppBuff = NULL;
	if (pContext->pDecCtx == NULL)
		return QC_ERR_STATUS;
	if (pContext->pPacket == NULL)// || pContext->pPacket->size == 0)
		return QC_ERR_NEEDMORE;

	if (pContext->nAVType == 1)
	{
		pContext->pDecCtx->skip_frame = AVDISCARD_DEFAULT;
		pContext->pDecCtx->skip_loop_filter = AVDISCARD_DEFAULT;
		if (pContext->llDelay >= 50)
			pContext->pDecCtx->skip_loop_filter = AVDISCARD_ALL;
		if (pContext->llDelay >= 100)
			pContext->pDecCtx->skip_frame = AVDISCARD_NONREF;

		AVPacket * pPacket = pContext->pPacket;
		nRC = avcodec_decode_video2(pContext->pDecCtx, pContext->pAVFrm, &nGotFrame, pContext->pPacket);
		if (nRC >= 0 && pPacket->size > nRC + 16)
		{
			pPacket->data = pPacket->data + nRC;
			pPacket->size = pPacket->size - nRC;
			pPacket->dts += 35;
			pPacket->pts += 35;
		}
		else
		{
			pContext->pPacket = NULL;
		}

		if (nGotFrame > 0)
		{
			pContext->pBufVideo->pBuff[0] = (unsigned char *)pContext->pAVFrm->data[0];
			pContext->pBufVideo->pBuff[1] = (unsigned char *)pContext->pAVFrm->data[1];
			pContext->pBufVideo->pBuff[2] = (unsigned char *)pContext->pAVFrm->data[2];
			pContext->pBufVideo->nStride[0] = pContext->pAVFrm->linesize[0];
			pContext->pBufVideo->nStride[1] = pContext->pAVFrm->linesize[1];
			pContext->pBufVideo->nStride[2] = pContext->pAVFrm->linesize[2];
			pContext->pBufVideo->nWidth = pContext->pAVFrm->width;
			pContext->pBufVideo->nHeight = pContext->pAVFrm->height;
			pContext->pBufVideo->nRatioNum = pContext->pAVFrm->sample_aspect_ratio.num;
			pContext->pBufVideo->nRatioDen = pContext->pAVFrm->sample_aspect_ratio.den;
			if (pContext->pAVFrm->format == AV_PIX_FMT_YUV420P || pContext->pAVFrm->format == AV_PIX_FMT_YUVJ420P)
				pContext->pBufVideo->nType = QC_VDT_YUV420_P;
			else if (pContext->pAVFrm->format == AV_PIX_FMT_YUYV422)
				pContext->pBufVideo->nType = QC_VDT_YUYV422;
			else if (pContext->pAVFrm->format == AV_PIX_FMT_YUV422P || pContext->pAVFrm->format == AV_PIX_FMT_YUVJ422P)
				pContext->pBufVideo->nType = QC_VDT_YUV422_P;
			else if (pContext->pAVFrm->format == AV_PIX_FMT_YUV444P || pContext->pAVFrm->format == AV_PIX_FMT_YUVJ444P)
				pContext->pBufVideo->nType = QC_VDT_YUV444_P;
			else if (pContext->pAVFrm->format == AV_PIX_FMT_YUV410P)
				pContext->pBufVideo->nType = QC_VDT_YUV410_P;
			else if (pContext->pAVFrm->format == AV_PIX_FMT_YUV411P)
				pContext->pBufVideo->nType = QC_VDT_YUV411_P;

			if (pContext->pAVFrm->pkt_pts >= 0)
				pContext->pBuffData->llTime = pContext->pAVFrm->pkt_pts;
			else if (pContext->pAVFrm->pkt_dts >= 0)
				pContext->pBuffData->llTime = pContext->pAVFrm->pkt_dts;
			else if (pContext->pAVFrm->pts >= 0)
				pContext->pBuffData->llTime = pContext->pAVFrm->pts;
			*ppBuff = pContext->pBuffData;

			return QC_ERR_NONE;
		}
	}
	else
	{
		AVPacket * pPacket = pContext->pPacket;
		nRC = avcodec_decode_audio4(pContext->pDecCtx, pContext->pAVFrm, &nGotFrame, pPacket);
		if (nRC >= 0 && pPacket->size > nRC + 2)
		{
			pPacket->data = pPacket->data + nRC;
			pPacket->size = pPacket->size - nRC;
			if (pContext->pAVFrm->sample_rate != 0)
			{
				pPacket->dts += pContext->pAVFrm->nb_samples * 1000 / pContext->pAVFrm->sample_rate;
				pPacket->pts += pContext->pAVFrm->nb_samples * 1000 / pContext->pAVFrm->sample_rate;
			}
			else
			{
				pPacket->dts += 30;
				pPacket->pts += 30;
			}
		}
		else
		{
			pContext->pPacket = NULL;
		}

		if (nGotFrame > 0)
		{
			pContext->pFrmAudio->nSampleRate = pContext->pAVFrm->sample_rate;
			pContext->pFrmAudio->nChannels = pContext->pAVFrm->channels;
			pContext->pFrmAudio->nFormat = pContext->pAVFrm->format;
			pContext->pFrmAudio->nNBSamples = pContext->pAVFrm->nb_samples;
			int i = 0;
			for (i = 0; i < 8; i++)
			{
				pContext->pFrmAudio->pDataBuff[i] = (char *)pContext->pAVFrm->data[i];
				pContext->pFrmAudio->nDataSize[i] = pContext->pAVFrm->linesize[i];
			}
			pContext->pBuffData->pData = pContext->pFrmAudio;

			pContext->pBuffData->pBuff = pContext->pAVFrm->data[0];
			pContext->pBuffData->uSize = pContext->pAVFrm->linesize[0];

			if (pContext->pAVFrm->pkt_pts >= 0)
				pContext->pBuffData->llTime = pContext->pAVFrm->pkt_pts;
			else if (pContext->pAVFrm->pkt_dts >= 0)
				pContext->pBuffData->llTime = pContext->pAVFrm->pkt_dts;
			else if (pContext->pAVFrm->pts >= 0)
				pContext->pBuffData->llTime = pContext->pAVFrm->pts;

			if (pContext->pFmtAudio->nChannels != pContext->pAVFrm->channels || pContext->pFmtAudio->nSampleRate != pContext->pAVFrm->sample_rate)
			{
				pContext->pFmtAudio->nChannels = pContext->pAVFrm->channels;
				pContext->pFmtAudio->nSampleRate = pContext->pAVFrm->sample_rate;
				pContext->pBuffData->uFlag = QCBUFF_NEW_FORMAT;
				pContext->pBuffData->pFormat = pContext->pFmtAudio;
			}
			*ppBuff = pContext->pBuffData;

			return QC_ERR_NONE;
		}
	}

	return QC_ERR_RETRY;
}

int qcCodec_SetBuff_V2(void * hCodec, QC_DATA_BUFF * pBuff)
{
	int					nRC = 0;
	QCCodec_Context *	pContext = (QCCodec_Context *)hCodec;

	if ((pBuff->uFlag & QCBUFF_HEADDATA) == QCBUFF_HEADDATA)
		nRC = avcodec_send_packet(pContext->pDecCtx, NULL);

	if (pBuff->uBuffType == QC_BUFF_TYPE_Packet)
	{
		memcpy(&pContext->pktData, pBuff->pBuffPtr, sizeof(AVPacket));
	}
	else
	{
		pContext->pktData.data = pBuff->pBuff;
		pContext->pktData.size = pBuff->uSize;
		pContext->pktData.pts = pBuff->llTime;
		if ((pBuff->uFlag & QCBUFF_KEY_FRAME) == QCBUFF_KEY_FRAME)
			pContext->pktData.flags = AV_PKT_FLAG_KEY;
		else if ((pBuff->uFlag & QCBUFF_HEADDATA) == QCBUFF_HEADDATA)
			pContext->pktData.flags = AV_PKT_FLAG_KEY;
		else
			pContext->pktData.flags = 0;
	}
	pContext->pPacket = &pContext->pktData;
	pContext->llDelay = pBuff->llDelay;
	nRC = avcodec_send_packet(pContext->pDecCtx, pContext->pPacket);
	if (nRC == 0)
		return QC_ERR_NONE;
	else if (nRC == AVERROR(EAGAIN))
		return QC_ERR_RETRY;
	else
		return QC_ERR_FAILED;
}

int qcCodec_GetBuff_V2(void * hCodec, QC_DATA_BUFF ** ppBuff)
{
	QCCodec_Context *	pContext = (QCCodec_Context *)hCodec;
	int					nRC = QC_ERR_NONE;

	if (pContext->nAVType == 1)
	{
		pContext->pDecCtx->skip_frame = AVDISCARD_DEFAULT;
		pContext->pDecCtx->skip_loop_filter = AVDISCARD_DEFAULT;
		if (pContext->llDelay >= 50)
			pContext->pDecCtx->skip_loop_filter = AVDISCARD_ALL;
		if (pContext->llDelay >= 100)
			pContext->pDecCtx->skip_frame = AVDISCARD_NONREF;
	}

	nRC = avcodec_receive_frame(pContext->pDecCtx, pContext->pAVFrm);
	if (nRC == 0)
	{
		if (pContext->nAVType == 1)
		{
			pContext->pBufVideo->pBuff[0] = (unsigned char *)pContext->pAVFrm->data[0];
			pContext->pBufVideo->pBuff[1] = (unsigned char *)pContext->pAVFrm->data[1];
			pContext->pBufVideo->pBuff[2] = (unsigned char *)pContext->pAVFrm->data[2];
			pContext->pBufVideo->nStride[0] = pContext->pAVFrm->linesize[0];
			pContext->pBufVideo->nStride[1] = pContext->pAVFrm->linesize[1];
			pContext->pBufVideo->nStride[2] = pContext->pAVFrm->linesize[2];
			pContext->pBufVideo->nWidth = pContext->pAVFrm->width;
			pContext->pBufVideo->nHeight = pContext->pAVFrm->height;
			pContext->pBufVideo->nType = QC_VDT_YUV420_P;

			if (pContext->pAVFrm->pts >= 0)
				pContext->pBuffData->llTime = pContext->pAVFrm->pts;
			else
				pContext->pBuffData->llTime = pContext->pktData.pts;
			*ppBuff = pContext->pBuffData;
		}
		else if (pContext->nAVType == 0)
		{
			pContext->pFrmAudio->nSampleRate = pContext->pAVFrm->sample_rate;
			pContext->pFrmAudio->nChannels = pContext->pAVFrm->channels;
			pContext->pFrmAudio->nFormat = pContext->pAVFrm->format;
			pContext->pFrmAudio->nNBSamples = pContext->pAVFrm->nb_samples;
			pContext->pFrmAudio->pDataBuff[0] = (char *)pContext->pAVFrm->data[0];
			pContext->pFrmAudio->pDataBuff[1] = (char *)pContext->pAVFrm->data[1];
			pContext->pFrmAudio->pDataBuff[2] = (char *)pContext->pAVFrm->data[2];
			pContext->pFrmAudio->nDataSize[0] = pContext->pAVFrm->linesize[0];
			pContext->pFrmAudio->nDataSize[1] = pContext->pAVFrm->linesize[1];
			pContext->pFrmAudio->nDataSize[2] = pContext->pAVFrm->linesize[2];
			pContext->pBuffData->pData = pContext->pFrmAudio;

			pContext->pBuffData->pBuff = pContext->pAVFrm->data[0];
			pContext->pBuffData->uSize = pContext->pAVFrm->linesize[0];

			if (pContext->pAVFrm->pts >= 0)
				pContext->pBuffData->llTime = pContext->pAVFrm->pts;
			else
				pContext->pBuffData->llTime = pContext->pktData.pts;

			if (pContext->pFmtAudio->nChannels != pContext->pAVFrm->channels || pContext->pFmtAudio->nSampleRate != pContext->pAVFrm->sample_rate)
			{
				pContext->pFmtAudio->nChannels = pContext->pAVFrm->channels;
				pContext->pFmtAudio->nSampleRate = pContext->pAVFrm->sample_rate;
				pContext->pBuffData->uFlag = QCBUFF_NEW_FORMAT;
				pContext->pBuffData->pFormat = pContext->pFmtAudio;
			}
			*ppBuff = pContext->pBuffData;
		}

		return QC_ERR_NONE;
	}
	else if (nRC == AVERROR_EOF)
		return QC_ERR_FINISH;
	else if (nRC == AVERROR(EAGAIN))
		return QC_ERR_NEEDMORE;
	else
		return QC_ERR_FAILED;
}

int	qcCodec_InitSpeex(QCCodec_Context * pContext, QC_AUDIO_FORMAT * pFmt)
{
	if (pContext == NULL || pContext->pSpeexDec == NULL || pFmt == NULL)
		return QC_ERR_ARG;

	QCCodec_Speex *		pSpeexDec = pContext->pSpeexDec;
	int					nRC = QC_ERR_NONE;
	const SpeexMode *	mode;
	int					spx_mode = 0;
	SpeexHeader *		header = NULL;
	if (pFmt->pHeadData != NULL && pFmt->nHeadSize >= 80)
	{
		header = speex_packet_to_header((char *)pFmt->pHeadData, pFmt->nHeadSize);
	}
	if (pFmt->nFourCC == MAKEFOURCC('S', 'P', 'X', 'N'))
	{
		int quality;
		if (pFmt->pHeadData == NULL || pFmt->nHeadSize < 47)
			return QC_ERR_ARG;
		quality = pFmt->pHeadData[37];
		if (quality > 10)
			return QC_ERR_ARG;
		pSpeexDec->m_nPacketSize = Speex_pkt_size[quality];
		spx_mode = 0;
	}
	else if (header != NULL)
	{
		pFmt->nChannels = header->nb_channels;
		pFmt->nSampleRate = header->rate;
		spx_mode = header->mode;
		speex_header_free(header);
	}
	else
	{
		switch (pFmt->nSampleRate)
		{
		case 8000:  spx_mode = 0; break;
		case 16000: spx_mode = 1; break;
		case 32000: spx_mode = 2; break;
		default:	spx_mode = 2; break;
		}
	}

	mode = speex_lib_get_mode(spx_mode);
	if (mode == NULL)
		return QC_ERR_FAILED;
	pSpeexDec->m_nFrameSize = SPEEX_FRAME_SIZE << spx_mode;
	if (pFmt->nSampleRate == 0)
		pFmt->nSampleRate = 8000 << spx_mode;
	if (pFmt->nChannels < 1 || pFmt->nChannels > 2)
		pFmt->nChannels = 2;

	speex_bits_init(&pSpeexDec->m_bits);
	pSpeexDec->m_pState = speex_decoder_init(mode);
	if (pSpeexDec->m_pState == NULL)
		return QC_ERR_FAILED;

	if (pFmt->nChannels == 2)
	{
		SpeexCallback callback;
		callback.callback_id = SPEEX_INBAND_STEREO;
		callback.func = speex_std_stereo_request_handler;
		callback.data = &pSpeexDec->m_stereo;
		//pSpeexDec->m_stereo = SPEEX_STEREO_STATE_INIT;
		pSpeexDec->m_stereo.balance = 1;
		pSpeexDec->m_stereo.e_ratio = 0.5;
		pSpeexDec->m_stereo.smooth_left = 1;
		pSpeexDec->m_stereo.smooth_right = 1;
		pSpeexDec->m_stereo.reserved1 = 0;
		pSpeexDec->m_stereo.reserved2 = 0;

		speex_decoder_ctl(pSpeexDec->m_pState, SPEEX_SET_HANDLER, &callback);
	}

	pContext->pBuffData->nMediaType = QC_MEDIA_Audio;
	pContext->pBuffData->uBuffType = QC_BUFF_TYPE_Data;
	pContext->pBuffData->uBuffSize = 1024 * 32;
	pContext->pBuffData->pBuff = (unsigned char *)malloc(pContext->pBuffData->uBuffSize);

	if (pContext->pFmtAudio != NULL)
	{
		pContext->pFrmAudio->nChannels = pFmt->nChannels;
		pContext->pFmtAudio->nSampleRate = pFmt->nSampleRate;
	}

	return nRC;
}

int	qcCodec_UninitSpeex(QCCodec_Context * pContext)
{
	if (pContext == NULL || pContext->pSpeexDec == NULL)
		return QC_ERR_ARG;

	int nRC = QC_ERR_NONE;
	if (pContext->pSpeexDec->m_pState != NULL)
	{
		speex_decoder_destroy(pContext->pSpeexDec->m_pState);
		speex_bits_destroy(&pContext->pSpeexDec->m_bits);
		pContext->pSpeexDec->m_pState = NULL;
	}

	if (pContext->pBuffData != NULL && pContext->pBuffData->pBuff != NULL)
		free(pContext->pBuffData->pBuff);
	pContext->pBuffData->pBuff = NULL;

	return nRC;
}

int	qcCodec_Release(QCCodec_Context * pContext)
{
	if (pContext == NULL)
		return QC_ERR_ARG;

	if (pContext->nCodecID == QC_CODEC_ID_SPEEX)
		qcCodec_UninitSpeex(pContext);

	if (pContext->pAVFrm != NULL)
		av_frame_free(&pContext->pAVFrm);
	pContext->pAVFrm = NULL;
	if (pContext->pDecCtx != NULL)
	{
		avcodec_close(pContext->pDecCtx);
		if (pContext->pDecCtxNew != NULL)
			avcodec_free_context(&pContext->pDecCtxNew);
		pContext->pDecCtx = NULL;
	}
	if (pContext->pBuffData != NULL)
		free(pContext->pBuffData);
	pContext->pBuffData = NULL;
	if (pContext->pBufVideo != NULL)
		free(pContext->pBufVideo);
	pContext->pBufVideo = NULL;
	if (pContext->pFmtAudio != NULL)
		free(pContext->pFmtAudio);
	pContext->pFmtAudio = NULL;
	if (pContext->pFrmAudio != NULL)
		free(pContext->pFrmAudio);
	pContext->pFrmAudio = NULL;

	if (pContext->pSpeexDec != NULL)
	{
		free(pContext->pSpeexDec);
		pContext->pSpeexDec = NULL;
	}
	free(pContext);
	return QC_ERR_NONE;
}
