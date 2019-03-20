#ifndef _TT_AV_UTILS_H_
#define _TT_AV_UTILS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "TTMediadef.h"
#include "TTBitReader.h"

#if __cplusplus
extern "C" {
#endif


enum {
    kAVCProfileBaseline      = 0x42,
    kAVCProfileMain          = 0x4d,
    kAVCProfileExtended      = 0x58,
    kAVCProfileHigh          = 0x64,
    kAVCProfileHigh10        = 0x6e,
    kAVCProfileHigh422       = 0x7a,
    kAVCProfileHigh444       = 0xf4,
    kAVCProfileCAVLC444Intra = 0x2c
};

// Optionally returns sample aspect ratio as well.
void FindAVCDimensions(
        TTBuffer* pInBuffer,
        int *width, int *height, int* numRef,
        int *sarWidth = NULL, int *sarHeight = NULL);

void FindHEVCDimensions(
        unsigned char* buffer, unsigned int size,
        int *width, int *height);

unsigned int parseUE(TTBitReader *br);

bool IsAVCReferenceFrame(TTBuffer* pInBuffer);

int ConvertAVCNalHead(
		unsigned char* pOutBuffer, int& nOutSize, 
		unsigned char* pInBuffer, int nInSize, int &nNalLength);

int ConvertHEVCNalHead(
		unsigned char* pOutBuffer, int& nOutSize, 
		unsigned char* pInBuffer, int nInSize, int &nNalLength);

int ConvertAVCNalFrame(
		unsigned char* pOutBuffer, int& nOutSize, 
		unsigned char* pInBuffer, int nInSize, 
		int nNalLength, int &IsKeyFrame, int nType = 0);

int ParseAACConfig(
        unsigned char* pBuffer, unsigned int size,
        int *out_sampling_rate, int *out_channels);

int ConstructAACHeader(
        unsigned char* pBuffer, unsigned int size,
        int in_sampling_rate, int in_channels, int in_framesize);

int GetAACFrameSize(
        unsigned char* pBuffer, unsigned int size, int *frame_size,
        int *out_sampling_rate, int *out_channels);

int GetMPEGAudioFrameSize(
        unsigned char* pBuf, unsigned int *frame_size,
        int *out_sampling_rate, int *out_channels,
        int *out_bitrate, int *out_num_samples);

#if __cplusplus
}  // extern "C"
#endif

#endif  // _TT_AV_UTILS_H_
