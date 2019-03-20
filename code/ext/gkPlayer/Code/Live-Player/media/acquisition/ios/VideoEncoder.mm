/**
 * File : RTMPPush.mm
 * Created on : 2015-12-12
 * Author : Kevin
 * Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
 */

#include <stdio.h>
#include <VideoToolbox/VideoToolbox.h>

#include "VideoEncoder.h"
#include "FileWrite.h"
#include "TTSysTime.h"
#include "GKTypedef.h"
#include "AVTimeStamp.h"
#include "GKCollectCommon.h"

#define AVCCHeaderLength 4
#define  BUFFER_KEYFRAME 1

#define CAMEAR_FRONT 0
#define CAMEAR_BACK  1

#define clip(x)  ((x) > 255 ? 255 : ( (x) < 0 ? 0 : (x) ))

CGKVideoEncoder::CGKVideoEncoder()
:mEncodingSession(NULL)
, mwidth(0)
, mhight(0)
, mframerate(0)
, mbitrate(0)
, mset640480(false)
, mIsConfigset(false)
, mPush(NULL)
, mData(NULL)
, mlastRGB(NULL)
, mPixelBuffer(NULL)
{
    mCtritData.Create();
    mCtritcode.Create();
#ifdef DUMP_ENCODER_H264
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docDir = [paths objectAtIndex:0];
    NSLog(@"doc dir: %@", docDir);
    NSString *filename = @"/abc.h264";
    NSString *realName = [docDir stringByAppendingString:filename];
    
    const char *h264Name = [realName UTF8String];
    fwc = new FileWrite();
    fwc->create(h264Name);
#endif
    mppslen =0;
    mspslen =0;
    msbuf = NULL;
    mpbuf = NULL;
    
    mStartEncoder = true;
    
    CVPixelBufferCreate(NULL, 360, 640, kCVPixelFormatType_420YpCbCr8BiPlanarFullRange, NULL, &mPixelBuffer);
}

CGKVideoEncoder::~CGKVideoEncoder()
{
    SAFE_FREE(msbuf);
    SAFE_FREE(mpbuf);
    if (mPixelBuffer) {
        CFRelease(mPixelBuffer);
    }
    mCtritData.Destroy();
    mCtritcode.Destroy();
    if (mlastRGB) {
        free(mlastRGB);
    }
    SAFE_FREE(mData);
}

void CGKVideoEncoder::set640480(bool bset)
{
    mset640480 = bset;
}

void CGKVideoEncoder::setPush(void* pushobj)
{
    mPush = (CGKPushWrap*)pushobj;
}

void CGKVideoEncoder::setvideoparameter(int width, int hight, int framerate, int bitrate)
{
    mset640480 = false;
    mwidth = width;
    mhight = hight;
    mframerate = framerate;
    mbitrate = bitrate;
    
    if (mPush) {
        mPush->setbitrate(mbitrate);
    }
    
    if (mlastRGB) {
        free(mlastRGB);
    }
    mlastRGB = (unsigned char* )malloc(mwidth*mhight*4);
    
}

