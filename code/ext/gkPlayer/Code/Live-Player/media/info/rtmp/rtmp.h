#ifndef __RTMP_H__
#define __RTMP_H__

#include "GKOsalConfig.h"

#if !defined(NO_CRYPTO) && !defined(CRYPTO)
#define CRYPTO
#endif

#include <errno.h>
#include "TTStdint.h"
#include <stddef.h>
#include "rtmphashtable.h"
#include "rtmpamf.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define RTMP_LIB_VERSION	0x020300	/* 2.3 */

#define RTMP_FEATURE_HTTP	0x01
#define RTMP_FEATURE_ENC	0x02
#define RTMP_FEATURE_SSL	0x04
#define RTMP_FEATURE_MFP	0x08	/* not yet supported */
#define RTMP_FEATURE_WRITE	0x10	/* publish, not play */
#define RTMP_FEATURE_HTTP2	0x20	/* server-side rtmpt */

#define RTMP_PROTOCOL_UNDEFINED	-1
#define RTMP_PROTOCOL_RTMP      0
#define RTMP_PROTOCOL_RTMPE     RTMP_FEATURE_ENC
#define RTMP_PROTOCOL_RTMPT     RTMP_FEATURE_HTTP
#define RTMP_PROTOCOL_RTMPS     RTMP_FEATURE_SSL
#define RTMP_PROTOCOL_RTMPTE    (RTMP_FEATURE_HTTP|RTMP_FEATURE_ENC)
#define RTMP_PROTOCOL_RTMPTS    (RTMP_FEATURE_HTTP|RTMP_FEATURE_SSL)
#define RTMP_PROTOCOL_RTMFP     RTMP_FEATURE_MFP

#define RTMP_DEFAULT_CHUNKSIZE	128

/* needs to fit largest number of bytes recv() may return */
#define RTMP_BUFFER_CACHE_SIZE (16*1024)

//cs
//#define	RTMP_CHANNELS	600

  extern const char RTMPProtocolStringsLower[][7];
  extern const AVal RTMP_DefaultFlashVer;
  extern int RTMP_ctrlC;

  uint32_t RTMP_GetTime(void);

#define RTMP_PACKET_TYPE_AUDIO 0x08
#define RTMP_PACKET_TYPE_VIDEO 0x09
#define RTMP_PACKET_TYPE_INFO  0x12

#define RTMP_MAX_HEADER_SIZE 18

#define RTMP_PACKET_SIZE_LARGE    0
#define RTMP_PACKET_SIZE_MEDIUM   1
#define RTMP_PACKET_SIZE_SMALL    2
#define RTMP_PACKET_SIZE_MINIMUM  3

  typedef struct RTMPChunk
  {
    int c_headerSize;
    int c_chunkSize;
    char *c_chunk;
    char c_header[RTMP_MAX_HEADER_SIZE];
  } RTMPChunk;

  typedef struct RTMPPacket
  {
    uint8_t m_headerType;
    uint8_t m_packetType;
    uint8_t m_hasAbsTimestamp;	/* timestamp absolute or relative? */
    int m_nChannel;
    uint32_t m_nTimeStamp;	/* timestamp */
    int32_t m_nInfoField2;	/* last 4 bytes in a long header */
    uint32_t m_nBodySize;
    uint32_t m_nBytesRead;
    RTMPChunk *m_chunk;
    char *m_body;
  } RTMPPacket;

  typedef struct RTMPSockBuf
  {
    int sb_socket;
    int sb_size;		/* number of unprocessed bytes in buffer */
    char *sb_start;		/* pointer into sb_pBuffer of next byte to process */
    char sb_buf[RTMP_BUFFER_CACHE_SIZE];	/* data read from socket */
    int sb_timedout;
    void *sb_ssl;
  } RTMPSockBuf;

  void RTMPPacket_Reset(RTMPPacket *p);
  void RTMPPacket_Dump(RTMPPacket *p);
  int RTMPPacket_Alloc(RTMPPacket *p, int nSize);
  void RTMPPacket_Free(RTMPPacket *p);

