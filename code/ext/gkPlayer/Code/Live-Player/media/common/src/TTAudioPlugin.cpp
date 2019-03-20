/**
* File : TTAudioPlugin.cpp  
* Created on : 2014-9-1
* Author : yongping.lin
* Copyright : Copyright (c) 2014 GoKu Software Ltd. All rights reserved.
* Description : TTAudioPlugin实现文件
*/

#include "TTAudioPlugin.h"
#include "TTMediainfoDef.h"
#include "TTDllLoader.h"
#include "GKOsalConfig.h"
#include "TTSysTime.h"
#include "TTLog.h"

#if (defined __TT_OS_ANDROID__) && (defined CPU_ARM)
#include <cpu-features.h>
#endif

TTChar CTTAudioPluginManager::mPluginPath[256] = "";

CTTAudioPluginManager::CTTAudioPluginManager()
:mHandle(NULL)
,mLibHandle(NULL)
,mFormat(0)
{
	memset(&mAudioCodecAPI, 0, sizeof(mAudioCodecAPI));

	mCritical.Create();
	
#ifdef PERFORMANCE_PROFILE
	tAudioDecode = 0;
#endif
}

CTTAudioPluginManager::~CTTAudioPluginManager()
{
	uninitPlugin();

	mCritical.Destroy();

#ifdef PERFORMANCE_PROFILE
	LOGI("\tPERFORMANCE_PROFILE: AudioDecode = %lld\n", tAudioDecode);
#endif
}

TTInt32 CTTAudioPluginManager::initPlugin(TTUint aFormat, void* aInitParam, TTBool /*aDecodeOrEncode = ETTFalse*/)
{
	GKCAutoLock Lock(&mCritical);

	TTInt32 nErr = TTKErrNotSupported;
	if(aFormat == mFormat && mLibHandle != NULL && mHandle != NULL) {
		nErr = setParam(TT_PID_AUDIO_DECODER_INFO, aInitParam);
		return nErr;
	}

	uninitPlugin();

	mFormat = aFormat;

	nErr = LoadLib ();

	if(nErr != TTKErrNone)	
		return nErr;
	
	if(mAudioCodecAPI.Open == NULL)
		return TTKErrNotSupported;

	nErr = mAudioCodecAPI.Open(&mHandle);

	if(mHandle == NULL || nErr != TTKErrNone) 
		return TTKErrNotSupported;

	if(aInitParam != NULL)
		nErr = setParam(TT_PID_AUDIO_DECODER_INFO, aInitParam);

	return nErr;
}

TTInt32 CTTAudioPluginManager::uninitPlugin()
{
	GKCAutoLock Lock(&mCritical);
	if(mLibHandle == NULL || mHandle == NULL || mAudioCodecAPI.Close == NULL)
		return TTKErrNotSupported;

	mAudioCodecAPI.Close(mHandle);
	
	mHandle = NULL;

	DllClose(mLibHandle);

	mLibHandle = NULL;

	memset(&mAudioCodecAPI, 0, sizeof(mAudioCodecAPI));
	
	return TTKErrNone;
}


TTInt32 CTTAudioPluginManager::resetPlugin()
{
	GKCAutoLock Lock(&mCritical);
	if(mHandle == NULL || mAudioCodecAPI.SetParam == NULL)
		return TTKErrNotSupported;

	TTInt32 nFlush = 1;

	return mAudioCodecAPI.SetParam(mHandle, TT_PID_AUDIO_FLUSH, &nFlush);
}

TTInt32 CTTAudioPluginManager::setInput(TTBuffer *InBuffer)
{
	GKCAutoLock Lock(&mCritical);

	if(mHandle == NULL || mAudioCodecAPI.SetInput == NULL)
		return TTKErrNotSupported;

	return mAudioCodecAPI.SetInput(mHandle, InBuffer);
}

TTInt32 CTTAudioPluginManager::process(TTBuffer * OutBuffer, TTAudioFormat* pOutInfo)
{
	GKCAutoLock Lock(&mCritical);
	
	if(mHandle == NULL || mAudioCodecAPI.Process == NULL)
		return TTKErrNotSupported;
	
	return mAudioCodecAPI.Process(mHandle, OutBuffer, pOutInfo);
}

TTInt32 CTTAudioPluginManager::setParam(TTInt32 uParamID, TTPtr pData)
{
	GKCAutoLock Lock(&mCritical);

	if(mHandle == NULL || mAudioCodecAPI.SetParam == NULL)
		return TTKErrNotSupported;

	return mAudioCodecAPI.SetParam(mHandle, uParamID, pData);
}

TTInt32 CTTAudioPluginManager::getParam(TTInt32 uParamID, TTPtr pData)
{
	GKCAutoLock Lock(&mCritical);

	if(mHandle == NULL || mAudioCodecAPI.GetParam == NULL)
		return TTKErrNotSupported;

	return mAudioCodecAPI.GetParam(mHandle, uParamID, pData);
}

void CTTAudioPluginManager::setPluginPath(const TTChar* aPath)
{
	if (aPath != NULL && strlen(aPath) > 0)
	{
		memset(mPluginPath, 0, sizeof(TTChar)*256);
		strcpy(mPluginPath, aPath);
	}
}

TTInt32 CTTAudioPluginManager::LoadLib ()
{
	TTInt			nCPUType = 6;	
	TTChar			DllFile[256];
	TTChar			APIName[128];
	int len = (int)strlen(mPluginPath);

	memset(DllFile, 0, sizeof(TTChar)*256);
	memset(APIName, 0, sizeof(TTChar)*128);

	strcpy(DllFile, mPluginPath); 
	if(len > 0) {
		if(DllFile[len-1] != '\\' && DllFile[len-1] != '/') strcat(DllFile, "/"); 
	}

#ifdef __TT_OS_ANDROID__
	strcat (DllFile, "lib");
#ifdef CPU_ARM
	if (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON){
		nCPUType = 7;
	}
#endif
#endif

	if (mFormat == TTAudioInfo::KTTMediaTypeAudioCodeAAC) {
		strcat (DllFile, "AACDec");
//		if(nCPUType >= 7)
//			strcat (DllFile, "_v7");
		strcat (DllFile, KCodecWildCard);
		strcpy (APIName, "ttGetAACDecAPI");
	}  else {
		return TTKErrNotSupported;
	}

	mLibHandle = DllLoad(DllFile);
	if (mLibHandle == NULL)	{
		LOGI("could not Load library: AudioDecode = %s, APIName %s", DllFile, APIName);
		return TTKErrNotSupported;
	}
	
	__GetAudioDECAPI  pGetAudioDec = (__GetAudioDECAPI)DllSymbol(mLibHandle, APIName);

	if(pGetAudioDec == NULL) {
		LOGI("could not find audio decoder api APIName %s", APIName);
		return TTKErrNotSupported;
	}

	return pGetAudioDec(&mAudioCodecAPI);
}

//end of file