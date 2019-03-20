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


/* �������� */
typedef enum RTSP_RTP_CONNECT_TYPE
{
	RTSP_RTP_OVER_TCP	=	0x01,		/* RTP Over TCP */
	RTSP_RTP_OVER_UDP					/* RTP Over UDP */
}RTSP_RTP_CONNECT_TYPE;

typedef struct 
{
	unsigned int	iCodec;				/* ����Ƶ��ʽ */

	unsigned int	iType;				/* ��Ƶ֡���� */
	unsigned char	iFps;				/* ��Ƶ֡�� */
	unsigned short	iWidth;				/* ��Ƶ�� */
	unsigned short  iHeight;			/* ��Ƶ�� */

	unsigned int	reserved1;			/* ��������1 */
	unsigned int	reserved2;			/* ��������2 */

	unsigned int	sample_rate;		/* ��Ƶ������ */
	unsigned int	channels;			/* ��Ƶ������ */
	unsigned int	bits_per_sample;	/* ��Ƶ�������� */

	unsigned int	iLength;			/* ����Ƶ֡��С */
	unsigned int    ilTimestampUsec;	/* ʱ���,΢�� */
	unsigned int	ilTimestampSec;		/* ʱ��� �� */

	float			fBitrate;			/* ������ */
	float			fLosspacket;		/* ������ */
}RTSP_FRAME_INFO;

#endif
