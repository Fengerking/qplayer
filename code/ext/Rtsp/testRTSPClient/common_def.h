#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#ifdef __QC_OS_WIN32__
#define CM_API __cdecl
#define CM_CBI __stdcall
#define CM_DLLIMPORT_C extern __declspec(dllimport)
#define CM_DLLEXPORT_C __declspec(dllexport)
#else
#define CM_API
#define CM_CBI
#define CM_DLLIMPORT_C 
#define CM_DLLEXPORT_C __attribute__ ((visibility("default")))
#define TCHAR	char
#endif // __QC_OS_WIN32__

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */


#define CM_RTSP_Handle void*
typedef int(*RTSPSourceCallBack)(int _channelId, void *_channelPtr, int _frameType, void* pBuf, void* _frameInfo);
typedef void * cm_ThreadHandle;
typedef int(*qcThreadProc) (void * pParam);


#define FRAME_TYPE_INFO  0
#define FRAME_TYPE_MEDIA_DATA 1
#define FRAME_TYPE_ERROR  2

#define   RTSP_STATE_NONE 0
#define   RTSP_STATE_SENDING_DESCRIBE_REQ 1
#define   RTSP_STATE_WAITING_DESCRIBE_RES 2
#define   RTSP_STATE_SENDING_SETUP_REQ 3
#define   RTSP_STATE_WAITING_SETUP_RES 4
#define   RTSP_STATE_SENDING_PLAY_REQ 5
#define   RTSP_STATE_WAITING_PLAY_RES 6
#define   RTSP_STATE_TRACK_INFO_READY 7
#define   RTSP_STATE_PLAY_END         8


#define MEDIA_TYPE_UNKNOWN 0
#define MEDIA_TYPE_VIDEO 1
#define MEDIA_TYPE_AUDIO 2
#define MEDIA_TYPE_INFO  3


#define MEDIA_VIDEO_CODEC_UNKNOWN 0
#define MEDIA_VIDEO_CODEC_H264 1
#define MEDIA_VIDEO_CODEC_H265 2

#define MEDIA_AUDIO_CODEC_UNKNOWN 0
#define MEDIA_AUDIO_CODEC_AAC 1
#define MEDIA_AUDIO_CODEC_G711_A 2
#define MEDIA_AUDIO_CODEC_G711_U 3
#define MEDIA_AUDIO_CODEC_MP3 4


enum
{
	RTSP_NoErr = 0,
	RTSP_RequestFailed = -1,
	RTSP_Unimplemented = -2,
	RTSP_RequestArrived = -3,
	RTSP_OutOfState = -4,
	RTSP_NotAModule = -5,
	RTSP_WrongVersion = -6,
	RTSP_IllegalService = -7,
	RTSP_BadIndex = -8,
	RTSP_ValueNotFound = -9,
	RTSP_BadArgument = -10,
	RTSP_ReadOnly = -11,
	RTSP_NotPreemptiveSafe = -12,
	RTSP_NotEnoughSpace = -13,
	RTSP_WouldBlock = -14,
	RTSP_NotConnected = -15,
	RTSP_FileNotFound = -16,
	RTSP_NoMoreData = -17,
	RTSP_AttrDoesntExist = -18,
	RTSP_AttrNameExists = -19,
	RTSP_InstanceAttrsNotAllowed = -20,
	RTSP_InvalidSocket = -21,
	RTSP_MallocError = -22,
	RTSP_ConnectError = -23,
	RTSP_SendError = -24
};

typedef int RTSP_Error;

typedef struct  
{
	int iWidth;
	int iHeight;
	char  aCodecConfig[256];
	int   iCodecConfigSize;
} S_VideoInfo;


typedef struct
{
	int iSampleRate;
	int iChannel;
	char  aCodecConfig[256];
	int   iCodecConfigSize;
} S_AudioInfo;


typedef  struct
{
	int iMediaType;
	int iCodecId;

	union
	{
		S_VideoInfo sVideoInfo;
		S_AudioInfo sAudioInfo;
	};
} S_Media_Track_Info;

typedef  struct
{
	int iFrameDataSize;
	long long illTimeStamp;
	int iMediaType;
	int iMediaCodec;
	int iInfoData;
} S_Media_Frame_Info;


#endif