//AVCC ：Nalu1 length + Nalu1 + Nalu2 length + Nalu2 + ... : ios videotoolbox format1
void didCompressH264(void *outputCallbackRefCon,
                     void *sourceFrameRefCon,
                     OSStatus status,
                     VTEncodeInfoFlags infoFlags,
                     CMSampleBufferRef sampleBuffer)
{
    if (status != 0){
        printf("CompressH264 fail, status %d infoFlags %d\n",(int)status, (int)infoFlags);
        return;
    }
    
    if (!CMSampleBufferDataIsReady(sampleBuffer))
    {
        printf("CompressH264 data is not ready\n");
        return;
    }
    
    CGKVideoEncoder *ihec = (CGKVideoEncoder *)outputCallbackRefCon;
    
    size_t ppslen = 0;
    size_t spslen = 0;
    const uint8_t * sbuf;
    const uint8_t * pbuf;
    bool keyframe;
    
    CMTime pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    
    if (!ihec->mIsConfigset) {
         keyframe = !CFDictionaryContainsKey(CFDictionaryRef(CFArrayGetValueAtIndex(CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, true), 0)), (void *)kCMSampleAttachmentKey_NotSync);
        
        if (keyframe)
        {
            CMFormatDescriptionRef format = CMSampleBufferGetFormatDescription(sampleBuffer);
            CMVideoFormatDescriptionGetH264ParameterSetAtIndex(format, 0, &sbuf, &spslen, NULL, 0 );
            CMVideoFormatDescriptionGetH264ParameterSetAtIndex(format, 1, &pbuf, &ppslen, NULL, 0 );
 
            if (ppslen > 0 && spslen >0) {
                //set sps pps
                ihec->setSpsPpsConfig(sbuf, spslen, pbuf, ppslen);
                ihec->mIsConfigset = true;
                printf("nal len = %zu,pps = %zu\n",spslen,ppslen);
            }
        }
    }

    CMBlockBufferRef dataBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
    size_t length, totalLength;
    char *dataPointer;
    //cs
    int type;
    OSStatus statusCodeRet = CMBlockBufferGetDataPointer(dataBuffer, 0, &length, &totalLength, &dataPointer);
    if (statusCodeRet == noErr)
    {
#ifdef DUMP_ENCODER_H264
            ihec->fwc->write(dataPointer, totalLength);
            ihec->fwc->flush();
#endif
        size_t bufferOffset = 0;
        while (bufferOffset < totalLength - AVCCHeaderLength)
        {
            // Read the NAL unit length
            uint32_t NALUnitLength = 0;
            memcpy(&NALUnitLength, dataPointer + bufferOffset, AVCCHeaderLength);
            
            // Convert the length value from Big-endian to Little-endian
            NALUnitLength = CFSwapInt32BigToHost(NALUnitLength);
            
            type = (*(dataPointer+bufferOffset+AVCCHeaderLength)) &0x1f ;

            if (type == (NAL_SLICE_IDR +1) || type > NAL_PPS) {
                bufferOffset += AVCCHeaderLength + NALUnitLength;
                totalLength -= (AVCCHeaderLength + NALUnitLength);
            }
            else{
                if (type == NAL_SLICE_IDR)
                    type = BUFFER_KEYFRAME;
                else
                    type = 0;
                
                //printf("send [%d],size= %lu\n",type,totalLength);
                //printf("--V.pts = %lld\n",pts.value);
                ihec->sendvideopacket((TTPBYTE)(dataPointer+bufferOffset), totalLength, pts.value,type);
                break;
            }
        }
    }
}

void CGKVideoEncoder::init()
{
    SAFE_FREE(mData);
    
    mData = (unsigned char*)malloc(mwidth*mhight*3/2);
    
    OSStatus status = VTCompressionSessionCreate(NULL, mwidth, mhight, kCMVideoCodecType_H264,
                                                 NULL, NULL, NULL,
                                                 didCompressH264,
                                                 this,  &mEncodingSession);
    // Set the properties
    VTSessionSetProperty(mEncodingSession, kVTCompressionPropertyKey_RealTime, kCFBooleanTrue);
    VTSessionSetProperty(mEncodingSession, kVTCompressionPropertyKey_ProfileLevel, kVTProfileLevel_H264_Main_4_0);
    
    const int32_t m_fps = mframerate;
    CFNumberRef ref = CFNumberCreate(NULL, kCFNumberSInt32Type, &m_fps);
    status = VTSessionSetProperty(mEncodingSession, kVTCompressionPropertyKey_ExpectedFrameRate, ref);
    CFRelease(ref);
    
    const int  m_bitrate = mbitrate;
    ref = CFNumberCreate(NULL, kCFNumberSInt32Type, &m_bitrate);
    status = VTSessionSetProperty(mEncodingSession, kVTCompressionPropertyKey_AverageBitRate, ref);
    CFRelease(ref);
    
    status = VTSessionSetProperty(mEncodingSession, kVTCompressionPropertyKey_AllowFrameReordering, kCFBooleanFalse);
    
    // Tell the encoder to start encoding
    VTCompressionSessionPrepareToEncodeFrames(mEncodingSession);
}

