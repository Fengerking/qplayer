/**
* File : TTHWDecoder.cpp 
* Created on : 2014-11-5
* Author : yongping.lin
* Copyright : Copyright (c) 2014 GoKu Software Ltd. All rights reserved.
* Description : CTTHWDecoder实现文件
*/

#include "HWDecoder.h"
#include "HWSrcRead.h"
#include "TTMediainfoDef.h"

#define  LOG_TAG    "TTHWDec"

#define  OMX_TI_COLOR_FormatYUV420PackedSemiPlanar 		0x7F000100
#define  OMX_QCOM_COLOR_FormatYVU420SemiPlanar  		0x7FA30C00
#define  QOMX_COLOR_FormatYVU420PackedSemiPlanar32m4ka 		0x7FA30C01
#define  QOMX_COLOR_FormatYUV420PackedSemiPlanar16m2ka		0x7FA30C02
#define  QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka	0x7FA30C03
#define  OMX_QCOM_COLOR_FormatYUV420PackedSemiPlanar32m 	0x7FA30C04
#define  OMX_SEC_COLOR_FormatNV12Tiled 				0x7FC00002


CTTHWDecoder::CTTHWDecoder(TTUint aCodecType)
:mEOS(false)
,mStarted(false)
,mCreated(false)
,mConnected(false)
,mObserver(NULL)
,mHeadBuffer(NULL)
,mHeadSize(0)
,mHeadNalBuffer(NULL)
,mHeadNalSize(0)
,mSeeking(0)
,mColorType(TT_COLOR_YUV_PLANAR420)
,mSource(NULL)
,mDecoder(NULL)
,mBuffer(NULL)
,mNativeWindow(NULL)
,mColorFormat(0)
,mUseSurfaceAlloc(1)
,mClient(NULL)
,mCompName(NULL)
{
	if(aCodecType == 264)	{
		mVideoCodec = TTVideoInfo::KTTMediaTypeVideoCodeH264;
	} else if(aCodecType == 4) {
		mVideoCodec = TTVideoInfo::KTTMediaTypeVideoCodeMPEG4;
	}
	memset(&mVideoFormat, 0, sizeof(mVideoFormat));
}

CTTHWDecoder::~CTTHWDecoder()
{
	uninitDecode();

	if (mHeadBuffer != NULL) {
		free (mHeadBuffer);
		mHeadBuffer = NULL;
	}

	if(mHeadNalBuffer != NULL) {
		free (mHeadNalBuffer);
		mHeadNalBuffer = NULL;
	}
    SAFE_FREE(mTmpBuffer);
}

static void Encode14(uint8_t **_ptr, size_t size) {
    uint8_t *ptr = *_ptr;

    *ptr++ = 0x80 | (size >> 7);
    *ptr++ = size & 0x7f;

    *_ptr = ptr;
}

static size_t MakeMPEGESDS(uint8_t *_ptr, size_t size, uint8_t *_output) {
    uint8_t *ptr = _output;
    *ptr++ = 0x03;
    Encode14(&ptr, 22 + size);

    *ptr++ = 0x00;  // ES_ID
    *ptr++ = 0x00;

    *ptr++ = 0x00;  // streamDependenceFlag, URL_Flag, OCRstreamFlag

    *ptr++ = 0x04;
    Encode14(&ptr, 16 + size);

    *ptr++ = 0x40;  // Audio ISO/IEC 14496-3

    for (size_t i = 0; i < 12; ++i) {
        *ptr++ = 0x00;
    }

    *ptr++ = 0x05;
    Encode14(&ptr, size);

    memcpy(ptr, _ptr, size);

    return 25+size;
}

