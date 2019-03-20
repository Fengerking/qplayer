/*******************************************************************************
File:		UAVFormatFunc.cpp

Contains:	The audio video format func implement file.

Written by:	Bangfei Jin

Change History (most recent first):
2017-01-12		Bangfei			Create file

*******************************************************************************/
#include "qcClrConv.h"

const unsigned char	*qccClip255 = &(qccClip255Base[384]);


void qcColorConvert_c (unsigned char * pSrc_y, unsigned char * pSrc_u, unsigned char * pSrc_v, const int inStride, 
						unsigned char * pOutBuf, const int outStride, int width, int height,
						const int uinStride, const int vinStride)
{
	int i;

	do{
		i = width;
		do{
			int a0, a1, a2, a3, a4;

			a3 = *(pSrc_v++) - 128;	
			a2 = *(pSrc_u++) - 128;

			a0 = a3 * ConstV1;
			a1 = a3 * ConstV2 + a2 *ConstU2;
			a2 = a2 * ConstU1;
			a3 = ((*pSrc_y) - 16) * ConstY;
			a4 = (*(pSrc_y + 1) - 16) * ConstY;

			a3 = ((qccClip255[((a3 + a0)>>20)]))|((qccClip255[((a3 - a1)>>20)])<<8)|((qccClip255[((a3 + a2)>>20)])<<16)|(255 << 24);
			a4 = ((qccClip255[((a4 + a0)>>20)]))|((qccClip255[((a4 - a1)>>20)])<<8)|((qccClip255[((a4 + a2)>>20)])<<16)|(255 << 24);
			*((int*)pOutBuf)       = a3;
			*((int*)(pOutBuf + 4)) = a4;

			/////////////////////////////////////////////////////////////////////
			a3 = (*(pSrc_y + inStride) - 16) * ConstY;
			a4 = (*(pSrc_y + inStride + 1) - 16) * ConstY;
			pSrc_y += 2;

			a3 = ((qccClip255[((a3 + a0)>>20)]))|((qccClip255[((a3 - a1)>>20)])<<8)|((qccClip255[((a3 + a2)>>20)])<<16)|(255 << 24);
			a4 = ((qccClip255[((a4 + a0)>>20)]))|((qccClip255[((a4 - a1)>>20)])<<8)|((qccClip255[((a4 + a2)>>20)])<<16)|(255 << 24);
			*((int*)(pOutBuf + outStride))     = a3;
			*((int*)(pOutBuf + outStride + 4)) = a4;
			pOutBuf += 8;

		}while((i-=2) != 0);

		i = (width >> 1);
		pSrc_u -= i;
		pSrc_v -= i;
		pSrc_u += uinStride;
		pSrc_v += vinStride;
		pSrc_y -= width;
		pSrc_y += (inStride << 1);

		pOutBuf -= (width << 2 );
		pOutBuf += (outStride << 1);
	}while((height-=2) != 0);
}

