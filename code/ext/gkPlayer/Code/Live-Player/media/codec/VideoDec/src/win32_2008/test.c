// Test.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TTMpegWrap.h"
#include <time.h>

#define READ_LEN 4096
#define INBUFFER_SIZE (READ_LEN*1024)

int OutputOneFrame(FILE* outFile, TTVideoBuffer *par,int width,int height,int frametype)
{
	int i;
	char* buf;
	if (outFile) {
		buf  = (char*)par->Buffer[0];

		for(i=0; i<height; i++) {
			fwrite(buf, 1, width, outFile);
			buf += par->Stride[0];
		}

		width /=2;
		height /=2;
		buf	=  (char*)par->Buffer[1];
		for(i=0; i<height; i++)	{
			fwrite(buf, 1, width, outFile); 
			buf+=par->Stride[1];
		}

		buf	= (char*)par->Buffer[2];
		for(i=0;i<height;i++) {
			fwrite(buf, 1, width, outFile);
			buf+=par->Stride[2];
		}	
		fflush(outFile);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	char *inputBuf;
	TTHandle hCodec;
	int		returnCode;
	TTBuffer inData;
	TTVideoBuffer  outData;
	TTVideoFormat outFormat;
	TTVideoCodecAPI decApi={0};
	int inSize;
	int leftSize=INBUFFER_SIZE;//maks no sense, just for consistent with !hasSizeHead case asdf;
	FILE *inFile, *outFile;
	int dataFrame;
	int nReadLen;
	clock_t cstart = 0,cends = 0;

// 	ttGetMPEG4DecAPI(&decApi);
// 	inFile= fopen (argv[1], "rb");
// 	if (!inFile){
// 		printf("\nError: cannot open input mpeg4 file!");
// 		exit(0);
// 	}
// 
// 	outFile = fopen(argv[2], "wb");
// 
// 	if (!outFile){
// 		printf("\nWarning: no output video file!");
// 	}
// 
// 	returnCode = decApi.Open(&hCodec);
// 	if(returnCode)
// 	{
// 		goto END;
// 	}
// 
// 	inputBuf = (char *)calloc(INBUFFER_SIZE ,1);
// 	inSize = fread(&nReadLen, sizeof(char), sizeof(int), inFile);
// 
// 	inSize = fread(inputBuf, sizeof(char), nReadLen, inFile);
// 
// 	dataFrame = 0;
// 	while(1){
// 		inData.pBuffer	= (TTPBYTE)inputBuf;
// 		inData.nSize 	= inSize;
// 		inData.llTime	= dataFrame*33;
// 
// 		returnCode = decApi.SetInput(hCodec,&inData);
// 		if(returnCode)
// 		{
// 			printf("\nError %d: Decod failed!\n", returnCode);
// 			//break;
// 		}
// 
// 		cstart = clock();
// 		returnCode=decApi.Process(hCodec,&outData,&outFormat);
// 		while(outData.Buffer[0] && returnCode == 0)
// 		{
//  			OutputOneFrame(outFile, &outData,outFormat.Width,outFormat.Height,outFormat.Type );
// // 
// // 			printf("\nMPEG4 outFormat.Width=%d,outFormat.Height=%d\n", outFormat.Width,outFormat.Height);
// // 
// 
// 			returnCode = decApi.Process(hCodec,&outData,&outFormat);
// 		}
// 		cends += (clock() - cstart);
// 
// 		inSize = fread(&nReadLen, sizeof(char), sizeof(int), inFile);
// 		if(inSize < 4)
// 			break;
// 
// 		inSize = fread(inputBuf, sizeof(char), nReadLen, inFile);
// 		if(inSize != nReadLen)
// 			break;
// 
// 		dataFrame++;
// 		printf("\r%d	", dataFrame);
// 	}
// 
// END:
// 	printf("cygnus MPEG4 time= %d  fps=%f\n", cends, (1.0f * dataFrame) / ((1.0 *cends) / CLOCKS_PER_SEC));
// 	decApi.Close(hCodec);
// 	free(inputBuf);
// 	fclose(inFile);
// 	if (outFile)
// 		fclose(outFile);
// 	inFile=outFile=NULL;

	//-------------------------------------------------------------------------

	ttGetH264DecAPI(&decApi);
	inFile= fopen (argv[3], "rb");
	if (!inFile){
		printf("\nError: cannot open input H.264 file!");
		exit(0);
	}

	outFile = fopen(argv[4], "wb");

	if (!outFile){
		printf("\nWarning: no output video file!");
	}

	returnCode = decApi.Open(&hCodec);
	if(returnCode)
	{
		goto END1;
	}

	inputBuf = (char *)calloc(INBUFFER_SIZE ,1);
	inSize = fread(&nReadLen, sizeof(char), sizeof(int), inFile);

	inSize = fread(inputBuf, sizeof(char), nReadLen, inFile);

	dataFrame = 0;
	while(1){
		inData.pBuffer	= (TTPBYTE)inputBuf;
		inData.nSize 	= inSize;
		inData.llTime	= dataFrame*33;

		returnCode = decApi.SetInput(hCodec,&inData);
		if(returnCode)
		{
			printf("\nError %d: Decod failed!\n", returnCode);
			//break;
		}

		cstart = clock();
		returnCode=decApi.Process(hCodec,&outData,&outFormat);
		while(outData.Buffer[0] && returnCode == 0)
		{
 			OutputOneFrame(outFile, &outData,outFormat.Width,outFormat.Height,outFormat.Type );
// 
// 			printf("\nH264 outFormat.Width=%d,outFormat.Height=%d\n", outFormat.Width,outFormat.Height);
// 

			returnCode = decApi.Process(hCodec,&outData,&outFormat);
		}
		cends += (clock() - cstart);

		inSize = fread(&nReadLen, sizeof(char), sizeof(int), inFile);
		if(inSize < 4)
			break;

		inSize = fread(inputBuf, sizeof(char), nReadLen, inFile);
		if(inSize != nReadLen)
			break;

		dataFrame++;
		printf("\r%d	", dataFrame);
	}

END1:
	printf("cygnus H264 time= %d  fps=%f\n", cends, (1.0f * dataFrame) / ((1.0 *cends) / CLOCKS_PER_SEC));
	decApi.Close(hCodec);
	free(inputBuf);
	fclose(inFile);
	if (outFile)
		fclose(outFile);
	inFile=outFile=NULL;


	return 0;
}

