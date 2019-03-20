/*******************************************************************************
	File:		CVTBVideoDec.mm

	Contains:	implement code

	Written by:	Jun Lin

	Change History (most recent first):
	2017-03-01		Jun Lin			Create file

*******************************************************************************/
#include "qcErr.h"

#include "AVCDecoderTypes.h"
#include "ULogFunc.h"
#include "USystemFunc.h"
#include "CVTBVideoDec.h"
#include "CMsgMng.h"

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <VideoToolbox/VideoToolbox.h>
#import <CoreVideo/CoreVideo.h>
#include <pthread.h>

char* m_szObjName = (char*)"VTB";

#define MAX_INPUT_SIZE (2*1024*1024)
#define VTB_OUTPUT_FRAMES_CACHE 5
#define VTB_TRASH_FRAMES (VTB_OUTPUT_FRAMES_CACHE + 10)
#define VTB_OUTPUT_FRAMES_MAX (VTB_OUTPUT_FRAMES_CACHE + 1)
#define VTB_INPUT_SAMPLES_CACHE 7

#define VDA_RB24(x)                          \
((((const uint8_t*)(x))[0] << 16) |        \
(((const uint8_t*)(x))[1] <<  8) |        \
((const uint8_t*)(x))[2])

#define VDA_RB32(x)                          \
((((const uint8_t*)(x))[0] << 24) |        \
(((const uint8_t*)(x))[1] << 16) |        \
(((const uint8_t*)(x))[2] <<  8) |        \
((const uint8_t*)(x))[3])

#define VTB_CONTENT_COPY(content,length_c,source,length_s) \
{ \
length_c = length_s; \
content = (unsigned char*)malloc(length_c); \
memset(content, 0, length_c); \
memcpy(content, source, length_s); \
}


typedef enum
{
    NAL_UNIT_TYPE_SLICE    = 1, // slice_layer_without_partitioning_rbsp()
    NAL_UNIT_TYPE_DPA      = 2, // slice_data_partition_a_layer_rbsp()
    NAL_UNIT_TYPE_DPB      = 3, // slice_data_partition_b_layer_rbsp()
    NAL_UNIT_TYPE_DPC      = 4, // slice_data_partition_c_layer_rbsp()
    NAL_UNIT_TYPE_IDR      = 5, // slice_layer_without_partitioning_rbsp()
    NAL_UNIT_TYPE_SEI      = 6, // sei_rbsp()
    NAL_UNIT_TYPE_SPS      = 7, // seq_parameter_set_rbsp()
    NAL_UNIT_TYPE_PPS      = 8, // pic_parameter_set_rbsp()
    NAL_UNIT_TYPE_AUD      = 9, // access_unit_delimiter_rbsp()
    NAL_UNIT_TYPE_EOSEQ    = 10,// end_of_seq_rbsp()
    NAL_UNIT_TYPE_EOSTREAM = 11,// end_of_stream_rbsp()
    NAL_UNIT_TYPE_FILL     = 12,// filler_data_rbsp()
    NAL_UNIT_TYPE_PREFIX   = 14,// seq_parameter_set_extension_rbsp()
    NAL_UNIT_TYPE_SUB_SPS  = 15,
    NAL_UNIT_TYPE_SLC_EXT  = 20,
    NAL_UNIT_TYPE_VDRD     = 24
}NalUnitType;

typedef struct
{
    unsigned char*  content;
    unsigned int    length;
}VTBContent;


typedef struct
{
    VTDecompressionSessionRef           decompressionSession;
    CMVideoFormatDescriptionRef         formatDescription;
    CFMutableDictionaryRef              destinationImageBufferAttributes;
    VTDecompressionOutputCallbackRecord callBackRecord;
    VTBContent                          spsContent;
    VTBContent                          ppsContent;

    int                                 width;
    int                                 height;
    
    OSType                              pixFmt;
    SInt64                              dts;
    
    // input
    NSMutableArray*                     inputSamples;
    
    // output
    NSMutableArray*                     outputPresentationTimes;
    NSMutableArray*                     outputFrames;
    
    // recycle, delay to destroy output frames
    NSMutableArray*                     trashTimes;
    NSMutableArray*                     trashFrames;
    
    bool                                outputAll;
    long long                           outputCount;
    
    bool                                pauseDecode;
    bool                                threadRunning;
    pthread_t                           decodeFrameThread;
    
    NSObject*                           apiObj;
    //bool								waitIDR;
    
#ifndef _VONDBG
    FILE* m_dumpInput;
#endif
}VTBParams;

#pragma mark Parse video size
int Ue(unsigned char* pBuff, unsigned int nLen, unsigned int &nStartBit)
{
    //计算0bit的个数
    unsigned int nZeroNum = 0;
    
    while (nStartBit < nLen * 8)
    {
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余
            break;
        
        nZeroNum++;
        nStartBit++;
    }
    
    nStartBit ++;
    
    //计算结果
    unsigned long uRet = 0;
    
    for (unsigned int i=0; i<nZeroNum; i++)
    {
        uRet <<= 1;
    
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
        {
            uRet += 1;
        }
        
        nStartBit++;
    }
    
    return (int)((1 << nZeroNum) - 1 + uRet);
}


