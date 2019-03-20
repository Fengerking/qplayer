/*
============================================================================
 Name        : TTAACHeader.h
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : TTAACHeader.h - header file
============================================================================
*/

#ifndef _TT_AACHEADER_H
#define _TT_AACHEADER_H

#include "GKTypedef.h"
#include "GKMacrodef.h"

#define AAC_FRAME_INFO_SYNC_FLAG 0xFFF
#define SAMPLES_PER_FRAME 1024

static const TTInt KAACMinFrameSize = 0x07;//AAC一帧的最小值
static const TTInt KAACMaxFrameSize = 6* KILO;//AAC一帧的最大值

enum ENAACTYPE
{
	  AACTYPE_ADTS
	, AACTYPE_ADIF
};

typedef struct 
{
	unsigned unused1			: 2;
	unsigned cpyrightstart		: 1;		// B
	unsigned cpyrightbit 		: 1;		// C
	unsigned home 				: 1;		// D
	unsigned orgOrcopy	 		: 1;		// E
	unsigned channels			: 3;		// F
	unsigned privatebit 		: 1;		// G
	unsigned samplerate 		: 4;		// H
	unsigned profile 			: 2;		// I
	unsigned Protection 		: 1;		// J
	unsigned Layer 				: 2;		// K
	unsigned ID 				: 1;		// L
	unsigned sync 				: 12;		// M
    
	unsigned numOfBlocksInFrame	: 2 ;
	unsigned buffullness		: 11;
	unsigned framelen			: 13;		// A	
	unsigned unused2            : 6 ;		
}AAC_HEADER;

typedef struct 
{
	ENAACTYPE nAACType;
	int nChannels;
	int nSampleRate;
	int nBitRate;
	int nSamplesPerFrame;
	int nFrameSize;
	
}AAC_FRAME_INFO;

class CTTAACHeader
{
public:
	static TTBool AACCheckHeader(const TTUint8* pbData, AAC_HEADER& ah);
	static TTBool AACParseFrame(AAC_HEADER ah, AAC_FRAME_INFO& ai);
	static TTBool AACSyncFrameHeader(const TTUint8 *pbData, int DataSize, int &SyncOffset, AAC_FRAME_INFO& ai);

private:
	ENAACTYPE iAACType;
};
#endif
// End of File
