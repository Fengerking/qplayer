/**
* File : TTHWDecoder.cpp 
* Created on : 2015-3-31
* Description : CTTHWDecoder
*/

#include "HWDecoder.h"
#include "TTMediainfoDef.h"
#include "TTLog.h"
#include "TTSysTime.h"

#define  LOG_TAG    "TTHWDec"
#define XRAW_IS_ANNEXB(p) ( !(*((p)+0)) && !(*((p)+1)) && (*((p)+2)==1))
#define XRAW_IS_ANNEXB2(p) ( !(*((p)+0)) && !(*((p)+1)) && !(*((p)+2))&& (*((p)+3)==1))

void didDecompress( void *decompressionOutputRefCon, void *sourceFrameRefCon, OSStatus status, VTDecodeInfoFlags infoFlags, CVImageBufferRef imageBuffer, CMTime pts, CMTime duration )
{
    if (status != noErr || !imageBuffer) {
        // error -8969 codecBadDataErr //printf("error --status: %d\n",
        // -12909 The operation couldn’t be completed. (OSStatus error -12909.)  (int)status);
        return;
    }

    if (CMTIME_IS_VALID(pts)){
        //framePTS = [NSNumber numberWithLongLong:pts.value];
    }
    else
    {
        CFRelease(imageBuffer);
        return;
    }
    
    TTInnerDataPacket* apk = new TTInnerDataPacket;
    apk->pts = pts.value;
    apk->imageBuffer = CVPixelBufferRetain(imageBuffer);
    
    CTTHWDecoder * phwobj = (CTTHWDecoder*)decompressionOutputRefCon;
    
    if (phwobj) {
        phwobj->InsertToUsedList(apk);
    }
}

CTTHWDecoder::CTTHWDecoder(TTUint aCodecType)
:mSeeking(false)
,mEOS(false)
,mNewStart(true)
,mDecSession(NULL)
,mSpsBuffer(NULL)
,mSpsSize(0)
,mPpsBuffer(NULL)
,mPpsSize(0)
,mConfigBuffer(NULL)
,mConfigSize(NULL)
,mTmpBuffer(NULL)
,mTmpSize(0)
,mVideoFormatDescr(NULL)
{
	if(aCodecType == 264)	{
		mVideoCodec = TTVideoInfo::KTTMediaTypeVideoCodeH264;
	} else if(aCodecType == 4) {
		mVideoCodec = TTVideoInfo::KTTMediaTypeVideoCodeMPEG4;
	}
    
	memset(&mVideoFormat, 0, sizeof(mVideoFormat));
    
    mCritical.Create();
    
}

CTTHWDecoder::~CTTHWDecoder()
{
    uninitDecode();
    mCritical.Destroy();
    
    if (mSpsBuffer != NULL)
    {
        free (mSpsBuffer);
    }
    if (mPpsBuffer != NULL)
    {
        free (mPpsBuffer);
    }
    if (mConfigBuffer != NULL)
    {
        free (mConfigBuffer);
    }
    SAFE_FREE(mTmpBuffer);
}


TTInt CTTHWDecoder::initDecode()
{
	LOGI("++initDecode");
	uninitDecode();

    mVideoCodec = TTVideoInfo::KTTMediaTypeVideoCodeH264;
    //else if(mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeMPEG4){}
	return TTKErrNone;
}

TTInt CTTHWDecoder::uninitDecode()
{
    if (mVideoFormatDescr) {
        CFRelease(mVideoFormatDescr);
        mVideoFormatDescr = NULL;
    }
    flush();
    if (mDecSession) {
        VTDecompressionSessionWaitForAsynchronousFrames(mDecSession);
        VTDecompressionSessionInvalidate(mDecSession);
        CFRelease(mDecSession);
        mDecSession = NULL;
    }
    
    mNewStart = true;

	return 0;
}

void CTTHWDecoder::InsertToUsedList(TTInnerDataPacket * apk)
{
    if (apk == NULL) {
        return;
    }
    
    mCritical.Lock();
    List<TTInnerDataPacket *>::iterator it = mListUsed.begin();
    while (it != mListUsed.end() && apk->pts >= (*it)->pts) {
        ++it;
    }
    mListUsed.insert(it, apk);
    mCritical.UnLock();
}

