/**
* File : TTVideoDecode.cpp  
* Created on : 2014-9-1
* Author : yongping.lin
* Description : TTVideoDecode实现文件
*/

#include "TTVideoDecode.h"
#include "TTMediainfoDef.h"
#include "GKOsalConfig.h"
#include "TTSysTime.h"
#include "AVCDecoderTypes.h"
#include "TTLog.h"

#ifdef __TT_OS_IOS__
extern TTInt gIos8Above;
#endif

CTTVideoDecode::CTTVideoDecode(CTTSrcDemux*	aSrcMux)
:mSrcMux(aSrcMux)
,mDropFrame(0)
,mVideoCodec(0)
,mCPUType(6)
,mCPUNum(1)
,mOutputNum(6)
,mCurBuffer(NULL)
,mNewCodec(0)
,mEOS(false)
,mStatus(EStatusStoped)
,mPreView(false)
,mStartSeek(false)
,mHwDecoder(0)
,mHwTimeCount(0)
,mDelayTime(0)
,mFlag(0)
,mDeblock(1)
,mLastFailed(0)
{
	mCritical.Create();
	mCriStatus.Create();
	mSemaphore.Create();
	mPluginManager = new CTTVideoPluginManager();

	memset(&mVideoFormat, 0, sizeof(mVideoFormat));
	memset(&mSrcBuffer, 0, sizeof(mSrcBuffer));
	GKASSERT(mPluginManager != NULL);

	mVideoDecObserver.pObserver = videoHWCallBack;
	mVideoDecObserver.pUserData = this;

#ifdef __DUMP_H264__
	DumpFile = fopen("D:\\Test\\Dump.H264", "wb+");
#endif
}

CTTVideoDecode::~CTTVideoDecode()
{
	stop();
	uninitDecode();
	SAFE_DELETE(mPluginManager);
	mSemaphore.Destroy();
	mCriStatus.Destroy();
	mCritical.Destroy();

#ifdef __DUMP_H264__
	if(DumpFile)
		fclose(DumpFile);
#endif
}

TTInt CTTVideoDecode::initDecode(TTVideoInfo* pCurVideoInfo, TTInt aHwDecoder)
{
	GKCAutoLock Lock(&mCritical);
	if(mPluginManager == NULL) 
		return TTKErrNotFound;

	mCriStatus.Lock();
	mStatus = EStatusStarting;
	mCriStatus.UnLock();

	mSemaphore.Reset();

	if(pCurVideoInfo != NULL)
		mVideoCodec = pCurVideoInfo->iMediaTypeVideoCode;

	void* pInitParam = NULL;
	if(pCurVideoInfo != NULL)
		pInitParam = pCurVideoInfo->iDecInfo;

	mHwDecoder = aHwDecoder;

	TTInt32 nErr = mPluginManager->initPlugin(mVideoCodec, pInitParam, aHwDecoder);
	if(nErr != TTKErrNone) 
		return nErr;

#ifdef __DUMP_H264__
	if(pCurVideoInfo) {
		TTAVCDecoderSpecificInfo *pDecInfo = (TTAVCDecoderSpecificInfo *)pCurVideoInfo->iDecInfo;
		if(DumpFile)
		{
			//fwrite(&pDecInfo->iSize, 4, 1, DumpFile);
			fwrite(pDecInfo->iData, pDecInfo->iSize, 1, DumpFile);
		}
	}
#endif

	initConfig();
    
    memset( &mVideoFormat, 0, sizeof(mVideoFormat));
	mPluginManager->getParam(TT_PID_VIDEO_FORMAT, &mVideoFormat);

	if(mVideoFormat.Width == 0 && pCurVideoInfo != NULL)
		mVideoFormat.Width = pCurVideoInfo->iWidth;

	if(mVideoFormat.Height == 0 && pCurVideoInfo != NULL)
		mVideoFormat.Height = pCurVideoInfo->iHeight;

	mPluginManager->setParam(TT_PID_VIDEO_FORMAT, &mVideoFormat);

    mLastFailed = 0;
	mNewCodec = 0;
	mCurBuffer = NULL;
	setEOS(false);

	mCriStatus.Lock();
	mStatus = EStatusPrepared;
	mCriStatus.UnLock();

	return nErr;
}

TTInt CTTVideoDecode::uninitDecode()
{
	GKCAutoLock Lock(&mCritical);
	if(mPluginManager == NULL)
		return TTKErrNotFound;

	mPluginManager->uninitPlugin();
	mNewCodec = 0;
	mCurBuffer = NULL;

	return TTKErrNone;
}

