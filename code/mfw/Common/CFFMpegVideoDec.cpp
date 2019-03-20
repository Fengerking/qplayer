/*******************************************************************************
	File:		CFFMpegVideoDec.cpp

	Contains:	The ffmpeg video dec implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-15		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CFFMpegVideoDec.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

CFFMpegVideoDec::CFFMpegVideoDec(void * hInst)
	: CBaseVideoDec (hInst)
	, m_hLib (NULL)
	, m_pDecCtx (NULL)
	, m_pNewCtx (NULL)
	, m_pDecVideo (NULL)
	, m_pFrmVideo (NULL)
	, m_pPacket (NULL)
{
	SetObjectName ("CFFMpegVideoDec");
	memset (&m_fmtVideo, 0, sizeof (m_fmtVideo));

	av_init_packet (&m_pktData);
	m_pktData.data = NULL;
	m_pktData.size = 0;

	avcodec_register_all();
}

CFFMpegVideoDec::~CFFMpegVideoDec(void)
{
	Uninit ();
}

int CFFMpegVideoDec::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;
	Uninit ();
/*
	m_hLib = (qcLibHandle)qcLibLoad ("qcMedia", 0);
	if (m_hLib == NULL)
		return QC_ERR_FAILED;
	AVCODEC_REGISTER_ALL	fRegAll		= (AVCODEC_REGISTER_ALL)qcLibGetAddr (m_hLib, "avcodec_register_all", 0);
	AVCODEC_FIND_DECODER	fFindDec	= (AVCODEC_FIND_DECODER)qcLibGetAddr (m_hLib, "avcodec_find_decoder", 0);
	AVCODEC_ALLOC_CONTEXT3	fAllocC3	= (AVCODEC_ALLOC_CONTEXT3)qcLibGetAddr (m_hLib, "avcodec_alloc_context3", 0);
	AVCODEC_OPEN2			fOpen2		= (AVCODEC_OPEN2)qcLibGetAddr (m_hLib, "avcodec_open2", 0);
	AV_FRAME_ALLOC			fAllocFrm	= (AV_FRAME_ALLOC)qcLibGetAddr (m_hLib, "av_frame_alloc", 0);


	fRegAll ();
*/
	AVCodecID nCodecID = AV_CODEC_ID_H264;
	if (pFmt->nCodecID == QC_CODEC_ID_H265)
		nCodecID = AV_CODEC_ID_HEVC;
	m_pDecVideo = avcodec_find_decoder (nCodecID);
	if (m_pDecVideo == NULL)
		return QC_ERR_FAILED;
	if (pFmt->nSourceType == QC_SOURCE_FF && pFmt->pPrivateData != NULL)
		m_pDecCtx = (AVCodecContext *)pFmt->pPrivateData;
	else
	{
		m_pNewCtx = avcodec_alloc_context3 (m_pDecVideo);
		m_pDecCtx = m_pNewCtx;
	}
	if (m_pDecCtx == NULL)
		return QC_ERR_MEMORY;
	int nCPUNum = qcGetCPUNum ();
	m_pDecCtx->thread_count = nCPUNum;
	m_pDecCtx->thread_type = FF_THREAD_FRAME;		
	if (nCPUNum > 2)
	{
//		m_pDecCtx->thread_count = 1;//nCPUNum;
//		m_pDecCtx->thread_type = 3;//FF_THREAD_FRAME;		
	}
//#define FF_THREAD_FRAME   1 ///< Decode more than one frame at once
//#define FF_THREAD_SLICE   2 ///< Decode more than one part of a single frame at once


	if (m_pDecVideo->capabilities & AV_CODEC_CAP_TRUNCATED)
		m_pDecCtx->flags |= AV_CODEC_FLAG_TRUNCATED; // we do not send complete frames

	int nRC = avcodec_open2 (m_pDecCtx, m_pDecVideo, NULL);
	if (nRC < 0)
		return QC_ERR_FAILED;

	m_pFrmVideo = av_frame_alloc ();
	if (pFmt->pPrivateData == NULL && pFmt->pHeadData != NULL)
	{
		int nGotFrame = 0;
		m_pktData.data = pFmt->pHeadData;
		m_pktData.size = pFmt->nHeadSize;
		m_pktData.pts = 0;
		nRC = avcodec_decode_video2(m_pDecCtx, m_pFrmVideo, &nGotFrame, &m_pktData);
	}

	CBaseVideoDec::Init (pFmt);
	return QC_ERR_NONE;
}

int CFFMpegVideoDec::Uninit (void)
{
	if (m_pFrmVideo != NULL)
		av_frame_free (&m_pFrmVideo);
	m_pFrmVideo = NULL;

	if (m_pDecCtx != NULL)
		avcodec_close (m_pDecCtx);
	m_pDecCtx = NULL;
	if (m_pNewCtx != NULL)
		avcodec_free_context (&m_pNewCtx);
	m_pNewCtx = NULL;

	CBaseVideoDec::Uninit ();
	return QC_ERR_NONE;
}

int CFFMpegVideoDec::Flush (void)
{
	CAutoLock lock (&m_mtBuffer);
	if (m_pDecVideo != NULL)
		avcodec_flush_buffers (m_pDecCtx);
	return QC_ERR_NONE;
}

int CFFMpegVideoDec::FlushAll (void)
{
	return Flush ();
}