int Se(unsigned char* pBuff, unsigned int nLen, unsigned int &nStartBit)
{
    int UeVal = Ue(pBuff, nLen, nStartBit);
    
    double k = UeVal;
    int nValue = ceil(k/2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
    
    if (UeVal % 2==0)
        nValue = -nValue;
    
    return nValue;
}


int u(unsigned int nBitCount, unsigned char* buf, unsigned int &nStartBit)
{
    unsigned long uRet = 0;
    
    for (unsigned int i=0; i<nBitCount; i++)
    {
        uRet <<= 1;
        
        if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
        {
            uRet += 1;
        }
        
        nStartBit++;
    }
    
    return (int)uRet;
}

bool h264_parse_seq_parameter_set(unsigned char* buf, unsigned int nLen, int &nWidth, int &nHeight)
{
    unsigned int StartBit = 0;
    
    int forbidden_zero_bit  = u(1, buf, StartBit);
    int nal_ref_idc         = u(2, buf, StartBit);
    int nal_unit_type       = u(5, buf, StartBit);
    
    if(nal_unit_type == 7)
    {
        int profile_idc             = u(8, buf, StartBit);
        int constraint_set0_flag    = u(1, buf, StartBit);//(buf[1] & 0x80)>>7;
        int constraint_set1_flag    = u(1, buf, StartBit);//(buf[1] & 0x40)>>6;
        int constraint_set2_flag    = u(1, buf, StartBit);//(buf[1] & 0x20)>>5;
        int constraint_set3_flag    = u(1, buf, StartBit);//(buf[1] & 0x10)>>4;
        int reserved_zero_4bits     = u(4, buf, StartBit);
        int level_idc               = u(8, buf, StartBit);
        
        int seq_parameter_set_id    = Ue(buf, nLen, StartBit);
        
        if( profile_idc == 100 || profile_idc == 110 ||
           profile_idc == 122 || profile_idc == 144 )
        {
            int chroma_format_idc   = Ue(buf, nLen, StartBit);
            if( chroma_format_idc == 3 )
                int residual_colour_transform_flag = u(1, buf, StartBit);
            
            int bit_depth_luma_minus8                   = Ue(buf, nLen, StartBit);
            int bit_depth_chroma_minus8                 = Ue(buf, nLen, StartBit);
            int qpprime_y_zero_transform_bypass_flag    = u(1, buf, StartBit);
            int seq_scaling_matrix_present_flag         = u(1, buf, StartBit);
            
            int seq_scaling_list_present_flag[8];
            
            if( seq_scaling_matrix_present_flag )
            {
                for( int i = 0; i < 8; i++ )
                {
                    seq_scaling_list_present_flag[i] = u(1, buf, StartBit);
                }
            }
        }
        
        int log2_max_frame_num_minus4   = Ue(buf, nLen, StartBit);
        int pic_order_cnt_type          = Ue(buf, nLen, StartBit);
        
        if(pic_order_cnt_type == 0)
            int log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, StartBit);
        else if(pic_order_cnt_type == 1)
        {
            int delta_pic_order_always_zero_flag        = u(1, buf, StartBit);
            int offset_for_non_ref_pic                  = Se(buf, nLen, StartBit);
            int offset_for_top_to_bottom_field          = Se(buf, nLen, StartBit);
            int num_ref_frames_in_pic_order_cnt_cycle   = Ue(buf, nLen, StartBit);
            
            int *offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
            
            for( int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
                offset_for_ref_frame[i] = Se(buf, nLen, StartBit);
            
            delete []offset_for_ref_frame;
        }
        
        int num_ref_frames                          = Ue(buf, nLen, StartBit);
        int gaps_in_frame_num_value_allowed_flag    = u(1, buf, StartBit);
        int pic_width_in_mbs_minus1                 = Ue(buf, nLen, StartBit);
        int pic_height_in_map_units_minus1          = Ue(buf, nLen, StartBit);
        
        int frame_mbs_only_flag                     = u(1, buf, StartBit);
        if(frame_mbs_only_flag == 0)
            int mb_adaptive_frame_field_flag = u(1, buf, StartBit);
        
        int direct_8x8_inference_flag = u(1, buf, StartBit);
        int frame_cropping_flag = u(1, buf, StartBit);
        
        if(frame_cropping_flag == 1)
        {
            int frame_crop_left_offset = Ue(buf, nLen, StartBit);
            int frame_crop_right_offset = Ue(buf, nLen, StartBit);
            int frame_crop_top_offset = Ue(buf, nLen, StartBit);
            int frame_crop_bottom_offset = Ue(buf, nLen, StartBit);
            
            nWidth = ((pic_width_in_mbs_minus1 +1)*16) - frame_crop_right_offset*2 - frame_crop_left_offset*2;
            nHeight = ((2 - frame_mbs_only_flag)* (pic_height_in_map_units_minus1 +1) * 16) - (frame_crop_bottom_offset * 2) - (frame_crop_top_offset * 2);
        }
        else
        {
            nWidth   = (pic_width_in_mbs_minus1 + 1)*16;
            nHeight  = (pic_height_in_map_units_minus1 + 1)*16;
        }
        
        int vui_parameters_present_flag = u(1, buf, StartBit);
        
        return true;
    }
    
    return false;
}

#pragma mark Basic function
void createMutex(pthread_mutex_t* mtx)
{
    int                   rc = 0;
    pthread_mutexattr_t   mta;
    rc = pthread_mutexattr_init(&mta);
    rc = pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
    
    rc = pthread_mutex_init (mtx, &mta);
    rc = pthread_mutexattr_destroy(&mta);
}

void destroyMutex(pthread_mutex_t* mtx)
{
    if(mtx)
        pthread_mutex_destroy (mtx);
}


void dumpInput(VTBParams* pParams, unsigned char* data, int dataSize)
{
#if 0
    if(!pParams)
        return;
    
    if(!data)
    {
        if(pParams->m_dumpInput)
        {
            fclose(pParams->m_dumpInput);
            pParams->m_dumpInput = NULL;
        }
    }
    else
    {
        if(!pParams->m_dumpInput)
        {
            char szPath[1024];
            qcGetAppPath(NULL, szPath, 1024);
            strcat(szPath, "dump.stream");
            pParams->m_dumpInput = fopen(szPath, "wb");
        }
        
        if(pParams->m_dumpInput)
        {
            fwrite((const void*)data, 1, dataSize, pParams->m_dumpInput);
        }
    }
#endif
}


void write_b8(Byte **content, int val)
{
    *(*content)++ = val;
}

void write_b16(Byte **content, unsigned int val)
{
    write_b8(content, (int) val >> 8);
    write_b8(content, (uint8_t)val);
}

void write_b32(Byte **content, unsigned int val)
{
    write_b8(content, val >> 24);
    write_b8(content, (uint8_t)(val >> 16));
    write_b8(content, (uint8_t)(val >> 8));
    write_b8(content, (uint8_t)val);
}

void write_content(Byte **content, const Byte *buf, int size)
{
    memcpy(*content, buf, size);
    *content+=size;
}

#pragma mark Implement
void releaseOutputBuffer(VTBParams* pParams, bool bDestroyList)
{
    @synchronized(pParams->outputPresentationTimes)
    {
        NSUInteger len = 0;
        CVPixelBufferRef pixelBuffer = NULL;
        [pParams->outputPresentationTimes removeAllObjects];
        
        if(bDestroyList)
        {
            [pParams->outputPresentationTimes release];
            pParams->outputPresentationTimes = nil;
        }
        
        len = [pParams->outputFrames count];
        
        for (NSUInteger i = 0; i < len; i++)
        {
            pixelBuffer = (__bridge CVPixelBufferRef)[pParams->outputFrames objectAtIndex:i];
            if (NULL != pixelBuffer)
            {
                CVPixelBufferRelease(pixelBuffer);
                pixelBuffer = NULL;
            }
        }
        
        [pParams->outputFrames removeAllObjects];
        
        if(bDestroyList)
        {
            [pParams->outputFrames release];
            pParams->outputFrames = nil;
        }
        
        [pParams->trashTimes removeAllObjects];
        
        if(bDestroyList)
        {
            [pParams->trashTimes release];
            pParams->trashTimes = nil;
        }
        
        len = [pParams->trashFrames count];
        
        for (NSUInteger i = 0; i < len; i++)
        {
            pixelBuffer = (__bridge CVPixelBufferRef)[pParams->trashFrames objectAtIndex:i];
            if (NULL != pixelBuffer)
            {
                CVPixelBufferRelease(pixelBuffer);
                pixelBuffer = NULL;
            }
        }
        
        [pParams->trashFrames removeAllObjects];
        
        if(bDestroyList)
        {
            [pParams->trashFrames release];
            pParams->trashFrames = nil;
        }
    }
}


void releaseInputSample(VTBParams* pParams, bool bDestroyList)
{
    @synchronized(pParams->inputSamples)
    {
        if (pParams->inputSamples)
        {
            //QCLOGI("[VTB]+Release input sample, count %lu, handle %X, session %X", (unsigned long)[pParams->inputSamples count], pParams, pParams->decompressionSession);
            
            CMSampleBufferRef sampleBuffer = NULL;
            NSUInteger len = [pParams->inputSamples count];
            
            for (NSUInteger i = 0; i < len; i++)
            {
                sampleBuffer = (__bridge CMSampleBufferRef)[pParams->inputSamples objectAtIndex:i];
                
                if (NULL != sampleBuffer)
                {
                    CFRelease(sampleBuffer);
                    sampleBuffer = NULL;
                }
            }
            
            [pParams->inputSamples removeAllObjects];
            
            if(bDestroyList)
            {
                [pParams->inputSamples release];
                pParams->inputSamples = nil;
            }
            
            //QCLOGI("[VTB]-Release input sample, handle %X, session %X", pParams, pParams->decompressionSession);
        }
    }
}

CMVideoFormatDescriptionRef createVideoFormatDesccriptionRef(VTBParams* pParams)
{
    CMVideoFormatDescriptionRef video_format_desc = NULL;
    OSStatus status;
    
    if(NULL != pParams)
    {
        uint8_t* props[] = {(uint8_t *)pParams->spsContent.content, (uint8_t *)pParams->ppsContent.content};
        size_t sizes[] = {(size_t)pParams->spsContent.length, (size_t)pParams->ppsContent.length};
        
        status = CMVideoFormatDescriptionCreateFromH264ParameterSets(kCFAllocatorDefault,
                                                                     2,
                                                                     props,
                                                                     sizes,
                                                                     4,
                                                                     &video_format_desc);
        if (status)
        {
            return NULL;
        }
    }
    
    return video_format_desc;
}

CFMutableDictionaryRef createDestinationPixelBufferAttr(VTBParams* pParams)
{
    CFMutableDictionaryRef dstPixlBuffAttr = NULL;
    CFMutableDictionaryRef io_surface_prop = NULL;
    
    CFNumberRef cv_pixFmt = NULL;
    CFNumberRef cv_width = NULL;
    CFNumberRef cv_height = NULL;
    
    if(NULL != pParams)
    {
        cv_width = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pParams->width);
        cv_height = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pParams->height);
        cv_pixFmt = CFNumberCreate(kCFAllocatorDefault,
                                    kCFNumberSInt32Type,
                                    &pParams->pixFmt);
        
        io_surface_prop = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                    0,
                                                    &kCFTypeDictionaryKeyCallBacks,
                                                    &kCFTypeDictionaryValueCallBacks);
        
        dstPixlBuffAttr = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                    4,
                                                    &kCFTypeDictionaryKeyCallBacks,
                                                    &kCFTypeDictionaryValueCallBacks);
        
        CFDictionarySetValue(dstPixlBuffAttr, kCVPixelBufferPixelFormatTypeKey, cv_pixFmt);
        
        if (kCVPixelFormatType_420YpCbCr8BiPlanarFullRange == pParams->pixFmt)
        {
            CFDictionarySetValue(dstPixlBuffAttr, kCVPixelBufferOpenGLESCompatibilityKey, kCFBooleanFalse);
        }
        else
        {
            CFDictionarySetValue(dstPixlBuffAttr, kCVPixelBufferIOSurfacePropertiesKey, io_surface_prop);
        }
        
        CFDictionarySetValue(dstPixlBuffAttr, kCVPixelBufferWidthKey, cv_width);
        CFDictionarySetValue(dstPixlBuffAttr, kCVPixelBufferHeightKey, cv_height);
        
        CFRelease(io_surface_prop);
        CFRelease(cv_pixFmt);
        CFRelease(cv_width);
        CFRelease(cv_height);
    }
    
    return dstPixlBuffAttr;
}