TTInt CTTVideoDecode::start()
{
	mCritical.Lock();
	TTInt nStart = 1;
	TTInt nErr = TTKErrNone;
    
#ifdef __TT_OS_IOS__
    if(gIos8Above > 0)
    {
        if(mPluginManager) {
            nErr = mPluginManager->setParam(TT_PID_VIDEO_START, &nStart);
        }
    }
#else
	if(mPluginManager && mHwDecoder != TT_VIDEODEC_SOFTWARE) {
		nErr = mPluginManager->setParam(TT_PID_VIDEO_START, &nStart);
	}
#endif
    
	mCritical.UnLock();

	GKCAutoLock Lock(&mCriStatus);
	mStatus = EStatusPlaying;
	setEOS(false);
	return nErr;
}

TTInt CTTVideoDecode::stop(TTBool aDecoder)
{
	mCriStatus.Lock();
	mStartSeek = false;
	if(!aDecoder) {
		mStatus = EStatusStoped;
	}
	mCriStatus.UnLock();

	mCritical.Lock();
	TTInt nStop = 1;
	if(mPluginManager)
		mPluginManager->setParam(TT_PID_VIDEO_STOP, &nStop);
	mCritical.UnLock();
	return TTKErrNone;
}

TTInt CTTVideoDecode::pause()
{
	GKCAutoLock Lock(&mCriStatus);
	mStatus = EStatusPaused;
	return TTKErrNone;
}


TTInt CTTVideoDecode::resume()
{
	GKCAutoLock Lock(&mCriStatus);
	mStatus = EStatusPlaying;
	return TTKErrNone;
}

TTInt CTTVideoDecode::flush()
{
	mSemaphore.Signal();
	if(mHwDecoder == TT_VIDEODEC_IOMX_ICS || mHwDecoder == TT_VIDEODEC_IOMX_JB) {
		return TTKErrNone;
	}

	GKCAutoLock Lock(&mCritical);
	if(mPluginManager == NULL)
		return TTKErrNotFound;
	
	mPluginManager->resetPlugin();
	memset(&mSrcBuffer, 0, sizeof(mSrcBuffer));

    mLastFailed = 0;
    mNewCodec = 0;
	mCurBuffer = NULL;

	return TTKErrNone;
}

TTInt CTTVideoDecode::syncPosition(TTUint64 aPosition, TTInt aOption)
{
	mCriStatus.Lock();
	mStartSeek = true;
	mHwTimeCount = 0;
	setEOS(false);
	mCriStatus.UnLock();

	return TTKErrNone;
}


#ifdef __DUMP_YUV__
int OutputOneFrame(FILE* outFile, TTVideoBuffer *par,int width,int height,int frametype)
{
	unsigned char* buf;
	if (outFile) {
		buf  = (unsigned char*)par->Buffer[0];

		for(i=0; i<height; i++) {
			fwrite(buf, 1, width, outFile);
			buf += par->Stride[0];
		}

		width /=2;
		height /=2;
		buf	=  (unsigned char*)par->Buffer[1];
		for(i=0; i<height; i++)	{
			fwrite(buf, 1, width, outFile); 
			buf+=par->Stride[1];
		}	

		buf	= (unsigned char*)par->Buffer[2];
		for(i=0;i<height;i++) {
			fwrite(buf, 1, width, outFile);
			buf+=par->Stride[2];
		}	
		fflush(outFile);
	}
	return 0;
}
#endif

