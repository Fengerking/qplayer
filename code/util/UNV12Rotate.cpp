/*******************************************************************************
	File:		UBitmapFunc.cpp

	Contains:	The nv12 rotateimplement file.

	Written by:	Fenger King

	Change History (most recent first):
	2014-05-22		Fenger			Create file

*******************************************************************************/
#include "stdlib.h"
#include "UNV12Rotate.h"

void yyNV12Rotate00 (unsigned char * pU, unsigned char * pV, int nW, int nH, int nLine, unsigned char * pUV, int nStride)
{
	LPBYTE pU1 = pU;
	LPBYTE pV1 = pV;
	LPBYTE pUV2 = pUV;
	int i, j;
	for (i = 0; i < nH; i++)
	{
		pUV2 = pUV + i * nStride;
		pU1  = pU + nLine * i;
		pV1  = pV + nLine * i;
		for (j = 0; j < nW; j++)
		{
			*pUV2++ = *pU1++;
			*pUV2++ = *pV1++;
		}
	}
}

void yyNV12Rotate90 (unsigned char * pU, unsigned char * pV, int nW, int nH, int nLine, unsigned char * pUV, int nStride)
{
	unsigned int *	pU1, * pU2, * pU3, * pU4;
	unsigned int *	pV1, * pV2, * pV3, * pV4;
	unsigned int	nU1, nU2, nU3, nU4;
	unsigned int	nV1, nV2, nV3, nV4;
	int i, j;

	unsigned int * pDest =  (unsigned int *)pUV;

	for (i = 0; i < nH; i+=4)
	{
		pU1 = (unsigned int *)(pU + i * nLine);
		pU2 = (unsigned int *)(pU + (i + 1) * nLine);
		pU3 = (unsigned int *)(pU + (i + 2) * nLine);
		pU4 = (unsigned int *)(pU + (i + 3) * nLine);
		pV1 = (unsigned int *)(pV + i * nLine);
		pV2 = (unsigned int *)(pV + (i + 1) * nLine);
		pV3 = (unsigned int *)(pV + (i + 2) * nLine);
		pV4 = (unsigned int *)(pV + (i + 3) * nLine);

		for (j = 0; j < nW; j+=4)
		{
			nU1 = *pU1++; nU2 = *pU2++; nU3 = *pU3++; nU4 = *pU4++;
			nV1 = *pV1++; nV2 = *pV2++; nV3 = *pV3++; nV4 = *pV4++;

			pDest = (unsigned int *)(pUV + j * nStride + (nH -i) * 2 - 8);

			*pDest++ = (nU4 & 0XFF) | ((nV4 & 0XFF) << 8) | ((nU3 & 0XFF) << 16) | ((nV3 & 0XFF) << 24);
			*pDest = (nU2 & 0XFF) | ((nV2 & 0XFF) << 8) | ((nU1 & 0XFF) << 16) | ((nV1 & 0XFF) << 24);
			pDest = pDest + nStride / 4 - 1;

			*pDest++ = ((nU4 & 0XFF00) >> 8) | ((nV4 & 0XFF00)) | ((nU3 & 0XFF00) << 8) | ((nV3 & 0XFF00) << 16);
			*pDest = ((nU2 & 0XFF00) >> 8) | ((nV2 & 0XFF00)) | ((nU1 & 0XFF00) << 8) | ((nV1 & 0XFF00) << 16);
			pDest = pDest + nStride / 4 - 1;

			*pDest++ = ((nU4 & 0XFF0000) >> 16) | ((nV4 & 0XFF0000) >> 8) | ((nU3 & 0XFF0000)) | ((nV2 & 0XFF0000) << 8);
			*pDest = ((nU2 & 0XFF0000) >> 16) | ((nV2 & 0XFF0000) >> 8) | ((nU1 & 0XFF0000)) | ((nV1 & 0XFF0000) << 8);
			pDest = pDest + nStride / 4 - 1;
		
			*pDest++ = ((nU4 & 0XFF000000) >> 24) | ((nV4 & 0XFF000000) >> 16) | ((nU3 & 0XFF000000) >> 8) | ((nV3 & 0XFF000000));
			*pDest++ = ((nU2 & 0XFF000000) >> 24) | ((nV2 & 0XFF000000) >> 16) | ((nU1 & 0XFF000000) >> 8) | ((nV1 & 0XFF000000));
			pDest = pDest + nStride / 4 - 1;
		}
	}
}

