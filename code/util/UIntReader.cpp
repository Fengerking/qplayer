/*******************************************************************************
	File:		UIntReader.cpp

	Contains:	The int reander implement file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-08		Bangfei			Create file

*******************************************************************************/
#include "string.h"
#include "UIntReader.h"

unsigned short qcIntReadUint16 (const unsigned char* aReadPtr)	
{
	return (unsigned short)(aReadPtr[1] << 8) | aReadPtr[0];
}

unsigned short qcIntReadUint16BE (const unsigned char* aReadPtr)
{
	return (unsigned short)(aReadPtr[0] << 8) | aReadPtr[1];
}

unsigned int qcIntReadUint32 (const unsigned char* aReadPtr)
{
	return (unsigned int)(aReadPtr[3] << 24) | (unsigned int)(aReadPtr[2] << 16) | (unsigned int)(aReadPtr[1] << 8) | aReadPtr[0];
}

unsigned int qcIntReadUint32BE (const unsigned char* aReadPtr)
{
	return (unsigned int)(aReadPtr[0] << 24) | (unsigned int)(aReadPtr[1] << 16) | (unsigned int)(aReadPtr[2] << 8) | aReadPtr[3];
}

unsigned long long qcIntReadUint64 (const unsigned char* aReadPtr)
{
	unsigned int nSizeLow = (unsigned int)(aReadPtr[3] << 24) | (unsigned int)(aReadPtr[2] << 16) | (unsigned int)(aReadPtr[1] << 8) | aReadPtr[0];
	unsigned int nSizeHigh = (unsigned int)(aReadPtr[7] << 24) | (unsigned int)(aReadPtr[6] << 16) | (unsigned int)(aReadPtr[5] << 8) | aReadPtr[4];

	unsigned long long nRetVal = nSizeHigh;

	return ((nRetVal << 32) | nSizeLow);
}

double qcIntReadDouble64 (const unsigned char* aReadPtr)
{
	double dVal;
	unsigned char *ci, *co;
	ci = (unsigned char *)aReadPtr;
	co = (unsigned char *)&dVal;
	co[0] = ci[7];
	co[1] = ci[6];
	co[2] = ci[5];
	co[3] = ci[4];
	co[4] = ci[3];
	co[5] = ci[2];
	co[6] = ci[1];
	co[7] = ci[0];

	return dVal;
}

unsigned long long	qcIntReadUint64BE (const unsigned char* aReadPtr)
{
	unsigned int nSizeHigh  = (unsigned int)(aReadPtr[0] << 24) | (unsigned int)(aReadPtr[1] << 16) | (unsigned int)(aReadPtr[2] << 8) | aReadPtr[3];
	unsigned int nSizeLow = (unsigned int)(aReadPtr[4] << 24) | (unsigned int)(aReadPtr[5] << 16) | (unsigned int)(aReadPtr[6] << 8) | aReadPtr[7];

	unsigned long long nRetVal = nSizeHigh;

	return ((nRetVal << 32) | nSizeLow);
}

unsigned short qcIntReadWord (const unsigned char* aReadPtr)
{
	return (unsigned short)(aReadPtr[0] << 8) | aReadPtr[1];
}

unsigned int qcIntReadDWord (const unsigned char* aReadPtr)
{
	return (unsigned int)(aReadPtr[0] << 24) | (unsigned int)(aReadPtr[1] << 16) | (unsigned int)(aReadPtr[2] << 8) | aReadPtr[3];
}

unsigned int qcIntReadBytesNBE (const unsigned char* aReadPtr, int n)
{
	unsigned int	nRead = 0;
	switch(n)
	{
	case 1:
		nRead = aReadPtr[0];
		break;
	case 2:
		nRead = (unsigned int)(aReadPtr[0] << 8) | aReadPtr[1];
		break;
	case 3:
		nRead = (unsigned int)(aReadPtr[0] << 16) | (unsigned int)(aReadPtr[1] << 8) | aReadPtr[2];
		break;
	case 4:
		nRead = (unsigned int)(aReadPtr[0] << 24) | (unsigned int)(aReadPtr[1] << 16) | (unsigned int)(aReadPtr[2] << 8) | aReadPtr[3];
		break;
	}
	
	return nRead;
}

unsigned int qcIntReadBytesN (const unsigned char* aReadPtr, int n)
{
	unsigned int nRead = 0;
	switch(n)
	{
	case 1:
		nRead = aReadPtr[0];
		break;
	case 2:
		nRead = (unsigned int)(aReadPtr[1] << 8) | aReadPtr[0];
		break;
	case 3:
		nRead = (unsigned int)(aReadPtr[2] << 16) | (unsigned int)(aReadPtr[1] << 8) | aReadPtr[0];
		break;
	case 4:
		nRead = (unsigned int)(aReadPtr[3] << 24) | (unsigned int)(aReadPtr[2] << 16) | (unsigned int)(aReadPtr[1] << 8) | aReadPtr[0];
		break;
	}
	
	return nRead;
}
