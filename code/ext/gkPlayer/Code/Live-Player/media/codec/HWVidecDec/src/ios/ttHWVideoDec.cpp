/**
* File : TTHWVideoDec.cpp 
* Created on : 2015-3-31
* Author :
* Copyright : Copyright (c) 2014 Shuishi Software Ltd. All rights reserved.
* Description : TTHWVideoDeC
*/

#include "GKOsalConfig.h"
#include "GKTypedef.h"
#include "GKMacrodef.h"
#include "ttHWVideoDec.h"
#include "HWDecoder.h"

static TTInt32 ttHWH264DecOpen(TTHandle * phCodec)
{
	CTTHWDecoder *pHWDecoder = new CTTHWDecoder(264);

	*phCodec = (TTHandle)pHWDecoder;

	return TTKErrNone;
}

/*
static TTInt32 ttHWMPEG4DecOpen(TTHandle * phCodec)
{
	CTTHWDecoder *pHWDecoder = new CTTHWDecoder(4);

	*phCodec = (TTHandle)pHWDecoder;

	return TTKErrNone;
}*/


static TTInt32 ttHWVideoDecSetInputData(TTHandle hCodec, TTBuffer * pInput)
{
    CTTHWDecoder * pHWDecoder = (CTTHWDecoder *)(hCodec);
    if(pHWDecoder == NULL)
        return TTKErrArgument;
    
    if (pInput->nSize == 0)
        return TTKErrUnderflow;
    
    TTInt32 ret = pHWDecoder->SetInputData(pInput);
    
	return ret;
}

static TTInt32 ttHWVideoDecProcess(TTHandle hCodec, TTVideoBuffer * pOutput, TTVideoFormat* pOutInfo)
{
	CTTHWDecoder * pHWDecoder = (CTTHWDecoder *)(hCodec);
	if(pHWDecoder == NULL)
		return TTKErrArgument;

	return pHWDecoder->getOutputBuffer(pOutput, pOutInfo);
}

static TTInt32 ttHWVideoDecSetParam(TTHandle hCodec, TTInt32 uParamID, TTPtr pData)
{
	CTTHWDecoder * pHWDecoder = (CTTHWDecoder *)(hCodec);
	if(NULL == pHWDecoder) {
		return TTKErrArgument;
	}

	switch(uParamID)
	{
	case TT_PID_VIDEO_STOP:
		return pHWDecoder->uninitDecode();
    case TT_PID_VIDEO_FLUSH:
        return pHWDecoder->flush();
	}

	return pHWDecoder->setParam(uParamID, pData);
}

static TTInt32 ttHWVideoDecGetParam(TTHandle hCodec, TTInt32 uParamID, TTPtr pData)
{
	CTTHWDecoder * pHWDecoder = (CTTHWDecoder *)(hCodec);
	if(NULL == pHWDecoder) {
		return TTKErrArgument;
	}
	
	return pHWDecoder->getParam(uParamID, pData);
}

static TTInt32 ttHWVideoDecClose(TTHandle hCodec)
{
	CTTHWDecoder * pHWDecoder = (CTTHWDecoder *)(hCodec);
    if (pHWDecoder) {
        pHWDecoder->uninitDecode();
    }
	SAFE_DELETE(pHWDecoder);
	return TTKErrNone;
}

TTInt32 ttHWGetH264DecAPI (TTVideoCodecAPI* pDecHandle)
{
	pDecHandle->Open = ttHWH264DecOpen;
	pDecHandle->SetInput = ttHWVideoDecSetInputData;
	pDecHandle->Process = ttHWVideoDecProcess;
	pDecHandle->Close = ttHWVideoDecClose;
	pDecHandle->SetParam = ttHWVideoDecSetParam;
	pDecHandle->GetParam = ttHWVideoDecGetParam;

	return TTKErrNone;
}

/*TTInt32 ttGetMPEG4DecAPI(TTVideoCodecAPI* pDecHandle)
{
	pDecHandle->Open = ttHWMPEG4DecOpen;
	pDecHandle->SetInput = ttHWVideoDecSetInputData;
	pDecHandle->Process = ttHWVideoDecProcess;
	pDecHandle->Close = ttHWVideoDecClose;
	pDecHandle->SetParam = ttHWVideoDecSetParam;
	pDecHandle->GetParam = ttHWVideoDecGetParam;

	return TTKErrNone;
}*/
