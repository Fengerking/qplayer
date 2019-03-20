/**
* File : TTVideoPlugin.cpp  
* Created on : 2014-9-1
* Author : yongping.lin
* Copyright : Copyright (c) 2014 GoKu Software Ltd. All rights reserved.
* Description : TTVideoPlugin实现文件
*/

#include "TTVideoPlugin.h"
#include "TTMediainfoDef.h"
#include "TTDllLoader.h"
#include "GKOsalConfig.h"
#include "TTSysTime.h"
#include "TTLog.h"

TTChar CTTVideoPluginManager::mVideoPluginPath[256] = "";

CTTVideoPluginManager::CTTVideoPluginManager()
:mHandle(NULL)
,mLibHandle(NULL)
,mFormat(0)
,mCPUType(6)
,mParam(NULL)
,mHwDecoder(0)
{
	memset(&mVideoCodecAPI, 0, sizeof(mVideoCodecAPI));

	mCritical.Create();
}

CTTVideoPluginManager::~CTTVideoPluginManager()
{
	uninitPlugin();

	mCritical.Destroy();

}

TTInt32 CTTVideoPluginManager::initPlugin(TTUint aFormat, void* aInitParam, TTInt aHwDecoder)
{
	GKCAutoLock Lock(&mCritical);

	if((aFormat == 0 || aFormat == mFormat) && mLibHandle != NULL && mHandle != NULL && mHwDecoder == aHwDecoder) {
		//resetPlugin();
		if(aInitParam != NULL)
			mParam = aInitParam;

		setParam(TT_PID_VIDEO_DECODER_INFO, mParam);

		return TTKErrNone;
	}

	uninitPlugin();

	mHwDecoder = aHwDecoder;

	if(aFormat != 0)
		mFormat = aFormat;

	TTInt32 nErr = LoadLib ();

	if(nErr != TTKErrNone)	
		return nErr;

	
	if(mVideoCodecAPI.Open == NULL)
		return TTKErrNotSupported;

	nErr = mVideoCodecAPI.Open(&mHandle);

	if(mHandle == NULL) 
		return TTKErrNotSupported;

	if(aInitParam != NULL)
		mParam = aInitParam;
	
	setParam(TT_PID_VIDEO_DECODER_INFO, mParam);
	
	return nErr;
}

TTInt32 CTTVideoPluginManager::uninitPlugin()
{
	GKCAutoLock Lock(&mCritical);
	if(mLibHandle == NULL || mHandle == NULL || mVideoCodecAPI.Close == NULL)
		return TTKErrNotFound;

	TTInt nStop = 1;
	setParam(TT_PID_VIDEO_STOP, &nStop);

	mVideoCodecAPI.Close(mHandle);
	
	mHandle = NULL;

	DllClose(mLibHandle);

	mLibHandle = NULL;

	memset(&mVideoCodecAPI, 0, sizeof(mVideoCodecAPI));
	
	return TTKErrNone;
}

TTInt32 CTTVideoPluginManager::resetPlugin()
{
	GKCAutoLock Lock(&mCritical);
	if(mHandle == NULL || mVideoCodecAPI.SetParam == NULL)
		return TTKErrNotFound;

	TTInt32 nFlush = 1;

	TTInt nErr = mVideoCodecAPI.SetParam(mHandle, TT_PID_VIDEO_FLUSH, &nFlush);

	return nErr;
}

TTInt32 CTTVideoPluginManager::setInput(TTBuffer *InBuffer)
{
	GKCAutoLock Lock(&mCritical);

	if(mHandle == NULL || mVideoCodecAPI.SetInput == NULL)
		return TTKErrNotFound;

	return mVideoCodecAPI.SetInput(mHandle, InBuffer);
}

TTInt32 CTTVideoPluginManager::process(TTVideoBuffer* OutBuffer, TTVideoFormat* pOutInfo)
{
	GKCAutoLock Lock(&mCritical);
	
	if(mHandle == NULL || mVideoCodecAPI.Process == NULL)
		return TTKErrNotFound;
	
	return mVideoCodecAPI.Process(mHandle, OutBuffer, pOutInfo);
}

TTInt32 CTTVideoPluginManager::setParam(TTInt32 uParamID, TTPtr pData)
{
	GKCAutoLock Lock(&mCritical);

	if(uParamID == TT_PID_VIDEO_CPU_TYPE) {
		mCPUType = *((TTInt*)pData); 
	}

	if(mHandle == NULL || mVideoCodecAPI.SetParam == NULL)
		return TTKErrNotFound;

	return mVideoCodecAPI.SetParam(mHandle, uParamID, pData);
}

TTInt32 CTTVideoPluginManager::getParam(TTInt32 uParamID, TTPtr pData)
{
	GKCAutoLock Lock(&mCritical);

	if(mHandle == NULL || mVideoCodecAPI.GetParam == NULL)
		return TTKErrNotFound;

	return mVideoCodecAPI.GetParam(mHandle, uParamID, pData);
}

void CTTVideoPluginManager::setPluginPath(const TTChar* aPath)
{
	if (aPath != NULL && strlen(aPath) > 0)
	{
		memset(mVideoPluginPath, 0, sizeof(TTChar)*256);
		strcpy(mVideoPluginPath, aPath);
	}
}

TTInt32 CTTVideoPluginManager::LoadLib ()
{
	TTChar			DllFile[256];
	TTChar			APIName[128];
	int len = (int)strlen(mVideoPluginPath);

	memset(DllFile, 0, sizeof(TTChar)*256);
	memset(APIName, 0, sizeof(TTChar)*128);

	strcpy(DllFile, mVideoPluginPath); 
	if(len > 0) {
		if(DllFile[len-1] != '\\' && DllFile[len-1] != '/') strcat(DllFile, "/"); 
	}

#if (defined __TT_OS_ANDROID__)
	strcat (DllFile, "lib");
#endif

	if(mHwDecoder == TT_VIDEODEC_IOMX_ICS) {
		strcat (DllFile, "HWVideoDec40");
	} else if(mHwDecoder == TT_VIDEODEC_IOMX_JB) {
		strcat (DllFile, "HWVideoDec43");
	} else if(mHwDecoder == TT_VIDEODEC_MediaCODEC_JAVA) {
		strcat (DllFile, "MediaCodecJDec");
	} else {
		strcat (DllFile, "H264Dec");
		if(mCPUType >= 7)
			strcat (DllFile, "_v7");
	}

	strcat (DllFile, KCodecWildCard);
	if (mFormat == TTVideoInfo::KTTMediaTypeVideoCodeH264) {
		strcpy (APIName, "ttGetH264DecAPI");
	} else if (mFormat == TTVideoInfo::KTTMediaTypeVideoCodeMPEG4) {
		strcpy (APIName, "ttGetMPEG4DecAPI");
	} else if (mFormat == TTVideoInfo::KTTMediaTypeVideoCodeHEVC) {
		strcpy (APIName, "ttGetHEVCDecAPI");
	}

	mLibHandle = DllLoad(DllFile);
	if (mLibHandle == NULL)	{
		LOGI("could not Load library: VideoDecode = %s, APIName %s", DllFile, APIName);
		return TTKErrNotSupported;
	}
	
	__GetVideoDECAPI  pGetVideoDec = (__GetVideoDECAPI)DllSymbol(mLibHandle, APIName);

	if(pGetVideoDec == NULL) {
		LOGI("could not find video decoder api APIName %s", APIName);
		return TTKErrNotSupported;
	}

	return pGetVideoDec(&mVideoCodecAPI);
}

//end of file