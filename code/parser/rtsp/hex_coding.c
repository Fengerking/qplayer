#include "hex_coding.h"
#include "stdlib.h"

static char   aEncodeTable[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};


static char   aDecodeTable[] = {
	//0~f
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//10~1f
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//20~2f
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//30~3f
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  16, 16, 16, 16, 16, 16,
	//40~4f
	16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//50~5f
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//60~6f
	16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//70~7f
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//80~8f
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//90~9f
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//a0~af
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//b0~bf
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//c0~cf
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//d0~df
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//e0~ef
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	//f0~ff
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
};


int HexEncode(unsigned char*  pInput, int iInputSize, char* pOutput, int* pOutputSize, int iOutputMaxSize)
{
	int iRet = 0;
	int iIndex = 0;

	do 
	{
		if (pInput == NULL || iInputSize <= 0 || pOutput == NULL || pOutputSize == NULL || iOutputMaxSize<= 0)
		{
			iRet = -1;
			break;
		}

		if (iOutputMaxSize< 2*iInputSize)
		{
			iRet = -1;
			break;
		}

		for (iIndex = 0; iIndex < iInputSize; iIndex++)
		{
			pOutput[2*iIndex] = aEncodeTable[((pInput[iIndex]) >> 4)&(0xf)];
			pOutput[2*iIndex+1] = aEncodeTable[(pInput[iIndex])&(0xf)];
		}

		*pOutputSize = 2 * iInputSize;
	} while (0);

	return iRet;
}

int HexDecode(char*  pInput, int iInputSize, char* pOutput, int* pOutputSize, int iOutputMaxSize)
{
	int iRet = 0;
	unsigned char uValueHigh = 0;
	int iIndex = 0;
	unsigned char uValueLow = 0;


	do
	{
		if (pInput == NULL || iInputSize <= 0 || pOutput == NULL || pOutputSize == NULL || iOutputMaxSize <= 0)
		{
			iRet = -1;
			break;
		}

		if ((iInputSize%2) != 0 ||  iOutputMaxSize <  (iInputSize/2))
		{
			iRet = -1;
			break;
		}

		for (iIndex = 0; iIndex < iInputSize; iIndex+=2)
		{
			uValueHigh = aDecodeTable[pInput[iIndex]];
			uValueLow = aDecodeTable[pInput[iIndex + 1]];
			if (uValueHigh == 16 || uValueLow == 16)
			{
				iRet = -1;
				break;
			}

			pOutput[iIndex / 2] = (uValueHigh << 4) | (uValueLow);
		}

		*pOutputSize = iInputSize/2;
	} while (0);

	return iRet;
}