void didDecompress(void* decompressionOutputRefCon, void* sourceFrameRefCon, OSStatus status, VTDecodeInfoFlags infoFlags, CVImageBufferRef imageBuffer, CMTime presentationTimeStamp, CMTime presentationDuration)
{
    VTBParams* pParams = (VTBParams *)decompressionOutputRefCon;
    
//    int begin_time = qcGetSysTime();
    
    if ((status == noErr) && (NULL != pParams))
    {
        @synchronized(pParams->outputPresentationTimes)
        {
            if (imageBuffer != NULL)
            {
                NSNumber* framePTS      = nil;
                id imageBufferObject    = nil;
                
                if (CMTIME_IS_VALID(presentationTimeStamp))
                {
                    framePTS = [NSNumber numberWithLongLong:CMTimeGetSeconds(presentationTimeStamp)];
                }
                else
                {
                    QCLOGI("[VTB]Invalid timestamp");
                    return;
                }
                
                // find the correct position for this frame in the output frames array
                imageBufferObject = (__bridge id)imageBuffer;
                
                CVPixelBufferRetain(imageBuffer);
                
                [pParams->outputPresentationTimes addObject:framePTS];
                [pParams->outputFrames addObject:imageBufferObject];
                
//                QCLOGI("[VTB] Add output info - (%ld - %ld), time %lld, handle %X, session %X",
//                       [pParams->outputPresentationTimes count], [pParams->outputFrames count],
//                       [framePTS longLongValue], pParams, pParams->decompressionSession);
                
            }
        } // @synchronized
    }
    else
    {
        if (CMTIME_IS_VALID(presentationTimeStamp))
        {
            NSNumber* framePTS = [NSNumber numberWithLongLong:CMTimeGetSeconds(presentationTimeStamp)];
            QCLOGE("[VTB]Decode fail, time: %lld, error: %d, infoFlags: 0x%x, handle %X", [framePTS longLongValue], status, infoFlags, pParams);
        }
        else
        {
            QCLOGE("[VTB]Decode fail, invalid timestamp, handle %X", pParams);
        }
    }
    
//    int end_time = qcGetSysTime();
//    QCLOGI("didDecompress time elapse = %d", end_time - begin_time);
}

void stopDecodeThread(VTBParams* pParams)
{
    if (!pParams || !pParams->decodeFrameThread)
        return;
    
    int sleepTime = 2*1000;
    int totalWaitTime = 0;
    int waitTimeThreshold = 5000;
    
    pParams->threadRunning = false;
    pParams->pauseDecode = false;
    
    while (pParams->decodeFrameThread)
    {
        qcSleep(sleepTime);
        totalWaitTime += sleepTime;
        
        if(totalWaitTime > waitTimeThreshold)
            break;
    }
    
    QCLOGI("[VTB]Decode is stopped, wait time: %d, result: %s", totalWaitTime/1000, totalWaitTime>waitTimeThreshold?"NOT SAFE":"SAFE");
}

