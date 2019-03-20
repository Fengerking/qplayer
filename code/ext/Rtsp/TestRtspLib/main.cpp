#include "stdio.h"
#include "RtspClientAPI.h"
#include "hex_coding.h"
#include "b64.h"
#include "UAVParser.h"

#ifdef _WIN32
#include "windows.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

FILE*  pFileDump = NULL;
FILE*  pFileDumpAudio = NULL;

long long   ulMilSecFirstI = 0;
long long   ulMilSecFirstISys = 0;

int   igSampleRate = 0;
int   igChannels = 0;


int  __RTSPClientCallBack(int _chid, void *_chPtr, int _frameType, void *_pBuf, void* _frameInfo)
{
	S_Media_Frame_Info*   pFrameInfo = (S_Media_Frame_Info*)(_frameInfo);
	S_Media_Track_Info*   pTrackInfo = (S_Media_Track_Info*)_pBuf;
	unsigned char*        pFrameBuf = (unsigned char*)_pBuf;
	unsigned char         aAudioCodecConfig[64] = { 0 };
	int                   iAudioCodecConfigSize = 0;
	unsigned char         aADTSHeader[8];

	unsigned char asyc[4] = { 0, 0, 0, 1 };
	if (pFileDump == NULL)
	{
		pFileDump = fopen("rtsp_h264.h264", "wb");
	}

	switch (_frameType)
	{
		case FRAME_TYPE_INFO:
		{
			if (_frameInfo != NULL && pFrameInfo->iInfoData == RTSP_STATE_TRACK_INFO_READY)
			{
				if (pTrackInfo->iMediaType == MEDIA_TYPE_VIDEO)
				{
					printf("iTrack Media Type:%d, Media Codec:%d, CodecConfig:%s\n", pTrackInfo->iMediaType, pTrackInfo->iCodecId, pTrackInfo->sVideoInfo.aCodecConfig);
				}

				if (pTrackInfo->iMediaType == MEDIA_TYPE_AUDIO)
				{
					printf("iTrack Media Type:%d, Media Codec:%d, CodecConfig:%s\n", pTrackInfo->iMediaType, pTrackInfo->iCodecId, pTrackInfo->sAudioInfo.aCodecConfig);
					HexDecode(pTrackInfo->sAudioInfo.aCodecConfig, pTrackInfo->sAudioInfo.iCodecConfigSize, (char*)aAudioCodecConfig, &iAudioCodecConfigSize, 64);
					qcAV_ParseAACConfig(aAudioCodecConfig, iAudioCodecConfigSize, &igSampleRate, &igChannels);
				}
			}
			break;
		}

	default:
		break;
	}

	if (pFileDump != NULL)
	{
		if (pFrameInfo->iMediaCodec == MEDIA_VIDEO_CODEC_H264 && pFrameInfo->iMediaType != MEDIA_TYPE_INFO)
		{
			//printf("Nal Header:%x, Nal Size:%d, time:%lld \n", pFrameBuf[0], pFrameInfo->iFrameDataSize, pFrameInfo->illTimeStamp);

			if (ulMilSecFirstISys == 0 && ((pFrameBuf[0] & 0x1f) == 5))
			{
				ulMilSecFirstISys = GetTickCount();
				ulMilSecFirstI = pFrameInfo->illTimeStamp;
			}
			else
			{
				if (ulMilSecFirstISys != 0)
				{
					int ilDisCurSys = GetTickCount() - ulMilSecFirstISys;
					int       iDisMedia = pFrameInfo->illTimeStamp - ulMilSecFirstI;
					//printf("Sys Dis:%d, Media Dis:%d, rate:%f, total dis(Media - Sys):%d\n", ilDisCurSys, iDisMedia, (float)iDisMedia / (float)ilDisCurSys, iDisMedia - ilDisCurSys);
				}
			}

			SYSTEMTIME   sysTime;
			char        szTime[256] = { 0 };
			GetLocalTime(&sysTime);
			sprintf(szTime, "%d:%d:%d %d:%d:%d:%d", sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
			//printf("I frame come at%s, frame timestamp:%lld\n", szTime, pFrameInfo->illTimeStamp);


			fwrite(asyc, 1, 4, pFileDump);
			fwrite(_pBuf, 1, pFrameInfo->iFrameDataSize, pFileDump);
			fflush(pFileDump);
		}

		if (pFrameInfo->iMediaType == MEDIA_TYPE_AUDIO && pFrameInfo->iMediaCodec == MEDIA_AUDIO_CODEC_AAC)
		{
			qcAV_ConstructAACHeader(aADTSHeader, 8, igSampleRate, igChannels, pFrameInfo->iFrameDataSize);
			if (pFileDumpAudio == NULL)
			{
				pFileDumpAudio = fopen("test.aac", "wb");
			}
			printf("audio timestamp:%llu, audio size:%d\n", pFrameInfo->illTimeStamp, pFrameInfo->iFrameDataSize);
			if (pFileDumpAudio != NULL)
			{
				fwrite(aADTSHeader, 1, 7, pFileDumpAudio);
				fwrite(_pBuf, 1, pFrameInfo->iFrameDataSize, pFileDumpAudio);
				fflush(pFileDumpAudio);
			}

		}
	}

	return 0;
}

int main(int argc, char** argv)
{
	HMODULE			hDllPlay = NULL;
	CM_Rtsp_Ins   sRtspIns;
	char        szTime[256] = { 0 };
	SYSTEMTIME   sysTime;
	int          iValid = 0;
	memset(&sRtspIns, 0, sizeof(CM_Rtsp_Ins));

	hDllPlay = LoadLibrary("qcRTSP.dll");
	QCCREATERTSPINS * fCreate = (QCCREATERTSPINS *)GetProcAddress(hDllPlay, "qcCreateRtspIns");
	QCDESTROYRTSPINS* fDestroy = (QCDESTROYRTSPINS *)GetProcAddress(hDllPlay, "qcDestroyRtspIns");

	fCreate(&sRtspIns, NULL);

	if (sRtspIns.hRtspIns == NULL)
	{
		return -1;
	}

#ifdef _WIN32
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	void*             hUserHandle = (void *)&iValid;

	sRtspIns.RTSP_SetCallback(sRtspIns.hRtspIns, __RTSPClientCallBack);

	GetLocalTime(&sysTime);
	sprintf(szTime, "%d:%d:%d %d:%d:%d:%d", sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
	printf("open stream at %s\n", szTime);
	sRtspIns.RTSP_OpenStream(sRtspIns.hRtspIns, 1, argv[1], 1, 1, hUserHandle, 0);

	getchar();

	sRtspIns.RTSP_CloseStream(sRtspIns.hRtspIns);
	fDestroy(&sRtspIns);

#ifdef _WIN32
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}