TTInt CTTHWDecoder::initDecode()
{
	LOGI("++initDecode");
	uninitDecode();

	TTInt nErr = TTKErrNotSupported;
	sp<MetaData> meta = new MetaData;
	if (meta == NULL) {
		nErr = TTKErrNoMemory;
		return nErr;
	}

	if (mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeH264) {
		meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);
		meta->setData(kKeyAVCC, kTypeAVCC, mHeadBuffer, mHeadSize);
	} else if(mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeMPEG4){
		meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_MPEG4);	
		uint8_t *esds = new uint8_t[mHeadSize + 25];
		size_t nSize = MakeMPEGESDS(mHeadBuffer, mHeadSize, esds);
		meta->setData(kKeyESDS, kTypeESDS, esds, nSize);
		delete esds;
	}

	meta->setInt32(kKeyWidth, mVideoFormat.Width);
	meta->setInt32(kKeyHeight, mVideoFormat.Height);

	android::ProcessState::self()->startThreadPool();

	mSource   = new HWSource(this, meta);
	mClient    = new OMXClient;
	if (mSource == NULL || !mClient ) {
		nErr = TTKErrNoMemory;
		meta.clear();
		return nErr;
	}

	if (mClient->connect() !=  OK) {
		meta.clear();
		return nErr;
	}

	mConnected = true;

	if(mUseSurfaceAlloc != 0 && mNativeWindow != NULL)	{
		status_t err = OK;
		if(NULL != mNativeWindow.get()) {
			//err = native_window_api_connect(mNativeWindow.get(), NATIVE_WINDOW_API_MEDIA);
			//if(OK != err){
			//	LOGI("failed to native_window_api_connect %s (%d)", strerror(-err), -err);
			// 	return nErr;
			//}

			err = native_window_set_scaling_mode(mNativeWindow.get(),
				NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);
		}
	}


	mDecoder = OMXCodec::Create(mClient->interface(), meta,
		false, mSource,  NULL, OMXCodec::kHardwareCodecsOnly, mUseSurfaceAlloc ? mNativeWindow : NULL);

	meta.clear();
	if(mDecoder == NULL) {
		return nErr;	
	}

	mCreated = true;
	mStarted = false;

	sp<MetaData> outFormat = mDecoder->getFormat();
	outFormat->findInt32(kKeyColorFormat, &mColorFormat);
	if (mColorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar ||
		mColorFormat == QOMX_COLOR_FormatYUV420PackedSemiPlanar16m2ka ||
		mColorFormat == QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka ||
		mColorFormat == OMX_QCOM_COLOR_FormatYUV420PackedSemiPlanar32m ||
		mColorFormat == OMX_SEC_COLOR_FormatNV12Tiled ||
		mColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
		mColorType = TT_COLOR_YUV_NV12;
	else if(mColorFormat == OMX_QCOM_COLOR_FormatYVU420SemiPlanar ||
		mColorFormat ==	QOMX_COLOR_FormatYVU420PackedSemiPlanar32m4ka)
		mColorType = TT_COLOR_YUV_NV21;
	else if (mColorFormat == OMX_COLOR_FormatYCbYCr)
		mColorType = TT_COLOR_YUV_YUYV422;
	else if (mColorFormat == OMX_COLOR_FormatCbYCrY)
		mColorType = TT_COLOR_YUV_UYVY422;

	outFormat->findCString(kKeyDecoderComponent, &mCompName);
	if (mCompName)
		mCompName = strdup(mCompName);

	LOGI("HW Decoder start OK mCompName %s, mColorFormat %x, mColorType %d", mCompName, mColorFormat, mColorType);

	if (!strncmp("OMX.google.", mCompName, 11) || !strncmp("OMX.PV.", mCompName, 7) || !strncmp("OMX.MTK.VIDEO.", mCompName, 14))
		return nErr;

	LOGI("--initHWDecode OK");
	return TTKErrNone;
}

TTInt CTTHWDecoder::uninitDecode()
{   
	stop();

	//if(mUseSurfaceAlloc != 0 && mNativeWindow != NULL)	{
	//	status_t err = OK;
	//	if(NULL != mNativeWindow.get())
	//	{
			//err = native_window_api_disconnect(mNativeWindow.get(), NATIVE_WINDOW_API_MEDIA);
			//if(OK != err)
			//{
			//	LOGI("failed to native_window_api_disconnect %s (%d)", strerror(-err), -err);
			//}
	//	}
	//}

	if(mClient) {
		if(mConnected) {
			mClient->disconnect();
			mConnected = false;
		}
		delete mClient;
		mClient = NULL;
	}

	wp<MediaSource> tmp = mDecoder;
	mDecoder.clear();
	while (tmp.promote() != NULL) {
		usleep(1000);
	}

	android::IPCThreadState::self()->flushCommands();
    LOGI("video decoder shutdown completed");

	mSource.clear();

	mStarted = false;
	mCreated = false;

	return 0;
}

TTInt CTTHWDecoder::start()
{
	if(mStarted) {
		return 0;
	}

	if(mDecoder == NULL) {
		return -1;
	}

	if (mDecoder->start() !=  OK) {
		return -1;        
	}

	mStarted = true;

	return 0;
}

TTInt CTTHWDecoder::stop()
{
	if(mBuffer) {
		mBuffer->release();
		mBuffer = NULL;
	}

	if(mStarted && mDecoder != NULL ) {
		return mDecoder->stop();
	}

	mStarted = false;	
	
     return TTKErrNone;
}

