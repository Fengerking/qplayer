/*******************************************************************************
	File:		qcCodec.cpp

	Contains:	Create the Codec with codec id

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-24		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcCodec.h"

#ifdef __QC_OS_NDK__
#include <unistd.h>
#endif // __QC_OS_NDK__

#if defined(__QC_OS_IOS__) || defined(__QC_OS_MACOS__)
#include <sys/sysctl.h>
#endif

#ifndef INT64_C
#define UINT64_C
#endif

#include "libavcodec/avcodec.h"
#include "qcFFLog.h"

typedef struct
{
	AVCodecContext *		pEncCtx;
	AVCodec *				pAVEnc;
	AVFrame *				pAVFrm;
	AVPacket				pktData;
	QC_VIDEO_FORMAT *		pFormat;
}QCEncoder_Context;

int	qcCreateEncoder(void ** phEnc, QC_VIDEO_FORMAT * pFormat)
{
	QCEncoder_Context * pEncCtx = NULL;
	enum AVCodecID		nEncID = AV_CODEC_ID_MJPEG;

	if (pFormat == NULL || phEnc == NULL)
		return QC_ERR_ARG;
	*phEnc = NULL;
	if (pFormat->nCodecID == QC_CODEC_ID_MJPEG)
		nEncID = AV_CODEC_ID_MJPEG;
	else
		return QC_ERR_UNSUPPORT;

	pEncCtx = (QCEncoder_Context *) malloc (sizeof(QCEncoder_Context));
	memset(pEncCtx, 0, sizeof(QCEncoder_Context));

	qclog_init();
	avcodec_register_all();
	pEncCtx->pAVEnc = avcodec_find_encoder(nEncID);
	if (pEncCtx->pAVEnc == NULL)
	{
		free(pEncCtx);
		return QC_ERR_UNSUPPORT;
	}
	pEncCtx->pEncCtx = avcodec_alloc_context3(pEncCtx->pAVEnc);
	if (pEncCtx->pEncCtx == NULL) 
	{
		free(pEncCtx);
		return QC_ERR_UNSUPPORT;
	}
	pEncCtx->pEncCtx->bit_rate = pFormat->nWidth * pFormat->nHeight / 5;
	pEncCtx->pEncCtx->width = pFormat->nWidth;
	pEncCtx->pEncCtx->height = pFormat->nHeight;
	pEncCtx->pEncCtx->time_base = (AVRational){ 1, 25 };
	pEncCtx->pEncCtx->gop_size = 10;
	pEncCtx->pEncCtx->max_b_frames = 0;
	pEncCtx->pEncCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;

	//if (nEncID == AV_CODEC_ID_H264)
	//	av_opt_set(pEncCtx->pEncCtx->priv_data, "preset", "slow", 0);
	if (avcodec_open2(pEncCtx->pEncCtx, pEncCtx->pAVEnc, NULL) < 0) 
	{
		avcodec_free_context (&pEncCtx->pEncCtx);
		free(pEncCtx);
		return QC_ERR_UNSUPPORT;
	}
	pEncCtx->pAVFrm = av_frame_alloc();
	if (pEncCtx->pAVFrm == NULL)
	{
		avcodec_free_context(&pEncCtx->pEncCtx);
		free(pEncCtx);
		return QC_ERR_UNSUPPORT;
	}

	pEncCtx->pAVFrm->format = AV_PIX_FMT_YUVJ420P;
	pEncCtx->pAVFrm->width = pFormat->nWidth;
	pEncCtx->pAVFrm->height = pFormat->nHeight;

	*phEnc = pEncCtx;

	return QC_ERR_NONE;
}

int	qcEncodeImage(void * hEnc, QC_VIDEO_BUFF * pVideo, QC_DATA_BUFF * pData)
{
	int					nRC = 0;
	int					nGetOut = 0;
	QCEncoder_Context * pEncCtx = (QCEncoder_Context *)hEnc;
	if (pEncCtx == NULL)
		return QC_ERR_ARG;
	if (pEncCtx->pAVFrm == NULL)
		return QC_ERR_STATUS;
    if (pVideo != NULL && pVideo->nType != QC_VDT_YUV420_P)
        return QC_ERR_ARG;

	if (pVideo != NULL)
	{
		pEncCtx->pAVFrm->data[0] = pVideo->pBuff[0];
		pEncCtx->pAVFrm->data[1] = pVideo->pBuff[1];
		pEncCtx->pAVFrm->data[2] = pVideo->pBuff[2];
		pEncCtx->pAVFrm->linesize[0] = pVideo->nStride[0];
		pEncCtx->pAVFrm->linesize[1] = pVideo->nStride[1];
		pEncCtx->pAVFrm->linesize[2] = pVideo->nStride[2];

		pEncCtx->pAVFrm->format = AV_PIX_FMT_YUVJ420P;
		pEncCtx->pAVFrm->width = pVideo->nWidth;
		pEncCtx->pAVFrm->height = pVideo->nHeight;

		pEncCtx->pAVFrm->pts = pData->llTime;
	}

	if (pEncCtx->pktData.data != NULL)
		av_packet_unref(&pEncCtx->pktData);
	av_init_packet(&pEncCtx->pktData);
	// packet data will be allocated by the encoder
	pEncCtx->pktData.data = NULL;    
	pEncCtx->pktData.size = 0;

	// encode the image 
	if (pVideo != NULL)
		nRC = avcodec_encode_video2(pEncCtx->pEncCtx, &pEncCtx->pktData, pEncCtx->pAVFrm, &nGetOut);
	else
		nRC = avcodec_encode_video2(pEncCtx->pEncCtx, &pEncCtx->pktData, NULL, &nGetOut);
	if (nRC < 0)
		return QC_ERR_FAILED;

	if (nGetOut) 
	{
		pData->pBuff = pEncCtx->pktData.data;
		pData->uSize = pEncCtx->pktData.size;
		pData->llTime = pEncCtx->pktData.pts;
		return QC_ERR_NONE;
	}

	return QC_ERR_FAILED;
}

int	qcDestroyEncoder(void * hEnc)
{
	qclog_uninit();

	QCEncoder_Context * pEncCtx = (QCEncoder_Context *)hEnc;
	if (pEncCtx == NULL)
		return QC_ERR_ARG;

	if (pEncCtx->pktData.data != NULL)
		av_packet_unref(&pEncCtx->pktData);

	avcodec_free_context(&pEncCtx->pEncCtx);

	av_frame_free(&pEncCtx->pAVFrm);

	free(pEncCtx);

	return QC_ERR_NONE;
}



/*
static void video_encode_example(const char *filename, int codec_id)
{
	AVCodec *codec;
	AVCodecContext *c = NULL;
	int i, ret, x, y, got_output;
	FILE *f;
	AVFrame *frame;
	AVPacket pkt;
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };


	// find the video encoder 
	codec = avcodec_find_encoder(codec_id);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}

	c = avcodec_alloc_context3(codec);
	if (!c) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	// put sample parameters 
	c->bit_rate = 400000;
	// resolution must be a multiple of two 
	c->width = 352;
	c->height = 288;
	// frames per second 
	c->time_base = (AVRational){ 1, 25 };
	// emit one intra frame every ten frames
	// check frame pict_type before passing frame
	// to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	// then gop_size is ignored and the output of encoder
	// will always be I frame irrespective to gop_size
	
	c->gop_size = 10;
	c->max_b_frames = 1;
	c->pix_fmt = AV_PIX_FMT_YUV420P;

	if (codec_id == AV_CODEC_ID_H264)
		av_opt_set(c->priv_data, "preset", "slow", 0);

	// open it 
	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;

	// the image can be allocated by any means and av_image_alloc() is
	// just the most convenient way if av_malloc() is to be used 
	ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
		c->pix_fmt, 32);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate raw picture buffer\n");
		exit(1);
	}

	// encode 1 second of video
	for (i = 0; i < 25; i++) {
		av_init_packet(&pkt);
		pkt.data = NULL;    // packet data will be allocated by the encoder
		pkt.size = 0;


		frame->pts = i;

		// encode the image 
		ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
		if (ret < 0) {
			fprintf(stderr, "Error encoding frame\n");
			exit(1);
		}

		if (got_output) {
			printf("Write frame %3d (size=%5d)\n", i, pkt.size);
			fwrite(pkt.data, 1, pkt.size, f);
			av_packet_unref(&pkt);
		}
	}

	// get the delayed frames
	for (got_output = 1; got_output; i++) {
		fflush(stdout);

		ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
		if (ret < 0) {
			fprintf(stderr, "Error encoding frame\n");
			exit(1);
		}

		if (got_output) {
			printf("Write frame %3d (size=%5d)\n", i, pkt.size);
			fwrite(pkt.data, 1, pkt.size, f);
			av_packet_unref(&pkt);
		}
	}

	avcodec_free_context(&c);
	av_freep(&frame->data[0]);
	av_frame_free(&frame);
	printf("\n");
}
*/