void createVTB(VTBParams* pParams)
{
    @synchronized (pParams->apiObj)
    {
        if(pParams->formatDescription && pParams->destinationImageBufferAttributes)
            return;
        
        pParams->formatDescription = createVideoFormatDesccriptionRef(pParams);
        pParams->destinationImageBufferAttributes = createDestinationPixelBufferAttr(pParams);
        
        pParams->callBackRecord.decompressionOutputCallback = didDecompress;
        pParams->callBackRecord.decompressionOutputRefCon = (void*)pParams;
        
        OSStatus status = VTDecompressionSessionCreate(kCFAllocatorDefault, pParams->formatDescription, NULL, pParams->destinationImageBufferAttributes, &pParams->callBackRecord, &pParams->decompressionSession);
        
        pParams->outputAll = false;
        pParams->outputCount = 0;
        
        if(0 != status)
            QCLOGE("[VTB]Create VTB failed");
    }
}

void destroyVTB(VTBParams* pParams)
{
    @synchronized (pParams->apiObj)
    {
        if (NULL != pParams->formatDescription)
        {
            CFRelease(pParams->formatDescription);
            pParams->formatDescription = NULL;
        }
        
        if (NULL != pParams->destinationImageBufferAttributes)
        {
            CFRelease(pParams->destinationImageBufferAttributes);
            pParams->destinationImageBufferAttributes = NULL;
        }
        
        if (NULL != pParams->decompressionSession)
        {
            VTDecompressionSessionInvalidate(pParams->decompressionSession);
            CFRelease(pParams->decompressionSession);
            pParams->decompressionSession = NULL;
        }
    }
}

bool isIDR(unsigned char* pBuff, unsigned int uSize)
{
    unsigned char* curr_nal = NULL;
    
    curr_nal = pBuff;
    
    while ((int)(curr_nal - pBuff) < uSize)
    {
        if (VDA_RB32(curr_nal) == 0x00000001)
        {
            if (NAL_UNIT_TYPE_IDR  == (*(curr_nal + 4) & 0x1f))
            {
                QCLOGI("[VTB]Found IDR");
                return true;
            }
        }
        
        curr_nal++;
    }
    
    return false;
}

bool isHeadData(unsigned char* pBuff, unsigned int uSize)
{
    unsigned char* curr_nal = NULL;
    int flag = 0; //1 is sps, 2 is pps
    
    curr_nal = pBuff;
    
    while ((int)(curr_nal - pBuff) < uSize)
    {
        if (VDA_RB32(curr_nal) == 0x00000001)
        {
            if (NAL_UNIT_TYPE_SPS  == (*(curr_nal + 4) & 0x1f))
            {
                flag |= 1;
            }
            
            if (NAL_UNIT_TYPE_PPS == (*(curr_nal + 4) & 0x1f))
            {
                flag |= 2;
            }
            
            if(flag&1 && flag&2)
            {
                QCLOGI("[VTB]Found head data. %d", flag);
                return true;
            }
            
            curr_nal += 4;
        }
        else
        {
            curr_nal++;
        }
    }
    
    return false;
}

bool isHeadData(VTBParams* pParams, QC_DATA_BUFF* buffer)
{
    if(!buffer)
        return false;
    
    return isHeadData(buffer->pBuff, buffer->uSize);
}

void preprocessHeadData(VTBParams* pParams, QC_DATA_BUFF* buffer)
{
    if(!buffer)
        return;
    
    unsigned char* curr_nal = NULL;
    unsigned char* pps_start = NULL;
    unsigned char* sps_start = NULL;
    unsigned char* pps_append = NULL;
    int pps_append_size = -1;
    int pps_size = -1;
    int sps_size = -1;
    int flag = 0; //1 is sps, 2 is pps
    
    QCLOGI("[VTB]Head data comes, length %d", buffer->uSize);
    
    curr_nal = buffer->pBuff;
    
    while ((int)(curr_nal - buffer->pBuff) < buffer->uSize)
    {
        if (VDA_RB32(curr_nal) == 0x00000001)
        {
            if (NAL_UNIT_TYPE_SPS  == (*(curr_nal + 4) & 0x1f))
            {
                flag = 1;
                sps_start = curr_nal + 4;
            }
            
            if (NAL_UNIT_TYPE_PPS == (*(curr_nal + 4) & 0x1f))
            {
                flag = 2;
                
                if (NULL == pps_start)
                {
                    pps_start = curr_nal + 4;
                }
                else
                {
                    pps_append = curr_nal + 4;
                }
            }
            
            curr_nal += 4;
        }
        else
        {
            curr_nal++;
        }
        
        if (1 == flag)
        {
            sps_size++;
        }
        else if (2 == flag)
        {
            if (NULL == pps_append)
            {
                pps_size++;
            }
            else
            {
                pps_append_size++;
            }
        }
    }
    
    if (sps_size > 0)
    {
        QCLOGI("[VTB]SPS start: %X", *sps_start);
        
        if (pParams->spsContent.content)
        {
            free(pParams->spsContent.content);
        }
        
        VTB_CONTENT_COPY(pParams->spsContent.content, pParams->spsContent.length,sps_start, sps_size)
    }
    
    if (pps_size > 0)
    {
        QCLOGI("[VTB]PPS start: %X", *pps_start);
        
        if (NULL == pParams->ppsContent.content)
        {
            if (NULL == pps_append)
            {
                VTB_CONTENT_COPY(pParams->ppsContent.content, pParams->ppsContent.length,pps_start, pps_size)
            }
            else
            {
                VTB_CONTENT_COPY(pParams->ppsContent.content, pParams->ppsContent.length,pps_start, (pps_size + pps_append_size))
                
                if ((pps_append_size > 0) && pps_append)
                {
                    memcpy((pParams->ppsContent.content+pps_size), pps_append, pps_append_size);
                }
            }
        }
        else
        {
            //pps content change
            free(pParams->ppsContent.content);
            
            if (NULL == pps_append)
            {
                VTB_CONTENT_COPY(pParams->ppsContent.content, pParams->ppsContent.length,pps_start, pps_size)
            }
            else
            {
                VTB_CONTENT_COPY(pParams->ppsContent.content, pParams->ppsContent.length,pps_start, (pps_size + pps_append_size))
                
                if ((pps_append_size > 0) && pps_append)
                {
                    memcpy((pParams->ppsContent.content+pps_size), pps_append, pps_append_size);
                }
            }
        }
    }
    
    h264_parse_seq_parameter_set(pParams->spsContent.content, pParams->spsContent.length, pParams->width, pParams->height);
}


bool isCanDecode(VTBParams* pParams)
{
    @synchronized(pParams->outputPresentationTimes)
    {
        // YUV buffer is full
        if (([pParams->outputFrames count] > VTB_OUTPUT_FRAMES_MAX) && (!pParams->outputAll))
        {
            //QCLOGW("[VTB]YUV buffer is full, %lu", [pParams->outputFrames count]);
            return false;
        }
    }
    
    @synchronized(pParams->inputSamples)
    {
        // H264 buffer is empty
        if ([pParams->inputSamples count] <= 0)
            return false;
    }
    
    return true;
}