void yyNV12Rotate180 (unsigned char * pU, unsigned char * pV, int nW, int nH, int nLine, unsigned char * pUV, int nStride)
{
	unsigned int * pU1 = (unsigned int *)pU;
	unsigned int * pV1 = (unsigned int *)pV;
	unsigned int * pDest = (unsigned int *)pUV;
	int nU, nV;
	int i, j;

	for (i = 0; i < nH; i++)
	{
		pU1 = (unsigned int *)(pV + i * nLine);
		pV1 = (unsigned int *)(pU + i * nLine);
		pDest = (unsigned int *)(pUV + (nH - i - 1) * nStride + nW * 2 - 4);
		for (j = 0; j < nW; j+=4)
		{
			nU = *pU1++;
			nV = *pV1++;
			*pDest-- = ((nU & 0XFF) << 24) | ((nV & 0XFF) << 16) | (nU & 0XFF00) | ((nV & 0XFF00) >> 8);
			*pDest-- = ((nU & 0XFF0000) << 8) | ((nV & 0XFF0000)) | ((nU & 0XFF000000) >> 16) | ((nV & 0XFF000000) >> 24);
		}
	}
}

void yyNV12Rotate270 (unsigned char * pU, unsigned char * pV, int nW, int nH, int nLine, unsigned char * pUV, int nStride)
{
	unsigned int *	pU1, * pU2, * pU3, * pU4;
	unsigned int *	pV1, * pV2, * pV3, * pV4;
	unsigned int	nU1, nU2, nU3, nU4;
	unsigned int	nV1, nV2, nV3, nV4;
	int i, j;

	unsigned int * pDest =  (unsigned int *)pUV;

	for (i = 0; i < nH; i+=4)
	{
		pU1 = (unsigned int *)(pU + i * nLine);
		pU2 = (unsigned int *)(pU + (i + 1) * nLine);
		pU3 = (unsigned int *)(pU + (i + 2) * nLine);
		pU4 = (unsigned int *)(pU + (i + 3) * nLine);
		pV1 = (unsigned int *)(pV + i * nLine);
		pV2 = (unsigned int *)(pV + (i + 1) * nLine);
		pV3 = (unsigned int *)(pV + (i + 2) * nLine);
		pV4 = (unsigned int *)(pV + (i + 3) * nLine);

		for (j = 0; j < nW; j+=4)
		{
			nU1 = *pU1++; nU2 = *pU2++; nU3 = *pU3++; nU4 = *pU4++;
			nV1 = *pV1++; nV2 = *pV2++; nV3 = *pV3++; nV4 = *pV4++;

			pDest = (unsigned int *)(pUV + (nW - j - 1) * nStride + i * 2);

			*pDest++ = (nU4 & 0XFF) | ((nV4 & 0XFF) << 8) | ((nU3 & 0XFF) << 16) | ((nV3 & 0XFF) << 24);
			*pDest = (nU2 & 0XFF) | ((nV2 & 0XFF) << 8) | ((nU1 & 0XFF) << 16) | ((nV1 & 0XFF) << 24);
			pDest = pDest - nStride / 4 - 1;

			*pDest++ = ((nU4 & 0XFF00) >> 8) | ((nV4 & 0XFF00)) | ((nU3 & 0XFF00) << 8) | ((nV3 & 0XFF00) << 16);
			*pDest = ((nU2 & 0XFF00) >> 8) | ((nV2 & 0XFF00)) | ((nU1 & 0XFF00) << 8) | ((nV1 & 0XFF00) << 16);
			pDest = pDest - nStride / 4 - 1;

			*pDest++ = ((nU4 & 0XFF0000) >> 16) | ((nV4 & 0XFF0000) >> 8) | ((nU3 & 0XFF0000)) | ((nV2 & 0XFF0000) << 8);
			*pDest = ((nU2 & 0XFF0000) >> 16) | ((nV2 & 0XFF0000) >> 8) | ((nU1 & 0XFF0000)) | ((nV1 & 0XFF0000) << 8);
			pDest = pDest - nStride / 4 - 1;
		
			*pDest++ = ((nU4 & 0XFF000000) >> 24) | ((nV4 & 0XFF000000) >> 16) | ((nU3 & 0XFF000000) >> 8) | ((nV3 & 0XFF000000));
			*pDest++ = ((nU2 & 0XFF000000) >> 24) | ((nV2 & 0XFF000000) >> 16) | ((nU1 & 0XFF000000) >> 8) | ((nV1 & 0XFF000000));
			pDest = pDest - nStride / 4 - 1;
		}
	}
}
