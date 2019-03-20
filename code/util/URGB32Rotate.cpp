/*******************************************************************************
	File:		UBitmapFunc.cpp

	Contains:	The utility for bitmap implement file.

	Written by:	Fenger King

	Change History (most recent first):
	2014-04-27		Fenger			Create file

*******************************************************************************/
#include "stdlib.h"
#include "URGB32Rotate.h"

void yyRGB32Rotate00 (unsigned char * pInput, int nWidth, int nHeight, int nInStride, unsigned char * pOutput, int nOutStride)
{
	for (int i = 0; i < nHeight; i++)
	{
		memcpy (pOutput + nOutStride * i, pInput + nInStride * i, nWidth * 4);
	}
}

void yyRGB32Rotate90 (unsigned char * pInput, int nWidth, int nHeight, int nInStride, unsigned char * pOutput, int nOutStride)
{
	int i, j;
	int * pSource =  (int *)pInput;
	int * pTarget =  (int *)pOutput;
	int	nStride = nOutStride / 4 ;

	for (i = 0; i < nHeight; i++)
	{
		pSource =  (int *)(pInput + nInStride * i);
		for (j = 0; j < nWidth; j++)
		{
			*(pTarget + j * nStride + (nHeight - i - 1)) = *pSource++;
		}
	}
}

void yyRGB32Rotate180 (unsigned char * pInput, int nWidth, int nHeight, int nInStride, unsigned char * pOutput, int nOutStride)
{
	int i, j;
	int * pSource =  (int *)pInput;
	int * pTarget =  (int *)pOutput;

	for (i = 0; i < nHeight; i++)
	{
		pSource =  (int *)(pInput + nInStride * i);
		pTarget =  (int *)(pOutput + nOutStride * (nHeight - i - 1) +  nWidth * 4 - 4);
		for (j = 0; j < nWidth; j++)
		{
			*(pTarget--) = *pSource++;
		}
	}
}

void yyRGB32Rotate270 (unsigned char * pInput, int nWidth, int nHeight, int nInStride, unsigned char * pOutput, int nOutStride)
{
	int i, j;
	int * pSource =  (int *)pInput;
	int * pTarget =  (int *)pOutput;
	int	nStride = nOutStride / 4 ;

	for (i = 0; i < nHeight; i++)
	{
		pSource =  (int *)(pInput + nInStride * i);
		for (j = 0; j < nWidth; j++)
		{
			*(pTarget + (nWidth - j - 1) * nStride + i) = *pSource++;
		}
	}
}