TTInnerDataPacket* CTTHWDecoder::GetDataFromUsedList()
{
    TTInnerDataPacket* apk = NULL;
    List<TTInnerDataPacket *>::iterator it;
    mCritical.Lock();
    if (mListUsed.size()>=3) {
        it = mListUsed.begin();
        apk = (*it);
        mListUsed.erase(it);
        mListFree.push_back(apk);
    }
    else if (mListUsed.size()> 0 && mEOS)
    {
        it = mListUsed.begin();
        apk = (*it);
        mListUsed.erase(it);
        mListFree.push_back(apk);
        //printf("\n--- will end ----\n");
    }
    
    mCritical.UnLock();
    return apk;
}

TTInt CTTHWDecoder::flush()
{
    if(mDecSession) {
        VTDecompressionSessionWaitForAsynchronousFrames(mDecSession);
    }
    TTInnerDataPacket* apk = NULL;
    List<TTInnerDataPacket *>::iterator it ;
    mCritical.Lock();
    if (!mListFree.empty())
    {
        List<TTInnerDataPacket *>::iterator it = mListFree.begin();
        while(it != mListFree.end())
        {
            apk = (*it);
            CFRelease(apk->imageBuffer);
            apk->imageBuffer = NULL;
            SAFE_DELETE(apk);
            it = mListFree.erase(it);
        }
    }
    
    if (!mListUsed.empty())
    {
        List<TTInnerDataPacket *>::iterator it = mListUsed.begin();
        while(it != mListUsed.end())
        {
            apk = (*it);
            CFRelease(apk->imageBuffer);
            apk->imageBuffer = NULL;
            SAFE_DELETE(apk);
            it = mListUsed.erase(it);
        }
    }
    mCritical.UnLock();
    mNewStart = true;
    mEOS = false;
    //printf("\n--flush\n");
    return 0;
}

void CTTHWDecoder::ReleaseVideoBuffer()
{
    TTInnerDataPacket* apk;
    mCritical.Lock();
    if (!mListFree.empty())
    {
        List<TTInnerDataPacket *>::iterator it = mListFree.begin();
        apk = (*it);
        CFRelease(apk->imageBuffer);
        
        apk->imageBuffer = NULL;
        SAFE_DELETE(apk);
        mListFree.erase(it);
    }
    mCritical.UnLock();
}


static void dict_set_string(CFMutableDictionaryRef dict, CFStringRef key, const char * value)
{
    CFStringRef string;
    string = CFStringCreateWithCString(NULL, value, kCFStringEncodingASCII);
    CFDictionarySetValue(dict, key, string);
    CFRelease(string);
}

static void dict_set_boolean(CFMutableDictionaryRef dict, CFStringRef key, BOOL value)
{
    CFDictionarySetValue(dict, key, value ? kCFBooleanTrue: kCFBooleanFalse);
}


static void dict_set_object(CFMutableDictionaryRef dict, CFStringRef key, CFTypeRef *value)
{
    CFDictionarySetValue(dict, key, value);
}

static void dict_set_data(CFMutableDictionaryRef dict, CFStringRef key, uint8_t * value, uint64_t length)
{
    CFDataRef data;
    data = CFDataCreate(NULL, value, (CFIndex)length);
    CFDictionarySetValue(dict, key, data);
    CFRelease(data);
}

static void dict_set_i32(CFMutableDictionaryRef dict, CFStringRef key,
                         int32_t value)
{
    CFNumberRef number;
    number = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
    CFDictionarySetValue(dict, key, number);
    CFRelease(number);
}

static CMFormatDescriptionRef CreateFormatDescriptionFromCodecData(uint32_t format_id, int width, int height, const uint8_t *extradata, int extradata_size, uint32_t atom)
{
    CMFormatDescriptionRef fmt_desc = NULL;
    OSStatus status;
    
    CFMutableDictionaryRef par = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    CFMutableDictionaryRef atoms = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    CFMutableDictionaryRef extensions = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    /* CVPixelAspectRatio dict */
    dict_set_i32(par, CFSTR ("HorizontalSpacing"), 0);
    dict_set_i32(par, CFSTR ("VerticalSpacing"), 0);
    /* SampleDescriptionExtensionAtoms dict */
    dict_set_data(atoms, CFSTR ("avcC"), (uint8_t *)extradata, extradata_size);
    
    /* Extensions dict */
    dict_set_string(extensions, CFSTR ("CVImageBufferChromaLocationBottomField"), "left");
    dict_set_string(extensions, CFSTR ("CVImageBufferChromaLocationTopField"), "left");
    dict_set_boolean(extensions, CFSTR("FullRangeVideo"), FALSE);
    dict_set_object(extensions, CFSTR ("CVPixelAspectRatio"), (CFTypeRef *) par);
    dict_set_object(extensions, CFSTR ("SampleDescriptionExtensionAtoms"), (CFTypeRef *) atoms);
    status = CMVideoFormatDescriptionCreate(NULL, format_id, width, height, extensions, &fmt_desc);
    
    CFRelease(extensions);
    CFRelease(atoms);
    CFRelease(par);
    
    if (status == 0)
        return fmt_desc;
    else
        return NULL;
}


