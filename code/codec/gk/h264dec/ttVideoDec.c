#include "ttVideoDec.h"
#include "ttAvcodec.h"
#include "AVCDecoderTypes.h"
#include "M4a.h"

typedef struct __TTVideoDecContext {
	TTCodec *codec;
	TTCodecContext *c;
	TTPacket avpkt;
	int nCpuNum;
	unsigned char * pHeadBuffer;
	int	nHeadSize;
	TTFrame *frameDecode;
	TTPicture mOutpic;
	int nType;
} TTVideoDecContext;

TTInt32 ttMPEG4DecInit(TTVideoDecContext* pVideoDecContext)
{
	TTVideoDecContext * pDecContext;
	if (pVideoDecContext == NULL)
		return TTKErrArgument;

	pDecContext = (TTVideoDecContext *)pVideoDecContext;
	
	ttcodec_register();

	/* find the mpeg audio decoder */
	pDecContext->codec = ttcodec_find_decoder(TTV_CODEC_ID_MPEG4);
	if (!pDecContext->codec) {
		return TTKErrUnknown;
	}

	pDecContext->c = ttcodec_alloc_context3(pDecContext->codec);
	if (!pDecContext->c) {
		return TTKErrUnknown;
	}

	if (pDecContext->nCpuNum > 1) {
		//for multi-frame setting
		pDecContext->codec->capabilities |= CODEC_CAP_FRAME_THREADS;
		pDecContext->c->flags &= ~(CODEC_FLAG_TRUNCATED|CODEC_FLAG_LOW_DELAY);
		pDecContext->c->flags2 &= ~CODEC_FLAG2_CHUNKS;
		pDecContext->c->thread_count = pDecContext->nCpuNum;
		pDecContext->c->skip_loop_filter = AVDISCARD_NONE;		
	} else{
		// we do not send complete frames 
		pDecContext->c->flags2|= CODEC_FLAG2_CHUNKS;
	}

	tt_init_packet(&pDecContext->avpkt);

	/* open it */
	if (ttcodec_open2(pDecContext->c, pDecContext->codec, NULL) < 0) {
		return TTKErrUnknown;
	}

	if (!(pDecContext->frameDecode = ttv_frame_alloc())) {
		return TTKErrUnknown;
	}

	return TTKErrNone;
}


TTInt32 ttH264DecInit(TTVideoDecContext* pVideoDecContext)
{
	TTVideoDecContext * pDecContext;
	if (pVideoDecContext == NULL)
		return TTKErrArgument;

	pDecContext = (TTVideoDecContext *)pVideoDecContext;
	
	ttcodec_register();

	/* find the mpeg audio decoder */
	pDecContext->codec = ttcodec_find_decoder(TTV_CODEC_ID_H264);
	if (!pDecContext->codec) {
		return TTKErrUnknown;
	}

	pDecContext->c = ttcodec_alloc_context3(pDecContext->codec);
	if (!pDecContext->c) {
		return TTKErrUnknown;
	}

	if (pDecContext->nCpuNum > 1) {
		//for multi-frame setting
		if(pDecContext->nType==0)
		{
			pDecContext->codec->capabilities |= CODEC_CAP_FRAME_THREADS;
		}
		else
		{
			pDecContext->codec->capabilities |= CODEC_CAP_SLICE_THREADS;
		}

		pDecContext->c->flags &= ~(CODEC_FLAG_TRUNCATED|CODEC_FLAG_LOW_DELAY);
		pDecContext->c->flags2 &= ~CODEC_FLAG2_CHUNKS;
		pDecContext->c->thread_count = pDecContext->nCpuNum;
		pDecContext->c->skip_loop_filter = AVDISCARD_NONE;
	} else{
		// we do not send complete frames 
		pDecContext->c->flags2|= CODEC_FLAG2_CHUNKS; 
	}

	tt_init_packet(&pDecContext->avpkt);

	/* open it */
	if (ttcodec_open2(pDecContext->c, pDecContext->codec, NULL) < 0) {
		return TTKErrUnknown;
	}

	if (!(pDecContext->frameDecode = ttv_frame_alloc())) {
		return TTKErrUnknown;
	}

	return TTKErrNone;
}


