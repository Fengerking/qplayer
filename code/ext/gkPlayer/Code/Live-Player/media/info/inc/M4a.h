#ifndef _M4A_H
#define _M4A_H

typedef struct
{
	TTInt	iSampleRate;
	TTInt	iChannels;
	TTInt	iSampleBit;
}TTM4AWaveFormat;

typedef struct
{
	TTUint8*	iData;
	TTUint32	iSize;
}TTMP4DecoderSpecificInfo;

#endif