TTInt CTTHWDecoder::createSession()
{
    if (mDecSession != NULL) {
        return TTKErrNone;
    }
    
    flush();
    
    if (mSpsBuffer == NULL || mPpsBuffer == NULL ) {
        return TTKErrArgument;
    }
    
    if(mConfigBuffer) {
        mVideoFormatDescr = CreateFormatDescriptionFromCodecData(kCMVideoCodecType_H264, mVideoFormat.Width, mVideoFormat.Height, mConfigBuffer, mConfigSize, 0);
    } else {
        NSData *spsData;
        NSData *ppsData;
    
        spsData = [NSData dataWithBytes:&(mSpsBuffer[0]) length: mSpsSize];
        ppsData = [NSData dataWithBytes:&(mPpsBuffer[0]) length: mPpsSize];
    
        const uint8_t* const parameterSetPointers[2] = { (const uint8_t*)[spsData bytes], (const uint8_t*)[ppsData bytes] };
        const size_t parameterSetSizes[2] = { [spsData length], [ppsData length] };
        OSStatus status = CMVideoFormatDescriptionCreateFromH264ParameterSets(kCFAllocatorDefault, 2, parameterSetPointers, parameterSetSizes, 4, &mVideoFormatDescr);
    
        if(status!= noErr){
            return TTKErrArgument;
        }
    }
    
    VTDecompressionOutputCallbackRecord callback;
    callback.decompressionOutputCallback = didDecompress;
    callback.decompressionOutputRefCon = (__bridge void *)this;
    
    NSDictionary *destAttributes =[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:YES],(id)kCVPixelBufferOpenGLESCompatibilityKey,[NSNumber numberWithInt:kCVPixelFormatType_420YpCbCr8BiPlanarFullRange],(id)kCVPixelBufferPixelFormatTypeKey,nil];
    
    if (destAttributes == NULL) {
        return TTKErrNoMemory;
    }
    
    OSStatus status = VTDecompressionSessionCreate(kCFAllocatorDefault, mVideoFormatDescr, NULL, (CFDictionaryRef)destAttributes, &callback, &mDecSession);
    
    if(status!= noErr){
        return TTKErrArgument;
    }
    
    mEOS = false;
    
    return TTKErrNone;
}