TTInt CTTVideoDecode::getOutputBuffer(TTVideoBuffer* OutBuffer)
{
	GKCAutoLock Lock(&mCritical);
	if(mSrcMux == NULL || OutBuffer == NULL)
		return TTKErrNotFound;

	if(mPluginManager == NULL)
		return TTKErrNotFound;

	if(OutBuffer->nFlag & TT_FLAG_BUFFER_SEEKING) {
		flush();
	}

	if(mHwDecoder == TT_VIDEODEC_IOMX_ICS || mHwDecoder == TT_VIDEODEC_IOMX_JB) {
		return getHWOutputBuffer(OutBuffer);
	}
	
	mCriStatus.Lock();
	if(mStartSeek && (OutBuffer->nFlag & TT_FLAG_BUFFER_SEEKING)) {
		mStartSeek = false;
		mSrcBuffer.nFlag |= TT_FLAG_BUFFER_SEEKING;
	}

	if(mStartSeek || (mStatus != EStatusPlaying && !mPreView)) {
		mCriStatus.UnLock();
		return TTKErrInUse;
	}
	mCriStatus.UnLock();

	TTInt64 nTime = OutBuffer->Time;
	TTVideoFormat	VideoFormat;
	memset(&VideoFormat, 0, sizeof(VideoFormat));
    
    TTInt32 nErr = TTKErrInUse;
    OutBuffer->nFlag = 0;

    if(mLastFailed) {
        nErr = mPluginManager->process(OutBuffer, &VideoFormat);
        mLastFailed = 0;
        if(nErr == TTKErrNone)	{
            if(VideoFormat.Width != mVideoFormat.Width || VideoFormat.Height != mVideoFormat.Height) {
                mVideoFormat.Width = VideoFormat.Width;
                mVideoFormat.Height = VideoFormat.Height;
                return TTKErrFormatChanged;
            }

            if(OutBuffer->Buffer[0] == NULL && mHwDecoder == TT_VIDEODEC_SOFTWARE)
                nErr = TTKErrInUse;

            return nErr;
        }
    }
    
	if(mNewCodec) {
		TTInt64 nStartTime = GetTimeOfDay();
		TTVideoInfo* pCurVideoInfo = (TTVideoInfo*)mSrcBuffer.pData;
		if(pCurVideoInfo == NULL) {
			return TTKErrEof;
		}
		VideoFormat.Width = pCurVideoInfo->iWidth;
		VideoFormat.Height = pCurVideoInfo->iHeight;
		mPluginManager->setParam(TT_PID_VIDEO_FORMAT, &VideoFormat);
		initDecode(pCurVideoInfo, mHwDecoder);
		mNewCodec = 0;
		start();
		TTInt64 nEndTime = GetTimeOfDay();
		LOGI("initDecode and start use the time %lld", nEndTime - nStartTime);
		return TTKErrFormatChanged;
	}

	//if(mCurBuffer && mCurBuffer->nSize > 0)	{
	//	nErr = mPluginManager->setInput(&mSrcBuffer);
	//}
	mCurBuffer = NULL;


	if(getEOS()) {
		return TTKErrEof;
	}

	mSrcBuffer.llTime = nTime;
	nErr = mSrcMux->GetMediaSample(EMediaTypeVideo, &mSrcBuffer);	
	//LOGI("GetMediaSample Video nErr %d, mSrcBuffer.nSize %d, mSrcBuffer.llTime %lld, mSrcBuffer.nFlag %d", nErr, mSrcBuffer.nSize, mSrcBuffer.llTime, mSrcBuffer.nFlag);
	if(nErr != TTKErrNone) {
		if(nErr == TTKErrEof)	{
			TTInt flush = 1;
			nErr = mPluginManager->setParam(TT_PID_VIDEO_FLUSHALL, &flush);
			setEOS(true);
            mLastFailed = 1;
			return TTKErrInUse;
		} else if(nErr == TTKErrNotReady) {
			mSemaphore.Wait(20);
		}
		return nErr;
	}

#ifdef __DUMP_H264__
	if(DumpFile)
	{
		fwrite(mSrcBuffer.pBuffer, mSrcBuffer.nSize, 1, DumpFile);
	}
#endif

	if((mSrcBuffer.nFlag & TT_FLAG_BUFFER_NEW_PROGRAM) || (mSrcBuffer.nFlag & TT_FLAG_BUFFER_NEW_FORMAT)) {
		mNewCodec = 1;
		TTInt flush = 1;
		mPluginManager->setParam(TT_PID_VIDEO_FLUSHALL, &flush);
		mCurBuffer = &mSrcBuffer;
        mLastFailed = 1;
		return TTKErrInUse;
	}

	if(mSrcBuffer.nFlag & TT_FLAG_BUFFER_FLUSH) {
		TTInt flush = 1;
		mPluginManager->setParam(TT_PID_VIDEO_FLUSH, &flush);
	}

#ifdef __TT_OS_IOS__
    if (gIos8Above == 0) {
        if(nTime > mSrcBuffer.llTime + 150)	{
            if(!checkRefFrame(&mSrcBuffer)) {
                return TTKErrTimedOut;
            }
        }
    } else {
        if(nTime > mSrcBuffer.llTime + 150) {
            if(mDropFrame < 5) {
                mSrcBuffer.nFlag |= TT_FLAG_BUFFER_DROP_FRAME;
                mDropFrame++;
            } else {
                mDropFrame = 0;
            }
        }
    }
#else
    if(nTime > mSrcBuffer.llTime + 150)	{
        if(!checkRefFrame(&mSrcBuffer)) {
            return TTKErrTimedOut;
        }
    }
#endif

	if(mDelayTime > 100) {
		if(mDeblock == 1)  {
			mDeblock = 0;
			mPluginManager->setParam(TT_PID_VIDEO_ENDEBLOCK, &mDeblock);
		}
	} else {
		if(mDeblock == 0) {
			mDeblock = 1;
			mPluginManager->setParam(TT_PID_VIDEO_ENDEBLOCK, &mDeblock);
		}
	}

	nErr = mPluginManager->setInput(&mSrcBuffer);
	mCurBuffer = &mSrcBuffer;
	if(nErr != TTKErrNone) {
		if(nErr == TTKErrHardwareNotAvailable) {
			OutBuffer->Time = mSrcBuffer.llTime;
			mCurBuffer = NULL;
		}
		return nErr;
	}

	mCurBuffer = NULL;
	mSrcBuffer.nFlag = 0;

	OutBuffer->nFlag = 0;
	nErr = mPluginManager->process(OutBuffer, &VideoFormat);
	if(nErr == TTKErrNone)	{
		if(VideoFormat.Width != mVideoFormat.Width || VideoFormat.Height != mVideoFormat.Height) {
			mVideoFormat.Width = VideoFormat.Width;
			mVideoFormat.Height = VideoFormat.Height;
			return TTKErrFormatChanged;
		}

		if(OutBuffer->Buffer[0] == NULL && mHwDecoder == TT_VIDEODEC_SOFTWARE)
			nErr = TTKErrInUse;

		mDelayTime = nTime - OutBuffer->Time;
	}
    
    if(nErr != TTKErrNone) {
        mLastFailed = 1;
    }

	return nErr;
}

