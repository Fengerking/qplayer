/*******************************************************************************
	File:		UNV12Rotate.h

	Contains:	The nv12 rotate header file.

	Written by:	Fenger King

	Change History (most recent first):
	2014-05-22		Fenger			Create file

*******************************************************************************/
#ifndef __UNV12Rotate_H__
#define __UNV12Rotate_H__

#include "qcData.h"

void yyNV12Rotate00 (unsigned char * pU, unsigned char * pV, int nW, int nH, int nLine, unsigned char * pUV, int nStride);
void yyNV12Rotate90 (unsigned char * pU, unsigned char * pV, int nW, int nH, int nLine, unsigned char * pUV, int nStride);
void yyNV12Rotate180 (unsigned char * pU, unsigned char * pV, int nW, int nH, int nLine, unsigned char * pUV, int nStride);
void yyNV12Rotate270 (unsigned char * pU, unsigned char * pV, int nW, int nH, int nLine, unsigned char * pUV, int nStride);


#endif // __UNV12Rotate_H__