int decode(VTBParams* pParams)
{
    if(!isCanDecode(pParams))
        return 1;
    
    id sampleObject = nil;
    CMSampleBufferRef sample_buf = NULL;
    
    @synchronized(pParams->inputSamples)
    {
        //QCLOGI("[VTB]+Remove input sample, count %d, handle %X, session %X", [pParams->inputSamples count], pParams, pParams->decompressionSession);
        sampleObject = [pParams->inputSamples firstObject];
        sample_buf = (__bridge CMSampleBufferRef)sampleObject;
        
        [pParams->inputSamples removeObjectAtIndex:0];
    }
    
    if (!sample_buf)
        return 1;
    
    @synchronized(pParams->apiObj)
    {
//        CMTime presentationTimeStamp = CMSampleBufferGetPresentationTimeStamp(sample_buf);
//        long long timeStamp = (presentationTimeStamp.value) / presentationTimeStamp.timescale;
//        QCLOGI("[VTB][IO]Input frame into decoder, time %lld", timeStamp);
//        
//        int useTime = qcGetSysTime();
        OSStatus status = noErr;
        VTDecodeInfoFlags flagOut;
        status = VTDecompressionSessionDecodeFrame(pParams->decompressionSession, sample_buf,
                                                   kVTDecodeFrame_EnableAsynchronousDecompression, NULL, &flagOut);
        
        if(status == kVTInvalidSessionErr)
        {
            QCLOGW("[VTB]VTB session is invalid, re-create it");
            destroyVTB(pParams);
            createVTB(pParams);
        }
        
        //QCLOGI("[VTB]-Decoder input use time %d, return code %d", qcGetSysTime()-useTime, status);
        CFRelease(sample_buf);
        sample_buf = NULL;
        
        return 0;
    }
    
    return 1;
}

unsigned int decodeFrameProc(void* pParam)
{
    VTBParams* pParams = (VTBParams*)pParam;
    
    while (pParams->threadRunning)
    {
        if (pParams->pauseDecode)
        {
            qcSleep(2000);
            continue;
        }
        
        if ( 0 != decode(pParams))
        {
            qcSleep(1000);
        }
    }
    
    QCLOGI("[VTB]Decode thread exit!");
    pParams->decodeFrameThread = NULL;
    
    return QC_ERR_NONE;
}


void createDecoderThread(VTBParams* pParams)
{
    if (pParams->decodeFrameThread)
        return;
    
    pthread_attr_t  attr;
    pthread_t       posixThreadID = 0;
    int             returnVal = -1;
    
    pParams->threadRunning = true;
    pParams->pauseDecode = false;
    returnVal = pthread_attr_init(&attr);
    returnVal = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    pthread_create(&posixThreadID, &attr, (void*(*)(void*))decodeFrameProc, pParams);
    pthread_attr_destroy(&attr);
    
    pthread_detach(posixThreadID);
    pParams->decodeFrameThread = posixThreadID;
}

unsigned char* findStartCode(unsigned char* input, int inputSize, unsigned char** outputStart, unsigned int* outputSize, int* startCodeSize)
{
    unsigned char* tmp    = input;
    unsigned char* stop   = input + inputSize - 4;
    
    while (tmp < stop)
    {
        if(tmp[0] == 0 && tmp[1] == 0 )
        {
            if(tmp[2] == 0 && tmp[3] == 1)
            {
                *startCodeSize = 4;
                return tmp;
            }
            else if(tmp[2] == 1)
            {
                *startCodeSize = 3;
                return tmp;
            }
        }
        
        tmp += 1;
    }
    
    return NULL;
}

void populateBuffer(VTBParams* pParams, QC_DATA_BUFF* pInput, unsigned char** frame_content, unsigned int* frame_size)
{
    unsigned char* input    = pInput->pBuff;
    unsigned int inputSize  = pInput->uSize;
    unsigned char* end      = pInput->pBuff + pInput->uSize;
    
    int startCodeSize = 0;
    int packetSize  = 0;
    int packetCount = 0;
    
    unsigned char* found      = NULL;
    
    *frame_size     = 0;
    *frame_content  = (unsigned char*)malloc(inputSize+16);
    
    if(!*frame_content)
        return;
    
    found = findStartCode(input, inputSize, NULL, NULL, &startCodeSize);
    
    while(found)
    {
        int tmpStartCodeSize = startCodeSize;
        
        unsigned char* next = findStartCode(found+4, inputSize-4, NULL, NULL, &startCodeSize);
        
        unsigned char* curr = (*frame_content) + *frame_size;
        
        if(next)
        {
            packetSize = next - found - tmpStartCodeSize;
            
            write_b32(&curr, packetSize);
            write_content(&curr, found+tmpStartCodeSize, packetSize);
            
            inputSize -= tmpStartCodeSize+packetSize;
            found = next;
        }
        else
        {
            packetSize = end - found - tmpStartCodeSize;
            
            write_b32(&curr, packetSize);
            write_content(&curr, found+tmpStartCodeSize, packetSize);
            
            // exit loop
            found = NULL;
        }
        
        packetCount++;
        //QCLOGI("[VTB]Packet found %d, %d, %lld", packetSize, packetCount, pInput->llTime);
        (*frame_size) += (packetSize + 4);
    }
}

unsigned int vtbInit (void** phDec, int nWidth, int nHeight)
{
    QCLOGI("[VTB]+Create VTB wrapper, device name:%s, model:%s, OS name:%s, OS version:%s", [[[UIDevice currentDevice] name] UTF8String],
           [[[UIDevice currentDevice] model] UTF8String],
           [[[UIDevice currentDevice] systemName] UTF8String],
           [[[UIDevice currentDevice] systemVersion]UTF8String]);
    
    if([[UIDevice currentDevice] systemVersion].floatValue < 8.0)
    {
        QCLOGE("[VTB]Does not support due to OS version is low than 8.0, %f", [[UIDevice currentDevice] systemVersion].floatValue);
        return QC_ERR_UNSUPPORT;
    }
    
    unsigned int returnCode = QC_ERR_NONE;
    VTBParams* pParams = NULL;
    
    pParams = (VTBParams*)malloc(sizeof(VTBParams));
    
    if(NULL == pParams)
        return QC_ERR_MEMORY;
    
    memset((void*)pParams, 0, sizeof(VTBParams));
    
    pParams->apiObj = [[NSObject alloc] init];
    
    //kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
    // YUV420: kCVPixelFormatType_420YpCbCr8Planar;
    // NV12:   kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
    pParams->pixFmt = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
    
    pParams->outputAll          = false;
    pParams->width              = nWidth;
    pParams->height             = nHeight;
    
    pParams->inputSamples       =       [[NSMutableArray alloc] init];
    pParams->outputFrames       =       [[NSMutableArray alloc] init];
    pParams->outputPresentationTimes =  [[NSMutableArray alloc] init];
    pParams->trashFrames =              [[NSMutableArray alloc] init];
    pParams->trashTimes =               [[NSMutableArray alloc] init];
    
    *phDec = (void*)pParams;
    
    QCLOGI("[VTB]-Create VTB wrapper, output type: %s",  (pParams->pixFmt == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange) ? "NV12":"YUV420");
    return returnCode;
}

