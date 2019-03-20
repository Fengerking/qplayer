#ifdef _WIN32

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif


#include "stdio.h"
#include "RtspClientAPI.h"

FILE*  pFileDump = NULL;

int  __RTSPClientCallBack(int _chid, void *_chPtr, int _frameType, char *_pBuf, void* _frameInfo)
{
	S_Media_Frame_Info*   pFrameInfo = (S_Media_Frame_Info*)(_frameInfo);
	unsigned char asyc[4] = { 0, 0, 0, 1 };
	if (pFileDump == NULL)
	{
		pFileDump = fopen("rtsp_h264.h264", "wb");
	}

	if (pFileDump != NULL)
	{
		if (pFrameInfo->iMediaCodec == MEDIA_VIDEO_CODEC_H264)
		{
			printf("Nal Header:%x, Nal Size:%d, time:%lld \n", _pBuf[0], pFrameInfo->iFrameDataSize, pFrameInfo->illTimeStamp);
			fwrite(asyc, 1, 4, pFileDump);
			fwrite(_pBuf, 1, pFrameInfo->iFrameDataSize, pFileDump);
			fflush(pFileDump);
		}
	}

	return 0;
}

int main(int argc, char** argv)
{
#ifdef _WIN32
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	int   iValid = 1;
	CM_RTSP_Handle   hHandle = NULL; 
	void*             hUserHandle = (void *)&iValid;

	RTSP_Init(&hHandle);
	RTSP_SetCallback(hHandle, __RTSPClientCallBack);
	RTSP_OpenStream(hHandle, 1, argv[1], 1, 1, hUserHandle, 1);

	getchar();

	RTSP_CloseStream(hHandle);
	RTSP_Deinit(&hHandle);

#ifdef _WIN32
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}
