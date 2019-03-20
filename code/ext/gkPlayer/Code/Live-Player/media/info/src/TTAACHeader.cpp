/**
* File : TTAACHeader.cpp
* Created on : 2010/09/02
* Description : TTAACHeader 
*/

// INCLUDES

#include "TTAACHeader.h"

static const int SAMPLE_RATE_TABLE[12] =
{
	96000, 88200, 64000, 48000, 44100, 32000,
	24000, 22050, 16000, 12000, 11025, 8000
};


TTBool CTTAACHeader::AACCheckHeader(const TTUint8* pbData, AAC_HEADER& ah)
{
	TTBool bValidHeader = ETTFalse;
	if (pbData[0] == 0xff && (pbData[1] & 0xF0))
	{
		TTUint8* pmh = (TTUint8*)&ah;
		pmh[3] = pbData[0];
		pmh[2] = pbData[1];
		pmh[1] = pbData[2];
		pmh[0] = pbData[3]&0xFC;


		pmh[4] = pbData[6];
		pmh[5] = pbData[5];
		pmh[6] = pbData[4];
		pmh[7] = pbData[3]&0x03;

		bValidHeader = (ah.sync == AAC_FRAME_INFO_SYNC_FLAG) && (ah.samplerate < 0x0C);
	}
	return bValidHeader;
}

TTBool CTTAACHeader::AACParseFrame(AAC_HEADER ah, AAC_FRAME_INFO& ai)
{
	ai.nSamplesPerFrame = SAMPLES_PER_FRAME;
	/*
	*  For implicit signalling, no hint that sbr or ps is used, so we need to
	*  check the sampling frequency of the aac content, if lesser or equal to
	*  24 KHz, by defualt upsample, otherwise, do nothing
	*/
	if (ah.samplerate >= 6)
	{
		ah.samplerate -= 3;
		ai.nSamplesPerFrame *= 2;
	}	
	ai.nSampleRate = SAMPLE_RATE_TABLE[ah.samplerate];
	//we default to 2 channel, even for mono files, (where channels have same content)
	ai.nChannels = 2;
	ai.nFrameSize = ah.framelen;
	return ETTTrue;
}

TTBool CTTAACHeader::AACSyncFrameHeader(const TTUint8 *pbData, int DataSize, int &SyncOffset, AAC_FRAME_INFO& ai)
{
	TTBool bFoundSync = ETTFalse;
	if (!(ai.nAACType == AACTYPE_ADIF || DataSize < 7))
	{
		AAC_HEADER ah;
		SyncOffset = DataSize;

		do 
		{
			if (AACCheckHeader(pbData, ah) && AACParseFrame(ah, ai) && (ai.nFrameSize > 0) 
				&& (ai.nFrameSize < KAACMaxFrameSize) && ((ai.nChannels == 2) || (ai.nChannels = 1)))
			{
				bFoundSync = ETTTrue;
				SyncOffset -= DataSize;
				break;
			}
			pbData++;
			DataSize--;
		} while (DataSize >= 4);
	}

	return bFoundSync;
}