void CGKVideoEncoder::transfer640480to640360(CMSampleBufferRef sampleBuffer)
{
    CVImageBufferRef imageBuffer = (CVImageBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    
    CVPixelBufferLockBaseAddress(imageBuffer,0);
    CVPixelBufferLockBaseAddress(mPixelBuffer, 0);
    
    unsigned char* targetY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 0);
    unsigned char* targetUV=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 1);
    size_t targetRowByter0  = CVPixelBufferGetBytesPerRowOfPlane(mPixelBuffer, 0);
    size_t targetRowByter1 = CVPixelBufferGetBytesPerRowOfPlane(mPixelBuffer, 1);
    
    //CVPlanarPixelBufferInfo_YCbCrBiPlanar *bufferInfo = (CVPlanarPixelBufferInfo_YCbCrBiPlanar *)baseAddress;
    size_t sourceRowByte0 = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, 0);
    size_t sourceRowByte1 = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, 1);
    
    
    unsigned char* sourceY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
    unsigned char* sourceUV=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 1);
    
    unsigned char* pY = targetY;
    
    unsigned char* pYSrc = sourceY;
    
    unsigned char* pVUSrc = sourceUV;
    
    int i = 0;
    int j = 0;
    for(j = 0; j < mhight; j++){
        for(i = 0; i < mwidth; i++) {
            pY[i] = pYSrc[i+60];
        }
        pYSrc += sourceRowByte0;
        pY += targetRowByter0;
    }
    
    for (i=0;i<mhight;i+=2) {
        int k = 0;
        for(j=0;j<mwidth;j++)
        {
            targetUV[k++] = pVUSrc[j+60];
        }
        
        targetUV += targetRowByter1;
        
        pVUSrc += sourceRowByte1;
    }
    
    CVPixelBufferUnlockBaseAddress(mPixelBuffer,0);
    CVPixelBufferUnlockBaseAddress(imageBuffer,0);
}

void CGKVideoEncoder::rescale_2(CMSampleBufferRef sampleBuffer)
{
    CVImageBufferRef imageBuffer = (CVImageBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    
    CVPixelBufferLockBaseAddress(imageBuffer,0);
    CVPixelBufferLockBaseAddress(mPixelBuffer, 0);
    
    unsigned char* targetY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 0);
    unsigned char* targetUV=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 1);
    size_t targetRowByter0  = CVPixelBufferGetBytesPerRowOfPlane(mPixelBuffer, 0);
    size_t targetRowByter1 = CVPixelBufferGetBytesPerRowOfPlane(mPixelBuffer, 1);

    //CVPlanarPixelBufferInfo_YCbCrBiPlanar *bufferInfo = (CVPlanarPixelBufferInfo_YCbCrBiPlanar *)baseAddress;
    size_t sourceRowByte0 = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, 0);
    size_t sourceRowByte1 = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, 1);
    
    
    unsigned char* sourceY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
    unsigned char* sourceUV=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 1);
    
    size_t OriWidth = CVPixelBufferGetWidth(imageBuffer);
    //int OriHeight = CVPixelBufferGetHeight(imageBuffer);
    
    unsigned char* pY = targetY;

    unsigned char* pYSrc1 = sourceY;
    unsigned char* pYSrc2 = sourceY + sourceRowByte0;
    
    unsigned char* pVUSrc1 = sourceUV;
    unsigned char* pVUSrc2 = pVUSrc1 + sourceRowByte1;
    
    int i = 0;
    int j = 0;
    unsigned int tmp = 0;
    for(j = 0; j < mhight; j++){
        for(i = 0; i < mwidth; i++) {
            tmp = (pYSrc1[0] + pYSrc1[1] + pYSrc2[0] + pYSrc2[1])/4;
            *pY++ = clip(tmp);
            
            pYSrc1 += 2;
            pYSrc2 += 2;
        }
        
        pYSrc1 += OriWidth;
        pYSrc2 += OriWidth;
        
        pY +=(targetRowByter0 - mwidth);
    }
    
    int k = 0;
    for (i=0;i<mhight;i+=2) {
        for(j=0;j<mwidth;j+=2)
        {
            tmp = (pVUSrc1[0]+pVUSrc1[2]+pVUSrc2[0]+pVUSrc2[2])/4;
            targetUV[k++] = (unsigned char)clip(tmp);
            
            tmp = (pVUSrc1[1]+pVUSrc1[3]+pVUSrc2[1]+pVUSrc2[3])/4;
            targetUV[k++] = (unsigned char)clip(tmp);
            
            pVUSrc1 += 4;
            pVUSrc2 += 4;
        }
        
        k +=(targetRowByter1 - mwidth);
        
        pVUSrc1 +=  sourceRowByte1 + (sourceRowByte1 - OriWidth);
        pVUSrc2 +=  sourceRowByte1 + (sourceRowByte1 - OriWidth);
    }
    
    CVPixelBufferUnlockBaseAddress(mPixelBuffer,0);
    CVPixelBufferUnlockBaseAddress(imageBuffer,0);

}