unsigned int vtbSetInputData (void* hDec, QC_DATA_BUFF * pInput)
{
    OSStatus status;
    unsigned int ret                = QC_ERR_NONE;
    CMBlockBufferRef block_buf      = NULL;
    CMSampleBufferRef sample_buf    = NULL;
    CMSampleTimingInfo timeInfo     = {0};
    Byte* frame_content             = NULL;
    unsigned int frame_size         = 0;
    VTBParams* pParams              = (VTBParams*)hDec;
    
    if ((NULL == pParams) || (NULL == pInput) || (pInput->uSize > MAX_INPUT_SIZE) || (pInput->uSize == 0))
        return QC_ERR_ARG;
    
    //QCLOGI("[VTB][IO]+Input frame into decoder,   time %lld, size %08d", pInput->llTime, pInput->uSize);
    
    @synchronized(pParams->apiObj)
    {
        if (!pParams->decompressionSession)
        {
            return QC_ERR_RETRY;
        }
    }
    
    @synchronized(pParams->inputSamples)
    {
        if ([pParams->inputSamples count] > VTB_INPUT_SAMPLES_CACHE)
        {
            // disable max input count
//            qcSleep(2000);
//            QCLOGI("[VTB]Input buffer is full, total input %lu, YUV buffer %lu, total output %lld, time %lld",
//                   (unsigned long)[pParams->inputSamples count], [pParams->outputFrames count], pParams->outputCount, pInput->llTime);
//            return QC_ERR_RETRY;
        }
    }

    dumpInput(pParams, pInput->pBuff, pInput->uSize);
    
    populateBuffer(pParams, pInput, &frame_content, &frame_size);
    
    if(!frame_content || frame_size<=0)
    {
        QCLOGI("[VTB]Frame might be invalid, drop it, time %lld, size %d", pInput->llTime, pInput->uSize);
        return QC_ERR_NONE;
    }
    
    status = CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault,
                                                frame_content,
                                                (size_t)frame_size,
                                                NULL,//kCFAllocatorNull,
                                                NULL,
                                                0,
                                                (size_t)frame_size,
                                                0,
                                                &block_buf);
    
    if ((status != kCMBlockBufferNoErr) || !block_buf)
    {
        QCLOGW("[VTB]Block buffer alloc fail, drop it, time %lld, size %d, RC %d", pInput->llTime, pInput->uSize, status);
        
        if (frame_content)
        {
            free(frame_content);
            frame_content = NULL;
        }
        return QC_ERR_NONE;
    }
    
    pParams->dts++;
    
    CMTime PTime = CMTimeMake(pInput->llTime, 1);
    CMTime DTime = CMTimeMake(pParams->dts, 1);
    CMTime duration = CMTimeMake(1, 30);
    
    timeInfo.presentationTimeStamp = PTime;
    timeInfo.duration =  duration;
    timeInfo.decodeTimeStamp = DTime;
    
    const size_t sampleSizeArray[] = {(size_t)frame_size};
    
    //QCLOGI("[VTB][IO]-Input frame pre-processing, time %lld, size %08d, diff %d, DTS %lld", pInput->llTime, frame_size, pInput->uSize-frame_size, pParams->dts);
    
    status = CMSampleBufferCreate(kCFAllocatorDefault,
                                  block_buf,
                                  TRUE,
                                  0,
                                  0,
                                  pParams->formatDescription,
                                  1,
                                  1,
                                  &timeInfo,
                                  1,
                                  sampleSizeArray,
                                  &sample_buf);
    
    CFRelease(block_buf);
    block_buf = NULL;
    
    if ((status != noErr) || !sample_buf)
    {
        QCLOGI("[VTB]Sample buffer alloc fail, drop it, time %lld, size %d, RC %d", pInput->llTime, pInput->uSize, status);
        pParams->dts--;
        return QC_ERR_NONE;
    }
    
    @synchronized(pParams->inputSamples)
    {
        [pParams->inputSamples addObject:(__bridge id)sample_buf];
        //QCLOGI("[VTB]-Add input sample, count %d, handle %X, session %X", [pParams->inputSamples count], pParams, pParams->decompressionSession);
    }
    
    createDecoderThread(pParams);
    
    return ret;
}

