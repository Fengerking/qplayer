#ifndef __RTSPTYPES_H__
#define __RTSPTYPES_H__

#ifdef _WIN32
#define RTSP_API  __declspec(dllexport)
#define RTSP_APICALL  __stdcall
#define WIN32_LEAN_AND_MEAN
#else
#define API
#define APICALL 
#endif

// Handle Type
#define RTSP_Handle void*


//Video Codec
#define RTSP_VIDEO_CODEC_H264	0x1C		/* H264  */
#define RTSP_VIDEO_CODEC_H265	0x48323635	/* 1211250229 */
#define	RTSP_VIDEO_VIDEO_CODEC_MJPEG	0x08		/* MJPEG */
#define	RTSP_VIDEO_VIDEO_CODEC_MPEG4	0x0D		/* MPEG4 */

//Audio Codec
#define RTSP_VIDEO_AUDIO_CODEC_AAC	0x15002		/* AAC */
#define RTSP_VIDEO_AUDIO_CODEC_G711U	0x10006		/* G711 ulaw*/
#define RTSP_VIDEO_AUDIO_CODEC_G711A	0x10007		/* G711 alaw*/
#define RTSP_VIDEO_AUDIO_CODEC_G726	0x1100B		/* G726 */


/* 连接类型 */
typedef enum RTSP_RTP_CONNECT_TYPE
{
	RTSP_RTP_OVER_TCP	=	0x01,		/* RTP Over TCP */
	RTSP_RTP_OVER_UDP					/* RTP Over UDP */
}RTSP_RTP_CONNECT_TYPE;

typedef struct 
{
	unsigned int	iCodec;				/* 音视频格式 */

	unsigned int	iType;				/* 视频帧类型 */
	unsigned char	iFps;				/* 视频帧率 */
	unsigned short	iWidth;				/* 视频宽 */
	unsigned short  iHeight;			/* 视频高 */

	unsigned int	reserved1;			/* 保留参数1 */
	unsigned int	reserved2;			/* 保留参数2 */

	unsigned int	sample_rate;		/* 音频采样率 */
	unsigned int	channels;			/* 音频声道数 */
	unsigned int	bits_per_sample;	/* 音频采样精度 */

	unsigned int	iLength;			/* 音视频帧大小 */
	unsigned int    ilTimestampUsec;	/* 时间戳,微妙 */
	unsigned int	ilTimestampSec;		/* 时间戳 秒 */

	float			fBitrate;			/* 比特率 */
	float			fLosspacket;		/* 丢包率 */
}RTSP_FRAME_INFO;

#endif
