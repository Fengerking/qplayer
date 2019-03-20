/*******************************************************************************
	File:		UYUV420Rotate.h

	Contains:	The YUV420 rotate header file.

	Written by:	Fenger King

	Change History (most recent first):
	2014-05-22		Fenger			Create file

*******************************************************************************/
#ifndef __UYUV420Rotate_H__
#define __UYUV420Rotate_H__

#include "qcData.h"

void yyYUVRotate90 (unsigned char * pInput, int nWidth, int nHeight, int nInStride, unsigned char * pOutput, int nOutStride);
void yyYUVRotate270 (unsigned char * pInput, int nWidth, int nHeight, int nInStride, unsigned char * pOutput, int nOutStride);
void yyYUVRotate180 (unsigned char * pInput, int nWidth, int nHeight, int nInStride, unsigned char * pOutput, int nOutStride);
void yyYUVRotate00 (unsigned char * pInput, int nWidth, int nHeight, int nInStride, unsigned char * pOutput, int nOutStride);
void yyYUV420Rotate90 (unsigned char *y, unsigned char *u, unsigned char *v,
						int in_width, int in_height, int * Pin_stride,
						unsigned char *out_buf, int out_stride);
void yyYUV420Rotate270 (unsigned char *y, unsigned char *u, unsigned char *v,
						int in_width, int in_height, int * Pin_stride,
						unsigned char *out_buf, int out_stride);
void yyYUV420Rotate180 (unsigned char *y, unsigned char *u, unsigned char *v,
						int in_width, int in_height, int * Pin_stride,
						unsigned char *out_buf, int out_stride);


#endif // __UYUV420Rotate_H__
