/*******************************************************************************
	File:		URGB32Rotate.h

	Contains:	The YUV420 rotate header file.

	Written by:	Fenger King

	Change History (most recent first):
	2014-05-22		Fenger			Create file

*******************************************************************************/
#ifndef __URGB32Rotate_H__
#define __URGB32Rotate_H__

#include "qcData.h"

void yyRGB32Rotate00 (unsigned char * pInput, int nWidth, int nHeight, int nInStride, unsigned char * pOutput, int nOutStride);
void yyRGB32Rotate90 (unsigned char * pInput, int nWidth, int nHeight, int nInStride, unsigned char * pOutput, int nOutStride);
void yyRGB32Rotate180 (unsigned char * pInput, int nWidth, int nHeight, int nInStride, unsigned char * pOutput, int nOutStride);
void yyRGB32Rotate270 (unsigned char * pInput, int nWidth, int nHeight, int nInStride, unsigned char * pOutput, int nOutStride);


#endif // __URGB32Rotate_H__