TTInt32 ttVideoDecOpen(TTHandle * hHandle)
{
	TTVideoDecContext * pDecContext = (TTVideoDecContext *)malloc (sizeof (TTVideoDecContext));
	if (pDecContext == NULL)
		return TTKErrNoMemory;

	memset(pDecContext, 0, sizeof(TTVideoDecContext));

	pDecContext->nCpuNum = 4;
		
	*hHandle = pDecContext;
	
	return TTKErrNone;
}

TTInt32 ttMPEG4DecSetInputData(TTHandle hHandle, TTBuffer *InBuffer)
{
	int nErr = 0;
	TTVideoDecContext * pDecContext;
	
	if (hHandle == NULL)
		return TTKErrArgument;
		
	pDecContext = (TTVideoDecContext *)hHandle;
	if (pDecContext->codec == NULL)
	{
		nErr = ttMPEG4DecInit(pDecContext);
		if (nErr != TTKErrNone)
			return nErr;
	}
	
	if (InBuffer->nSize == 0)
		return TTKErrUnderflow;
		
	if (pDecContext->nHeadSize > 0 && pDecContext->pHeadBuffer != NULL)
	{
		memcpy (pDecContext->pHeadBuffer + pDecContext->nHeadSize, InBuffer->pBuffer, InBuffer->nSize);
		pDecContext->avpkt.data = pDecContext->pHeadBuffer;			
		pDecContext->avpkt.size = pDecContext->nHeadSize + InBuffer->nSize;
		pDecContext->nHeadSize = 0;
	}
	else
	{
		pDecContext->avpkt.data = InBuffer->pBuffer;	
		pDecContext->avpkt.size = InBuffer->nSize;
		if (pDecContext->pHeadBuffer != NULL)
		{
			free (pDecContext->pHeadBuffer);
			pDecContext->pHeadBuffer = NULL;
		}
	}		
	
	pDecContext->avpkt.pts = InBuffer->llTime;
	pDecContext->avpkt.dts = InBuffer->llTime + 1;
	
	return TTKErrNone;
}

TTInt32 ttVideoDecSetInputData(TTHandle hHandle, TTBuffer *InBuffer)
{
	int nErr = 0;
	TTVideoDecContext * pDecContext;
	
	if (hHandle == NULL)
		return TTKErrArgument;
		
	pDecContext = (TTVideoDecContext *)hHandle;
	if (pDecContext->codec == NULL)
	{
		nErr = ttH264DecInit (pDecContext);
		if (nErr != TTKErrNone)
			return nErr;
	}
	
	if (InBuffer->nSize == 0)
		return TTKErrUnderflow;
		
	if (pDecContext->nHeadSize > 0 && pDecContext->pHeadBuffer != NULL)
	{
		memcpy (pDecContext->pHeadBuffer + pDecContext->nHeadSize, InBuffer->pBuffer, InBuffer->nSize);
		pDecContext->avpkt.data = pDecContext->pHeadBuffer;			
		pDecContext->avpkt.size = pDecContext->nHeadSize + InBuffer->nSize;
		pDecContext->nHeadSize = 0;
	}
	else
	{
		pDecContext->avpkt.data = InBuffer->pBuffer;	
		pDecContext->avpkt.size = InBuffer->nSize;
		if (pDecContext->pHeadBuffer != NULL)
		{
			free (pDecContext->pHeadBuffer);
			pDecContext->pHeadBuffer = NULL;
		}
	}		
	
	pDecContext->avpkt.pts = InBuffer->llTime;
	pDecContext->avpkt.dts = InBuffer->llTime + 1;
	
	return TTKErrNone;
}