TTInt32 CTTHWDecoder::SetInputData(TTBuffer *InBuffer)
{
    uint8_t * srcData = InBuffer->pBuffer;
    int size = InBuffer->nSize;
    int duration = InBuffer->nDuration;
    
    int64_t pts = InBuffer->llTime;
    int64_t dts = InBuffer->llTime + 1;
    
    //uint8_t * pdata = srcData;
    //nalu_type = ((uint8_t)pdata[4] & 0x0F);
    if (mNewStart && !(InBuffer->nFlag & TT_FLAG_BUFFER_KEYFRAME)) {
        return TTKErrNone;
    }
    
    if (InBuffer->nFlag & TT_FLAG_BUFFER_KEYFRAME) {
        mNewStart = false;
    }
    
    if (mTmpSize < size) {
        SAFE_FREE(mTmpBuffer);
        mTmpSize = size + 256;
        mTmpBuffer = (unsigned char *)malloc(mTmpSize);
    }
    unsigned char* dst = mTmpBuffer;
    unsigned char* p = srcData;
    unsigned char* startPos = NULL;
    unsigned char* endPos = srcData + size;
    int naluType=0,firstmb=-1,naluLast=0,laststep = 0,framesize;
    int totalsize = 0;
    bool foundSlice = false;
    for (; p < endPos - 3; p++)
    {
        if (XRAW_IS_ANNEXB(p) || XRAW_IS_ANNEXB2(p))	{

            unsigned int step = 3;
            if(XRAW_IS_ANNEXB(p)) {
                naluType = p[3]&0x0f;
                firstmb = p[4] & 0x80;
            } else if(XRAW_IS_ANNEXB2(p)) {
                naluType = p[4]&0x0f;
                firstmb = p[5] & 0x80;
                step = 4;
            }
            
            if ((naluLast == 1 || naluLast == 5) && (naluType != naluLast)) {
                framesize = p - startPos - laststep;
                memcpy(dst + 4 , startPos + laststep, framesize);
                dst[0] = (framesize & 0xff000000) >>24;
                dst[1] = (framesize & 0x00ff0000) >>16;
                dst[2] = (framesize & 0x0000ff00) >>8;
                dst[3] = (framesize & 0x000000ff);
                dst += (4+framesize);
                
                laststep = step;
                startPos = p;
                
                totalsize +=(4+framesize);
                foundSlice = true;
            }
            
            if(naluType != 1 && naluType != 5 ) {
                naluLast = naluType;
                p += step;
                continue;
            }
            
            if (naluType != naluLast && (naluType == 5 || naluType == 1)) {
                startPos = p;
                foundSlice = true;
                laststep = step;
            }
            
            if (naluType == naluLast && (naluType == 5 || naluType == 1)) {
                framesize = p- startPos - laststep;
                memcpy(dst + 4 , startPos + laststep, framesize);
                dst[0] = (framesize & 0xff000000) >>24;
                dst[1] = (framesize & 0x00ff0000) >>16;
                dst[2] = (framesize & 0x0000ff00) >>8;
                dst[3] = (framesize & 0x000000ff);
                dst += (4+framesize);
                
                laststep = step;
                startPos = p;
                
                totalsize +=(4+framesize);
                foundSlice = true;
            }
            naluLast = naluType;
            p += step;
        }
    }
    
    if (foundSlice && (naluType == 1 || naluType ==5)) {
        framesize = endPos - startPos - laststep;
        memcpy(dst + 4 , startPos + laststep, framesize);
        dst[0] = (framesize & 0xff000000) >>24;
        dst[1] = (framesize & 0x00ff0000) >>16;
        dst[2] = (framesize & 0x0000ff00) >>8;
        dst[3] = (framesize & 0x000000ff);
        dst += (4+framesize);
        totalsize +=(4+framesize);
    }
    
   /* if (nalu_type != 1 && nalu_type != 5 ) {
        nalen = *pdata++;
        for (int i = 0; i < 3; i++) {
            nalen = nalen <<8;
            nalen += *pdata++;
        }
        pdata += nalen;
        
        if (size == nalen + 4) {
            return TTKErrArgument;
        }
        
        size -= (nalen + 4);
        nalu_type = (pdata[4] & 0x0F);
    }*/
    
    if (!foundSlice) {
        return TTKErrNone;
    }
    //printf(" data = %d ,nal = %d, time = %d \n",size,nalu_type,(int)pts);
    CMBlockBufferRef videoBlock = NULL;
    OSStatus status = CMBlockBufferCreateWithMemoryBlock(NULL, mTmpBuffer, totalsize, kCFAllocatorNull, NULL, 0, totalsize, 0, &videoBlock);
    
    if(status!= noErr){
        return TTKErrArgument;
    }
    
    CMSampleTimingInfo timeInfo;
    timeInfo.presentationTimeStamp = CMTimeMake(pts, 25000);
    timeInfo.decodeTimeStamp = CMTimeMake(dts, 25000);
    timeInfo.duration = CMTimeMake(duration , 25000);
    
    CMSampleBufferRef sbRef = NULL;
    const size_t sampleSize = totalsize;
    status = CMSampleBufferCreate(kCFAllocatorDefault, videoBlock, true, NULL, NULL, mVideoFormatDescr, 1, 1, &timeInfo, 1, &sampleSize, &sbRef);
    
    if(status!= noErr){
        return TTKErrArgument;
    }
    
    if (videoBlock) {
        CFRelease(videoBlock);
    }
    
    VTDecodeFrameFlags flags = kVTDecodeFrame_EnableAsynchronousDecompression;
    VTDecodeInfoFlags flagOut = 0;
    
    if(InBuffer->nFlag & TT_FLAG_BUFFER_DROP_FRAME) {
        flags |= kVTDecodeFrame_DoNotOutputFrame;
    }
    
    status = VTDecompressionSessionDecodeFrame(mDecSession, sbRef, flags, &sbRef, &flagOut);
    
    CFRelease(sbRef);
    
    if(status!= noErr){
        return TTKErrArgument;
    }
    
    //VTDecompressionSessionWaitForAsynchronousFrames(mDecSession);
    return TTKErrNone;
}

