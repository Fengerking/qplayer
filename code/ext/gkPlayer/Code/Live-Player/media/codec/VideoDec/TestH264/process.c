// Test.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ttMpegWrap.h"
#include <android/log.h>
#include		<sys/time.h>
#include		<limits.h>
#include		<dlfcn.h>
#include <android/log.h>
#define  LOG_TAG    "TestH264Dec"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define READ_LEN 4096
#define INBUFFER_SIZE (READ_LEN*1024)

static long int GetSysTime()
{
	long int nTime;
	struct timeval tval;
	gettimeofday(&tval, NULL);
	TTInt64 second = tval.tv_sec;
	second = second*1000 + tval.tv_usec/1000;
	nTime = second & 0x000000007FFFFFFF;
	return nTime;
}

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

int main(int argc, char **argv) 
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
	int nCPUNum = atoi(argv[3]);
	int nType   = atoi(argv[4]);
	int start, end, nTotal;

	char libname[256];
	void *m_hDll = NULL;
	strcpy(libname, "/data/local/tmp/h264/libH264Dec.so");
	m_hDll = dlopen (libname, RTLD_NOW);
	
	if (NULL == m_hDll) {
		return 0;
	}
	
	__ttGetH264DecAPI pApi = (__ttGetH264DecAPI) dlsym (m_hDll, ("ttGetH264DecAPI"));
	
	if (NULL == pApi) {
		return 0;
	}
	pApi(&decApi);

	inFile= fopen (argv[1], "rb");
	if (!inFile){
		LOGI("cygnus Error: cannot open input H.264 file! src=%s", argv[1]);
		exit(0);
	}

	outFile = fopen(argv[2], "wb+");

	if (!outFile){
		LOGI("cygnus Warning: no output video file!");
	}

	returnCode = decApi.Open(&hCodec);
	if(returnCode)
	{
		goto END;
	}

	inputBuf = (char *)calloc(INBUFFER_SIZE ,1);
	inSize = fread(&nReadLen, sizeof(char), sizeof(int), inFile);

	inSize = fread(inputBuf, sizeof(char), nReadLen, inFile);

	decApi.SetParam(hCodec, TT_PID_VIDEO_THREAD_NUM, &nCPUNum);
	
	decApi.SetParam(hCodec, TT_PID_VIDEO_TYPE, &nType);

	nTotal = 0;
	dataFrame = 0;
	while(1){
		inData.pBuffer	= (TTPBYTE)inputBuf;
		inData.nSize 	= inSize;
		inData.llTime	= dataFrame*33;

		start = GetSysTime();

		returnCode = decApi.SetInput(hCodec,&inData);
		if(returnCode)
		{
			LOGI("cygnus Error %d: Decode failed!\n", returnCode);
			//break;
		}

		returnCode=decApi.Process(hCodec,&outData,&outFormat);
		while(outData.Buffer[0] && returnCode == 0)
		{
			dataFrame++;
			//OutputOneFrame(outFile, &outData,outFormat.Width,outFormat.Height,outFormat.Type );
			returnCode = decApi.Process(hCodec,&outData,&outFormat);
		}
		end = GetSysTime();
		nTotal += end - start;

		inSize = fread(&nReadLen, sizeof(char), sizeof(int), inFile);
		//LOGI("cygnus read size= %d\n", inSize);
		if(inSize < 4)
			break;

		inSize = fread(inputBuf, sizeof(char), nReadLen, inFile);
		if(inSize != nReadLen)
			break;
	}

	LOGI("Decoder nType=%s threadnum=%d Total Time = %d, Total Frame %d  ps=%f", nType==0 ? "frame":"slice", nCPUNum, nTotal, dataFrame, (1.0f * dataFrame) / (1.0f * nTotal / 1000));

END:
	decApi.Close(hCodec);
	free(inputBuf);
	fclose(inFile);
	if (outFile)
		fclose(outFile);
	inFile=outFile=NULL;

	if(m_hDll)
		dlclose(m_hDll);

	return 0;
}

