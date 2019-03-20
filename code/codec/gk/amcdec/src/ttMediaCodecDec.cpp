/**
* File : ttMediaCodecDec.cpp 
* Created on : 2015-1-1
* Author : yongping.lin
* Copyright : Copyright (c) 2016 GoKu Software Ltd. All rights reserved.
* Description : ttMediaCodecDec实现文件
*/

#include "GKOsalConfig.h"
#include "GKTypedef.h"
#include "GKMacrodef.h"
#include "ttMediaCodecJDec.h"
#include "MediaCodecJava.h"

#if defined __cplusplus
extern "C" {
#endif

static TTInt32 ttMCJH264DecOpen(TTHandle * phCodec)
{
	CMediaCodecJava *pMCJDecoder = new CMediaCodecJava(264);

	*phCodec = (TTHandle)pMCJDecoder;

	return TTKErrNone;
}

static TTInt32 ttMCJMPEG4DecOpen(TTHandle * phCodec)
{
	CMediaCodecJava *pMCJDecoder = new CMediaCodecJava(4);

	*phCodec = (TTHandle)pMCJDecoder;

	return TTKErrNone;
}

static TTInt32 ttMCJHEVCDecOpen(TTHandle * phCodec)
{
	CMediaCodecJava *pMCJDecoder = new CMediaCodecJava(265);

	*phCodec = (TTHandle)pMCJDecoder;

	return TTKErrNone;
}


static TTInt32 ttMCJVideoDecSetInputData(TTHandle hCodec, TTBuffer * pInput)
{
	CMediaCodecJava *pMCJDecoder = (CMediaCodecJava *)hCodec;
	TTInt nErr = TTKErrNotReady;

	if(pMCJDecoder != NULL)
		nErr = pMCJDecoder->setInputBuffer(pInput);

	return nErr;
}

static TTInt32 ttMCJVideoDecProcess(TTHandle hCodec, TTVideoBuffer * pOutput, TTVideoFormat* pOutInfo)
{
	CMediaCodecJava *pMCJDecoder = (CMediaCodecJava *)hCodec;
	TTInt nErr = TTKErrNotReady;

	if(pMCJDecoder != NULL)
		nErr = pMCJDecoder->getOutputBuffer(pOutput, pOutInfo);

	return nErr;
}

static TTInt32 ttMCJVideoDecSetParam(TTHandle hCodec, TTInt32 uParamID, TTPtr pData)
{
	CMediaCodecJava *pMCJDecoder = (CMediaCodecJava *)hCodec;
	TTInt nErr = TTKErrNotReady;
	if(NULL == pMCJDecoder) {
		return TTKErrArgument;
	}

	switch(uParamID)
	{
	case TT_PID_VIDEO_START:
		return pMCJDecoder->start();
	case TT_PID_VIDEO_STOP:
		return pMCJDecoder->uninitDecode();
	case TT_PID_VIDEO_RENDERBUFFER:
		return pMCJDecoder->renderOutputBuffer((TTVideoBuffer* )pData, true);
	}

	return pMCJDecoder->setParam(uParamID, pData);
}

static TTInt32 ttMCJVideoDecGetParam(TTHandle hCodec, TTInt32 uParamID, TTPtr pData)
{
	CMediaCodecJava *pMCJDecoder = (CMediaCodecJava *)hCodec;
	if(NULL == pMCJDecoder) {
		return TTKErrArgument;
	}
	
	return pMCJDecoder->getParam(uParamID, pData);
}

static TTInt32 ttMCJVideoDecClose(TTHandle hCodec)
{
	CMediaCodecJava *pMCJDecoder = (CMediaCodecJava *)hCodec;
	SAFE_DELETE(pMCJDecoder);
	return TTKErrNone;
}

TTInt32 ttGetH264DecAPI (TTVideoCodecAPI* pDecHandle)
{
	pDecHandle->Open = ttMCJH264DecOpen;
	pDecHandle->SetInput = ttMCJVideoDecSetInputData;
	pDecHandle->Process = ttMCJVideoDecProcess;
	pDecHandle->Close = ttMCJVideoDecClose;
	pDecHandle->SetParam = ttMCJVideoDecSetParam;
	pDecHandle->GetParam = ttMCJVideoDecGetParam;

	return TTKErrNone;
}

TTInt32 ttGetMPEG4DecAPI(TTVideoCodecAPI* pDecHandle)
{
	pDecHandle->Open = ttMCJMPEG4DecOpen;
	pDecHandle->SetInput = ttMCJVideoDecSetInputData;
	pDecHandle->Process = ttMCJVideoDecProcess;
	pDecHandle->Close = ttMCJVideoDecClose;
	pDecHandle->SetParam = ttMCJVideoDecSetParam;
	pDecHandle->GetParam = ttMCJVideoDecGetParam;

	return TTKErrNone;
}

TTInt32 ttGetHEVCDecAPI(TTVideoCodecAPI* pDecHandle)
{
	pDecHandle->Open = ttMCJHEVCDecOpen;
	pDecHandle->SetInput = ttMCJVideoDecSetInputData;
	pDecHandle->Process = ttMCJVideoDecProcess;
	pDecHandle->Close = ttMCJVideoDecClose;
	pDecHandle->SetParam = ttMCJVideoDecSetParam;
	pDecHandle->GetParam = ttMCJVideoDecGetParam;

	return TTKErrNone;
}

#if defined __cplusplus
}
#endif