TTInt CTTHWDecoder::getOutputBuffer(TTVideoBuffer* DstBuffer, TTVideoFormat* pOutInfo)
{
	if(mDecSession == NULL)
		return TTKErrNotReady;
    
	if ((DstBuffer->nFlag&TT_FLAG_BUFFER_SEEKING)) {
		LOGI("seek options Flags added");
	}
    
    ReleaseVideoBuffer();
    
    TTInnerDataPacket* apk = GetDataFromUsedList();
    //cs printf("\nused num = %d, free = %d, t = %d\n",mListUsed.size(),mListFree.size() ,mListUsed.size()+mListFree.size());
    if (apk == NULL) {
        return TTKErrNotReady;
    }
 
    TTInt32 nX = CVPixelBufferGetWidth(apk->imageBuffer);
    TTInt32 nY = CVPixelBufferGetHeight(apk->imageBuffer);
    //int vv = CVPixelBufferGetDataSize(apk->imageBuffer);
    if (!mVideoFormat.Width || !mVideoFormat.Height || mVideoFormat.Width != nX || mVideoFormat.Height != nY) {
        mVideoFormat.Width  = nX;
        mVideoFormat.Height = nY;
    }
    
    DstBuffer->ColorType = TT_COLOR_YUV_NV12;

    nX = nX / 2;
    nY = nY / 2;
  
    DstBuffer->Buffer[0] = (TTPBYTE)apk->imageBuffer;
    DstBuffer->Stride[0] = nX*2;;
    DstBuffer->Stride[1] = nX;

    DstBuffer->Time = apk->pts;
    
    if(pOutInfo) {
        memcpy(pOutInfo, &mVideoFormat, sizeof(mVideoFormat));
    }
    /*static int cunt = 3;
    if (cunt< 1) {
        
     
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docDir = [paths objectAtIndex:0];
    const char* tmp = [docDir UTF8String];
    char strpath[1028] = {0};
    sprintf(strpath, "%s/hard%d.yuv", tmp,cunt);
        FILE * F=  fopen(strpath,"wb+");
        fwrite((TTPBYTE)baseAddress + offset, 1, Ysize - offset + nY*nX*2,F);
        fflush(F);
        fclose(F);
    }
    cunt++;*/
    //return TTKErrFormatChanged TTKErrNotReady;

	return TTKErrNone;
}

TTInt CTTHWDecoder::setParam(TTInt aID, void* pValue)
{
	if(aID == TT_PID_VIDEO_FORMAT)
    {
		if(pValue)
			memcpy(&mVideoFormat, pValue, sizeof(mVideoFormat));
		return TTKErrNone;
	}
    else if(aID == TT_PID_VIDEO_FLUSHALL)
    {
        mEOS = true;
		return TTKErrNone;
	}
    else if(aID == TT_PID_VIDEO_START)
    {
        if (mPpsBuffer && mSpsBuffer) {
            createSession();
        }
        return TTKErrNone;
    }
    else if(aID == TT_PID_VIDEO_DECODER_INFO)
    {
		if(mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeH264)
        {
            uninitDecode();
            
			TTAVCDecoderSpecificInfo * pBuffer = (TTAVCDecoderSpecificInfo *)pValue;
			if(pValue == NULL)
				return TTKErrArgument;
            
            if(pBuffer->iConfigData && pBuffer->iConfigSize) {
                if (mConfigBuffer != NULL)
                    free (mConfigBuffer);
                
                mConfigSize = pBuffer->iConfigSize;
                mConfigBuffer = (unsigned char *)malloc (mConfigSize + 8);
                memcpy (mConfigBuffer, pBuffer->iConfigData, mConfigSize);
            
            }
            
			if (mSpsBuffer != NULL)
				free (mSpsBuffer);

			unsigned char *buffer = pBuffer->iSpsData;
			int size = pBuffer->iSpsSize;
							
			if (buffer[2]==0 && buffer[3]==1) {
				buffer+=4;
				size -= 4;
			} else {
				buffer+=3;
				size -= 3;
			}

			mSpsSize = size;
			mSpsBuffer = (unsigned char *)malloc (mSpsSize + 8);
			memcpy (mSpsBuffer, buffer, mSpsSize);

			if (mPpsBuffer != NULL)
				free (mPpsBuffer);
				
            buffer = pBuffer->iPpsData;
			size = pBuffer->iPpsSize;
							
			if (buffer[2]==0 && buffer[3]==1) {
				buffer+=4;
				size -= 4;
			} else {
				buffer+=3;
				size -= 3;
			}
						
			mPpsSize = size;
			mPpsBuffer = (unsigned char *)malloc (mPpsSize + 8);
			memcpy (mPpsBuffer, buffer, mPpsSize);
		}
        if(mVideoCodec == TTVideoInfo::KTTMediaTypeVideoCodeMPEG4)
        {
		}
		return TTKErrNone;
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