TTInt32 ttVideoDecProcess(TTHandle hHandle, TTVideoBuffer* OutBuffer, TTVideoFormat* pOutInfo)
{	
	int nLen = 0;
	int nGotPic = 0;
	TTVideoDecContext * pDecContext;

	if (hHandle == NULL)
		return TTKErrArgument;
		
	pDecContext = (TTVideoDecContext *)hHandle;
	if (pDecContext->codec == NULL)
		return TTKErrUnknown;
		
	if (pDecContext->avpkt.size <= 0)
		return TTKErrUnderflow;

	nLen = ttcodec_decode_video2(pDecContext->c, pDecContext->frameDecode, &nGotPic, &pDecContext->avpkt);
	if(nLen > 0) {
		pDecContext->avpkt.data += nLen;
		pDecContext->avpkt.size -= nLen;
	}

	if (nLen < 0 || nGotPic <= 0)
		return TTKErrUnderflow;
		
	OutBuffer->Buffer[0] = pDecContext->frameDecode->data[0];
	OutBuffer->Stride[0] = pDecContext->frameDecode->linesize[0];	
	
	OutBuffer->Buffer[1] = pDecContext->frameDecode->data[1];
	OutBuffer->Stride[1] = pDecContext->frameDecode->linesize[1];	
	
	OutBuffer->Buffer[2] = pDecContext->frameDecode->data[2];
	OutBuffer->Stride[2] = pDecContext->frameDecode->linesize[2];	
	
	OutBuffer->ColorType = TT_COLOR_YUV_PLANAR420;
	OutBuffer->Time = pDecContext->frameDecode->pkt_pts;
	
	pOutInfo->Width = pDecContext->frameDecode->width;
	pOutInfo->Height = pDecContext->frameDecode->height;
		
	return TTKErrNone;
}

TTInt32 ttVideoDecClose(TTHandle hHandle)
{
	TTVideoDecContext * pDecContext;
	if (hHandle == NULL)
		return TTKErrArgument;

	pDecContext = (TTVideoDecContext *)hHandle;	

	if (pDecContext->c != NULL)
	{
		ttcodec_close(pDecContext->c);	
		ttv_free(pDecContext->c);
		ttv_free(pDecContext->frameDecode);
		pDecContext->c = NULL;
	}
	
	if (pDecContext->pHeadBuffer != NULL) {
		free (pDecContext->pHeadBuffer);
		pDecContext->nHeadSize = 0;
	}

	free (pDecContext);
		
	return TTKErrNone;
	
}

TTInt32 ttVideoDecSetParam(TTHandle hHandle, TTInt32 uParamID, TTPtr pData)
{
	TTVideoDecContext * pDecContext;
	if (hHandle == NULL)
		return TTKErrArgument;

	pDecContext = (TTVideoDecContext *)hHandle;	
		
	switch (uParamID)
	{
	case TT_PID_VIDEO_DECODER_INFO:
		{
			TTAVCDecoderSpecificInfo * pBuffer = (TTAVCDecoderSpecificInfo *)pData;
			if(pData == NULL)
				return TTKErrArgument;
			if (pDecContext->pHeadBuffer != NULL)
				free (pDecContext->pHeadBuffer);
			pDecContext->nHeadSize = pBuffer->iSize;
			pDecContext->pHeadBuffer = malloc (1024 * 1024);
			memcpy (pDecContext->pHeadBuffer, pBuffer->iData, pBuffer->iSize);
		}
		return TTKErrNone;
		
	case TT_PID_VIDEO_THREAD_NUM:
		{
			pDecContext->nCpuNum = *(int *)pData;
		}
		return TTKErrNone;
	case TT_PID_VIDEO_FLUSH:
		{
			if(pDecContext->c)
				ttcodec_flush_buffers(pDecContext->c);
		}
		return TTKErrNone;
	case TT_PID_VIDEO_ENDEBLOCK:
		{
			if(pData == NULL || *(int *)pData == 0) {
				if(pDecContext->c)
					pDecContext->c->skip_loop_filter = AVDISCARD_ALL;			
			} else if(pData != NULL && *(int *)pData != 0) {
				if(pDecContext->c)
					pDecContext->c->skip_loop_filter = AVDISCARD_NONE;
			}
		}
		return TTKErrNone;

	default:
		break;
	}

	return TTKErrArgument;
}

TTInt32 ttVideoDecGetParam(TTHandle hHandle, TTInt32 uParamID, TTPtr pData)
{
	return TTKErrArgument;
}

TTInt32 ttGetH264DecAPI (TTVideoCodecAPI* pDecHandle)
{
	pDecHandle->Open = ttVideoDecOpen;
	pDecHandle->SetInput = ttVideoDecSetInputData;
	pDecHandle->Process = ttVideoDecProcess;
	pDecHandle->Close = ttVideoDecClose;
	pDecHandle->SetParam = ttVideoDecSetParam;
	pDecHandle->GetParam = ttVideoDecGetParam;

	return TTKErrNone;
}