unsigned int vtbGetOutputData (void* hDec, QC_DATA_BUFF* pOutput, QC_VIDEO_BUFF* pInfo)
{
    VTBParams* pParams = (VTBParams*)hDec;
    unsigned int ret = QC_ERR_NONE;
    NSNumber* framePTS = nil;
    id imageBufferObject = nil;
    id trash_imageBufferObject = nil;
    CVPixelBufferRef pixelBuffer = NULL;
    CVPixelBufferRef trash_pixelBuffer = NULL;
    
    if ((NULL == pParams) || (NULL == pOutput))
    {
        QCLOGI("[VTB] GetOutputData, param is null!");
        return QC_ERR_ARG;
    }
    
    //unsigned int begin_time = voOS_GetSysTime();
    @synchronized(pParams->outputPresentationTimes)
    {
        if ((NULL == pParams->outputPresentationTimes) ||
            (NULL == pParams->outputFrames) ||
            (NULL == pParams->trashTimes) ||
            (NULL == pParams->trashFrames))
        {
            QCLOGI("[VTB] GetOutputData, Invalid output and trash buffer list - (%X, %X, %X, %X)", pParams->outputPresentationTimes, pParams->outputFrames, pParams->trashTimes, pParams->trashFrames);
            return QC_ERR_RETRY;
        }
        
        if (0 == [pParams->outputPresentationTimes count] ||
            0 == [pParams->outputFrames count])
        {
            //QCLOGI("[VTB] GetOutputData, buffer count is 0!");
            pParams->outputAll = false;
            return QC_ERR_STATUS;
        }
        
        if ((VTB_OUTPUT_FRAMES_CACHE > [pParams->outputPresentationTimes count]) &&
            (!pParams->outputAll))
        {
            //QCLOGI("[VTB] Need more, buffer count is %d!", [pParams->outputPresentationTimes count]);
            return QC_ERR_NEEDMORE;
        }
        
//        QCLOGI("[VTB]+ output info, (%ld, %ld, %ld, %ld), handle %X, session %X",
//               [pParams->outputFrames count], [pParams->outputPresentationTimes count], [pParams->trashTimes count], [pParams->trashFrames count], pParams, pParams->decompressionSession);
        
        // ===== find earliest frame ======
        framePTS = [pParams->outputPresentationTimes lastObject];
        NSInteger nFindIndex = 0;
        NSInteger nIndex = [pParams->outputPresentationTimes count] - 1;
        while (nIndex >= 0)
        {
            if ([framePTS longLongValue] >= [pParams->outputPresentationTimes[nIndex] longLongValue])
            {
                framePTS = pParams->outputPresentationTimes[nIndex];
                nFindIndex = nIndex;
            }
            
            nIndex--;
        }
        // =======================
        
        //QCLOGI("[VTB] the earliest pts nFindIndex:%d", nFindIndex);
        imageBufferObject = [pParams->outputFrames objectAtIndex:nFindIndex];
        
        if ((framePTS != nil) && (imageBufferObject != nil))
        {
            // step 1, set output data
            {
                pOutput->llTime = [framePTS longLongValue];
                
                pixelBuffer = (__bridge CVPixelBufferRef)imageBufferObject;
                
                CVReturn nCVRet = CVPixelBufferLockBaseAddress(pixelBuffer, 0);
                
                if (nCVRet == kCVReturnSuccess)
                {
                    if (CVPixelBufferIsPlanar(pixelBuffer))
                    {
                        size_t count = CVPixelBufferGetPlaneCount(pixelBuffer);
                        for (size_t i = 0; i < count; i++)
                        {
                            pInfo->nStride[i]   = (int)CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, i);
                            pInfo->pBuff[i]     = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, i);
                        }
                    }
                    else
                    {
                        pInfo->nStride[0] = (int)CVPixelBufferGetBytesPerRow(pixelBuffer);
                    }

                    if(pParams->pixFmt == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange)
                    {
//                        int type = CVPixelBufferGetPixelFormatType(pixelBuffer);
//                        QCLOGI("Pixcel format type %d, %d", type, kCVPixelFormatType_420YpCbCr8BiPlanarFullRange);
                        pInfo->nType = QC_VDT_NV12;
                        // Overwrite pInfo->pBuff[0]
                        pInfo->pBuff[0] = (unsigned char*)pixelBuffer;
                        CVPixelBufferRetain(pixelBuffer);
                    }
                    else if(pParams->pixFmt == kCVPixelFormatType_420YpCbCr8Planar)
                        pInfo->nType = QC_VDT_YUV420_P;
                    
                    pInfo->nWidth = pParams->width;//(int)CVPixelBufferGetWidthOfPlane(pixelBuffer, 0);
                    pInfo->nHeight = (int)CVPixelBufferGetHeightOfPlane(pixelBuffer, 0);

                    nCVRet = CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
                    
                    if (nCVRet != kCVReturnSuccess)
                    {
                        [pParams->outputPresentationTimes removeObjectAtIndex:nFindIndex];
                        [pParams->outputFrames removeObjectAtIndex:nFindIndex];
                        QCLOGI("[VTB] Output decoded frame - CVPixelBufferUnlockBaseAddress error, nCVRet:%d, pts:%lld", nCVRet, [framePTS longLongValue]);
                        return QC_ERR_RETRY;
                    }
                }
                else
                {
                    [pParams->outputPresentationTimes removeObjectAtIndex:nFindIndex];
                    [pParams->outputFrames removeObjectAtIndex:nFindIndex];
                    QCLOGI("[VTB] Output decoded frame - CVPixelBufferLockBaseAddress error, nCVRet:%d, pts:%lld", nCVRet, [framePTS longLongValue]);
                    return QC_ERR_RETRY;
                }
                
                //QCLOGI("[VTB]Output frame info, time:%lld", pOutput->llTime);
            }
            
            // step 2, add the frame into trash_array
            {
                [pParams->trashTimes addObject:framePTS];
                [pParams->trashFrames addObject:imageBufferObject];
            }
            
            // step 3, remove and release frame info
            // NOTE: this step must be after step 2
            {
                [pParams->outputPresentationTimes removeObjectAtIndex:nFindIndex];
                [pParams->outputFrames removeObjectAtIndex:nFindIndex];
                
                if (pParams->outputCount > VTB_TRASH_FRAMES)
                {
                    trash_imageBufferObject = [pParams->trashFrames firstObject];
                    trash_pixelBuffer = (__bridge CVPixelBufferRef)trash_imageBufferObject;
                    
                    [pParams->trashTimes removeObjectAtIndex:0];
                    [pParams->trashFrames removeObjectAtIndex:0];
                    
                    if (NULL != trash_pixelBuffer)
                    {
                        CVPixelBufferRelease(trash_pixelBuffer);
                        trash_pixelBuffer = NULL;
                    }
                }
            }
            
//            QCLOGI("[VTB]- output info, %lld, (%ld, %ld, %ld, %ld), handle %X, session %X", pOutput->llTime,
//                   [pParams->outputFrames count], [pParams->outputPresentationTimes count], [pParams->trashTimes count], [pParams->trashFrames count], pParams, pParams->decompressionSession);
            
        } // if (framePTS != nil)
    } // @synchronized
    
    pParams->outputCount++;
    return ret;
}

unsigned int vtbFlush(void* hDec)
{
    VTBParams* pParams = (VTBParams*)hDec;
    
    stopDecodeThread(pParams);
    //destroyVTB(pParams);
    
    releaseOutputBuffer(pParams, false);
    releaseInputSample(pParams, false);

    pParams->outputCount = 0;
    
//    destroyVTB(pParams);
//    createVTB(pParams);

    return QC_ERR_NONE;
}

unsigned int vtbForceOutputAll(void* hDec)
{
    VTBParams* pParams = (VTBParams*)hDec;
    
    pParams->outputAll = true;
    
    return QC_ERR_NONE;
}

unsigned int vtbSetHeadData (void* hDec, unsigned char* pBuff, int nBufferSize)
{
    VTBParams* pParams = (VTBParams*)hDec;
    
    if(!isHeadData(pBuff, nBufferSize))
        return QC_ERR_ARG;
    
    dumpInput(pParams, NULL, 0);
    dumpInput(pParams, pBuff, nBufferSize);
    
    QC_DATA_BUFF buff;
    buff.pBuff = pBuff;
    buff.uSize = nBufferSize;
    preprocessHeadData(pParams, &buff);
    
    destroyVTB(pParams);
    createVTB(pParams);

    return QC_ERR_NONE;
}


unsigned int vtbUninit(void* hDec)
{
    //QCLOGI("[VTB]+Destroy VTB wrapper");
    
    VTBParams* pParams = (VTBParams*)hDec;
    if (NULL != pParams)
    {
        stopDecodeThread(pParams);
        destroyVTB(pParams);
        
        releaseOutputBuffer(pParams, true);
        
        if (pParams->ppsContent.content)
        {
            free(pParams->ppsContent.content);
            pParams->ppsContent.content = NULL;
        }
        
        if (pParams->spsContent.content)
        {
            free(pParams->spsContent.content);
            pParams->ppsContent.content = NULL;
        }
        
        releaseInputSample(pParams, true);
        
        if (pParams->apiObj)
        {
            [pParams->apiObj release];
            pParams->apiObj = nil;
        }
        
        dumpInput(pParams, NULL, 0);
        
        free(hDec);
        hDec = NULL;
    }
    
    //QCLOGI("[VTB]-Destroy VTB wrapper");
    
    return QC_ERR_NONE;
}

int  qcGetVTBDecAPI (VTB_DECAPI* pDec, unsigned int uFlag)
{
    if (pDec == NULL)
        return QC_ERR_ARG;
    
    pDec->Init			= vtbInit;
    pDec->GetOutputData	= vtbGetOutputData;
    pDec->SetInputData	= vtbSetInputData;
    pDec->Uninit	    = vtbUninit;
    pDec->Flush         = vtbFlush;
    pDec->ForceOutputAll= vtbForceOutputAll;
    pDec->SetHeadData   = vtbSetHeadData;
    return QC_ERR_NONE;
}