TTInt CTTHWDecoder::getOutputBuffer(TTVideoBuffer* DstBuffer, TTVideoFormat* pOutInfo)
{
	//MediaBuffer *buffer = NULL;
	int32_t w, h;
	int32_t w1, h1;
	int64_t timeUs;

	if(mDecoder == NULL || !mStarted) 
		return TTKErrNotReady;
	MediaSource::ReadOptions options;
	if ((DstBuffer->nFlag&TT_FLAG_BUFFER_SEEKING)) {
		LOGI("seek options Flags added");
		options.setSeekTo(DstBuffer->Time*1000,
			MediaSource::ReadOptions::SEEK_CLOSEST_SYNC);
	}

	if(mBuffer) {
		mBuffer->release();
		mBuffer = NULL;
	}

	status_t status = mDecoder->read(&mBuffer, &options);
	options.clearSeekTo();

	if (status == OK) {
		sp<MetaData> outFormat = mDecoder->getFormat();
		outFormat->findInt32(kKeyWidth , &w);
		outFormat->findInt32(kKeyHeight, &h);

		if ((w & 15 || h & 15) && (mColorType == TT_COLOR_YUV_NV12 || mColorType == TT_COLOR_YUV_NV21 || mColorType == TT_COLOR_YUV_PLANAR420)) {
			if (((w + 15)&~15) * ((h + 15)&~15) * 3/2 == mBuffer->range_length()) {
				w = (w + 15)&~15;
				h = (h + 15)&~15;
			} else if(((w + 1)&~1) * ((h + 15)&~15) * 3/2 == mBuffer->range_length()) {
				w = (w + 1)&~1;
				h = (h + 15)&~15;
			} else if(((w + 15)&~15) * ((h + 1)&~1) * 3/2 == mBuffer->range_length()) {
				w = (w + 15)&~15;
				h = (h + 1)&~1;
			}
		} else if(mColorType == TT_COLOR_YUV_YUYV422 || mColorType == TT_COLOR_YUV_UYVY422) {
			if (((w + 15)&~15) * ((h + 15)&~15) * 2 == mBuffer->range_length()) {
				w = (w + 15)&~15;
				h = (h + 15)&~15;
			} else if(((w + 1)&~1) * ((h + 15)&~15) * 2 == mBuffer->range_length()) {
				w = (w + 1)&~1;
				h = (h + 15)&~15;
			} else if(((w + 15)&~15) * ((h + 1)&~1) * 2 == mBuffer->range_length()) {
				w = (w + 15)&~15;
				h = (h + 1)&~1;
			}		
		}
		
		if (!mVideoFormat.Width || !mVideoFormat.Height || mVideoFormat.Width > w || mVideoFormat.Height > h) {
			mVideoFormat.Width  = w;
			mVideoFormat.Height = h;
		}

		DstBuffer->ColorType = mColorType;
		if(mUseSurfaceAlloc == 0) {
			if(mColorType == TT_COLOR_YUV_NV12 || mColorType == TT_COLOR_YUV_NV21) {
				DstBuffer->Stride[0] = w;
				DstBuffer->Stride[1] = w;
				DstBuffer->Stride[2] = 0;

				DstBuffer->Buffer[0] = (uint8_t*)mBuffer->data();
				DstBuffer->Buffer[1] = DstBuffer->Buffer[0] + DstBuffer->Stride[0] * h;
			} else if(mColorType == TT_COLOR_YUV_YUYV422 || mColorType == TT_COLOR_YUV_UYVY422){
				DstBuffer->Stride[0] = 2*w;
				DstBuffer->Stride[1] = 0;
				DstBuffer->Stride[2] = 0;

				DstBuffer->Buffer[0] = (uint8_t*)mBuffer->data();
			} else {
				DstBuffer->Stride[0] = w;
				DstBuffer->Stride[1] = ((w/2) + 15) & ~15;
				DstBuffer->Stride[2] = ((w/2) + 15) & ~15;

				DstBuffer->Buffer[0] = (uint8_t*)mBuffer->data();
				DstBuffer->Buffer[1] = DstBuffer->Buffer[0] + DstBuffer->Stride[0] * h;
				DstBuffer->Buffer[2] = DstBuffer->Buffer[1] + DstBuffer->Stride[1] * h/2;
			}
		}

		if(mBuffer->range_length() == 0) {
			mBuffer->release();
			mBuffer = NULL;
			return TTKErrNotReady;
		}

		mBuffer->meta_data()->findInt64(kKeyTime, &timeUs);
		DstBuffer->Time = timeUs/1000;
		DstBuffer->pReserve = mBuffer;

		if(pOutInfo) {
			memcpy(pOutInfo, &mVideoFormat, sizeof(mVideoFormat));
		}
	} else if (status == INFO_FORMAT_CHANGED) {
		sp<MetaData> outFormat = mDecoder->getFormat();
		outFormat->findInt32(kKeyWidth , &w);
		outFormat->findInt32(kKeyHeight, &h);

		if (!w && !h) {
			mVideoFormat.Width  = w;
			mVideoFormat.Height = h;
		}

		return TTKErrFormatChanged;
	} else {
		return TTKErrNotReady;
	}

	return TTKErrNone;
}

