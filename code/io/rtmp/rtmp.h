#ifndef __RTMP_H__
#define __RTMP_H__
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *      Copyright (C) 2008-2009 Andrej Stepanchuk
 *      Copyright (C) 2009-2010 Howard Chu
 *
 *  This file is part of librtmp.
 *
 *  librtmp is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1,
 *  or (at your option) any later version.
 *
 *  librtmp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with librtmp see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/lgpl.html
 */

#if !defined(NO_CRYPTO) && !defined(CRYPTO)
#define CRYPTO
#endif

#include <errno.h>
#include <stdint.h>
#include <stddef.h>


#include "amf.h"

#ifdef __cplusplus
        extern "C"
        {
#endif

#define qcRTMP_LIB_VERSION	0x020300	/* 2.3 */

#define qcRTMP_FEATURE_HTTP	0x01
#define qcRTMP_FEATURE_ENC	0x02
#define qcRTMP_FEATURE_SSL	0x04
#define qcRTMP_FEATURE_MFP	0x08	/* not yet supported */
#define qcRTMP_FEATURE_WRITE	0x10	/* publish, not play */
#define qcRTMP_FEATURE_HTTP2	0x20	/* server-side rtmpt */

#define qcRTMP_PROTOCOL_UNDEFINED	-1
#define qcRTMP_PROTOCOL_RTMP      0
#define qcRTMP_PROTOCOL_RTMPE     qcRTMP_FEATURE_ENC
#define qcRTMP_PROTOCOL_RTMPT     qcRTMP_FEATURE_HTTP
#define qcRTMP_PROTOCOL_RTMPS     qcRTMP_FEATURE_SSL
#define qcRTMP_PROTOCOL_RTMPTE    (qcRTMP_FEATURE_HTTP|qcRTMP_FEATURE_ENC)
#define qcRTMP_PROTOCOL_RTMPTS    (qcRTMP_FEATURE_HTTP|qcRTMP_FEATURE_SSL)
#define qcRTMP_PROTOCOL_RTMFP     qcRTMP_FEATURE_MFP

#define qcRTMP_DEFAULT_CHUNKSIZE	128

/* needs to fit largest number of bytes recv() may return */
#define qcRTMP_BUFFER_CACHE_SIZE (16*1024)

#define	qcRTMP_CHANNELS	65600

  extern const char qcRTMPProtocolStringsLower[][7];
  extern const AVal qcRTMP_DefaultFlashVer;
  extern int qcRTMP_ctrlC;

  uint32_t qcRTMP_GetTime(void);

/*      RTMP_PACKET_TYPE_...                0x00 */
#define qcRTMP_PACKET_TYPE_CHUNK_SIZE         0x01
/*      RTMP_PACKET_TYPE_...                0x02 */
#define qcRTMP_PACKET_TYPE_BYTES_READ_REPORT  0x03
#define qcRTMP_PACKET_TYPE_CONTROL            0x04
#define qcRTMP_PACKET_TYPE_SERVER_BW          0x05
#define qcRTMP_PACKET_TYPE_CLIENT_BW          0x06
/*      RTMP_PACKET_TYPE_...                0x07 */
#define qcRTMP_PACKET_TYPE_AUDIO              0x08
#define qcRTMP_PACKET_TYPE_VIDEO              0x09
/*      RTMP_PACKET_TYPE_...                0x0A */
/*      RTMP_PACKET_TYPE_...                0x0B */
/*      RTMP_PACKET_TYPE_...                0x0C */
/*      RTMP_PACKET_TYPE_...                0x0D */
/*      RTMP_PACKET_TYPE_...                0x0E */
#define qcRTMP_PACKET_TYPE_FLEX_STREAM_SEND   0x0F
#define qcRTMP_PACKET_TYPE_FLEX_SHARED_OBJECT 0x10
#define qcRTMP_PACKET_TYPE_FLEX_MESSAGE       0x11
#define qcRTMP_PACKET_TYPE_INFO               0x12
#define qcRTMP_PACKET_TYPE_SHARED_OBJECT      0x13
#define qcRTMP_PACKET_TYPE_INVOKE             0x14
/*      RTMP_PACKET_TYPE_...                0x15 */
#define qcRTMP_PACKET_TYPE_FLASH_VIDEO        0x16

#define qcRTMP_MAX_HEADER_SIZE 18

#define qcRTMP_PACKET_SIZE_LARGE    0
#define qcRTMP_PACKET_SIZE_MEDIUM   1
#define qcRTMP_PACKET_SIZE_SMALL    2
#define qcRTMP_PACKET_SIZE_MINIMUM  3

  typedef struct qcRTMPChunk
  {
    int c_headerSize;
    int c_chunkSize;
    char *c_chunk;
    char c_header[qcRTMP_MAX_HEADER_SIZE];
  } qcRTMPChunk;

  typedef struct qcRTMPPacket
  {
    uint8_t m_headerType;
    uint8_t m_packetType;
    uint8_t m_hasAbsTimestamp;	/* timestamp absolute or relative? */
    int m_nChannel;
    uint32_t m_nTimeStamp;	/* timestamp */
    int32_t m_nInfoField2;	/* last 4 bytes in a long header */
    uint32_t m_nBodySize;
    uint32_t m_nBytesRead;
    qcRTMPChunk *m_chunk;
    char *m_body;
  } qcRTMPPacket;

  typedef struct qcRTMPSockBuf
  {
    int sb_socket;
    int sb_size;		/* number of unprocessed bytes in buffer */
    char *sb_start;		/* pointer into sb_pBuffer of next byte to process */
    char sb_buf[qcRTMP_BUFFER_CACHE_SIZE];	/* data read from socket */
    int sb_timedout;
    void *sb_ssl;
  } qcRTMPSockBuf;

  void qcRTMPPacket_Reset(qcRTMPPacket *p);
  void qcRTMPPacket_Dump(qcRTMPPacket *p);
  int qcRTMPPacket_Alloc(qcRTMPPacket *p, uint32_t nSize);
  void qcRTMPPacket_Free(qcRTMPPacket *p);

#define qcRTMPPacket_IsReady(a)	((a)->m_nBytesRead == (a)->m_nBodySize)

  typedef struct qcRTMP_LNK
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
    AVal usherToken;
    AVal token;
    AVal pubUser;
    AVal pubPasswd;
    qcAMFObject extras;
    int edepth;

    int seekTime;
    int stopTime;

#define qcRTMP_LF_AUTH	0x0001	/* using auth param */
#define qcRTMP_LF_LIVE	0x0002	/* stream is live */
#define qcRTMP_LF_SWFV	0x0004	/* do SWF verification */
#define qcRTMP_LF_PLST	0x0008	/* send playlist before play */
#define qcRTMP_LF_BUFX	0x0010	/* toggle stream on BufferEmpty msg */
#define qcRTMP_LF_FTCU	0x0020	/* free tcUrl on close */
#define qcRTMP_LF_FAPU	0x0040	/* free app on close */
    int lFlags;

    int swfAge;

    int protocol;
    int timeout;		/* connection timeout in seconds */

    int pFlags;			/* unused, but kept to avoid breaking ABI */

    unsigned short socksport;
    unsigned short port;

#ifdef CRYPTO
#define qcRTMP_SWF_HASHLEN	32
    void *dh;			/* for encryption */
    void *rc4keyIn;
    void *rc4keyOut;

    uint32_t SWFSize;
    uint8_t SWFHash[qcRTMP_SWF_HASHLEN];
    char SWFVerificationResponse[qcRTMP_SWF_HASHLEN+10];
#endif
    struct sockaddr_storage* sockAddr;
    int timeoutConnect;
  } qcRTMP_LNK;

  /* state for read() wrapper */
  typedef struct qcRTMP_READ
  {
    char *buf;
    char *bufpos;
    unsigned int buflen;
    uint32_t timestamp;
    uint8_t dataType;
    uint8_t flags;
#define qcRTMP_READ_HEADER	0x01
#define qcRTMP_READ_RESUME	0x02
#define qcRTMP_READ_NO_IGNORE	0x04
#define qcRTMP_READ_GOTKF		0x08
#define qcRTMP_READ_GOTFLVK	0x10
#define qcRTMP_READ_SEEKING	0x20
    int8_t status;
#define qcRTMP_READ_COMPLETE	-3
#define qcRTMP_READ_ERROR	-2
#define qcRTMP_READ_EOF	-1
#define qcRTMP_READ_IGNORE	0

    /* if bResume == TRUE */
    uint8_t initialFrameType;
    uint32_t nResumeTS;
    char *metaHeader;
    char *initialFrame;
    uint32_t nMetaHeaderSize;
    uint32_t nInitialFrameSize;
    uint32_t nIgnoredFrameCounter;
    uint32_t nIgnoredFlvFrameCounter;
    uint32_t nCurrPacketType;
  } qcRTMP_READ;

  typedef struct qcRTMP_METHOD
  {
    AVal name;
    int num;
  } qcRTMP_METHOD;
  
  typedef int 	(*QC_GetAddrInfo) (void* pUserData, const char* pszHostName, const char* pszService,
                               const struct addrinfo* pHints, struct addrinfo** ppResult);
  typedef int 	(*QC_FreeAddrInfo) (void* pUserData, struct addrinfo *res);
  typedef void* (*QC_GetCache) (void* pUserData, char* pszHostName);
  typedef int 	(*QC_AddCache) (void* pUserData, char* pHostName, void * pAddress, unsigned int pAddressSize, int nConnectTime);
  typedef struct qcRTMP_IPAddrInfo
  {
      void* pUserData;
      QC_GetAddrInfo fGetAddrInfo;
      QC_FreeAddrInfo fFreeAddrInfo;
      QC_GetCache fGetCache;
      QC_AddCache fAddCache;
  }qcRTMP_IPAddrInfo;

  typedef struct qcRTMP
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
    qcRTMP_METHOD *m_methodCalls;	/* remote method calls queue */

    int m_channelsAllocatedIn;
    int m_channelsAllocatedOut;
    qcRTMPPacket **m_vecChannelsIn;
    qcRTMPPacket **m_vecChannelsOut;
    int *m_channelTimestamp;	/* abs timestamp of last packet */

    double m_fAudioCodecs;	/* audioCodecs for the connect packet */
    double m_fVideoCodecs;	/* videoCodecs for the connect packet */
    double m_fEncoding;		/* AMF0 or AMF3 */

    double m_fDuration;		/* duration of stream in seconds */

    int m_msgCounter;		/* RTMPT stuff */
    int m_polling;
    int m_resplen;
    int m_unackd;
    AVal m_clientID;

    qcRTMP_READ m_read;
    qcRTMPPacket m_write;
    qcRTMPSockBuf m_sb;
    qcRTMP_LNK Link;
    int forceClose;
    qcRTMP_IPAddrInfo* fIPAddrInfo;
  } qcRTMP;

  int qcRTMP_ParseURL(const char *url, int *protocol, AVal *host,
		     unsigned int *port, AVal *playpath, AVal *app);

  void qcRTMP_ParsePlaypath(AVal *in, AVal *out);
  void qcRTMP_SetBufferMS(qcRTMP *r, int size);
  void qcRTMP_UpdateBufferMS(qcRTMP *r);

  int qcRTMP_SetOpt(qcRTMP *r, const AVal *opt, AVal *arg);
  int qcRTMP_SetupURL(qcRTMP *r, char *url);
  void qcRTMP_SetupStream(qcRTMP *r, int protocol,
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
			AVal *usherToken,
			int dStart,
			int dStop, int bLiveStream, long int timeout);

  int qcRTMP_Connect(qcRTMP *r, qcRTMPPacket *cp);
  struct sockaddr;
  int qcRTMP_Connect0(qcRTMP *r, struct sockaddr *svc);
  int qcRTMP_Connect1(qcRTMP *r, qcRTMPPacket *cp);
  int qcRTMP_Serve(qcRTMP *r);
  int qcRTMP_TLS_Accept(qcRTMP *r, void *ctx);

  int qcRTMP_ReadPacket(qcRTMP *r, qcRTMPPacket *packet);
  int qcRTMP_SendPacket(qcRTMP *r, qcRTMPPacket *packet, int queue);
  int qcRTMP_SendChunk(qcRTMP *r, qcRTMPChunk *chunk);
  int qcRTMP_IsConnected(qcRTMP *r);
  int qcRTMP_Socket(qcRTMP *r);
  int qcRTMP_IsTimedout(qcRTMP *r);
  double RTMP_GetDuration(qcRTMP *r);
  int qcRTMP_ToggleStream(qcRTMP *r);

  int qcRTMP_ConnectStream(qcRTMP *r, int seekTime);
  int qcRTMP_ReconnectStream(qcRTMP *r, int seekTime);
  void qcRTMP_DeleteStream(qcRTMP *r);
  int qcRTMP_GetNextMediaPacket(qcRTMP *r, qcRTMPPacket *packet);
  int qcRTMP_ClientPacket(qcRTMP *r, qcRTMPPacket *packet);

  void qcRTMP_Init(qcRTMP *r);
  void qcRTMP_Close(qcRTMP *r);
  qcRTMP *qcRTMP_Alloc(void);
  void qcRTMP_Free(qcRTMP *r);
  void qcRTMP_EnableWrite(qcRTMP *r);

  void *qcRTMP_TLS_AllocServerContext(const char* cert, const char* key);
  void qcRTMP_TLS_FreeServerContext(void *ctx);

  int qcRTMP_LibVersion(void);
  void qcRTMP_UserInterrupt(void);	/* user typed Ctrl-C */

  int qcRTMP_SendCtrl(qcRTMP *r, short nType, unsigned int nObject,
		     unsigned int nTime);

  /* caller probably doesn't know current timestamp, should
   * just use RTMP_Pause instead
   */
  int qcRTMP_SendPause(qcRTMP *r, int DoPause, int dTime);
  int qcRTMP_Pause(qcRTMP *r, int DoPause);

  int qcRTMP_FindFirstMatchingProperty(qcAMFObject *obj, const AVal *name,
				      qcAMFObjectProperty * p);
  int qcRTMPSockBuf_Fill(qcRTMP *r, qcRTMPSockBuf *sb);
  int qcRTMPSockBuf_Send(qcRTMPSockBuf *sb, const char *buf, int len);
  int qcRTMPSockBuf_Close(qcRTMPSockBuf *sb);

  int qcRTMP_SendCreateStream(qcRTMP *r);
  int qcRTMP_SendSeek(qcRTMP *r, int dTime);
  int qcRTMP_SendServerBW(qcRTMP *r);
  int qcRTMP_SendClientBW(qcRTMP *r);
  void qcRTMP_DropRequest(qcRTMP *r, int i, int freeit);
  int qcRTMP_Read(qcRTMP *r, char *buf, int size);
  int qcRTMP_Write(qcRTMP *r, const char *buf, int size);
  void qcRTMP_SetSocketBlock(qcRTMP *r);
  void qcRTMP_SetSocketNonBlock(qcRTMP *r);
  int qcRTMP_WaitSocketWriteBuffer(qcRTMP* r);

/* hashswf.c */
  int qcRTMP_HashSWF(const char *url, unsigned int *size, unsigned char *hash,
		   int age);

  typedef char * (*QC_INETNTOP) (int nFamily, void * pAddr, char * pStringBuf, int nStringBufSize);

#ifdef __cplusplus
    };
#endif // __cplusplus
        
#endif
