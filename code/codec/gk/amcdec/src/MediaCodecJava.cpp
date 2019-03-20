#include <sys/system_properties.h>
#include "MediaCodecJava.h"
#include "TTMediainfoDef.h"
#include "TTJniEnvUtil.h"
#include "TTLog.h"

#ifndef LOG_TAG
#define  LOG_TAG "TTMediaCodecJava"
#endif // LOG_TAG

#define	 OMX_COLOR_FormatYUV420SemiPlanar				21
#define  OMX_COLOR_FormatYCbYCr							25	
#define  OMX_COLOR_FormatYCrYCb							26
#define  OMX_COLOR_FormatCbYCrY							27
#define  OMX_COLOR_FormatCrYCbY							28
#define  OMX_COLOR_FormatYUV420PackedSemiPlanar			39
#define  OMX_COLOR_FormatYUV422PackedSemiPlanar			40
#define  OMX_TI_COLOR_FormatYUV420PackedSemiPlanar 		0x7F000100
#define  OMX_QCOM_COLOR_FormatYVU420SemiPlanar  		0x7FA30C00
#define  QOMX_COLOR_FormatYVU420PackedSemiPlanar32m4ka 		0x7FA30C01
#define  QOMX_COLOR_FormatYUV420PackedSemiPlanar16m2ka		0x7FA30C02
#define  QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka	0x7FA30C03
#define  OMX_QCOM_COLOR_FormatYUV420PackedSemiPlanar32m 	0x7FA30C04
#define  OMX_SEC_COLOR_FormatNV12Tiled 				0x7FC00002

#define AVC_MIME               "video/avc"
#define HEVC_MIME              "video/hevc"
#define MPEG4_MIME             "video/mp4v-es"

#define FEATURE_AdaptivePlayback	"adaptive-playback"
#define KEY_MAX_WIDTH				"max-width"
#define KEY_MAX_HEIGHT				"max-height"
#define MAX_ADAPTIVE_PLAYBACK_WIDTH		1920
#define MAX_ADAPTIVE_PLAYBACK_HEIGHT	1080

#define INFO_OUTPUT_BUFFERS_CHANGED -3
#define INFO_OUTPUT_FORMAT_CHANGED  -2
#define INFO_TRY_AGAIN_LATER        -1

int  g_LogOpenFlag = 1;

CMediaCodecJava::CMediaCodecJava(TTUint aCodecType)
:mEOS(false)
,mStarted(false)
,mAllocated(false)
,mNewStart(true)
,mAdaptivePlayback(false)
,mHeadBuffer(NULL)
,mHeadSize(0)
,mHeadConfigBuffer(NULL)
,mHeadConfigSize(0)
,mSpsBuffer(NULL)
,mSpsSize(0)
,mPpsBuffer(NULL)
,mPpsSize(0)
,mVideoIndex(-1)
,mJVM(NULL)
,mSurfaceObj(NULL)
,mMediaCodec(NULL)
,mBufferInfo(NULL)
,mVideoFormatObj(NULL)
,mInputBuffers(NULL)
,mOutputBuffers(NULL)
,mMediaCodecClass(NULL)
,mMediaFormatClass(NULL)
,mBufferInfoClass(NULL) 
,mByteBufferClass(NULL)
,mToString(NULL)
,mCreateByCodecType(NULL)
,mConfigure(NULL)
,mStart(NULL)
,mStop(NULL)
,mFlush(NULL)
,mRelease(NULL)
,mGetOutputFormat(NULL)
,mGetInputBuffers(NULL)
,mGetOutputBuffers(NULL)
,mDequeueInputBuffer(NULL)
,mDequeueOutputBuffer(NULL)
,mQueueInputBuffer(NULL)
,mReleaseOutputBuffer(NULL)
,mCreateVideoFormat(NULL)
,mSetInteger(NULL)
,mSetByteBuffer(NULL)
,mGetInteger(NULL)
,mBufferInfoConstructor(NULL)
,mSizeField(NULL)
,mOffsetField(NULL)
,mPtsField(NULL)
{
	if(aCodecType == 264)	{
		mVideoCodec = TTVideoInfo::KTTMediaTypeVideoCodeH264;
	} else if(aCodecType == 265)	{
		mVideoCodec = TTVideoInfo::KTTMediaTypeVideoCodeHEVC;
	} else if(aCodecType == 4) {
		mVideoCodec = TTVideoInfo::KTTMediaTypeVideoCodeMPEG4;
	}
	memset(&mVideoFormat, 0, sizeof(mVideoFormat));
}

CMediaCodecJava::~CMediaCodecJava()
{
	uninitDecode();

	if(mJVM != NULL) {
		CJniEnvUtil	env(mJVM);
		JNIEnv* pEnv = env.getEnv();
		if(mSurfaceObj) {
			pEnv->DeleteGlobalRef(mSurfaceObj);
			mSurfaceObj = 0;
		}
	}

	if (mHeadBuffer != NULL) {
		free (mHeadBuffer);
		mHeadBuffer = NULL;
	}

	if(mHeadConfigBuffer != NULL) {
		free (mHeadConfigBuffer);
		mHeadConfigBuffer = NULL;
	}

	if (mSpsBuffer != NULL) {
		free (mSpsBuffer);
		mSpsBuffer = NULL;
	}

	if(mPpsBuffer != NULL) {
		free (mPpsBuffer);
		mPpsBuffer = NULL;
	}
}