TTInt CTTHWDecoder::setParam(TTInt aID, void* pValue)
{
	if(aID == TT_PID_VIDEO_FORMAT){
		if(pValue)
			memcpy(&mVideoFormat, pValue, sizeof(mVideoFormat));
		return TTKErrNone;
	}else if(aID == TT_PID_VIDEO_CALLFUNCTION){
		if(pValue)
			mObserver = (TTObserver*)pValue;
		LOGI("set Callback funtion");
		return TTKErrNone;
	} else if(aID == TT_PID_VIDEO_DECODER_INFO)	{
		if(mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeH264) {
			TTAVCDecoderSpecificInfo * pBuffer = (TTAVCDecoderSpecificInfo *)pValue;
			if(pValue == NULL)
				return TTKErrArgument;
			if (mHeadBuffer != NULL)
				free (mHeadBuffer);
			mHeadSize = pBuffer->iConfigSize;
			mHeadBuffer = (unsigned char *)malloc (mHeadSize + 8);
			memcpy (mHeadBuffer, pBuffer->iConfigData, pBuffer->iConfigSize);

			if (mHeadNalBuffer != NULL)
				free (mHeadNalBuffer);
			mHeadNalSize = pBuffer->iSize;
			mHeadNalBuffer = (unsigned char *)malloc (mHeadNalSize + 8);
			memcpy (mHeadNalBuffer, pBuffer->iData, pBuffer->iSize);
			LOGI("set head data, mHeadSize %d, mHeadNalSize %d", mHeadSize, mHeadNalSize);
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
		return TTKErrNone;
	}  else if(aID == TT_PID_VIDEO_NATIVWINDOWS){ 
		mNativeWindow = (ANativeWindow *)pValue;

		TTInt nErr = initDecode();
		return nErr;	
	}

	return TTKErrNotFound;
}

TTInt CTTHWDecoder::getParam(TTInt aID, void* pValue)
{
	if(aID == TT_PID_VIDEO_FORMAT){
		if(pValue)
			memcpy(pValue, &mVideoFormat, sizeof(mVideoFormat));
		return TTKErrNone;
	}
	
	return TTKErrNotFound;
}

TTInt CTTHWDecoder::renderOutputBuffer(TTVideoBuffer* DstBuffer)
{
	MediaBuffer *buffer = (MediaBuffer *)mBuffer;
	if(mNativeWindow == NULL || mUseSurfaceAlloc == 0)	
		return 	TTKErrNotFound;

	if(buffer != DstBuffer->pReserve || !mStarted) {
		LOGI("error buffer render, buffer: %x DstBuffer->pReserve %x, mStarted %d", (int)buffer, (int)DstBuffer->pReserve, mStarted);
		return TTKErrNotFound;
	}

	status_t err = -1;
#ifdef _TT_JB43_
	if(buffer) {
       		err = mNativeWindow->queueBuffer(
                mNativeWindow.get(), buffer->graphicBuffer().get(), -1);

			if(err == 0) {
				sp<MetaData> metaData = buffer->meta_data();
				metaData->setInt32(kKeyRendered, 1);
			}
	}
#else	
	if(buffer) {
       		err = mNativeWindow->queueBuffer(
                mNativeWindow.get(), buffer->graphicBuffer().get());

			if(err == 0) {
				sp<MetaData> metaData = buffer->meta_data();
				metaData->setInt32(kKeyRendered, 1);
			}
	}
#endif

	return err;	
}

TTObserver* CTTHWDecoder::getObserver()
{
	return mObserver;
}

TTInt CTTHWDecoder::getConfigData(TTBuffer* DstBuffer)
{
	if(DstBuffer == NULL)
		return -1;

	DstBuffer->pBuffer = mHeadNalBuffer;
	DstBuffer->nSize = mHeadNalSize;
	return 0;
}