TTInt CTTVideoDecode::getHWOutputBuffer(TTVideoBuffer* OutBuffer)
{
	TTVideoFormat	VideoFormat;
	memset(&VideoFormat, 0, sizeof(VideoFormat));
	//LOGI("begin to HW output");
	TTInt32 nErr = mPluginManager->process(OutBuffer, &VideoFormat);
	//LOGI("end to HW output, nErr %d", nErr);
	if(nErr == TTKErrNone)	{
		if(VideoFormat.Width != mVideoFormat.Width || VideoFormat.Height != mVideoFormat.Height) {
			mVideoFormat.Width = VideoFormat.Width;
			mVideoFormat.Height = VideoFormat.Height;
			return TTKErrFormatChanged;
		}
	} else {
		if(getEOS()) {
			return TTKErrEof;
		}
	}

	return nErr;
}

TTInt CTTVideoDecode::setParam(TTInt aID, void* pValue)
{
	if(aID == TT_PID_VIDEO_PREVIEW){
		mCriStatus.Lock();
		if(pValue)
			mPreView = *((TTBool*)pValue);
		mCriStatus.UnLock();
		return TTKErrNone;
	}else if(aID == TT_PID_VIDEO_THREAD_NUM){
		if(pValue)
			mCPUNum = *((TTInt*)pValue);
	}else if(aID == TT_PID_VIDEO_CPU_TYPE){
		if(pValue)
			mCPUType = *((TTInt*)pValue);
	} else if(aID == TT_PID_COMMON_DATASOURCE) {
		mCritical.Lock();
		if(pValue)
			mSrcMux = (CTTSrcDemux*)pValue;
		mCritical.UnLock();
		return TTKErrNone;
	}

	GKCAutoLock Lock(&mCritical);

	if(mPluginManager == NULL)
		return TTKErrNotFound;

	return mPluginManager->setParam(aID, pValue);
}

TTInt CTTVideoDecode::getParam(TTInt aID, void* pValue)
{
	if(aID == TT_PID_VIDEO_FORMAT){
		if(pValue)
		{
			((TTVideoFormat *)pValue)->Width = (mVideoFormat.Width + 1) & ~1;
			((TTVideoFormat *)pValue)->Height = (mVideoFormat.Height + 1) & ~1;
			((TTVideoFormat *)pValue)->Type = mVideoFormat.Type;
			((TTVideoFormat *)pValue)->nReserved = mVideoFormat.nReserved;
		}
		return TTKErrNone;
	}

	if(mPluginManager == NULL)
		return TTKErrNotFound;

	return mPluginManager->getParam(aID, pValue);
}