int CFFMpegVideoDec::SetBuff (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL)
		return QC_ERR_ARG;

	CAutoLock lock (&m_mtBuffer);
	CBaseVideoDec::SetBuff (pBuff);

	if ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
	{
		Flush ();
	}
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		if (pFmt->pHeadData != NULL)
		{
			int nGotFrame = 0;
			m_pktData.data = pFmt->pHeadData;
			m_pktData.size = pFmt->nHeadSize;
			m_pktData.pts = 0;
			avcodec_decode_video2(m_pDecCtx, m_pFrmVideo, &nGotFrame, &m_pktData);
		}
	}
	if (pBuff->uBuffType != QC_BUFF_TYPE_Packet)
	{
		m_pktData.data = pBuff->pBuff;
		m_pktData.size = pBuff->uSize;
		m_pktData.pts = pBuff->llTime;
		if (pBuff->uBuffSize < pBuff->uSize + 32)
			pBuff->uSize = pBuff->uSize;
		memset (pBuff->pBuff + pBuff->uSize, 0, 32);
		if ((pBuff->uFlag & QCBUFF_KEY_FRAME) == QCBUFF_KEY_FRAME)
			m_pktData.flags = AV_PKT_FLAG_KEY;
		else
			m_pktData.flags = 0;
		m_pPacket = &m_pktData;
	}
	else
	{
		m_pPacket = (AVPacket *)pBuff->pBuff;
	}
	m_pPacket->dts = abs ((int)pBuff->llDelay);

	return QC_ERR_NONE;
}

int CFFMpegVideoDec::GetBuff (QC_DATA_BUFF ** ppBuff)
{
	int nRC = QC_ERR_NONE;
	int	nGotFrame = 0;

	if (m_pDecCtx == NULL)
		return QC_ERR_STATUS;
	if (m_pPacket == NULL)
		return QC_ERR_NEEDMORE;

	CAutoLock lock (&m_mtBuffer);

	m_pDecCtx->skip_frame = AVDISCARD_DEFAULT;
	m_pDecCtx->skip_loop_filter = AVDISCARD_DEFAULT;
/*
#ifdef _OS_WINPC
	if (pBuff->llDelay > QCCFG_DDEBLOCK_TIME * 5)
		m_pDecCtx->skip_loop_filter = AVDISCARD_ALL;
	if (pBuff->llDelay > QCCFG_SKIP_BFRAME_TIME * 5)
		m_pDecCtx->skip_frame = AVDISCARD_NONREF;
#else
	if (pBuff->llDelay > QCCFG_DDEBLOCK_TIME)
		m_pDecCtx->skip_loop_filter = AVDISCARD_ALL;
	if (pBuff->llDelay > QCCFG_SKIP_BFRAME_TIME)
		m_pDecCtx->skip_frame = AVDISCARD_NONREF;
#endif // _OS_WINPC

	if ((pBuff->uFlag & QCBUFF_DEC_DISA_DEBLOCK) == QCBUFF_DEC_DISA_DEBLOCK)
		m_pDecCtx->skip_loop_filter = AVDISCARD_ALL;
	if ((pBuff->uFlag & QCBUFF_DEC_SKIP_BFRAME) == QCBUFF_DEC_SKIP_BFRAME)
		m_pDecCtx->skip_frame = AVDISCARD_NONREF;
	
	if (m_bDropFrame)
		m_pDecCtx->skip_frame = AVDISCARD_NONREF;
*/		
	m_pDecCtx->skip_loop_filter = AVDISCARD_ALL;		
	m_pDecCtx->skip_frame = AVDISCARD_NONREF;
	m_pDecCtx->skip_loop_filter = AVDISCARD_ALL;

	if (m_pPacket->data == NULL)
		return QC_ERR_RETRY;
	nRC = avcodec_decode_video2(m_pDecCtx, m_pFrmVideo, &nGotFrame, m_pPacket);
	m_pPacket = NULL;
	if (nGotFrame > 0)
	{
		if (m_pFrmVideo->width != m_fmtVideo.nWidth || m_pFrmVideo->height != m_fmtVideo.nHeight)
		{
			m_fmtVideo.nWidth = m_pFrmVideo->width;
			m_fmtVideo.nHeight = m_pFrmVideo->height;
			m_pBuffData->uFlag |= QCBUFF_NEW_FORMAT;
			m_pBuffData->pFormat = &m_fmtVideo;
		}
		m_buffVideo.pBuff[0] = (unsigned char *)m_pFrmVideo->data[0];
		m_buffVideo.pBuff[1] = (unsigned char *)m_pFrmVideo->data[1];
		m_buffVideo.pBuff[2] = (unsigned char *)m_pFrmVideo->data[2];
		m_buffVideo.nStride[0] = m_pFrmVideo->linesize[0];
		m_buffVideo.nStride[1] = m_pFrmVideo->linesize[1];
		m_buffVideo.nStride[2] = m_pFrmVideo->linesize[2];

		m_buffVideo.nWidth = m_pFrmVideo->width;
		m_buffVideo.nHeight = m_pFrmVideo->height;
		m_buffVideo.nType = QC_VDT_YUV420_P;

		if (m_pFrmVideo->pkt_pts >= 0)
			m_pBuffData->llTime = m_pFrmVideo->pkt_pts;
		else if (m_pFrmVideo->pkt_dts >= 0)
			m_pBuffData->llTime = m_pFrmVideo->pkt_dts;
		else if (m_pFrmVideo->pts >= 0)
			m_pBuffData->llTime = m_pFrmVideo->pts;

		CBaseVideoDec::GetBuff (&m_pBuffData);
		*ppBuff = m_pBuffData;
		return QC_ERR_NONE;
	}
	return QC_ERR_RETRY;
}