#pragma mark Wrapper class
CVTBVideoDec::CVTBVideoDec(CBaseInst* pInst, void * hInst)
: CBaseVideoDec (pInst, hInst)
, m_hDec (NULL)
, m_bInitHeadData(false)
,m_bStop(false)
{
    SetObjectName ("CVTBVideoDec");
    memset (&m_fmtVideo, 0, sizeof (m_fmtVideo));
    
    qcGetVTBDecAPI(&m_api, 0);
    
    if (m_pBaseInst)
        m_pBaseInst->AddListener(this);
}

CVTBVideoDec::~CVTBVideoDec(void)
{
    if (m_pBaseInst)
        m_pBaseInst->RemListener(this);
    Uninit ();
}

int CVTBVideoDec::Start (void)
{
    CBaseVideoDec::Start();
    m_bStop = false;
    return QC_ERR_NONE;
}

int CVTBVideoDec::Stop (void)
{
    CBaseVideoDec::Stop();
    m_bStop = true;
    m_bInitHeadData = false;
    return QC_ERR_NONE;
}

int CVTBVideoDec::Init (QC_VIDEO_FORMAT * pFmt)
{    
    Uninit ();
    
    if(pFmt)
    	memcpy (&m_fmtVideo, pFmt, sizeof (m_fmtVideo));
    m_fmtVideo.pHeadData = NULL;
    m_fmtVideo.nHeadSize = 0;
    m_fmtVideo.pPrivateData = NULL;
    
    int nRC = m_api.Init(&m_hDec, 0, 0);
    
    if (m_fmtVideo.nCodecID == QC_CODEC_ID_H265 || nRC != QC_ERR_NONE)
    {
        if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
            m_pBaseInst->m_pMsgMng->Notify(QC_MSG_VIDEO_HWDEC_FAILED, 0, 0);
        return QC_ERR_FAILED;
    }

    
    if (m_pBuffData == NULL)
    {
        m_pBuffData = new QC_DATA_BUFF ();
        memset (m_pBuffData, 0, sizeof (QC_DATA_BUFF));
        m_pBuffData->uBuffType = QC_BUFF_TYPE_Video;
        m_pBuffData->pBuffPtr = &m_buffVideo;
    }
    
	return QC_ERR_NONE;
}

int CVTBVideoDec::Uninit (void)
{
	if (m_hDec != NULL)
		m_api.Uninit (m_hDec);
	m_hDec = NULL;

	QC_DEL_P (m_pBuffData);

	return QC_ERR_NONE;
}

int CVTBVideoDec::Flush (void)
{
	CAutoLock lock (&m_mtBuffer);
    
    QCLOGI("[VTB]Flush VTB");
    
    if(m_hDec)
        m_api.Flush(m_hDec);

	return QC_ERR_NONE;
}


int CVTBVideoDec::SetBuff (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || m_hDec == NULL)
		return QC_ERR_ARG;
    
	CAutoLock lock (&m_mtBuffer);
	CBaseVideoDec::SetBuff (pBuff);
    
    if(!m_bInitHeadData)
    {
        if(QC_ERR_NONE == UpdateHeadData(pBuff))
            return QC_ERR_NONE;
    }
    

	if ( (pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
	{
        Flush ();
        
        if(QC_ERR_NONE == UpdateHeadData(pBuff))
            return QC_ERR_NONE;
    }
    
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		if (pFmt != NULL/* && pFmt->pHeadData != NULL*/)
			return InitNewFormat (pFmt, pBuff);
	}
    
	int nRet = m_api.SetInputData(m_hDec, pBuff);
    
    return nRet;
}

int CVTBVideoDec::GetBuff (QC_DATA_BUFF ** ppBuff)
{
	if (ppBuff == NULL || m_hDec == NULL)
		return QC_ERR_ARG;

	CAutoLock lock (&m_mtBuffer);
    if (m_pBuffData != NULL)
        m_pBuffData->uFlag = 0;

    QC_DATA_BUFF buf;
    QC_VIDEO_BUFF info;
	int nRC = m_api.GetOutputData (m_hDec, &buf, &info);
    
	if (nRC != QC_ERR_NONE)
	{
        if(nRC == QC_ERR_VIDEO_DEC)
            return nRC;
		return QC_ERR_RETRY;
	}
    
	m_buffVideo.pBuff[0] = info.pBuff[0];
	m_buffVideo.pBuff[1] = info.pBuff[1];
	m_buffVideo.pBuff[2] = info.pBuff[2];
	m_buffVideo.nStride[0] = info.nStride[0];
	m_buffVideo.nStride[1] = info.nStride[1];
	m_buffVideo.nStride[2] = info.nStride[2];
    m_buffVideo.nWidth = info.nWidth;
    m_buffVideo.nHeight = info.nHeight;
    
    //QC_VDT_NV12
    //QC_VDT_YUV420_P
    m_buffVideo.nType   = info.nType;
	m_pBuffData->llTime = buf.llTime;
    m_pBuffData->nMediaType = QC_MEDIA_Video;

	if (m_fmtVideo.nWidth != info.nWidth || m_fmtVideo.nHeight !=info.nHeight)
	{
		m_fmtVideo.nWidth = info.nWidth;
		m_fmtVideo.nHeight = info.nHeight;
		m_pBuffData->uFlag |= QCBUFF_NEW_FORMAT;
		m_pBuffData->pFormat = &m_fmtVideo;
	}

	CBaseVideoDec::GetBuff (&m_pBuffData);
	*ppBuff = m_pBuffData;

	return QC_ERR_NONE;
}

int CVTBVideoDec::InitNewFormat (QC_VIDEO_FORMAT * pFmt, QC_DATA_BUFF * pBuff)
{
    Uninit();
    Init(pFmt);

    if (m_hDec == NULL)
        return QC_ERR_FAILED;
    
    UpdateHeadData(pBuff);
	return QC_ERR_NONE;
}

int CVTBVideoDec::PushRestOut (void)
{
    CAutoLock lock (&m_mtBuffer);
    
    QCLOGI("[VTB]Force all frames output");
    
    if (m_hDec != NULL)
    {
        m_api.ForceOutputAll (m_hDec);
    }
    
    return QC_ERR_NONE;
}

int CVTBVideoDec::RecvEvent(int nEventID)
{
    if(nEventID == QC_BASEINST_EVENT_REOPEN)
    {
        if (m_hDec != NULL)
        {
            m_api.Flush(m_hDec);
        }
    }
    
    return QC_ERR_NONE;
}

int CVTBVideoDec::UpdateHeadData (QC_DATA_BUFF* pBuff)
{
    if(!m_hDec)
        return QC_ERR_EMPTYPOINTOR;
    
    int nRet = m_api.SetHeadData(m_hDec, pBuff->pBuff, pBuff->uSize);
    m_bInitHeadData = true;
    
    return nRet;
}