void CGKVideoEncoder::resetEncoder(int framerate, int bitrate)
{
    /*mCtritData.Lock();
    mStartEncoder = false;
    mCtritData.UnLock();
    
    uninit();
    
    mframerate = framerate;
    mbitrate = bitrate;
    
    if (mPush) {
        mPush->setbitrate(mbitrate);
    }
    
    init();
    
    mCtritData.Lock();
    mStartEncoder = true;
    mCtritData.UnLock();*/
}

void CGKVideoEncoder::encoder(CMSampleBufferRef sampleBuffer, void *ihecHandle)
{
    VTEncodeInfoFlags flags;
    
    /*mCtritData.Lock();
    bool ret = mStartEncoder;
    mCtritData.UnLock();
    
    if (ret == false) {
        return;
    }*/
    
    TTUint64 pts = CAVTimeStamp::getCurrentTime();
    CMTime presentationTimeStamp = CMTimeMake(pts, 1000);
    
    OSStatus statusCode = 0;
    
    if (mwidth == 360 && mhight == 640) {
        
        mCtritData.Lock();
        
        if(mset640480 == true){
            transfer640480to640360(sampleBuffer);
        }
        else
            rescale_2(sampleBuffer);
        
        if (mEncodingSession != NULL)
            statusCode = VTCompressionSessionEncodeFrame(mEncodingSession,
                                                     mPixelBuffer,
                                                     presentationTimeStamp,
                                                     kCMTimeInvalid,
                                                     NULL, NULL, &flags);
         mCtritData.UnLock();
    }
    else{
        CVImageBufferRef imageBuffer = (CVImageBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
        
        mCtritData.Lock();
        if (mEncodingSession != NULL)
            statusCode = VTCompressionSessionEncodeFrame(mEncodingSession,
                                                     imageBuffer,
                                                     presentationTimeStamp,
                                                     kCMTimeInvalid,
                                                     NULL, NULL, &flags);
         mCtritData.UnLock();
    }
    
    // Check for error
    if (statusCode != noErr)
    {
        printf("H264: VTCompressionSessionEncodeFrame failed with %d", (int)statusCode);
        
        // End the session
        mCtritData.Lock();
        if (mEncodingSession != NULL) {
            VTCompressionSessionInvalidate(mEncodingSession);
            CFRelease(mEncodingSession);
            mEncodingSession = NULL;
        }
        mCtritData.UnLock();
        
        return;
    }
}

void CGKVideoEncoder::encoder2(CVPixelBufferRef PixelBuffer, void *ihecHandle, int cameraType)
{
    VTEncodeInfoFlags flags;
    
    /*mCtritData.Lock();
     bool ret = mStartEncoder;
     mCtritData.UnLock();
     
     if (ret == false) {
     return;
     }*/
    
    TTUint64 pts = CAVTimeStamp::getCurrentTime();
    CMTime presentationTimeStamp = CMTimeMake(pts, 1000);
    
    OSStatus statusCode = 0;
    
    if (mwidth == 360 && mhight == 640) {
        
        copy(PixelBuffer);
        if (cameraType == CAMEAR_FRONT) {
            //revert pic left to right
            revert(PixelBuffer);
        }
        
        mCtritData.Lock();
        
        if (mEncodingSession != NULL)
            statusCode = VTCompressionSessionEncodeFrame(mEncodingSession,
                                                         PixelBuffer,
                                                         presentationTimeStamp,
                                                         kCMTimeInvalid,
                                                         NULL, NULL, &flags);
        
        mCtritData.UnLock();
    }
    
    // Check for error
    if (statusCode != noErr)
    {
        printf("H264: VTCompressionSessionEncodeFrame failed with %d", (int)statusCode);
        
        // End the session
        mCtritData.Lock();
        if (mEncodingSession != NULL) {
            VTCompressionSessionInvalidate(mEncodingSession);
            CFRelease(mEncodingSession);
            mEncodingSession = NULL;
        }
        mCtritData.UnLock();
        
        return;
    }
 
}

//left-right revert!
void CGKVideoEncoder::revert(CVPixelBufferRef PixelBuffer)
{
    CVPixelBufferLockBaseAddress(PixelBuffer,0);
    
    unsigned char* pY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(PixelBuffer, 0);
    unsigned char* pVUSrc = (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(PixelBuffer, 1);
    size_t targetRowByter0  = CVPixelBufferGetBytesPerRowOfPlane(PixelBuffer, 0);
    
    int i = 0;
    int j = 0;
    unsigned char tmp = 0;
    unsigned char tmp1 = 0;
    for(j = 0; j < mhight; j++){
        for(i = 0; i < mwidth/2; i++) {
            tmp = pY[i];
            pY[i] = pY[mwidth-1-i];
            pY[mwidth-1-i] = tmp;
        }
        pY +=targetRowByter0;
    }
    
    //revert uv
    for(j = 0; j < mhight/2; j++){
        for(i = 0; i < mwidth/2; i+=2) {
            tmp = pVUSrc[i];
            tmp1 = pVUSrc[i+1];
            pVUSrc[i] = pVUSrc[mwidth-1-i-1];
            pVUSrc[i + 1] = pVUSrc[mwidth-1-i];
            pVUSrc[mwidth-1-i] = tmp1;
            pVUSrc[mwidth-1-i-1] = tmp;
        }
        pVUSrc +=targetRowByter0;
    }
    
    CVPixelBufferUnlockBaseAddress(PixelBuffer,0);
}

void CGKVideoEncoder::copy(CVPixelBufferRef PixelBuffer)
{
    CVPixelBufferLockBaseAddress(mPixelBuffer, 0);
    CVPixelBufferLockBaseAddress(PixelBuffer, 0);
    unsigned char* pY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(PixelBuffer, 0);
    unsigned char* pVUSrc = (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(PixelBuffer, 1);
    size_t targetRowByter0  = CVPixelBufferGetBytesPerRowOfPlane(PixelBuffer, 0);
    
    unsigned char* targetY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 0);
    unsigned char* targetUV=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 1);

    memcpy(targetY,pY,targetRowByter0*mhight);
    memcpy(targetUV,pVUSrc,targetRowByter0*mhight/2);
    
    CVPixelBufferUnlockBaseAddress(PixelBuffer,0);
    CVPixelBufferUnlockBaseAddress(mPixelBuffer,0);
}

void CGKVideoEncoder::uninit()
{
    mCtritData.Lock();
    if (mEncodingSession) {
        VTCompressionSessionCompleteFrames(mEncodingSession, kCMTimeInvalid);
        VTCompressionSessionInvalidate(mEncodingSession);
        CFRelease(mEncodingSession);
    }
    mEncodingSession = NULL;
    mCtritData.UnLock();
    
#ifdef DUMP_ENCODER_H264
    fwc->close();
    delete ihec->fwc;
#endif
}

void  CGKVideoEncoder::sendvideopacket(TTPBYTE arry,int size, long pts ,int flag)
{
    if (mPush) {
        mPush->sendvideopacket(arry,size,pts ,flag);
    }
}

void CGKVideoEncoder::setSpsPpsConfig(const uint8_t * sps, size_t spslen, const uint8_t * pps, size_t ppslen)
{
    SAFE_FREE(msbuf);
    SAFE_FREE(mpbuf);
    msbuf = (TTPBYTE)malloc(spslen);
    mpbuf = (TTPBYTE)malloc(ppslen);
    memcpy(msbuf,sps,spslen);
    memcpy(mpbuf,pps,ppslen);
    if (mPush) {
        mPush->SetSpsPpsConfig(msbuf,spslen,mpbuf,ppslen);
    }
}
/*
void CTTVideoEncoder::YUV2RGB(unsigned char *pYuvBuf,int *pRgbBuf,int nWidth,int nHeight)
{
    unsigned char *yBuf, *uBuf, *vBuf;
    unsigned char *uvBuf;
    int nHfWidth = (nWidth>>1);
    int rgb[4];
    int i, j, m, n, x, y, py, rdif, invgdif, bdif;
    char addhalf = 1;
    m = -nWidth;
    
    yBuf = pYuvBuf;
    
    n = -nHfWidth;
    uBuf = pYuvBuf + nWidth * nHeight;
    vBuf = pYuvBuf + nWidth * nHeight * 5/4;
    for(y=0; y<nHeight;y++)
    {
        m += nWidth;
        if(addhalf)
        {
            n+=nHfWidth;
            addhalf = 0;
        }
        else
        {
            addhalf = 1;
        }
        for(x=0; x<nWidth;x++)
        {
            i = m + x;
            j = n + (x>>1);
            py = yBuf[i];
            // search tables to get rdif invgdif and bidif
            rdif = Table_FV1[vBuf[j]];    // fv1
            invgdif = Table_FU1[uBuf[j]] + Table_FV2[vBuf[j]]; // fu1+fv2
            bdif = Table_FU2[uBuf[j]]; // fu2
            
            rgb[0] = clip(py+bdif);    // B
            rgb[1] = clip(py-invgdif); // G
            rgb[2] = clip(py+rdif);    // R
            
            i = y * nWidth + x;
            // copy this pixel to rgb data
            pRgbBuf[i] = 0xff000000 | (rgb[0]<<16)| (rgb[1]<< 8) | (rgb[2]) ;
        }
    }
}*/

void CGKVideoEncoder::NV12RGB(unsigned char *pYuvBuf,unsigned char *pRgbBuf,int nWidth,int nHeight,int padding)
{
    unsigned char *yBuf, *uBuf;
    int i, j, m, n, x, y, py, rdif, invgdif, bdif;
    char addhalf = 1;
    m = -nWidth;
    
    unsigned char *pRgbline = pRgbBuf;
    
    yBuf = pYuvBuf;
    
    n = -nWidth;
    uBuf = pYuvBuf + nWidth * nHeight;
    for(y=0; y<nHeight;y++)
    {
        m += nWidth;
        if(addhalf)
        {
            n+=nWidth;
            addhalf = 0;
        }
        else
        {
            addhalf = 1;
        }
        for(x=0; x<nWidth;x++)
        {
            i = m + x;
            j = n + ((x>>1) << 1);
            py = yBuf[i];
            // search tables to get rdif invgdif and bidif
            rdif = Table_FV1[uBuf[j +1]];    // fv1
            invgdif = Table_FU1[uBuf[j]] + Table_FV2[uBuf[j+1]]; // fu1+fv2
            bdif = Table_FU2[uBuf[j]]; // fu2
            
            i =  4*x;
            
            pRgbline[i] = clip(py+bdif);
            pRgbline[i+1] = clip(py-invgdif);
            pRgbline[i+2] = clip(py+rdif);
            pRgbline[i+3] = 0xff;
        }
        pRgbline += padding;
    }
}

unsigned char* CGKVideoEncoder::getRGB()
{
    unsigned char* pFrameData = new unsigned char[mwidth * mhight * 3 / 2];
    
    if (pFrameData == NULL || mlastRGB == NULL) {
        return NULL;
    }
    
    mCtritData.Lock();
    
    CVPixelBufferLockBaseAddress(mPixelBuffer, 0);
    
    unsigned char* targetY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 0);
    unsigned char* targetUV=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 1);
    size_t targetRowByter0  = CVPixelBufferGetBytesPerRowOfPlane(mPixelBuffer, 0);
    size_t targetRowByter1 = CVPixelBufferGetBytesPerRowOfPlane(mPixelBuffer, 1);
    
    if (targetRowByter0 == mwidth )
    {
        memcpy(pFrameData, targetY, mwidth * mhight*3/2);
    }
    else{
        //padding handle
        unsigned char* yuvpalne = pFrameData;
        for (int i= 0; i< mhight; i++) {
            memcpy(yuvpalne, targetY, mwidth);
            yuvpalne += mwidth;
            targetY += targetRowByter0;
        }
        
        //yuvpalne = m_pFrameData + m_nTextureWidth*m_nTextureHeight;
        for (int i= 0; i<mhight/2; i++) {
            memcpy(yuvpalne, targetUV, mwidth);
            yuvpalne += mwidth;
            targetUV += targetRowByter1;
        }
    }
    
    CVPixelBufferUnlockBaseAddress(mPixelBuffer,0);
    
    mCtritData.UnLock();
    
    NV12RGB(pFrameData,mlastRGB,mwidth,mhight,mwidth*4);
    
    free(pFrameData);
    
    /*NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docDir = [paths objectAtIndex:0];
    const char* tmp = [docDir UTF8String];
    char* strpath = new char[strlen(tmp)+64];
    sprintf(strpath, "%s/p1.pcm", tmp);
    
    FILE *f2 = fopen(strpath, "wb+");
    fwrite(mlastRGB, 1, mhight*mwidth*4, f2);
    fclose(f2);*/
    
    return mlastRGB;
}