TTInt CMediaCodecJava::initDecode(void *object)
{
	LOGI("++init MediaCodec Decode mVideoCodec %d", mVideoCodec);
	TTInt nErr = TTKErrNotSupported;

	if(mJVM == NULL)
		return TTKErrNotReady;

	CJniEnvUtil	env(mJVM);
	JNIEnv* pEnv = env.getEnv();

	jclass lclass = pEnv->FindClass("android/media/MediaCodec");
	if(lclass == NULL){
		LOGI("can not find the android/media/MediaCodec class \n");
		if (pEnv->ExceptionOccurred()) {				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}
	mMediaCodecClass = (jclass)pEnv->NewGlobalRef(lclass);
	pEnv->DeleteLocalRef(lclass);

	lclass = pEnv->FindClass("android/media/MediaFormat");
	if(lclass == NULL){
		LOGI("can not find the android/media/MediaFormat class \n");
		if (pEnv->ExceptionOccurred()) {				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}
	mMediaFormatClass = (jclass)pEnv->NewGlobalRef(lclass);
	pEnv->DeleteLocalRef(lclass);

	lclass = pEnv->FindClass("android/media/MediaCodec$BufferInfo");
	if(lclass == NULL){
		LOGI("can not find the android/media/MediaCodec$BufferInfo class \n");
		if (pEnv->ExceptionOccurred()) {				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}
	mBufferInfoClass = (jclass)pEnv->NewGlobalRef(lclass);
	pEnv->DeleteLocalRef(lclass);

	lclass = pEnv->FindClass("java/nio/ByteBuffer");
	if(lclass == NULL){
		LOGI("can not find the java/nio/ByteBuffer class \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}
	mByteBufferClass = (jclass)pEnv->NewGlobalRef(lclass);
	pEnv->DeleteLocalRef(lclass);

	mCreateByCodecType = pEnv->GetStaticMethodID(mMediaCodecClass, "createDecoderByType", "(Ljava/lang/String;)Landroid/media/MediaCodec;");	
	if(mCreateByCodecType == NULL) {
		LOGI("can not find the createDecoderByType fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	mConfigure = pEnv->GetMethodID(mMediaCodecClass, "configure", "(Landroid/media/MediaFormat;Landroid/view/Surface;Landroid/media/MediaCrypto;I)V");	
	if(mConfigure == NULL) {
		LOGI("can not find the configure fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	mCreateVideoFormat = pEnv->GetStaticMethodID(mMediaFormatClass, "createVideoFormat", "(Ljava/lang/String;II)Landroid/media/MediaFormat;");	
	if(mCreateVideoFormat == NULL) {
		LOGI("can not find the createVideoFormat fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	const char* Mime; 
	if(mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeH264) {
		Mime = AVC_MIME;
	} else if(mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeMPEG4) {
		Mime = MPEG4_MIME;
	} else if(mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeHEVC) {
		Mime = HEVC_MIME;
	} else {
		return TTKErrNotReady;
	}

	jstring jstr = pEnv->NewStringUTF(Mime);
	jobject codecObj = NULL;
	codecObj = pEnv->CallStaticObjectMethod(mMediaCodecClass, mCreateByCodecType, jstr);
	if(codecObj == NULL){
		if (pEnv->ExceptionCheck()) {			
			LOGI("Could not create mediacodec for %s", Mime);
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
			pEnv->DeleteLocalRef(jstr);
			return TTKErrNotReady;
		}
	}
	mMediaCodec = pEnv->NewGlobalRef(codecObj);
	pEnv->DeleteLocalRef(codecObj);
	mAllocated = true;
	
	LOGI("create videoFormat for with %d height %d", mVideoFormat.Width, mVideoFormat.Height);

    jobject formatObj = pEnv->CallStaticObjectMethod(mMediaFormatClass, mCreateVideoFormat, jstr, mVideoFormat.Width, mVideoFormat.Height);
	if(formatObj == NULL){
		if (pEnv->ExceptionCheck()) {			
			LOGI("Could not create videoFormat for %s", Mime);
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
			pEnv->DeleteLocalRef(jstr);
			pEnv->DeleteLocalRef(formatObj);
			return TTKErrNotReady;
		}
	}
	mVideoFormatObj = pEnv->NewGlobalRef(formatObj);

	isSupportAdpater(jstr);

	pEnv->DeleteLocalRef(jstr);
	pEnv->DeleteLocalRef(formatObj);

	nErr = setCSData();
	if(nErr != TTKErrNone) {
		return nErr;
	}

	pEnv->CallVoidMethod(mMediaCodec, mConfigure, mVideoFormatObj, mSurfaceObj, NULL, 0);
    if (pEnv->ExceptionOccurred()) {
		pEnv->ExceptionClear();
		pEnv->DeleteLocalRef(formatObj);
		return TTKErrNotReady;
    }

	LOGI("--init MediaCodec Decode OK");
	return TTKErrNone;
}

TTInt CMediaCodecJava::uninitDecode()
{   
	stop();

	if(mJVM == NULL)
		return TTKErrNotReady;

	CJniEnvUtil	env(mJVM);
	JNIEnv* pEnv = env.getEnv();

    if (mMediaCodec) {
        if (mAllocated) {
			if(mRelease == NULL) {
				mRelease = pEnv->GetMethodID(mMediaCodecClass, "release", "()V");
			}
			
			if(mRelease != NULL) {
	            pEnv->CallVoidMethod(mMediaCodec, mRelease);
			    if (pEnv->ExceptionOccurred()) {
					LOGI("Exception in MediaCodec.release");
					pEnv->ExceptionClear();
				}
			}
        }
        pEnv->DeleteGlobalRef(mMediaCodec);
		mMediaCodec = 0;
		mAllocated = false;
	}

	if (mBufferInfo) {
        pEnv->DeleteGlobalRef(mBufferInfo);
		mBufferInfo = 0;
	}

	if(mVideoFormatObj) {
        pEnv->DeleteGlobalRef(mVideoFormatObj);
		mVideoFormatObj = 0;
	}

	if(mMediaCodecClass) {
		pEnv->DeleteGlobalRef(mMediaCodecClass);
		mMediaCodecClass = 0;
	}
	if(mMediaFormatClass){
		pEnv->DeleteGlobalRef(mMediaFormatClass);
		mMediaFormatClass = 0;
	}

	if(mBufferInfoClass){
		pEnv->DeleteGlobalRef(mBufferInfoClass);
		mBufferInfoClass = 0;
	}

	if(mByteBufferClass){
		pEnv->DeleteGlobalRef(mByteBufferClass);
		mByteBufferClass = 0;
	}

	if (mInputBuffers) {
        pEnv->DeleteGlobalRef(mInputBuffers);
		mInputBuffers = 0;
	}

	if (mOutputBuffers) {
        pEnv->DeleteGlobalRef(mOutputBuffers);
		mOutputBuffers = 0;
	}

	mCreateByCodecType = NULL;
	mConfigure = NULL;
	mStart = NULL;
	mStop = NULL;
	mFlush = NULL;
	mRelease = NULL;
	mGetOutputFormat = NULL;
	mGetInputBuffers = NULL;
	mGetOutputBuffers = NULL;
	mDequeueInputBuffer = NULL;
	mDequeueOutputBuffer = NULL;
	mQueueInputBuffer = NULL;
	mReleaseOutputBuffer = NULL;
	mCreateVideoFormat = NULL;
	mSetInteger = NULL;
	mSetByteBuffer = NULL;
	mGetInteger = NULL;
	mBufferInfoConstructor = NULL;
	mSizeField = NULL;
	mOffsetField = NULL;
	mPtsField = NULL;

	return TTKErrNone;
}

TTInt CMediaCodecJava::start()
{
	if(mStarted) {
		return TTKErrNone;
	}

	if(mJVM == NULL || mMediaCodec == NULL)
		return TTKErrNotReady;

	CJniEnvUtil	env(mJVM);
	JNIEnv* pEnv = env.getEnv();

	if(mStart == NULL) {
		mStart = pEnv->GetMethodID(mMediaCodecClass, "start", "()V"); 
		if(mStart == NULL)
			return TTKErrNotReady;
	}

	pEnv->CallVoidMethod(mMediaCodec, mStart);
	LOGI("MediaCodec.start");
	if (pEnv->ExceptionOccurred()) {
		LOGI("Exception in MediaCodec.start");
		pEnv->ExceptionClear();
		return TTKErrNotReady;
	}

	mStarted = true;
	mNewStart = true;

	return TTKErrNone;
}

TTInt CMediaCodecJava::stop()
{
	if(mJVM == NULL)
		return TTKErrNotReady;

	CJniEnvUtil	env(mJVM);
	JNIEnv* pEnv = env.getEnv();

	if(mStarted && mMediaCodec != NULL) {
		if(mVideoIndex >= 0) {
			renderOutputBuffer(NULL, false);
		}

		if(mStop == NULL) {
			mStop = pEnv->GetMethodID(mMediaCodecClass, "stop", "()V"); 
			if(mStop == NULL)
				return TTKErrNotReady;
		}

		pEnv->CallVoidMethod(mMediaCodec, mStop);
		if (pEnv->ExceptionOccurred()) {
			LOGI("Exception in MediaCodec.stop");
			pEnv->ExceptionClear();
		}
	}

	if (mInputBuffers) {
        pEnv->DeleteGlobalRef(mInputBuffers);
		mInputBuffers = 0;
	}
	if (mOutputBuffers) {
        pEnv->DeleteGlobalRef(mOutputBuffers);
		mOutputBuffers = 0;
	}

	mStarted = false;	
	return TTKErrNone;
}

TTInt CMediaCodecJava::setInputBuffer(TTBuffer *InBuffer)
{
	//LOGI("++++MediaCodec.setInputBuffer");
	TTInt nErr = TTKErrNotReady;
	if(mJVM == NULL || mMediaCodec == NULL || mStarted == false)
		return nErr;

	if(mDequeueInputBuffer == NULL || mQueueInputBuffer == NULL || mGetOutputFormat == NULL) {
		nErr = updateMCJFunc();
		if(nErr != TTKErrNone)
			return nErr;
	}

	if(mInputBuffers == NULL) {
		nErr = updateBuffers();
		if(nErr != TTKErrNone)
			return nErr;
	}

	//LOGI("setInputBuffer TimeStamp nTimeUs %lld, nFlag %d", InBuffer->llTime, InBuffer->nFlag);
	if(mNewStart && !(InBuffer->nFlag&TT_FLAG_BUFFER_KEYFRAME)) {
		return TTKErrNone;
	}

	//if(mNewStart) {
	//	nErr = setConfigData();
	//	if(nErr != TTKErrNone)
	//		return nErr;
	//}

	mNewStart = false;

	CJniEnvUtil	env(mJVM);
	JNIEnv* pEnv = env.getEnv();

	jlong nTimeUs = 10000;
	int index = pEnv->CallIntMethod(mMediaCodec, mDequeueInputBuffer, nTimeUs);

	//LOGI("MediaCodec.setInputBuffer index %d", index);

	if (pEnv->ExceptionOccurred()) {
		LOGI("Exception in MediaCodec.dequeueInputBuffer");
		pEnv->ExceptionClear();
		return TTKErrNotReady;
	}
	if(index >= 0) {		
		jobject buf = pEnv->GetObjectArrayElement(mInputBuffers, index);
		if(buf == 0) {
			LOGI("MediaCodec index buf is null");
			return TTKErrNotReady;
		}

        jsize size = pEnv->GetDirectBufferCapacity(buf);
        unsigned char *bufptr = ( unsigned char *)pEnv->GetDirectBufferAddress(buf);
		if(size < 0 || bufptr == NULL || InBuffer->nSize > size) {
			LOGI("MediaCodec index buf size %d, InBuffer->nSize %d, bufptr %x", size, InBuffer->nSize, (TTInt)bufptr);
			return TTKErrNotReady;
		}

		memcpy(bufptr, InBuffer->pBuffer, InBuffer->nSize);
		TTInt nSize = InBuffer->nSize;
		nTimeUs = InBuffer->llTime*1000;

		//LOGI("MediaCodec.dequeueInputBuffer TimeStamp nTimeUs %lld, nFlag %d", nTimeUs, InBuffer->nFlag);

		pEnv->CallVoidMethod(mMediaCodec, mQueueInputBuffer, index, 0, nSize, nTimeUs, 0);
		if (pEnv->ExceptionOccurred()) {
			LOGI("Exception in MediaCodec.dequeueInputBuffer");
			pEnv->ExceptionClear();
			pEnv->DeleteLocalRef(buf);
			return TTKErrNotReady;
		}        
		pEnv->DeleteLocalRef(buf);
	} else {
		return TTKErrNotReady;
	}

	//LOGI("----MediaCodec.setInputBuffer OK");
	return TTKErrNone;
}

TTInt CMediaCodecJava::setConfigData()
{
	//LOGI("+++MediaCodec.setConfigData");
	if(!mNewStart || mHeadSize == 0)
		return TTKErrNone;

	CJniEnvUtil	env(mJVM);
	JNIEnv* pEnv = env.getEnv();

	jlong nTimeUs = 10000;
	int index = pEnv->CallIntMethod(mMediaCodec, mDequeueInputBuffer, nTimeUs);
	if (pEnv->ExceptionOccurred()) {
		LOGI("Exception in MediaCodec.dequeueInputBuffer");
		pEnv->ExceptionClear();
		return TTKErrNotReady;
	}

	//LOGI("MediaCodec.setConfigData index %d", index);
	if(index >= 0) {		
		jobject buf = pEnv->GetObjectArrayElement(mInputBuffers, index);
		if(buf == 0) {
			LOGI("MediaCodec index buf is null");
			return TTKErrNotReady;
		}

        jsize size = pEnv->GetDirectBufferCapacity(buf);
        unsigned char *bufptr = ( unsigned char *)pEnv->GetDirectBufferAddress(buf);
		if(size < 0 || bufptr == NULL || size < mHeadSize) {
			LOGI("MediaCodec index buf size %d, header size %d, bufptr %x", size, mHeadSize, (TTInt)bufptr);
			return TTKErrNotReady;
		}

		TTInt nSize = 0;
		memcpy(bufptr, mHeadBuffer, mHeadSize);
		nSize = mHeadSize;
		pEnv->CallVoidMethod(mMediaCodec, mQueueInputBuffer, index, 0, nSize, 0, 2);
		if (pEnv->ExceptionOccurred()) {
			LOGI("Exception in MediaCodec.dequeueInputBuffer");
			pEnv->ExceptionClear();
			pEnv->DeleteLocalRef(buf);
			return TTKErrNotReady;
		} 
		pEnv->DeleteLocalRef(buf);
		mNewStart = false;
	} else {
		return TTKErrNotReady;
	}

	//LOGI("---MediaCodec.setConfigData");

	return TTKErrNone;
}

TTInt CMediaCodecJava::getOutputBuffer(TTVideoBuffer* DstBuffer, TTVideoFormat* pOutInfo)
{
	//LOGI("++++MediaCodec.getOutputBuffer");
	TTInt nErr = TTKErrNotReady;
	if(mJVM == NULL || mMediaCodec == NULL || mStarted == false || mNewStart)
		return nErr;

	if(mDequeueOutputBuffer == NULL) {
		nErr = updateMCJFunc();
		if(nErr != TTKErrNone)
			return nErr;
	}

	if(mOutputBuffers == NULL) {
		nErr = updateBuffers();
		if(nErr != TTKErrNone)
			return nErr;
	}

	if(mVideoIndex >= 0) {
		renderOutputBuffer(NULL, false);
	}

	CJniEnvUtil	env(mJVM);
	JNIEnv* pEnv = env.getEnv();

	jlong nTimeUs = 10000;
	int index = pEnv->CallIntMethod(mMediaCodec, mDequeueOutputBuffer, mBufferInfo, nTimeUs);

	//LOGI("MediaCodec.getOutputBuffer index %d", index);
	if (pEnv->ExceptionOccurred()) {
		LOGI("Exception in MediaCodec.dequeueOutputBuffer");
		pEnv->ExceptionClear();
		return TTKErrNotReady;
	}
	if(index >= 0) {
		mPtsField	 = pEnv->GetFieldID(mBufferInfoClass, "presentationTimeUs", "J");
		mSizeField	 = pEnv->GetFieldID(mBufferInfoClass, "size", "I");
		mOffsetField = pEnv->GetFieldID(mBufferInfoClass, "offset", "I");

		TTInt nSize = pEnv->GetIntField(mBufferInfo, mSizeField);
		//if(nSize == 0)
		//	return TTKErrNotReady;

		nTimeUs = pEnv->GetLongField(mBufferInfo, mPtsField);
		//LOGI("MediaCodec.mDequeueOutputBuffer TimeStamp nTimeUs %lld, size %d", nTimeUs, nSize);
		DstBuffer->Time = nTimeUs/1000;
		DstBuffer->lReserve = index;

		mVideoIndex = index;
		if(pOutInfo) {
			pOutInfo->Width  = mVideoFormat.Width;
			pOutInfo->Height = mVideoFormat.Height;
			pOutInfo->Type = mVideoFormat.Type;
		}
	} else if(index == INFO_OUTPUT_BUFFERS_CHANGED){
		if (mOutputBuffers) {
		    pEnv->DeleteGlobalRef(mOutputBuffers);
			mOutputBuffers = 0;
		}

		mGetOutputBuffers = pEnv->GetMethodID(mMediaCodecClass, "getOutputBuffers", "()[Ljava/nio/ByteBuffer;");	
		if(mGetOutputBuffers == NULL) {
			LOGI("can not find the getOutputBuffers fucntion \n");
			if (pEnv->ExceptionOccurred()) 	{				
				pEnv->ExceptionDescribe();
				pEnv->ExceptionClear();
			}
			return TTKErrNotReady;
		}
	
		jobjectArray output_buffers = (jobjectArray)pEnv->CallObjectMethod(mMediaCodec, mGetOutputBuffers);   
		mOutputBuffers = (jobjectArray)pEnv->NewGlobalRef(output_buffers);
		pEnv->DeleteLocalRef(output_buffers);
		return TTKErrNotReady;
	} else if(index == INFO_OUTPUT_FORMAT_CHANGED){
		jobject format = pEnv->CallObjectMethod(mMediaCodec, mGetOutputFormat);
		if (pEnv->ExceptionOccurred()) {
			LOGI("Exception in MediaCodec.getOutputFormat (GetOutput)");
			pEnv->ExceptionClear();
			return TTKErrNotReady;
		}

		jstring jstr = pEnv->NewStringUTF("width");
		mVideoFormat.Width = pEnv->CallIntMethod(format, mGetInteger, jstr);
		pEnv->DeleteLocalRef(jstr);

		jstr = pEnv->NewStringUTF("height");
		mVideoFormat.Height = pEnv->CallIntMethod(format, mGetInteger, jstr);
		pEnv->DeleteLocalRef(jstr);

		jstr = pEnv->NewStringUTF("color-format");
		TTUint ColorFormat = pEnv->CallIntMethod(format, mGetInteger, jstr);
		pEnv->DeleteLocalRef(jstr);

		if (ColorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar ||
			ColorFormat == QOMX_COLOR_FormatYUV420PackedSemiPlanar16m2ka ||
			ColorFormat == QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka ||
			ColorFormat == OMX_QCOM_COLOR_FormatYUV420PackedSemiPlanar32m ||
			ColorFormat == OMX_SEC_COLOR_FormatNV12Tiled ||
			ColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
			mVideoFormat.Type = TT_COLOR_YUV_NV12;
		else if(ColorFormat == OMX_QCOM_COLOR_FormatYVU420SemiPlanar ||
			ColorFormat ==	QOMX_COLOR_FormatYVU420PackedSemiPlanar32m4ka)
			mVideoFormat.Type = TT_COLOR_YUV_NV21;
		else if (ColorFormat == OMX_COLOR_FormatYCbYCr)
			mVideoFormat.Type = TT_COLOR_YUV_YUYV422;
		else if (ColorFormat == OMX_COLOR_FormatCbYCrY)
			mVideoFormat.Type = TT_COLOR_YUV_UYVY422;
		else 
			mVideoFormat.Type = TT_COLOR_YUV_PLANAR420;

		LOGI("MediaCodec.getOutputBuffer format changed ColorFormat %d, Width %d Height %d", ColorFormat, mVideoFormat.Width, mVideoFormat.Height);

		return TTKErrFormatChanged;
	} else if(index == INFO_TRY_AGAIN_LATER){
		return TTKErrNotReady;
	} else {
		return TTKErrNotReady;
	}

	//LOGI("----MediaCodec.getOutputBuffer OK");
	return TTKErrNone;
}

TTInt CMediaCodecJava::setParam(TTInt aID, void* pValue)
{
	if(aID == TT_PID_VIDEO_FORMAT){
		if(pValue)
			memcpy(&mVideoFormat, pValue, sizeof(mVideoFormat));
		return TTKErrNone;
	} else if(aID == TT_PID_VIDEO_DECODER_INFO)	{
		if(mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeH264 || mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeHEVC) {
			TTAVCDecoderSpecificInfo * pBuffer = (TTAVCDecoderSpecificInfo *)pValue;
			if(pValue == NULL)
				return TTKErrArgument;
			if(pBuffer->iConfigSize > 0) {
				if (mHeadConfigBuffer != NULL)
					free (mHeadConfigBuffer);
				mHeadConfigSize = pBuffer->iConfigSize;
				mHeadConfigBuffer = (unsigned char *)malloc (mHeadConfigSize + 8);
				memcpy (mHeadConfigBuffer, pBuffer->iConfigData, pBuffer->iConfigSize);
			} else {
				if (mHeadConfigBuffer != NULL)
					free (mHeadConfigBuffer);
				mHeadConfigBuffer = NULL;	
				mHeadConfigSize = 0;
			}

			if(pBuffer->iSize > 0) {
				if (mHeadBuffer != NULL)
					free (mHeadBuffer);
				mHeadSize = pBuffer->iSize;
				mHeadBuffer = (unsigned char *)malloc (mHeadSize + 8);
				memcpy (mHeadBuffer, pBuffer->iData, pBuffer->iSize);
			} else {
				if (mHeadBuffer != NULL)
					free (mHeadBuffer);
				mHeadBuffer = NULL;	
				mHeadSize = 0;
			}

			if(pBuffer->iSpsSize > 0) {
				if (mSpsBuffer != NULL)
					free (mSpsBuffer);
				mSpsSize = pBuffer->iSpsSize;
				mSpsBuffer = (unsigned char *)malloc (mSpsSize + 8);
				memcpy (mSpsBuffer, pBuffer->iSpsData, pBuffer->iSpsSize);
			} else {
				if (mSpsBuffer != NULL)
					free (mSpsBuffer);
				mSpsBuffer = NULL;	
				mSpsSize = 0;
			}

			if(pBuffer->iPpsSize > 0) {
				if (mPpsBuffer != NULL)
					free (mPpsBuffer);
				mPpsSize = pBuffer->iPpsSize;
				mPpsBuffer = (unsigned char *)malloc (mPpsSize + 8);
				memcpy (mPpsBuffer, pBuffer->iPpsData, pBuffer->iPpsSize);
			} else {
				if (mPpsBuffer != NULL)
					free (mPpsBuffer);
				mPpsBuffer = NULL;	
				mPpsSize = 0;
			}
			LOGI("set head data, mHeadSize %d, mHeadSize %d", mHeadSize, mHeadSize);
		} if(mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeMPEG4) {
			TTMP4DecoderSpecificInfo *pBuffer = (TTMP4DecoderSpecificInfo *)pValue;
			if(pValue == NULL)
				return TTKErrArgument;

			if (mHeadBuffer != NULL)
				free (mHeadBuffer);
			mHeadSize = pBuffer->iSize;
			mHeadBuffer = (unsigned char *)malloc (mHeadSize + 8);
			memcpy (mHeadBuffer, pBuffer->iData, pBuffer->iSize);
		}
		mNewStart = true;

		if(mStarted && mJVM != NULL) {
			if(!mAdaptivePlayback || mVideoFormat.Width > MAX_ADAPTIVE_PLAYBACK_WIDTH || mVideoFormat.Height > MAX_ADAPTIVE_PLAYBACK_HEIGHT) {
				uninitDecode();
				initDecode(&mSurfaceObj);
				mStarted = false;
			}
		}

		return TTKErrNone;
	}  else if(aID == TT_PID_COMMON_JAVAVM){
		mJVM = (JavaVM*)pValue;
		LOGI("set Java vm mJVM %d", (TTInt)mJVM);
		return TTKErrNone;	
	} if(aID == TT_PID_VIDEO_FLUSH){ 
		if(mJVM != NULL && mMediaCodec) {
			if(mVideoIndex >= 0) {
				renderOutputBuffer(NULL, false);
			}

			if(mNewStart)
				return TTKErrNone;

			CJniEnvUtil	env(mJVM);
			if(mFlush == NULL) {
				mFlush = env.getEnv()->GetMethodID(mMediaCodecClass, "flush", "()V"); 
				if(mFlush == NULL)
					return TTKErrNotReady;
			}
			LOGI("call flush for mediacodec");
			env.getEnv()->CallVoidMethod(mMediaCodec, mFlush);

			mNewStart = true;
		}
		return TTKErrNone;	
	} else if(aID == TT_PID_COMMON_SURFACEOBJ) {
		if(pValue == NULL) {
			uninitDecode();
			return TTKErrNotReady;
		}

		jobject LocalSur = *((jobject *)pValue);
		//LOGI("set surface object %x, LocalSur %x", (TTInt)pValue, LocalSur);
	
		if(mJVM != NULL && pValue != NULL) {			
			uninitDecode();
			CJniEnvUtil	env(mJVM);

			if(mSurfaceObj) {
				env.getEnv()->DeleteGlobalRef(mSurfaceObj);
				mSurfaceObj = 0;
			}

			mSurfaceObj = env.getEnv()->NewGlobalRef(*((jobject *)pValue));
			//LOGI("set surface object %x, LocalSur %x", (TTInt)pValue, LocalSur);
			TTInt nErr = initDecode(pValue);
			return nErr;
		}  else {
			return TTKErrNotReady;
		}
	}

	return TTKErrNotFound;
}

TTInt CMediaCodecJava::getParam(TTInt aID, void* pValue)
{
	if(aID == TT_PID_VIDEO_FORMAT){
		if(pValue)
			memcpy(pValue, &mVideoFormat, sizeof(mVideoFormat));
		return TTKErrNone;
	}
	
	return TTKErrNotFound;
}

TTInt CMediaCodecJava::renderOutputBuffer(TTVideoBuffer* DstBuffer, TTBool bRender)
{
	if(mStarted == false || mVideoIndex == -1 || mJVM == NULL || mMediaCodec == NULL)
		return TTKErrNotReady;

	if(DstBuffer && DstBuffer->lReserve != mVideoIndex) {
		return TTKErrNotReady;
	}

	CJniEnvUtil	env(mJVM);
	JNIEnv* pEnv = env.getEnv();

	//LOGI("MediaCodec.renderOutputBuffer mVideoIndex %d", mVideoIndex);

	jboolean brender = bRender;

	if(mReleaseOutputBuffer == NULL) {
		mReleaseOutputBuffer = pEnv->GetMethodID(mMediaCodecClass, "releaseOutputBuffer", "(IZ)V");
		if(mReleaseOutputBuffer == NULL) {
			LOGI("can not find the releaseOutputBuffer fucntion \n");
			if (pEnv->ExceptionOccurred()) 	{				
				pEnv->ExceptionDescribe();
				pEnv->ExceptionClear();
			}
			LOGI("can not find the releaseOutputBuffer fucntion \n");
			return TTKErrNotReady;
		}
	}

	//LOGI("MediaCodec.renderOutputBuffer mVideoIndex %d brender %d", mVideoIndex, brender);
	
	if(mReleaseOutputBuffer != NULL) {
		pEnv->CallVoidMethod(mMediaCodec, mReleaseOutputBuffer, mVideoIndex, brender);
	}

	mVideoIndex = -1;

	//LOGI("----MediaCodec.renderOutputBuffer");
	return TTKErrNone;	
}

TTInt CMediaCodecJava::updateMCJFunc()
{
	if(mStarted == false  || mJVM == NULL || mMediaCodecClass == NULL || mBufferInfoClass == NULL)
		return TTKErrNotReady;

	CJniEnvUtil	env(mJVM);
	JNIEnv* pEnv = env.getEnv();

	//LOGI("MediaCodec.updateMCJFunc");

	mGetOutputFormat = pEnv->GetMethodID(mMediaCodecClass, "getOutputFormat", "()Landroid/media/MediaFormat;");
	if(mGetOutputFormat == NULL) {
		LOGI("can not find the getOutputFormat fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	mDequeueInputBuffer = pEnv->GetMethodID(mMediaCodecClass, "dequeueInputBuffer", "(J)I");
	if(mDequeueInputBuffer == NULL) {
		LOGI("can not find the dequeueInputBuffer fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	mDequeueOutputBuffer = pEnv->GetMethodID(mMediaCodecClass, "dequeueOutputBuffer", "(Landroid/media/MediaCodec$BufferInfo;J)I");
	if(mDequeueOutputBuffer == NULL) {
		LOGI("can not find the dequeueOutputBuffer fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	mQueueInputBuffer = pEnv->GetMethodID(mMediaCodecClass, "queueInputBuffer", "(IIIJI)V");
	if(mQueueInputBuffer == NULL) {
		LOGI("can not find the queueInputBuffer fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	mBufferInfoConstructor = pEnv->GetMethodID(mBufferInfoClass, "<init>", "()V");
	if(mBufferInfoConstructor == NULL) {
		LOGI("can not find the bufferinfo construct fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	mSetInteger = pEnv->GetMethodID(mMediaFormatClass, "setInteger", "(Ljava/lang/String;I)V");
	if(mSetInteger == NULL) {
		LOGI("can not find the setInteger fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
	}

	mGetInteger = pEnv->GetMethodID(mMediaFormatClass, "getInteger", "(Ljava/lang/String;)I");
	if(mGetInteger == NULL) {
		LOGI("can not find the getInteger fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	if (mBufferInfo) {
        pEnv->DeleteGlobalRef(mBufferInfo);
		mBufferInfo = 0;
	}

	jobject buffer_info = pEnv->NewObject(mBufferInfoClass, mBufferInfoConstructor);
	mBufferInfo = pEnv->NewGlobalRef(buffer_info);
	pEnv->DeleteLocalRef(buffer_info);

	return TTKErrNone;
}

TTInt CMediaCodecJava::updateBuffers()
{
	if(mStarted == false  || mJVM == NULL || mMediaCodec == NULL)
		return TTKErrNotReady;

	//LOGI("MediaCodec.updateBuffers");

	CJniEnvUtil	env(mJVM);
	JNIEnv* pEnv = env.getEnv();

	if (mInputBuffers) {
        pEnv->DeleteGlobalRef(mInputBuffers);
		mInputBuffers = 0;
	}
	if (mOutputBuffers) {
        pEnv->DeleteGlobalRef(mOutputBuffers);
		mOutputBuffers = 0;
	}

	mGetInputBuffers = pEnv->GetMethodID(mMediaCodecClass, "getInputBuffers", "()[Ljava/nio/ByteBuffer;");	
	if(mGetInputBuffers == NULL) {
		LOGI("can not find the getInputBuffers fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	mGetOutputBuffers = pEnv->GetMethodID(mMediaCodecClass, "getOutputBuffers", "()[Ljava/nio/ByteBuffer;");	
	if(mGetOutputBuffers == NULL) {
		LOGI("can not find the getOutputBuffers fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}
	
	jobjectArray input_buffers = (jobjectArray)pEnv->CallObjectMethod(mMediaCodec, mGetInputBuffers);
	jobjectArray output_buffers = (jobjectArray)pEnv->CallObjectMethod(mMediaCodec, mGetOutputBuffers);   
	mInputBuffers = (jobjectArray)pEnv->NewGlobalRef(input_buffers);
	mOutputBuffers = (jobjectArray)pEnv->NewGlobalRef(output_buffers);
	pEnv->DeleteLocalRef(input_buffers);
	pEnv->DeleteLocalRef(output_buffers);
	return TTKErrNone;
}

TTInt CMediaCodecJava::setCSData()
{
	TTInt nErr = TTKErrNotSupported;	
	if(mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeMPEG4) {
		nErr = setCSDataJava(mHeadBuffer, mHeadSize, 0);
		return nErr;
	} else if(mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeHEVC) {
		nErr = setCSDataJava(mHeadBuffer, mHeadSize, 0);
		return nErr;
	}

	if(mPpsSize > 0 && mSpsSize > 0) {
		nErr = setCSDataJava(mSpsBuffer, mSpsSize, 0);
		if(nErr != TTKErrNone) {
			return TTKErrNotSupported;
		}

		nErr = setCSDataJava(mPpsBuffer, mPpsSize, 1);
		return nErr;	
	}

	if(mHeadConfigSize == 0)
		return TTKErrNone;

	unsigned char *pInBuffer = mHeadConfigBuffer;
	TTInt nInSzie = mHeadConfigSize;
	unsigned char *pOutBuffer = new unsigned char[mHeadConfigSize + 32];
	TTInt nIndex = 0;

	TTUint8 configurationVersion = pInBuffer[0];
	TTUint8 AVCProfileIndication = pInBuffer[1];
	TTUint8 profile_compatibility = pInBuffer[2];
	TTUint8 AVCLevelIndication  = pInBuffer[3];

	TTInt nNALLengthSize =  (pInBuffer[4]&0x03)+1;
	TTUint32 nNalWord = 0x01000000;
	if (nNALLengthSize == 3)
		nNalWord = 0X010000;

	TTInt nNalLen = nNALLengthSize;
	if (nNALLengthSize < 3)	{
		nNalLen = 4;
	}

	TTUint32 HeadSize = 0;
	TTInt i = 0;

	TTInt nSPSNum = pInBuffer[5]&0x1f;
	TTPBYTE pBuffer = pInBuffer + 6;

	memset(pOutBuffer, 0, mHeadConfigSize + 32);
	for (i = 0; i< nSPSNum; i++)
	{
		TTUint32 nSPSLength = (pBuffer[0]<<8)| pBuffer[1];
		pBuffer += 2;

		memcpy (pOutBuffer + HeadSize, &nNalWord, nNalLen);
		HeadSize += nNalLen;

		if(nSPSLength > (nInSzie - (pBuffer - pInBuffer))){
			delete pOutBuffer;
			return TTKErrNotSupported;
		}

		memcpy (pOutBuffer + HeadSize, pBuffer, nSPSLength);

		nIndex++;

		HeadSize += nSPSLength;
		pBuffer += nSPSLength;
	}
	
	//LOGI("set SPS buffer HeadSize %d", HeadSize);
	nErr = setCSDataJava(pOutBuffer, HeadSize, 0);
	if(nErr != TTKErrNone) {
		delete pOutBuffer;
		return TTKErrNotSupported;
	}
	
	memset(pOutBuffer, 0, mHeadConfigSize + 32);
	HeadSize = 0;
	TTInt nPPSNum = *pBuffer++;
	for (i=0; i< nPPSNum; i++)
	{
		TTUint32 nPPSLength = (pBuffer[0]<<8) | pBuffer[1];
		pBuffer += 2;

		memcpy (pOutBuffer + HeadSize, &nNalWord, nNalLen);
		HeadSize += nNalLen;
		
		if(nPPSLength > (nInSzie - (pBuffer - pInBuffer))){
			delete pOutBuffer;
			return TTKErrNotSupported;
		}

		memcpy (pOutBuffer + HeadSize, pBuffer, nPPSLength);

		HeadSize += nPPSLength;
		pBuffer += nPPSLength;
	}

	nErr = setCSDataJava(pOutBuffer, HeadSize, 1);
	//LOGI("set PPS buffer HeadSize %d", HeadSize);

	delete pOutBuffer;
	return nErr;	
}

TTInt CMediaCodecJava::setCSDataJava(unsigned char *pBuf, int nLen, int index)
{
	CJniEnvUtil	env(mJVM);
	JNIEnv* pEnv = env.getEnv();

	if(nLen == 0) {
		return TTKErrNone;
	}

	jmethodID	fAllocDirect = pEnv->GetStaticMethodID(mByteBufferClass, "allocateDirect", "(I)Ljava/nio/ByteBuffer;");	
	if(fAllocDirect == NULL) {
		LOGI("can not find the allocateDirect fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	mSetByteBuffer = pEnv->GetMethodID(mMediaFormatClass, "setByteBuffer", "(Ljava/lang/String;Ljava/nio/ByteBuffer;)V");
	if(mSetByteBuffer == NULL) {
		LOGI("can not find the setByteBuffer fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	jobject bytebuf = pEnv->CallStaticObjectMethod(mByteBufferClass, fAllocDirect, nLen);
	if(bytebuf == 0) {
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady; 
	}
	unsigned char *ptr = (unsigned char *)pEnv->GetDirectBufferAddress(bytebuf);
	memcpy(ptr, pBuf, nLen);
	jstring jstr = NULL;
	if(index == 0) {
		jstr = pEnv->NewStringUTF("csd-0");
	} else if(index == 1){
		jstr = pEnv->NewStringUTF("csd-1");
	}

	//LOGI("set PPS or SPS buffer nLen %d", nLen);
	pEnv->CallVoidMethod(mVideoFormatObj, mSetByteBuffer, jstr, bytebuf);
	pEnv->DeleteLocalRef(bytebuf);
	pEnv->DeleteLocalRef(jstr);

	return TTKErrNone;	
}

TTInt CMediaCodecJava::isSupportAdpater(jstring jstr)
{
	char szProp[64];
	memset (szProp, 0, 64);
	__system_property_get ("ro.build.version.release", szProp);
	if(!(strstr (szProp, "4.4") == szProp || strstr (szProp, "5.") == szProp || strstr (szProp, "6.") == szProp  || strstr (szProp, "7.") == szProp)) {
		return TTKErrNotReady;
	}

	LOGI("szProp %s", szProp);

	CJniEnvUtil	env(mJVM);
	JNIEnv* pEnv = env.getEnv();
	jmethodID mGetCodecInfo = pEnv->GetMethodID(mMediaCodecClass, "getCodecInfo", "()Landroid/media/MediaCodecInfo;");
	if(mGetCodecInfo == NULL) {
		LOGI("can not find the getCodecInfo fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	jclass linfoclass = pEnv->FindClass("android/media/MediaCodecInfo");
	if(linfoclass == NULL){
		LOGI("can not find the android/media/MediaCodecInfo class \n");
		if (pEnv->ExceptionOccurred()) {				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}		

	jmethodID mGetCapabilitiesForType = pEnv->GetMethodID(linfoclass, "getCapabilitiesForType", "(Ljava/lang/String;)Landroid/media/MediaCodecInfo$CodecCapabilities;");
	if(mGetCapabilitiesForType == NULL) {
		LOGI("can not find the mGetCapabilitiesForType fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	jclass lCapabilitiesclass = pEnv->FindClass("android/media/MediaCodecInfo$CodecCapabilities");
	if(lCapabilitiesclass == NULL){
		LOGI("can not find the android/media/MediaCodecInfo$CodecCapabilities class \n");
		if (pEnv->ExceptionOccurred()) {				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	jmethodID misFeatureSupported = pEnv->GetMethodID(lCapabilitiesclass, "isFeatureSupported", "(Ljava/lang/String;)Z");
	if(misFeatureSupported == NULL) {
		LOGI("can not find the isFeatureSupported fucntion \n");
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
		return TTKErrNotReady;
	}

	jobject info = pEnv->CallObjectMethod(mMediaCodec, mGetCodecInfo);

	jobject codec_capabilities = pEnv->CallObjectMethod(info, mGetCapabilitiesForType, jstr);
	if(codec_capabilities == NULL) {
		return TTKErrNotReady;
	}

	jstring jstrad = pEnv->NewStringUTF(FEATURE_AdaptivePlayback);
	jboolean bAdaptivePlayback =  pEnv->CallBooleanMethod(codec_capabilities, misFeatureSupported, jstrad);
	pEnv->DeleteLocalRef(jstrad);

	mAdaptivePlayback = (TTBool)bAdaptivePlayback;
	LOGI("mAdaptivePlayback %d, bAdaptivePlayback %d, FEATURE_AdaptivePlayback %s", mAdaptivePlayback, bAdaptivePlayback, FEATURE_AdaptivePlayback);

	if(mAdaptivePlayback && mVideoFormatObj != NULL && mSetInteger != NULL) {
		jstring jstr1 = pEnv->NewStringUTF(KEY_MAX_WIDTH);
		pEnv->CallVoidMethod(mVideoFormatObj, mSetInteger, jstr1, MAX_ADAPTIVE_PLAYBACK_WIDTH);
		pEnv->DeleteLocalRef(jstr1);

		jstr1 = pEnv->NewStringUTF(KEY_MAX_HEIGHT);
		pEnv->CallVoidMethod(mVideoFormatObj, mSetInteger, jstr1, MAX_ADAPTIVE_PLAYBACK_HEIGHT);
		pEnv->DeleteLocalRef(jstr1);		
	}

	//memset (szProp, 0, 64);
	//__system_property_get("ro.board.platform", szProp);
	//if(!strcmp(szProp, "hi6620oem")) {
	//	mAdaptivePlayback = 1;
	//}

	return TTKErrNone;	
}