TTInt CTTVideoDecode::initConfig()
{
	mPluginManager->setParam(TT_PID_VIDEO_OUTPUT_NUM, &mOutputNum);
	mPluginManager->setParam(TT_PID_VIDEO_THREAD_NUM, &mCPUNum);
	mPluginManager->setParam(TT_PID_VIDEO_CPU_TYPE, &mCPUType);
	LOGI("CTTVideoDecode::initConfig() mCPUNum %d", mCPUNum);
	mPluginManager->setParam(TT_PID_VIDEO_CALLFUNCTION, &mVideoDecObserver);

	return TTKErrNone;
}

#define XRAW_IS_ANNEXB(p) ( !(*((p)+0)) && !(*((p)+1)) && (*((p)+2)==1))
#define XRAW_IS_ANNEXB2(p) ( !(*((p)+0)) && !(*((p)+1)) && !(*((p)+2))&& (*((p)+3)==1))

bool CTTVideoDecode::checkRefFrame(TTBuffer* pInBuffer)
{
	if(mVideoCodec != TTVideoInfo::KTTMediaTypeVideoCodeH264)
		return true;
	
	TTPBYTE pBuffer = pInBuffer->pBuffer;
	int size = pInBuffer->nSize;
	TTPBYTE buffer = pBuffer;
	if (buffer[2]==0 && buffer[3]==1) {
		buffer+=4;
		size -= 4;
	} else {
		buffer+=3;
		size -= 3;
	}

	TTInt naluType = buffer[0]&0x0f;
	TTInt isRef	 = 1;
	while(naluType!=1 && naluType!=5)//find next NALU
	{
		TTPBYTE p = buffer;  
		TTPBYTE endPos = buffer+size;
		for (; p < endPos; p++) {
			if (XRAW_IS_ANNEXB(p))	{
				size  -= p-buffer;
				buffer = p+3;
				naluType = buffer[0]&0x0f;
				break;
			}

			if (XRAW_IS_ANNEXB2(p))	{
				size  -= p-buffer;
				buffer = p+4;
				naluType = buffer[0]&0x0f;
				break;
			}
		}

		if(p>=endPos)
			return false; 
	}
	
	if(naluType == 5)
		return true;

	if(naluType==1)	{
		isRef = (buffer[0]>>5) & 3;
	}

	return (isRef != 0);
}

TTInt CTTVideoDecode::videoHWCallBack (void* pUserData, TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3)
{
	CTTVideoDecode* pVideoDec = (CTTVideoDecode*)pUserData;

	TTInt nErr = TTKErrNotReady;
	
	if(pVideoDec == NULL)
		return TTKErrArgument;

	nErr = 	pVideoDec->handleHWCallBack(nID, nParam1, nParam2, pParam3);

	return nErr;
}

TTInt CTTVideoDecode::handleHWCallBack (TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3)
{
	TTInt nErr = TTKErrNotReady;
	if(nID == TT_HWDECDEC_CALLBACK_PID_READBUFFER)  {
		TTBuffer* pBuffer = (TTBuffer*)pParam3;

		mCriStatus.Lock();
		if(mStartSeek && nParam1) {
			mStartSeek = false;
			mHwTimeCount = 0;
			mFlag |= TT_FLAG_BUFFER_SEEKING;
		}

		if(mStatus == EStatusStoped) {
			mCriStatus.UnLock();
			return TTKErrEof;
		}
		//LOGI("CTTVideoDecode::getsample mStartSeek %d, mStatus %d", (int)mStartSeek, (int)mStatus);

		if(mStartSeek || (mStatus != EStatusPlaying && !mPreView)) {
			mHwTimeCount++;
			if(mHwTimeCount < 10) {
				mCriStatus.UnLock();
				mSemaphore.Wait(5);
				return TTKErrNotReady;
			}
		}
		mCriStatus.UnLock();

		if(mFlag)
			pBuffer->nFlag = mFlag;		
		nErr = mSrcMux->GetMediaSample(EMediaTypeVideo, pBuffer);
		mFlag = 0;
		if(nErr != TTKErrNone) {
			if(nErr == TTKErrEof) {
				setEOS(true);
			} else {
				mSemaphore.Wait(20);
			}
			return nErr;
		}
		//LOGI("CTTVideoDecode::getsample nErr %d", nErr);
	}
	
	return nErr; 
}

void CTTVideoDecode::setEOS(TTBool bEOS)
{
	GKCAutoLock Lock(&mCriStatus);
	mEOS = bEOS;
}

TTBool CTTVideoDecode::getEOS()
{
	TTBool bEOS;
	GKCAutoLock Lock(&mCriStatus);
	bEOS = mEOS;

	return bEOS;
}