#define RTMPPacket_IsReady(a)	((a)->m_nBytesRead == (a)->m_nBodySize)

  typedef struct RTMP_LNK
  {
    AVal hostname;
    AVal sockshost;

    AVal playpath0;	/* parsed from URL */
    AVal playpath;	/* passed in explicitly */
    AVal tcUrl;
    AVal swfUrl;
    AVal pageUrl;
    AVal app;
    AVal auth;
    AVal flashVer;
    AVal subscribepath;
    AVal token;
    AMFObject extras;
    int edepth;

    int seekTime;
    int stopTime;

#define RTMP_LF_AUTH	0x0001	/* using auth param */
#define RTMP_LF_LIVE	0x0002	/* stream is live */
#define RTMP_LF_SWFV	0x0004	/* do SWF verification */
#define RTMP_LF_PLST	0x0008	/* send playlist before play */
#define RTMP_LF_BUFX	0x0010	/* toggle stream on BufferEmpty msg */
#define RTMP_LF_FTCU	0x0020	/* free tcUrl on close */
    int lFlags;

    int swfAge;

    int protocol;
    int timeout;		/* connection timeout in seconds */

    unsigned short socksport;
    unsigned short port;

#ifdef CRYPTO
#define RTMP_SWF_HASHLEN	32
    void *dh;			/* for encryption */
    void *rc4keyIn;
    void *rc4keyOut;

    uint32_t SWFSize;
    uint8_t SWFHash[RTMP_SWF_HASHLEN];
    char SWFVerificationResponse[RTMP_SWF_HASHLEN+10];
#endif
  } RTMP_LNK;

  /* state for read() wrapper */
  typedef struct RTMP_READ
  {
    char *buf;
    char *bufpos;
    unsigned int buflen;
    uint32_t timestamp;
    uint8_t dataType;
    uint8_t flags;
#define RTMP_READ_HEADER	0x01
#define RTMP_READ_RESUME	0x02
#define RTMP_READ_NO_IGNORE	0x04
#define RTMP_READ_GOTKF		0x08
#define RTMP_READ_GOTFLVK	0x10
#define RTMP_READ_SEEKING	0x20
    int8_t status;
#define RTMP_READ_COMPLETE	    -3
#define RTMP_READ_ERROR	        -2
#define RTMP_READ_EOF	        -1
#define RTMP_READ_IGNORE	    0
#define RTMP_READ_TIMEOUT	    -4

//add
#define RTMP_READ_NOMEMORY	    -5
#define RTMP_READ_NETDATAERROR	-6
#define RTMP_READ_CANCLE	    -7
#define RTMP_READ_PEER_DISCONNET -8


    /* if bResume == TRUE */
    uint8_t initialFrameType;
    uint32_t nResumeTS;
    char *metaHeader;
    char *initialFrame;
    uint32_t nMetaHeaderSize;
    uint32_t nInitialFrameSize;
    uint32_t nIgnoredFrameCounter;
    uint32_t nIgnoredFlvFrameCounter;
  } RTMP_READ;

  typedef struct RTMP_METHOD
  {
    AVal name;
    int num;
  } RTMP_METHOD;

  typedef struct RTMP
  {
    int m_inChunkSize;
    int m_outChunkSize;
    int m_nBWCheckCounter;
    int m_nBytesIn;
    int m_nBytesInSent;
    int m_nBufferMS;
    int m_stream_id;		/* returned in _result from createStream */
    int m_mediaChannel;
    uint32_t m_mediaStamp;
    uint32_t m_pauseStamp;
    int m_pausing;
    int m_nServerBW;
    int m_nClientBW;
    uint8_t m_nClientBW2;
    uint8_t m_bPlaying;
    uint8_t m_bSendEncoding;
    uint8_t m_bSendCounter;

    int m_numInvokes;
    int m_numCalls;
    RTMP_METHOD *m_methodCalls;	/* remote method calls queue */

    //RTMPPacket *m_vecChannelsIn[RTMP_CHANNELS];
    //RTMPPacket *m_vecChannelsOut[RTMP_CHANNELS];
    hash_head_t m_vecChannelsIn;
    hash_head_t m_vecChannelsOut;
    hash_head_t m_channelTimestamp;	/* abs timestamp of last packet */

    double m_fAudioCodecs;	/* audioCodecs for the connect packet */
    double m_fVideoCodecs;	/* videoCodecs for the connect packet */
    double m_fEncoding;		/* AMF0 or AMF3 */

    double m_fDuration;		/* duration of stream in seconds */

    int m_msgCounter;		/* RTMPT stuff */
    int m_polling;
    int m_resplen;
    int m_unackd;
    AVal m_clientID;

    RTMP_READ m_read;
    RTMPPacket m_write;
    RTMPSockBuf m_sb;
    RTMP_LNK Link;
      
    int m_nServerStreamEof;
	unsigned int m_ip;
	int m_cancle;
  } RTMP;

  int RTMP_ParseURL(const char *url, int *protocol, AVal *host,
		     unsigned int *port, AVal *playpath, AVal *app);

  void RTMP_ParsePlaypath(AVal *in, AVal *out);
  void RTMP_SetBufferMS(RTMP *r, int size);
  void RTMP_UpdateBufferMS(RTMP *r);

  int RTMP_SetOpt(RTMP *r, const AVal *opt, AVal *arg);
  int RTMP_SetupURL(RTMP *r, char *url);
  void RTMP_SetupStream(RTMP *r, int protocol,
			AVal *hostname,
			unsigned int port,
			AVal *sockshost,
			AVal *playpath,
			AVal *tcUrl,
			AVal *swfUrl,
			AVal *pageUrl,
			AVal *app,
			AVal *auth,
			AVal *swfSHA256Hash,
			uint32_t swfSize,
			AVal *flashVer,
			AVal *subscribepath,
			int dStart,
			int dStop, int bLiveStream, int timeout);

  int RTMP_Connect(RTMP *r, RTMPPacket *cp);
  struct sockaddr;
  int RTMP_Connect0(RTMP *r, struct sockaddr *svc);
  int RTMP_Connect1(RTMP *r, RTMPPacket *cp);
  int RTMP_Serve(RTMP *r);

  int RTMP_ReadPacket(RTMP *r, RTMPPacket *packet);
  int RTMP_SendPacket(RTMP *r, RTMPPacket *packet, int queue);
  int RTMP_SendChunk(RTMP *r, RTMPChunk *chunk);
  int RTMP_IsConnected(RTMP *r);
  int RTMP_Socket(RTMP *r);
  int RTMP_IsTimedout(RTMP *r);
  double RTMP_GetDuration(RTMP *r);
  int RTMP_ToggleStream(RTMP *r);

  int RTMP_ConnectStream(RTMP *r, int seekTime);
  int RTMP_ReconnectStream(RTMP *r, int seekTime);
  void RTMP_DeleteStream(RTMP *r);
  int RTMP_GetNextMediaPacket(RTMP *r, RTMPPacket *packet);
  int RTMP_ClientPacket(RTMP *r, RTMPPacket *packet);

  void RTMP_Init(RTMP *r);
  void RTMP_Close(RTMP *r);
  RTMP *RTMP_Alloc(void);
  void RTMP_Free(RTMP *r);
  void RTMP_EnableWrite(RTMP *r);

  int RTMP_LibVersion(void);
  void RTMP_UserInterrupt(void);	/* user typed Ctrl-C */

  int RTMP_SendCtrl(RTMP *r, short nType, unsigned int nObject,
		     unsigned int nTime);

  /* caller probably doesn't know current timestamp, should
   * just use RTMP_Pause instead
   */
  int RTMP_SendPause(RTMP *r, int DoPause, int dTime);
  int RTMP_Pause(RTMP *r, int DoPause);

  int RTMP_FindFirstMatchingProperty(AMFObject *obj, const AVal *name,
				      AMFObjectProperty * p);

  int RTMPSockBuf_Fill(RTMP *r);
  int RTMPSockBuf_Send(RTMPSockBuf *sb, const char *buf, int len);
  int RTMPSockBuf_Close(RTMPSockBuf *sb);

  int RTMP_SendCreateStream(RTMP *r);
  int RTMP_SendSeek(RTMP *r, int dTime);
  int RTMP_SendServerBW(RTMP *r);
  int RTMP_SendClientBW(RTMP *r);
  void RTMP_DropRequest(RTMP *r, int i, int freeit);
  int RTMP_Read(RTMP *r, char *buf, int size);
  int RTMP_Write(RTMP *r, const char *buf, int size);

/* hashswf.c */
  int RTMP_HashSWF(const char *url, unsigned int *size, unsigned char *hash,
		   int age);

  void RTMP_LogHex(const uint8_t *data, unsigned long len);

  void RTMP_LogHexString(const uint8_t *data, unsigned long len);

  //int waitsocketwritebuffer(int aSocketHandle, timeval aTimeOut);

  //void setsocketnonblock(int aSocketHandle);

  //void setsocketblock(int aSocketHandle);

#ifdef __cplusplus
};
#endif

#endif
