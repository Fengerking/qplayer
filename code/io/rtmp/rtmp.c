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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "rtmp_sys.h"
#include "log.h"

#ifdef __QC_OS_WIN32__
#include <winsock2.h>
#include "Ws2tcpip.h"
#else
#include <fcntl.h>
#include <sys/time.h>
#endif

#ifdef CRYPTO
#ifdef USE_POLARSSL
#include <polarssl/havege.h>
#include <polarssl/md5.h>
#include <polarssl/base64.h>
#define MD5_DIGEST_LENGTH 16

static const char *qc_my_dhm_P =
    "E4004C1F94182000103D883A448B3F80" \
    "2CE4B44A83301270002C20D0321CFD00" \
    "11CCEF784C26A400F43DFB901BCA7538" \
    "F2C6B176001CF5A0FD16D2C48B1D0C1C" \
    "F6AC8E1DA6BCC3B4E1F96B0564965300" \
    "FFA1D0B601EB2800F489AA512C4B248C" \
    "01F76949A60BB7F00A40B1EAB64BDD48" \
    "E8A700D60B7F1200FA8E77B0A979DABF";

static const char *qc_my_dhm_G = "4";

#elif defined(USE_GNUTLS)
#include <gnutls/gnutls.h>
#define MD5_DIGEST_LENGTH 16
#include <nettle/base64.h>
#include <nettle/md5.h>
#else	/* USE_OPENSSL */
#include <openssl/ssl.h>
#include <openssl/rc4.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#endif
TLS_CTX qcRTMP_TLS_ctx;
#endif

#define qcRTMP_SIG_SIZE				1536
#define qcRTMP_LARGE_HEADER_SIZE	12
#define qcRTMP_ERR_NOTREADY			10035

static const int qc_packetSize[] = { 12, 8, 4, 1 };

int qcRTMP_ctrlC;

const char qcRTMPProtocolStrings[][7] = {
  "RTMP",
  "RTMPT",
  "RTMPE",
  "RTMPTE",
  "RTMPS",
  "RTMPTS",
  "",
  "",
  "RTMFP"
};

const char qcRTMPProtocolStringsLower[][7] = {
  "rtmp",
  "rtmpt",
  "rtmpe",
  "rtmpte",
  "rtmps",
  "rtmpts",
  "",
  "",
  "rtmfp"
};

static const char *qcRTMPT_cmds[] = {
  "open",
  "send",
  "idle",
  "close"
};

typedef enum {
  qcRTMPT_OPEN=0, qcRTMPT_SEND, qcRTMPT_IDLE, qcRTMPT_CLOSE
} qcRTMPTCmd;

static int qcDumpMetaData(qcAMFObject *obj);
static int qcHandShake(qcRTMP *r, int FP9HandShake);
static int qcSocksNegotiate(qcRTMP *r);

static int qcSendConnectPacket(qcRTMP *r, qcRTMPPacket *cp);
static int qcSendCheckBW(qcRTMP *r);
static int qcSendCheckBWResult(qcRTMP *r, double txn);
static int qcSendDeleteStream(qcRTMP *r, double dStreamId);
static int qcSendFCSubscribe(qcRTMP *r, AVal *subscribepath);
static int qcSendPlay(qcRTMP *r);
static int qcSendBytesReceived(qcRTMP *r);
static int qcSendUsherToken(qcRTMP *r, AVal *usherToken);

#if 0				/* unused */
static int SendBGHasStream(qcRTMP *r, double dId, AVal *playpath);
#endif

static int qcHandleInvoke(qcRTMP *r, const char *body, unsigned int nBodySize);
static int qcHandleMetadata(qcRTMP *r, char *body, unsigned int len);
static void qcHandleChangeChunkSize(qcRTMP *r, const qcRTMPPacket *packet);
static void qcHandleAudio(qcRTMP *r, const qcRTMPPacket *packet);
static void qcHandleVideo(qcRTMP *r, const qcRTMPPacket *packet);
static void qcHandleCtrl(qcRTMP *r, const qcRTMPPacket *packet);
static void qcHandleServerBW(qcRTMP *r, const qcRTMPPacket *packet);
static void qcHandleClientBW(qcRTMP *r, const qcRTMPPacket *packet);

static int qcReadN(qcRTMP *r, char *buffer, int n);
static int qcWriteN(qcRTMP *r, const char *buffer, int n);

static void qcDecodeTEA(AVal *key, AVal *text);

static int qcHTTP_Post(qcRTMP *r, qcRTMPTCmd cmd, const char *buf, int len);
static int qcHTTP_read(qcRTMP *r, int fill);

static void qcCloseInternal(qcRTMP *r, int reconnect);

#ifndef _WIN32
static int qc_clk_tck;
#endif

#ifdef CRYPTO
#include "handshake.h"
#endif

uint32_t
qcRTMP_GetTime()
{
#if defined(_WIN32)
  return timeGetTime();
#else
  struct tms t;
  if (!qc_clk_tck) qc_clk_tck = sysconf(_SC_CLK_TCK);
  return times(&t) * 1000 / qc_clk_tck;
#endif
}

void
qcRTMP_UserInterrupt()
{
  qcRTMP_ctrlC = TRUE;
}

void
qcRTMPPacket_Reset(qcRTMPPacket *p)
{
  p->m_headerType = 0;
  p->m_packetType = 0;
  p->m_nChannel = 0;
  p->m_nTimeStamp = 0;
  p->m_nInfoField2 = 0;
  p->m_hasAbsTimestamp = FALSE;
  p->m_nBodySize = 0;
  p->m_nBytesRead = 0;
}

int
qcRTMPPacket_Alloc(qcRTMPPacket *p, uint32_t nSize)
{
  char *ptr;
  if (nSize > SIZE_MAX - qcRTMP_MAX_HEADER_SIZE)
    return FALSE;
  ptr = calloc(1, nSize + qcRTMP_MAX_HEADER_SIZE);
  if (!ptr)
    return FALSE;
  p->m_body = ptr + qcRTMP_MAX_HEADER_SIZE;
  p->m_nBytesRead = 0;
  return TRUE;
}

void
qcRTMPPacket_Free(qcRTMPPacket *p)
{
  if (p->m_body)
    {
      free(p->m_body - qcRTMP_MAX_HEADER_SIZE);
      p->m_body = NULL;
    }
}

void
qcRTMPPacket_Dump(qcRTMPPacket *p)
{
  qcRTMP_Log(qcRTMP_LOGDEBUG,
      "RTMP PACKET: packet type: 0x%02x. channel: 0x%02x. info 1: %d info 2: %d. Body size: %u. body: 0x%02x",
      p->m_packetType, p->m_nChannel, p->m_nTimeStamp, p->m_nInfoField2,
      p->m_nBodySize, p->m_body ? (unsigned char)p->m_body[0] : 0);
}

int
qcRTMP_LibVersion()
{
  return qcRTMP_LIB_VERSION;
}

void
qcRTMP_TLS_Init()
{
#ifdef CRYPTO
#ifdef USE_POLARSSL
  /* Do this regardless of NO_SSL, we use havege for rtmpe too */
  qcRTMP_TLS_ctx = calloc(1,sizeof(struct qc_tls_ctx));
  havege_init(&qcRTMP_TLS_ctx->hs);
#elif defined(USE_GNUTLS) && !defined(NO_SSL)
  /* Technically we need to initialize libgcrypt ourselves if
   * we're not going to call gnutls_global_init(). Ignoring this
   * for now.
   */
  gnutls_global_init();
  qcRTMP_TLS_ctx = malloc(sizeof(struct qc_tls_ctx));
  gnutls_certificate_allocate_credentials(&qcRTMP_TLS_ctx->cred);
  gnutls_priority_init(&qcRTMP_TLS_ctx->prios, "NORMAL", NULL);
  gnutls_certificate_set_x509_trust_file(qcRTMP_TLS_ctx->cred,
  	"ca.pem", GNUTLS_X509_FMT_PEM);
#elif !defined(NO_SSL) /* USE_OPENSSL */
  /* libcrypto doesn't need anything special */
  SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_digests();
  qcRTMP_TLS_ctx = SSL_CTX_new(SSLv23_method());
  SSL_CTX_set_options(qcRTMP_TLS_ctx, SSL_OP_ALL);
  SSL_CTX_set_default_verify_paths(qcRTMP_TLS_ctx);
#endif
#endif
}

void *
qcRTMP_TLS_AllocServerContext(const char* cert, const char* key)
{
  void *ctx = NULL;
#ifdef CRYPTO
  if (!qcRTMP_TLS_ctx)
    qcRTMP_TLS_Init();
#ifdef USE_POLARSSL
  qc_tls_server_ctx *tc = ctx = calloc(1, sizeof(struct qc_tls_server_ctx));
  tc->dhm_P = my_dhm_P;
  tc->dhm_G = my_dhm_G;
  tc->hs = &qcRTMP_TLS_ctx->hs;
  if (x509parse_crtfile(&tc->cert, cert)) {
      free(tc);
      return NULL;
  }
  if (x509parse_keyfile(&tc->key, key, NULL)) {
      x509_free(&tc->cert);
      free(tc);
      return NULL;
  }
#elif defined(USE_GNUTLS) && !defined(NO_SSL)
  gnutls_certificate_allocate_credentials((gnutls_certificate_credentials*) &ctx);
  if (gnutls_certificate_set_x509_key_file(ctx, cert, key, GNUTLS_X509_FMT_PEM) != 0) {
    gnutls_certificate_free_credentials(ctx);
    return NULL;
  }
#elif !defined(NO_SSL) /* USE_OPENSSL */
  ctx = SSL_CTX_new(SSLv23_server_method());
  if (!SSL_CTX_use_certificate_chain_file(ctx, cert)) {
      SSL_CTX_free(ctx);
      return NULL;
  }
  if (!SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM)) {
      SSL_CTX_free(ctx);
      return NULL;
  }
#endif
#endif
  return ctx;
}

void
qcRTMP_TLS_FreeServerContext(void *ctx)
{
#ifdef CRYPTO
#ifdef USE_POLARSSL
  x509_free(&((qc_tls_server_ctx*)ctx)->cert);
  rsa_free(&((qc_tls_server_ctx*)ctx)->key);
  free(ctx);
#elif defined(USE_GNUTLS) && !defined(NO_SSL)
  gnutls_certificate_free_credentials(ctx);
#elif !defined(NO_SSL) /* USE_OPENSSL */
  SSL_CTX_free(ctx);
#endif
#endif
}

qcRTMP *
qcRTMP_Alloc()
{
  return calloc(1, sizeof(qcRTMP));
}

void
qcRTMP_Free(qcRTMP *r)
{
  free(r);
}

void
qcRTMP_Init(qcRTMP *r)
{
#ifdef CRYPTO
  if (!qcRTMP_TLS_ctx)
    qcRTMP_TLS_Init();
#endif

  memset(r, 0, sizeof(qcRTMP));
  r->m_sb.sb_socket = -1;
  r->m_inChunkSize = qcRTMP_DEFAULT_CHUNKSIZE;
  r->m_outChunkSize = qcRTMP_DEFAULT_CHUNKSIZE;
  r->m_nBufferMS = 30000;
  r->m_nClientBW = 2500000;
  r->m_nClientBW2 = 2;
  r->m_nServerBW = 2500000;
  r->m_fAudioCodecs = 3191.0;
  r->m_fVideoCodecs = 252.0;
  r->Link.timeout = 500;// 30; read tiemout in ms
  r->Link.swfAge = 30;
  r->forceClose = 0;
  r->Link.timeoutConnect = 10000; // connect timeout in ms
  r->fIPAddrInfo = NULL;
  r->Link.sockAddr = NULL;
}

void
qcRTMP_EnableWrite(qcRTMP *r)
{
  r->Link.protocol |= qcRTMP_FEATURE_WRITE;
}

double
qcRTMP_GetDuration(qcRTMP *r)
{
  return r->m_fDuration;
}

int
qcRTMP_IsConnected(qcRTMP *r)
{
  return r->m_sb.sb_socket != -1;
}

int
qcRTMP_Socket(qcRTMP *r)
{
  return r->m_sb.sb_socket;
}

int
qcRTMP_IsTimedout(qcRTMP *r)
{
  return r->m_sb.sb_timedout;
}

void
qcRTMP_SetBufferMS(qcRTMP *r, int size)
{
  r->m_nBufferMS = size;
}

void
qcRTMP_UpdateBufferMS(qcRTMP *r)
{
  qcRTMP_SendCtrl(r, 3, r->m_stream_id, r->m_nBufferMS);
}

#undef OSS
#ifdef _WIN32
#define OSS	"WIN"
#elif defined(__sun__)
#define OSS	"SOL"
#elif defined(__APPLE__)
#define OSS	"MAC"
#elif defined(__linux__)
#define OSS	"LNX"
#else
#define OSS	"GNU"
#endif
#define DEF_VERSTR	OSS " 10,0,32,18"
static const char QC_DEFAULT_FLASH_VER[] = DEF_VERSTR;
const AVal qcRTMP_DefaultFlashVer =
  { (char *)QC_DEFAULT_FLASH_VER, sizeof(QC_DEFAULT_FLASH_VER) - 1 };

static void
qcSocksSetup(qcRTMP *r, AVal *sockshost)
{
  if (sockshost->av_len)
    {
      const char *socksport = strchr(sockshost->av_val, ':');
      char *hostname = strdup(sockshost->av_val);

      if (socksport)
	hostname[socksport - sockshost->av_val] = '\0';
      r->Link.sockshost.av_val = hostname;
      r->Link.sockshost.av_len = strlen(hostname);

      r->Link.socksport = socksport ? atoi(socksport + 1) : 1080;
      qcRTMP_Log(qcRTMP_LOGDEBUG, "Connecting via SOCKS proxy: %s:%d", r->Link.sockshost.av_val,
	  r->Link.socksport);
    }
  else
    {
      r->Link.sockshost.av_val = NULL;
      r->Link.sockshost.av_len = 0;
      r->Link.socksport = 0;
    }
}

void
qcRTMP_SetupStream(qcRTMP *r,
		 int protocol,
		 AVal *host,
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
		 int dStop, int bLiveStream, long int timeout)
{
  qcRTMP_Log(qcRTMP_LOGDEBUG, "Protocol : %s", qcRTMPProtocolStrings[protocol&7]);
  qcRTMP_Log(qcRTMP_LOGDEBUG, "Hostname : %.*s", host->av_len, host->av_val);
  qcRTMP_Log(qcRTMP_LOGDEBUG, "Port     : %d", port);
  qcRTMP_Log(qcRTMP_LOGDEBUG, "Playpath : %s", playpath->av_val);

  if (tcUrl && tcUrl->av_val)
    qcRTMP_Log(qcRTMP_LOGDEBUG, "tcUrl    : %s", tcUrl->av_val);
  if (swfUrl && swfUrl->av_val)
    qcRTMP_Log(qcRTMP_LOGDEBUG, "swfUrl   : %s", swfUrl->av_val);
  if (pageUrl && pageUrl->av_val)
    qcRTMP_Log(qcRTMP_LOGDEBUG, "pageUrl  : %s", pageUrl->av_val);
  if (app && app->av_val)
    qcRTMP_Log(qcRTMP_LOGDEBUG, "app      : %.*s", app->av_len, app->av_val);
  if (auth && auth->av_val)
    qcRTMP_Log(qcRTMP_LOGDEBUG, "auth     : %s", auth->av_val);
  if (subscribepath && subscribepath->av_val)
    qcRTMP_Log(qcRTMP_LOGDEBUG, "subscribepath : %s", subscribepath->av_val);
  if (usherToken && usherToken->av_val)
    qcRTMP_Log(qcRTMP_LOGDEBUG, "NetStream.Authenticate.UsherToken : %s", usherToken->av_val);
  if (flashVer && flashVer->av_val)
    qcRTMP_Log(qcRTMP_LOGDEBUG, "flashVer : %s", flashVer->av_val);
  if (dStart > 0)
    qcRTMP_Log(qcRTMP_LOGDEBUG, "StartTime     : %d msec", dStart);
  if (dStop > 0)
    qcRTMP_Log(qcRTMP_LOGDEBUG, "StopTime      : %d msec", dStop);

  qcRTMP_Log(qcRTMP_LOGDEBUG, "live     : %s", bLiveStream ? "yes" : "no");
  qcRTMP_Log(qcRTMP_LOGDEBUG, "timeout  : %ld sec", timeout);

#ifdef CRYPTO
  if (swfSHA256Hash != NULL && swfSize > 0)
    {
      memcpy(r->Link.SWFHash, swfSHA256Hash->av_val, sizeof(r->Link.SWFHash));
      r->Link.SWFSize = swfSize;
      qcRTMP_Log(qcRTMP_LOGDEBUG, "SWFSHA256:");
      qcRTMP_LogHex(qcRTMP_LOGDEBUG, r->Link.SWFHash, sizeof(r->Link.SWFHash));
      qcRTMP_Log(qcRTMP_LOGDEBUG, "SWFSize  : %u", r->Link.SWFSize);
    }
  else
    {
      r->Link.SWFSize = 0;
    }
#endif

  qcSocksSetup(r, sockshost);

  if (tcUrl && tcUrl->av_len)
    r->Link.tcUrl = *tcUrl;
  if (swfUrl && swfUrl->av_len)
    r->Link.swfUrl = *swfUrl;
  if (pageUrl && pageUrl->av_len)
    r->Link.pageUrl = *pageUrl;
  if (app && app->av_len)
    r->Link.app = *app;
  if (auth && auth->av_len)
    {
      r->Link.auth = *auth;
      r->Link.lFlags |= qcRTMP_LF_AUTH;
    }
  if (flashVer && flashVer->av_len)
    r->Link.flashVer = *flashVer;
  else
    r->Link.flashVer = qcRTMP_DefaultFlashVer;
  if (subscribepath && subscribepath->av_len)
    r->Link.subscribepath = *subscribepath;
  if (usherToken && usherToken->av_len)
    r->Link.usherToken = *usherToken;
  r->Link.seekTime = dStart;
  r->Link.stopTime = dStop;
  if (bLiveStream)
    r->Link.lFlags |= qcRTMP_LF_LIVE;
  r->Link.timeout = timeout;

  r->Link.protocol = protocol;
  r->Link.hostname = *host;
  r->Link.port = port;
  r->Link.playpath = *playpath;

  if (r->Link.port == 0)
    {
      if (protocol & qcRTMP_FEATURE_SSL)
	r->Link.port = 443;
      else if (protocol & qcRTMP_FEATURE_HTTP)
	r->Link.port = 80;
      else
	r->Link.port = 1935;
    }
}

enum { OPT_STR=0, OPT_INT, OPT_BOOL, OPT_CONN };
static const char *qc_optinfo[] = {
	"string", "integer", "boolean", "AMF" };

#define OFF(x)	offsetof(struct qcRTMP,x)

static struct qc_urlopt {
  AVal name;
  off_t off;
  int otype;
  int omisc;
  char *use;
} qc_options[] = {
  { AVC("socks"),     OFF(Link.sockshost),     OPT_STR, 0,
  	"Use the specified SOCKS proxy" },
  { AVC("app"),       OFF(Link.app),           OPT_STR, 0,
	"Name of target app on server" },
  { AVC("tcUrl"),     OFF(Link.tcUrl),         OPT_STR, 0,
  	"URL to played stream" },
  { AVC("pageUrl"),   OFF(Link.pageUrl),       OPT_STR, 0,
  	"URL of played media's web page" },
  { AVC("swfUrl"),    OFF(Link.swfUrl),        OPT_STR, 0,
  	"URL to player SWF file" },
  { AVC("flashver"),  OFF(Link.flashVer),      OPT_STR, 0,
  	"Flash version string (default " DEF_VERSTR ")" },
  { AVC("conn"),      OFF(Link.extras),        OPT_CONN, 0,
  	"Append arbitrary AMF data to Connect message" },
  { AVC("playpath"),  OFF(Link.playpath),      OPT_STR, 0,
  	"Path to target media on server" },
  { AVC("playlist"),  OFF(Link.lFlags),        OPT_BOOL, qcRTMP_LF_PLST,
  	"Set playlist before play command" },
  { AVC("live"),      OFF(Link.lFlags),        OPT_BOOL, qcRTMP_LF_LIVE,
  	"Stream is live, no seeking possible" },
  { AVC("subscribe"), OFF(Link.subscribepath), OPT_STR, 0,
  	"Stream to subscribe to" },
  { AVC("jtv"), OFF(Link.usherToken),          OPT_STR, 0,
	"Justin.tv authentication token" },
  { AVC("token"),     OFF(Link.token),	       OPT_STR, 0,
  	"Key for SecureToken response" },
  { AVC("swfVfy"),    OFF(Link.lFlags),        OPT_BOOL, qcRTMP_LF_SWFV,
  	"Perform SWF Verification" },
  { AVC("swfAge"),    OFF(Link.swfAge),        OPT_INT, 0,
  	"Number of days to use cached SWF hash" },
  { AVC("start"),     OFF(Link.seekTime),      OPT_INT, 0,
  	"Stream start position in milliseconds" },
  { AVC("stop"),      OFF(Link.stopTime),      OPT_INT, 0,
  	"Stream stop position in milliseconds" },
  { AVC("buffer"),    OFF(m_nBufferMS),        OPT_INT, 0,
  	"Buffer time in milliseconds" },
  { AVC("timeout"),   OFF(Link.timeout),       OPT_INT, 0,
  	"Session timeout in seconds" },
  { AVC("pubUser"),   OFF(Link.pubUser),       OPT_STR, 0,
        "Publisher username" },
  { AVC("pubPasswd"), OFF(Link.pubPasswd),     OPT_STR, 0,
        "Publisher password" },
  { {NULL,0}, 0, 0}
};

static const AVal qc_truth[] = {
	AVC("1"),
	AVC("on"),
	AVC("yes"),
	AVC("true"),
	{0,0}
};

static void qcRTMP_OptUsage()
{
  int i;

  qcRTMP_Log(qcRTMP_LOGERROR, "Valid RTMP options are:\n");
  for (i=0; qc_options[i].name.av_len; i++) {
    qcRTMP_Log(qcRTMP_LOGERROR, "%10s %-7s  %s\n", qc_options[i].name.av_val,
    	qc_optinfo[qc_options[i].otype], qc_options[i].use);
  }
}

static int
qc_parseAMF(qcAMFObject *obj, AVal *av, int *depth)
{
  qcAMFObjectProperty prop = {{0,0}};
  int i;
  char *p, *arg = av->av_val;

  if (arg[1] == ':')
    {
      p = (char *)arg+2;
      switch(arg[0])
	{
	case 'B':
	  prop.p_type = qcAMF_BOOLEAN;
	  prop.p_vu.p_number = atoi(p);
	  break;
	case 'S':
	  prop.p_type = qcAMF_STRING;
	  prop.p_vu.p_aval.av_val = p;
	  prop.p_vu.p_aval.av_len = av->av_len - (p-arg);
	  break;
	case 'N':
	  prop.p_type = qcAMF_NUMBER;
	  prop.p_vu.p_number = strtod(p, NULL);
	  break;
	case 'Z':
	  prop.p_type = qcAMF_NULL;
	  break;
	case 'O':
	  i = atoi(p);
	  if (i)
	    {
	      prop.p_type = qcAMF_OBJECT;
	    }
	  else
	    {
	      (*depth)--;
	      return 0;
	    }
	  break;
	default:
	  return -1;
	}
    }
  else if (arg[2] == ':' && arg[0] == 'N')
    {
      p = strchr(arg+3, ':');
      if (!p || !*depth)
	return -1;
      prop.p_name.av_val = (char *)arg+3;
      prop.p_name.av_len = p - (arg+3);

      p++;
      switch(arg[1])
	{
	case 'B':
	  prop.p_type = qcAMF_BOOLEAN;
	  prop.p_vu.p_number = atoi(p);
	  break;
	case 'S':
	  prop.p_type = qcAMF_STRING;
	  prop.p_vu.p_aval.av_val = p;
	  prop.p_vu.p_aval.av_len = av->av_len - (p-arg);
	  break;
	case 'N':
	  prop.p_type = qcAMF_NUMBER;
	  prop.p_vu.p_number = strtod(p, NULL);
	  break;
	case 'O':
	  prop.p_type = qcAMF_OBJECT;
	  break;
	default:
	  return -1;
	}
    }
  else
    return -1;

  if (*depth)
    {
      qcAMFObject *o2;
      for (i=0; i<*depth; i++)
	{
	  o2 = &obj->o_props[obj->o_num-1].p_vu.p_object;
	  obj = o2;
	}
    }
  qcAMF_AddProp(obj, &prop);
  if (prop.p_type == qcAMF_OBJECT)
    (*depth)++;
  return 0;
}

int qcRTMP_SetOpt(qcRTMP *r, const AVal *opt, AVal *arg)
{
  int i;
  void *v;

  for (i=0; qc_options[i].name.av_len; i++) {
    if (opt->av_len != qc_options[i].name.av_len) continue;
    if (strcasecmp(opt->av_val, qc_options[i].name.av_val)) continue;
    v = (char *)r + qc_options[i].off;
    switch(qc_options[i].otype) {
    case OPT_STR: {
      AVal *aptr = v;
      *aptr = *arg; }
      break;
    case OPT_INT: {
      long l = strtol(arg->av_val, NULL, 0);
      *(int *)v = l; }
      break;
    case OPT_BOOL: {
      int j, fl;
      fl = *(int *)v;
      for (j=0; qc_truth[j].av_len; j++) {
        if (arg->av_len != qc_truth[j].av_len) continue;
        if (strcasecmp(arg->av_val, qc_truth[j].av_val)) continue;
        fl |= qc_options[i].omisc; break; }
      *(int *)v = fl;
      }
      break;
    case OPT_CONN:
      if (qc_parseAMF(&r->Link.extras, arg, &r->Link.edepth))
        return FALSE;
      break;
    }
    break;
  }
  if (!qc_options[i].name.av_len) {
    qcRTMP_Log(qcRTMP_LOGERROR, "Unknown option %s", opt->av_val);
    qcRTMP_OptUsage();
    return FALSE;
  }

  return TRUE;
}

int qcRTMP_SetupURL(qcRTMP *r, char *url)
{
  AVal opt, arg;
  char *p1, *p2, *ptr = strchr(url, ' ');
  int ret, len;
  unsigned int port = 0;

  if (ptr)
    *ptr = '\0';

  len = strlen(url);
  ret = qcRTMP_ParseURL(url, &r->Link.protocol, &r->Link.hostname,
  	&port, &r->Link.playpath0, &r->Link.app);
  if (!ret)
    return ret;
  r->Link.port = port;
  r->Link.playpath = r->Link.playpath0;

  while (ptr) {
    *ptr++ = '\0';
    p1 = ptr;
    p2 = strchr(p1, '=');
    if (!p2)
      break;
    opt.av_val = p1;
    opt.av_len = p2 - p1;
    *p2++ = '\0';
    arg.av_val = p2;
    ptr = strchr(p2, ' ');
    if (ptr) {
      *ptr = '\0';
      arg.av_len = ptr - p2;
      /* skip repeated spaces */
      while(ptr[1] == ' ')
      	*ptr++ = '\0';
    } else {
      arg.av_len = strlen(p2);
    }

    /* unescape */
    port = arg.av_len;
    for (p1=p2; port >0;) {
      if (*p1 == '\\') {
	unsigned int c;
	if (port < 3)
	  return FALSE;
	sscanf(p1+1, "%02x", &c);
	*p2++ = c;
	port -= 3;
	p1 += 3;
      } else {
	*p2++ = *p1++;
	port--;
      }
    }
    arg.av_len = p2 - arg.av_val;

    ret = qcRTMP_SetOpt(r, &opt, &arg);
    if (!ret)
      return ret;
  }

  if (!r->Link.tcUrl.av_len)
    {
      r->Link.tcUrl.av_val = url;
      if (r->Link.app.av_len)
        {
          if (r->Link.app.av_val < url + len)
    	    {
    	      /* if app is part of original url, just use it */
              r->Link.tcUrl.av_len = r->Link.app.av_len + (r->Link.app.av_val - url);
    	    }
    	  else
    	    {
    	      len = r->Link.hostname.av_len + r->Link.app.av_len +
    		  sizeof("rtmpte://:65535/");
	      r->Link.tcUrl.av_val = malloc(len);
	      r->Link.tcUrl.av_len = snprintf(r->Link.tcUrl.av_val, len,
		"%s://%.*s:%d/%.*s",
		qcRTMPProtocolStringsLower[r->Link.protocol],
		r->Link.hostname.av_len, r->Link.hostname.av_val,
		r->Link.port,
		r->Link.app.av_len, r->Link.app.av_val);
	      r->Link.lFlags |= qcRTMP_LF_FTCU;
	    }
        }
      else
        {
	  r->Link.tcUrl.av_len = strlen(url);
	}
    }

#ifdef CRYPTO
  if ((r->Link.lFlags & qcRTMP_LF_SWFV) && r->Link.swfUrl.av_len)
    qcRTMP_HashSWF(r->Link.swfUrl.av_val, &r->Link.SWFSize,
	  (unsigned char *)r->Link.SWFHash, r->Link.swfAge);
#endif

  qcSocksSetup(r, &r->Link.sockshost);

  if (r->Link.port == 0)
    {
      if (r->Link.protocol & qcRTMP_FEATURE_SSL)
	r->Link.port = 443;
      else if (r->Link.protocol & qcRTMP_FEATURE_HTTP)
	r->Link.port = 80;
      else
	r->Link.port = 1935;
    }
  return TRUE;
}

int
qcRTMP_Connect0(qcRTMP *r, struct sockaddr * service)
{
    int on = 1;
    r->m_sb.sb_timedout = FALSE;
    r->m_pausing = 0;
    r->m_fDuration = 0.0;
    
    int ret = TRUE;
    void* svraddr = NULL;
    int svraddr_len = 0;
    struct sockaddr_in svraddr_4;
    struct sockaddr_in6 svraddr_6;
    int port = r->Link.socksport?r->Link.socksport:r->Link.port;
    
//    int maxlen = 64;
//    memset(r->Link.resolveip, 0, maxlen);
    
    r->m_sb.sb_socket = socket(service->sa_family, SOCK_STREAM, 0);
    if(r->m_sb.sb_socket <= 0)
    {
        int err = GetSockError();
        qcRTMP_Log(qcRTMP_LOGERROR, "%s, failed to create socket handle. %d (%s), port %d",
                   __FUNCTION__, err, strerror(err), r->Link.port);
    }

    qcRTMP_SetSocketNonBlock(r);
    
    switch(service->sa_family)
    {
        case AF_INET://ipv4
            svraddr_4.sin_family = AF_INET;
            //svraddr_4.sin_addr.s_addr = inet_addr(r->Link.resolveip);
            svraddr_4.sin_addr.s_addr = ((struct sockaddr_in *)service)->sin_addr.s_addr;
            svraddr_4.sin_port = htons(port);
            svraddr_len = sizeof(svraddr_4);
            svraddr = &svraddr_4;
            break;
        case AF_INET6://ipv6
            memset(&svraddr_6, 0, sizeof(svraddr_6));
            svraddr_6.sin6_family = AF_INET6;
            svraddr_6.sin6_port = htons(port);
            memcpy(&svraddr_6.sin6_addr, &(((struct sockaddr_in6 *)service)->sin6_addr), sizeof(struct in6_addr));
            svraddr_len = sizeof(svraddr_6);
            svraddr = &svraddr_6;
            break;
            
        default:
            ret = FALSE;
    }
    
    if (r->m_sb.sb_socket != -1)
    {
        if (connect(r->m_sb.sb_socket, svraddr, svraddr_len) < 0)
        {
            if(1 != qcRTMP_WaitSocketWriteBuffer(r))
            {
                int err = GetSockError();
                qcRTMP_Log(qcRTMP_LOGERROR, "%s, failed to connect socket. %d (%s), port %d",
                           __FUNCTION__, err, strerror(err), r->Link.port);
                qcRTMP_Close(r);
                return FALSE;
            }
        }
        
        if (r->Link.socksport)
        {
            qcRTMP_Log(qcRTMP_LOGDEBUG, "%s ... SOCKS negotiation", __FUNCTION__);
            if (!qcSocksNegotiate(r))
            {
                qcRTMP_Log(qcRTMP_LOGERROR, "%s, SOCKS negotiation failed.", __FUNCTION__);
                qcRTMP_Close(r);
                return FALSE;
            }
        }
    }
    else
    {
        qcRTMP_Log(qcRTMP_LOGERROR, "%s, failed to create socket. Error: %d", __FUNCTION__,
                   GetSockError());
        return FALSE;
    }
    
#ifndef __QC_OS_WIN32__
    qcRTMP_SetSocketBlock(r);
#endif // __QC_OS_WIN32__    
    if (r->forceClose == 1)
    {
        qcRTMP_Log(qcRTMP_LOGWARNING, "%s, force to quit connect.", __FUNCTION__);
        return FALSE;
    }
    
    /* set timeout */
    {
        //SET_RCVTIMEO(tv, r->Link.timeout);
        //struct timeval tv = {0, 500000};
//        int mili = (r->Link.timeout % 1000) * 1000;
//        int secs = r->Link.timeout / 1000;
//        struct timeval tv = {secs, mili};
        struct timeval tv = {0, 100000};
        if (setsockopt
            (r->m_sb.sb_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)))
        {
            qcRTMP_Log(qcRTMP_LOGERROR, "%s, Setting socket recv timeout to %ds failed!",
                       __FUNCTION__, r->Link.timeout);
        }
        if (setsockopt
            (r->m_sb.sb_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)))
        {
            qcRTMP_Log(qcRTMP_LOGERROR, "%s, Setting socket send timeout to %ds failed!",
                       __FUNCTION__, r->Link.timeout);
        }
    }
    
    setsockopt(r->m_sb.sb_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &on, sizeof(on));
    
#ifdef __QC_OS_IOS__
    int set = 1;
    setsockopt(r->m_sb.sb_socket, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
    
    return TRUE;
}


static int
qc_add_addr_info(qcRTMP *r, struct sockaddr *service, AVal *host, int port)
{
    char *hostname = NULL;
    if (host->av_val[host->av_len])
    {
        hostname = malloc(host->av_len+1);
        memcpy(hostname, host->av_val, host->av_len);
        hostname[host->av_len] = '\0';
    }
    else
    {
        hostname = host->av_val;
    }
//    qcRTMP_Log(qcRTMP_LOGINFO, "sockaddr_in size %d, sockaddr_in6 size %d, sockaddr_storage %d", sizeof(svraddr_4),
//               sizeof(svraddr_6), sizeof(struct sockaddr_storage));
    
    struct addrinfo* result = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if(r->fIPAddrInfo && r->fIPAddrInfo->fGetAddrInfo && r->fIPAddrInfo->pUserData)
    {
        r->fIPAddrInfo->fGetAddrInfo(r->fIPAddrInfo->pUserData, hostname, NULL, &hints, &result);
    }
    else
        getaddrinfo(hostname, NULL, &hints, &result);
    
    if(result == NULL)
    {
        int err = GetSockError();
        qcRTMP_Log(qcRTMP_LOGERROR, "%s, failed to parse DNS. %d (%s), host %s", __FUNCTION__, err, strerror(err), hostname);
        if (hostname != host->av_val)
        	free(hostname);
        return 0;
    }
    
    memcpy(service, result->ai_addr, result->ai_addrlen);
    if(r->fIPAddrInfo && r->fIPAddrInfo->fFreeAddrInfo && r->fIPAddrInfo->pUserData)
        r->fIPAddrInfo->fFreeAddrInfo(r->fIPAddrInfo->pUserData, result);
    else
        freeaddrinfo(result);
    if (hostname != host->av_val)
        free(hostname);

    return TRUE;
}

/*
static int
qc_add_addr_info(struct sockaddr_in *service, AVal *host, int port)
{
  char *hostname;
  int ret = TRUE;
  if (host->av_val[host->av_len])
    {
      hostname = malloc(host->av_len+1);
      memcpy(hostname, host->av_val, host->av_len);
      hostname[host->av_len] = '\0';
    }
  else
    {
      hostname = host->av_val;
    }

  service->sin_addr.s_addr = inet_addr(hostname);
  if (service->sin_addr.s_addr == INADDR_NONE)
    {
      struct hostent *host = gethostbyname(hostname);
      if (host == NULL || host->h_addr == NULL)
	{
	  qcRTMP_Log(qcRTMP_LOGERROR, "Problem accessing the DNS. (addr: %s)", hostname);
	  ret = FALSE;
	  goto finish;
	}
      service->sin_addr = *(struct in_addr *)host->h_addr;
    }

  service->sin_port = htons(port);
finish:
  if (hostname != host->av_val)
    free(hostname);
  return ret;
}


int
qcRTMP_Connect0(qcRTMP *r, struct sockaddr * service)
{
  int on = 1;
  r->m_sb.sb_timedout = FALSE;
  r->m_pausing = 0;
  r->m_fDuration = 0.0;

  r->m_sb.sb_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (r->m_sb.sb_socket != -1)
    {
      if (connect(r->m_sb.sb_socket, service, sizeof(struct sockaddr)) < 0)
	{
	  int err = GetSockError();
	  qcRTMP_Log(qcRTMP_LOGERROR, "%s, failed to connect socket. %d (%s)",
	      __FUNCTION__, err, strerror(err));
	  qcRTMP_Close(r);
	  return FALSE;
	}

      if (r->Link.socksport)
	{
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s ... SOCKS negotiation", __FUNCTION__);
	  if (!qcSocksNegotiate(r))
	    {
	      qcRTMP_Log(qcRTMP_LOGERROR, "%s, SOCKS negotiation failed.", __FUNCTION__);
	      qcRTMP_Close(r);
	      return FALSE;
	    }
	}
    }
  else
    {
      qcRTMP_Log(qcRTMP_LOGERROR, "%s, failed to create socket. Error: %d", __FUNCTION__,
	  GetSockError());
      return FALSE;
    }

 
  {
    SET_RCVTIMEO(tv, r->Link.timeout);
      //struct timeval tv = {10, 500000};
    if (setsockopt
        (r->m_sb.sb_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)))
      {
        qcRTMP_Log(qcRTMP_LOGERROR, "%s, Setting socket recv timeout to %ds failed!",
	    __FUNCTION__, r->Link.timeout);
      }
    if (setsockopt
          (r->m_sb.sb_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)))
      {
          qcRTMP_Log(qcRTMP_LOGERROR, "%s, Setting socket send timeout to %ds failed!",
                     __FUNCTION__, r->Link.timeout);
      }
  }

  setsockopt(r->m_sb.sb_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &on, sizeof(on));

  return TRUE;
}
 */

int
qcRTMP_TLS_Accept(qcRTMP *r, void *ctx)
{
#if defined(CRYPTO) && !defined(NO_SSL)
  TLS_server(ctx, r->m_sb.sb_ssl);
  TLS_setfd(r->m_sb.sb_ssl, r->m_sb.sb_socket);
  if (TLS_accept(r->m_sb.sb_ssl) < 0)
    {
      qcRTMP_Log(qcRTMP_LOGERROR, "%s, TLS_Connect failed", __FUNCTION__);
      return FALSE;
    }
  return TRUE;
#else
  return FALSE;
#endif
}

int
qcRTMP_Connect1(qcRTMP *r, qcRTMPPacket *cp)
{
  if (r->Link.protocol & qcRTMP_FEATURE_SSL)
    {
#if defined(CRYPTO) && !defined(NO_SSL)
      TLS_client(qcRTMP_TLS_ctx, r->m_sb.sb_ssl);
      TLS_setfd(r->m_sb.sb_ssl, r->m_sb.sb_socket);
      if (TLS_connect(r->m_sb.sb_ssl) < 0)
	{
	  qcRTMP_Log(qcRTMP_LOGERROR, "%s, TLS_Connect failed", __FUNCTION__);
	  qcRTMP_Close(r);
	  return FALSE;
	}
#else
      qcRTMP_Log(qcRTMP_LOGERROR, "%s, no SSL/TLS support", __FUNCTION__);
      qcRTMP_Close(r);
      return FALSE;

#endif
    }
  if (r->Link.protocol & qcRTMP_FEATURE_HTTP)
    {
      r->m_msgCounter = 1;
      r->m_clientID.av_val = NULL;
      r->m_clientID.av_len = 0;
      qcHTTP_Post(r, qcRTMPT_OPEN, "", 1);
      if (qcHTTP_read(r, 1) != 0)
	{
	  r->m_msgCounter = 0;
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, Could not connect for handshake", __FUNCTION__);
	  qcRTMP_Close(r);
	  return 0;
	}
      r->m_msgCounter = 0;
    }
  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, ... connected, handshaking", __FUNCTION__);
  if (!qcHandShake(r, TRUE))
    {
      qcRTMP_Log(qcRTMP_LOGERROR, "%s, handshake failed.", __FUNCTION__);
      qcRTMP_Close(r);
      return FALSE;
    }
  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, handshaked", __FUNCTION__);

  if (!qcSendConnectPacket(r, cp))
    {
      qcRTMP_Log(qcRTMP_LOGERROR, "%s, RTMP connect failed.", __FUNCTION__);
      qcRTMP_Close(r);
      return FALSE;
    }
    qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, leave!!!!!", __FUNCTION__);
  return TRUE;
}

static int
qc_cache_addr_info(qcRTMP *r, AVal *host, struct sockaddr* addr, int addrSize)
{
    char *hostname = NULL;
    if (host->av_val[host->av_len])
    {
        hostname = malloc(host->av_len+1);
        memcpy(hostname, host->av_val, host->av_len);
        hostname[host->av_len] = '\0';
    }
    else
    {
        hostname = host->av_val;
    }

    if(r->fIPAddrInfo && r->fIPAddrInfo->fAddCache && r->fIPAddrInfo->pUserData)
    {
        r->fIPAddrInfo->fAddCache (r->fIPAddrInfo->pUserData, hostname, (void *)addr, addrSize, 999999);
        if (hostname != host->av_val)
            free(hostname);
        return 1;
    }
    
    if (hostname != host->av_val)
        free(hostname);
    return 0;
}

static int
qc_query_addr_info(qcRTMP *r, struct sockaddr *service, AVal *host, int port)
{
    char *hostname = NULL;
    if (host->av_val[host->av_len])
    {
        hostname = malloc(host->av_len+1);
        memcpy(hostname, host->av_val, host->av_len);
        hostname[host->av_len] = '\0';
    }
    else
    {
        hostname = host->av_val;
    }

    if(r->fIPAddrInfo && r->fIPAddrInfo->fGetCache && r->fIPAddrInfo->pUserData)
    {
        void* ai = r->fIPAddrInfo->fGetCache(r->fIPAddrInfo->pUserData, hostname);
        if(ai)
        {
            memcpy(service, ai, sizeof(struct sockaddr_storage));
            if (hostname != host->av_val)
                free(hostname);
            return 1;
        }
    }

    if (hostname != host->av_val)
        free(hostname);
    return 0;
}

int
qcRTMP_Connect(qcRTMP *r, qcRTMPPacket *cp)
{
    if (r->forceClose == 1)
    {
        qcRTMP_Log(qcRTMP_LOGWARNING, "%s, force to quit connect, 0.", __FUNCTION__);
        return FALSE;
    }

#ifdef __QC_OS_WIN32__
	QC_INETNTOP fInetNtop = NULL;
	void * hWSDll = NULL;
#endif // __QC_OS_WIN32__

    if(!r->Link.sockAddr)
    {
    	r->Link.sockAddr = malloc(sizeof(struct sockaddr_storage));
    }
    memset(r->Link.sockAddr, 0, sizeof(struct sockaddr_storage));

//  struct sockaddr_storage sockaddr;
//  memset(&sockaddr, 0, sizeof(struct sockaddr_storage));
  struct sockaddr* service = (struct sockaddr*)r->Link.sockAddr;
  if (!r->Link.hostname.av_len)
    return FALSE;

  //memset(service, 0, sizeof(struct sockaddr));
  //service.sin_family = AF_INET;

  int useTime = qcRTMP_GetTime();
  if (r->Link.socksport)
    {
        int ret = qc_query_addr_info(r, service, &r->Link.sockshost, r->Link.socksport);
        if(ret != 1)
        {
            /* Connect via SOCKS */
            if (!qc_add_addr_info(r, service, &r->Link.sockshost, r->Link.socksport))
                return FALSE;
            qc_cache_addr_info(r, &r->Link.sockshost, service, sizeof(struct sockaddr_storage));
        }
    }
  else
    {
        int ret = qc_query_addr_info(r, service, &r->Link.hostname, r->Link.port);
        if(ret != 1)
        {
            /* Connect directly */
            if (!qc_add_addr_info(r, service, &r->Link.hostname, r->Link.port))
                return FALSE;
            qc_cache_addr_info(r, &r->Link.hostname, service, sizeof(struct sockaddr_storage));
        }
    }
        
    qcRTMP_Log(qcRTMP_LOGINFO, "%s, DNS use time %d.", __FUNCTION__, qcRTMP_GetTime()-useTime);
    
  if (r->forceClose == 1)
  {
      qcRTMP_Log(qcRTMP_LOGWARNING, "%s, force to quit connect, 0.", __FUNCTION__);
      return FALSE;
  }

    useTime = qcRTMP_GetTime();
    int ret = qcRTMP_Connect0(r, (struct sockaddr *)service);
    qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, Connect0 use time %d. forceclose %d", __FUNCTION__, qcRTMP_GetTime()-useTime, r->forceClose);
    if (!ret)
        return FALSE;

  r->m_bSendCounter = TRUE;
//#ifdef __QC_OS_WIN32__
//    hWSDll = LoadLibrary("ws2_32.dll");
//    if (hWSDll != NULL)
//        fInetNtop = (QC_INETNTOP)GetProcAddress(hWSDll, "inet_ntop");
//    if (fInetNtop != NULL)
//        fInetNtop(service->sa_family, (void*)&(service->sa_family), r->Link.resolveip, INET6_ADDRSTRLEN);
//    if (hWSDll != NULL)
//        FreeLibrary(hWSDll);
//#else
//    inet_ntop(service->sa_family, (void*)&(((struct sockaddr_in*)service)->sin_addr), r->Link.resolveip, INET6_ADDRSTRLEN);
//#endif // __QC_OS_WIN32__
    
    qcRTMP_Log(qcRTMP_LOGINFO, "%s, DNS resolved, %s", __FUNCTION__,
               (service->sa_family==AF_INET6)?"ipv6":"ipv4");
    
    if (r->forceClose == 1)
    {
        qcRTMP_Log(qcRTMP_LOGWARNING, "%s, force to quit connect, 1.", __FUNCTION__);
        return FALSE;
    }

  useTime = qcRTMP_GetTime();
  ret = qcRTMP_Connect1(r, cp);
  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, Connect1 use time %d.", __FUNCTION__, qcRTMP_GetTime()-useTime);
  return ret;
}

static int
qcSocksNegotiate(qcRTMP *r)
{
  unsigned long addr;
  struct sockaddr_in service;
  memset(&service, 0, sizeof(struct sockaddr_in));

  qc_add_addr_info(r, &service, &r->Link.hostname, r->Link.port);
  addr = htonl(service.sin_addr.s_addr);

  {
    char packet[] = {
      4, 1,			/* SOCKS 4, connect */
      (r->Link.port >> 8) & 0xFF,
      (r->Link.port) & 0xFF,
      (char)(addr >> 24) & 0xFF, (char)(addr >> 16) & 0xFF,
      (char)(addr >> 8) & 0xFF, (char)addr & 0xFF,
      0
    };				/* NULL terminate */

    qcWriteN(r, packet, sizeof packet);

    if (qcReadN(r, packet, 8) != 8)
      return FALSE;

    if (packet[0] == 0 && packet[1] == 90)
      {
        return TRUE;
      }
    else
      {
        qcRTMP_Log(qcRTMP_LOGERROR, "%s, SOCKS returned error code %d", __FUNCTION__, packet[1]);
        return FALSE;
      }
  }
}

int
qcRTMP_ConnectStream(qcRTMP *r, int seekTime)
{
  qcRTMPPacket packet = { 0 };

  /* seekTime was already set by SetupStream / SetupURL.
   * This is only needed by ReconnectStream.
   */
  if (seekTime > 0)
    r->Link.seekTime = seekTime;

  r->m_mediaChannel = 0;

  while (!r->m_bPlaying && qcRTMP_IsConnected(r) && qcRTMP_ReadPacket(r, &packet))
    {
      if (qcRTMPPacket_IsReady(&packet))
	{
	  if (!packet.m_nBodySize)
	    continue;
	  if ((packet.m_packetType == qcRTMP_PACKET_TYPE_AUDIO) ||
	      (packet.m_packetType == qcRTMP_PACKET_TYPE_VIDEO) ||
	      (packet.m_packetType == qcRTMP_PACKET_TYPE_INFO))
	    {
	      qcRTMP_Log(qcRTMP_LOGWARNING, "Received FLV packet before play()! Ignoring.");
	      qcRTMPPacket_Free(&packet);
	      continue;
	    }

	  qcRTMP_ClientPacket(r, &packet);
	  qcRTMPPacket_Free(&packet);
	}
    }

  return r->m_bPlaying;
}

int
qcRTMP_ReconnectStream(qcRTMP *r, int seekTime)
{
  qcRTMP_DeleteStream(r);

  qcRTMP_SendCreateStream(r);

  return qcRTMP_ConnectStream(r, seekTime);
}

int
qcRTMP_ToggleStream(qcRTMP *r)
{
  int res;

  if (!r->m_pausing)
    {
      if (qcRTMP_IsTimedout(r) && r->m_read.status == qcRTMP_READ_EOF)
        r->m_read.status = 0;

      res = qcRTMP_SendPause(r, TRUE, r->m_pauseStamp);
      if (!res)
	return res;

      r->m_pausing = 1;
      sleep(1);
    }
  res = qcRTMP_SendPause(r, FALSE, r->m_pauseStamp);
  r->m_pausing = 3;
  return res;
}

void
qcRTMP_DeleteStream(qcRTMP *r)
{
  if (r->m_stream_id < 0)
    return;

  r->m_bPlaying = FALSE;

  qcSendDeleteStream(r, r->m_stream_id);
  r->m_stream_id = -1;
}

int
qcRTMP_GetNextMediaPacket(qcRTMP *r, qcRTMPPacket *packet)
{
  int bHasMediaPacket = 0;

  while (!bHasMediaPacket && qcRTMP_IsConnected(r)
	 && qcRTMP_ReadPacket(r, packet) && r->forceClose==0)
    {
      if (!qcRTMPPacket_IsReady(packet) || !packet->m_nBodySize)
	{
	  continue;
	}

      bHasMediaPacket = qcRTMP_ClientPacket(r, packet);

      if (!bHasMediaPacket)
	{
	  qcRTMPPacket_Free(packet);
	}
      else if (r->m_pausing == 3)
	{
	  if (packet->m_nTimeStamp <= r->m_mediaStamp)
	    {
	      bHasMediaPacket = 0;
#ifdef _DEBUG
	      qcRTMP_Log(qcRTMP_LOGDEBUG,
		  "Skipped type: %02X, size: %d, TS: %d ms, abs TS: %d, pause: %d ms",
		  packet->m_packetType, packet->m_nBodySize,
		  packet->m_nTimeStamp, packet->m_hasAbsTimestamp,
		  r->m_mediaStamp);
#endif
	      qcRTMPPacket_Free(packet);
	      continue;
	    }
	  r->m_pausing = 0;
	}
    }

  if (bHasMediaPacket)
    r->m_bPlaying = TRUE;
  else if (r->m_sb.sb_timedout && !r->m_pausing)
    r->m_pauseStamp = r->m_mediaChannel < r->m_channelsAllocatedIn ?
                      r->m_channelTimestamp[r->m_mediaChannel] : 0;

  return bHasMediaPacket;
}

int
qcRTMP_ClientPacket(qcRTMP *r, qcRTMPPacket *packet)
{
  int bHasMediaPacket = 0;
  switch (packet->m_packetType)
    {
    case qcRTMP_PACKET_TYPE_CHUNK_SIZE:
      /* chunk size */
      qcHandleChangeChunkSize(r, packet);
      break;

    case qcRTMP_PACKET_TYPE_BYTES_READ_REPORT:
      /* bytes read report */
      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, received: bytes read report", __FUNCTION__);
      break;

    case qcRTMP_PACKET_TYPE_CONTROL:
      /* ctrl */
      qcHandleCtrl(r, packet);
      break;

    case qcRTMP_PACKET_TYPE_SERVER_BW:
      /* server bw */
      qcHandleServerBW(r, packet);
      break;

    case qcRTMP_PACKET_TYPE_CLIENT_BW:
      /* client bw */
      qcHandleClientBW(r, packet);
      break;

    case qcRTMP_PACKET_TYPE_AUDIO:
      /* audio data */
      /*qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, received: audio %lu bytes", __FUNCTION__, packet.m_nBodySize); */
      qcHandleAudio(r, packet);
      bHasMediaPacket = 1;
      if (!r->m_mediaChannel)
	r->m_mediaChannel = packet->m_nChannel;
      if (!r->m_pausing)
	r->m_mediaStamp = packet->m_nTimeStamp;
      break;

    case qcRTMP_PACKET_TYPE_VIDEO:
      /* video data */
      /*qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, received: video %lu bytes", __FUNCTION__, packet.m_nBodySize); */
      qcHandleVideo(r, packet);
      bHasMediaPacket = 1;
      if (!r->m_mediaChannel)
	r->m_mediaChannel = packet->m_nChannel;
      if (!r->m_pausing)
	r->m_mediaStamp = packet->m_nTimeStamp;
      break;

    case qcRTMP_PACKET_TYPE_FLEX_STREAM_SEND:
      /* flex stream send */
      qcRTMP_Log(qcRTMP_LOGDEBUG,
	  "%s, flex stream send, size %u bytes, not supported, ignoring",
	  __FUNCTION__, packet->m_nBodySize);
      break;

    case qcRTMP_PACKET_TYPE_FLEX_SHARED_OBJECT:
      /* flex shared object */
      qcRTMP_Log(qcRTMP_LOGDEBUG,
	  "%s, flex shared object, size %u bytes, not supported, ignoring",
	  __FUNCTION__, packet->m_nBodySize);
      break;

    case qcRTMP_PACKET_TYPE_FLEX_MESSAGE:
      /* flex message */
      {
	qcRTMP_Log(qcRTMP_LOGDEBUG,
	    "%s, flex message, size %u bytes, not fully supported",
	    __FUNCTION__, packet->m_nBodySize);
	/*qcRTMP_LogHex(packet.m_body, packet.m_nBodySize); */

	/* some DEBUG code */
#if 0
	   qcRTMP_LIB_AMFObject obj;
	   int nRes = obj.Decode(packet.m_body+1, packet.m_nBodySize-1);
	   if(nRes < 0) {
	   qcRTMP_Log(qcRTMP_LOGERROR, "%s, error decoding AMF3 packet", __FUNCTION__);
	   /*return; */
	   }

	   obj.Dump();
#endif

	if (qcHandleInvoke(r, packet->m_body + 1, packet->m_nBodySize - 1) == 1)
	  bHasMediaPacket = 2;
	break;
      }
    case qcRTMP_PACKET_TYPE_INFO:
      /* metadata (notify) */
      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, received: notify %u bytes", __FUNCTION__,
	  packet->m_nBodySize);
      if (qcHandleMetadata(r, packet->m_body, packet->m_nBodySize))
	bHasMediaPacket = 1;
      break;

    case qcRTMP_PACKET_TYPE_SHARED_OBJECT:
      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, shared object, not supported, ignoring",
	  __FUNCTION__);
      break;

    case qcRTMP_PACKET_TYPE_INVOKE:
      /* invoke */
      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, received: invoke %u bytes", __FUNCTION__,
	  packet->m_nBodySize);
      /*qcRTMP_LogHex(packet.m_body, packet.m_nBodySize); */

      if (qcHandleInvoke(r, packet->m_body, packet->m_nBodySize) == 1)
	bHasMediaPacket = 2;
      break;

    case qcRTMP_PACKET_TYPE_FLASH_VIDEO:
      {
	/* go through FLV packets and handle metadata packets */
	unsigned int pos = 0;
	uint32_t nTimeStamp = packet->m_nTimeStamp;

	while (pos + 11 < packet->m_nBodySize)
	  {
	    uint32_t dataSize = qcAMF_DecodeInt24(packet->m_body + pos + 1);	/* size without header (11) and prevTagSize (4) */

	    if (pos + 11 + dataSize + 4 > packet->m_nBodySize)
	      {
		qcRTMP_Log(qcRTMP_LOGWARNING, "Stream corrupt?!");
		break;
	      }
	    if (packet->m_body[pos] == 0x12)
	      {
		qcHandleMetadata(r, packet->m_body + pos + 11, dataSize);
	      }
	    else if (packet->m_body[pos] == 8 || packet->m_body[pos] == 9)
	      {
		nTimeStamp = qcAMF_DecodeInt24(packet->m_body + pos + 4);
		nTimeStamp |= (packet->m_body[pos + 7] << 24);
	      }
	    pos += (11 + dataSize + 4);
	  }
	if (!r->m_pausing)
	  r->m_mediaStamp = nTimeStamp;

	/* FLV tag(s) */
	/*qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, received: FLV tag(s) %lu bytes", __FUNCTION__, packet.m_nBodySize); */
	bHasMediaPacket = 1;
	break;
      }
    default:
      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, unknown packet type received: 0x%02x", __FUNCTION__,
	  packet->m_packetType);
#ifdef _DEBUG
      qcRTMP_LogHex(qcRTMP_LOGDEBUG, packet->m_body, packet->m_nBodySize);
#endif
    }

  return bHasMediaPacket;
}

static int
qcReadN(qcRTMP *r, char *buffer, int n)
{
  int nOriginalSize = n;
  int avail;
  char *ptr;

  r->m_sb.sb_timedout = FALSE;

#ifdef _DEBUG
  memset(buffer, 0, n);
#endif

  ptr = buffer;
  while (n > 0 && r->forceClose==0)
    {
      int nBytes = 0, nRead;
      if (r->Link.protocol & qcRTMP_FEATURE_HTTP)
        {
	  int refill = 0;
	  while (!r->m_resplen && r->forceClose==0)
	    {
	      int ret;
	      if (r->m_sb.sb_size < 13 || refill)
	        {
		  if (!r->m_unackd)
		    qcHTTP_Post(r, qcRTMPT_IDLE, "", 1);
		  if (qcRTMPSockBuf_Fill(r, &r->m_sb) < 1)
		    {
		      if (!r->m_sb.sb_timedout)
		        qcRTMP_Close(r);
		      return 0;
		    }
		}
	      if ((ret = qcHTTP_read(r, 0)) == -1)
		{
		  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, No valid HTTP response found", __FUNCTION__);
		  qcRTMP_Close(r);
		  return 0;
		}
              else if (ret == -2)
                {
                  refill = 1;
                }
              else
                {
                  refill = 0;
                }
	    }
	  if (r->m_resplen && !r->m_sb.sb_size)
	    qcRTMPSockBuf_Fill(r, &r->m_sb);
          avail = r->m_sb.sb_size;
	  if (avail > r->m_resplen)
	    avail = r->m_resplen;
	}
      else
        {
          avail = r->m_sb.sb_size;
	  if (avail == 0)
	    {
	      if (qcRTMPSockBuf_Fill(r, &r->m_sb) < 1)
	        {
	          if (!r->m_sb.sb_timedout)
	            qcRTMP_Close(r);
	          return 0;
		}
	      avail = r->m_sb.sb_size;
	    }
	}
      nRead = ((n < avail) ? n : avail);
      if (nRead > 0)
	{
	  memcpy(ptr, r->m_sb.sb_start, nRead);
	  r->m_sb.sb_start += nRead;
	  r->m_sb.sb_size -= nRead;
	  nBytes = nRead;
	  r->m_nBytesIn += nRead;
	  if (r->m_bSendCounter
	      && r->m_nBytesIn > ( r->m_nBytesInSent + r->m_nClientBW / 10))
	    if (!qcSendBytesReceived(r))
	        return FALSE;
	}

      if (nBytes == 0)
	{
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, RTMP socket closed by peer", __FUNCTION__);
	  /*goto again; */
	  qcRTMP_Close(r);
	  break;
	}

      if (r->Link.protocol & qcRTMP_FEATURE_HTTP)
	r->m_resplen -= nBytes;

#ifdef CRYPTO
      if (r->Link.rc4keyIn)
	{
	  RC4_encrypt(r->Link.rc4keyIn, nBytes, ptr);
	}
#endif

      n -= nBytes;
      ptr += nBytes;
    }

  return nOriginalSize - n;
}

static int
qcWriteN(qcRTMP *r, const char *buffer, int n)
{
  const char *ptr = buffer;
#ifdef CRYPTO
  char *encrypted = 0;
  char buf[qcRTMP_BUFFER_CACHE_SIZE];

  if (r->Link.rc4keyOut)
    {
      if (n > sizeof(buf))
	encrypted = (char *)malloc(n);
      else
	encrypted = (char *)buf;
      ptr = encrypted;
      RC4_encrypt2(r->Link.rc4keyOut, n, buffer, ptr);
    }
#endif

  while (n > 0 && r->forceClose==0)
    {
      int nBytes;

      if (r->Link.protocol & qcRTMP_FEATURE_HTTP)
        nBytes = qcHTTP_Post(r, qcRTMPT_SEND, ptr, n);
      else
        nBytes = qcRTMPSockBuf_Send(&r->m_sb, ptr, n);
      /*qcRTMP_Log(qcRTMP_LOGDEBUG, "%s: %d\n", __FUNCTION__, nBytes); */

      if (nBytes < 0)
	{
	  int sockerr = GetSockError();
	  qcRTMP_Log(qcRTMP_LOGERROR, "%s, RTMP send error %d (%d bytes)", __FUNCTION__,
	      sockerr, n);

	  if (sockerr == EINTR && !qcRTMP_ctrlC)
	    continue;

	  qcRTMP_Close(r);
	  n = 1;
	  break;
	}

      if (nBytes == 0)
	break;

      n -= nBytes;
      ptr += nBytes;
    }

#ifdef CRYPTO
  if (encrypted && encrypted != buf)
    free(encrypted);
#endif

  return n == 0;
}

#define SAVC(x)	static const AVal av_##x = AVC(#x)

SAVC(app);
SAVC(connect);
SAVC(flashVer);
SAVC(swfUrl);
SAVC(pageUrl);
SAVC(tcUrl);
SAVC(fpad);
SAVC(capabilities);
SAVC(audioCodecs);
SAVC(videoCodecs);
SAVC(videoFunction);
SAVC(objectEncoding);
SAVC(secureToken);
SAVC(secureTokenResponse);
SAVC(type);
SAVC(nonprivate);

static int
qcSendConnectPacket(qcRTMP *r, qcRTMPPacket *cp)
{
  qcRTMPPacket packet;
  char pbuf[4096], *pend = pbuf + sizeof(pbuf);
  char *enc;

  if (cp)
    return qcRTMP_SendPacket(r, cp, TRUE);

  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_LARGE;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_connect);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_OBJECT;

  enc = qcAMF_EncodeNamedString(enc, pend, &av_app, &r->Link.app);
  if (!enc)
    return FALSE;
  if (r->Link.protocol & qcRTMP_FEATURE_WRITE)
    {
      enc = qcAMF_EncodeNamedString(enc, pend, &av_type, &av_nonprivate);
      if (!enc)
	return FALSE;
    }
  if (r->Link.flashVer.av_len)
    {
      enc = qcAMF_EncodeNamedString(enc, pend, &av_flashVer, &r->Link.flashVer);
      if (!enc)
	return FALSE;
    }
  if (r->Link.swfUrl.av_len)
    {
      enc = qcAMF_EncodeNamedString(enc, pend, &av_swfUrl, &r->Link.swfUrl);
      if (!enc)
	return FALSE;
    }
  if (r->Link.tcUrl.av_len)
    {
      enc = qcAMF_EncodeNamedString(enc, pend, &av_tcUrl, &r->Link.tcUrl);
      if (!enc)
	return FALSE;
    }
  if (!(r->Link.protocol & qcRTMP_FEATURE_WRITE))
    {
      enc = qcAMF_EncodeNamedBoolean(enc, pend, &av_fpad, FALSE);
      if (!enc)
	return FALSE;
      enc = qcAMF_EncodeNamedNumber(enc, pend, &av_capabilities, 15.0);
      if (!enc)
	return FALSE;
      enc = qcAMF_EncodeNamedNumber(enc, pend, &av_audioCodecs, r->m_fAudioCodecs);
      if (!enc)
	return FALSE;
      enc = qcAMF_EncodeNamedNumber(enc, pend, &av_videoCodecs, r->m_fVideoCodecs);
      if (!enc)
	return FALSE;
      enc = qcAMF_EncodeNamedNumber(enc, pend, &av_videoFunction, 1.0);
      if (!enc)
	return FALSE;
      if (r->Link.pageUrl.av_len)
	{
	  enc = qcAMF_EncodeNamedString(enc, pend, &av_pageUrl, &r->Link.pageUrl);
	  if (!enc)
	    return FALSE;
	}
    }
  if (r->m_fEncoding != 0.0 || r->m_bSendEncoding)
    {	/* AMF0, AMF3 not fully supported yet */
      enc = qcAMF_EncodeNamedNumber(enc, pend, &av_objectEncoding, r->m_fEncoding);
      if (!enc)
	return FALSE;
    }
  if (enc + 3 >= pend)
    return FALSE;
  *enc++ = 0;
  *enc++ = 0;			/* end of object - 0x00 0x00 0x09 */
  *enc++ = qcAMF_OBJECT_END;

  /* add auth string */
  if (r->Link.auth.av_len)
    {
      enc = qcAMF_EncodeBoolean(enc, pend, r->Link.lFlags & qcRTMP_LF_AUTH);
      if (!enc)
	return FALSE;
      enc = qcAMF_EncodeString(enc, pend, &r->Link.auth);
      if (!enc)
	return FALSE;
    }
  if (r->Link.extras.o_num)
    {
      int i;
      for (i = 0; i < r->Link.extras.o_num; i++)
	{
	  enc = qcAMFProp_Encode(&r->Link.extras.o_props[i], enc, pend);
	  if (!enc)
	    return FALSE;
	}
    }
  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, TRUE);
}

#if 0				/* unused */
SAVC(bgHasStream);

static int
qcSendBGHasStream(qcRTMP *r, double dId, AVal *playpath)
{
  qcRTMPPacket packet;
  char pbuf[1024], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_bgHasStream);
  enc = qcAMF_EncodeNumber(enc, pend, dId);
  *enc++ = qcAMF_NULL;

  enc = qcAMF_EncodeString(enc, pend, playpath);
  if (enc == NULL)
    return FALSE;

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, TRUE);
}
#endif

SAVC(createStream);

int
qcRTMP_SendCreateStream(qcRTMP *r)
{
  qcRTMPPacket packet;
  char pbuf[256], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_createStream);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_NULL;		/* NULL */

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, TRUE);
}

SAVC(FCSubscribe);

static int
qcSendFCSubscribe(qcRTMP *r, AVal *subscribepath)
{
  qcRTMPPacket packet;
  char pbuf[512], *pend = pbuf + sizeof(pbuf);
  char *enc;
  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  qcRTMP_Log(qcRTMP_LOGDEBUG, "FCSubscribe: %s", subscribepath->av_val);
  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_FCSubscribe);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_NULL;
  enc = qcAMF_EncodeString(enc, pend, subscribepath);

  if (!enc)
    return FALSE;

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, TRUE);
}

/* Justin.tv specific authentication */
static const AVal qc_av_NetStream_Authenticate_UsherToken = AVC("NetStream.Authenticate.UsherToken");

static int
qcSendUsherToken(qcRTMP *r, AVal *usherToken)
{
  qcRTMPPacket packet;
  char pbuf[1024], *pend = pbuf + sizeof(pbuf);
  char *enc;
  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  qcRTMP_Log(qcRTMP_LOGDEBUG, "UsherToken: %s", usherToken->av_val);
  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &qc_av_NetStream_Authenticate_UsherToken);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_NULL;
  enc = qcAMF_EncodeString(enc, pend, usherToken);

  if (!enc)
    return FALSE;

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, FALSE);
}
/******************************************/

SAVC(releaseStream);

static int
qcSendReleaseStream(qcRTMP *r)
{
  qcRTMPPacket packet;
  char pbuf[1024], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

 enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_releaseStream);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_NULL;
  enc = qcAMF_EncodeString(enc, pend, &r->Link.playpath);
  if (!enc)
    return FALSE;

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, FALSE);
}

SAVC(FCPublish);

static int
qcSendFCPublish(qcRTMP *r)
{
  qcRTMPPacket packet;
  char pbuf[1024], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_FCPublish);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_NULL;
  enc = qcAMF_EncodeString(enc, pend, &r->Link.playpath);
  if (!enc)
    return FALSE;

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, FALSE);
}

SAVC(FCUnpublish);

static int
qcSendFCUnpublish(qcRTMP *r)
{
  qcRTMPPacket packet;
  char pbuf[1024], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_FCUnpublish);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_NULL;
  enc = qcAMF_EncodeString(enc, pend, &r->Link.playpath);
  if (!enc)
    return FALSE;

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, FALSE);
}

SAVC(publish);
SAVC(live);
SAVC(record);

static int
qcSendPublish(qcRTMP *r)
{
  qcRTMPPacket packet;
  char pbuf[1024], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x04;	/* source channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_LARGE;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = r->m_stream_id;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_publish);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_NULL;
  enc = qcAMF_EncodeString(enc, pend, &r->Link.playpath);
  if (!enc)
    return FALSE;

  /* FIXME: should we choose live based on Link.lFlags & qcRTMP_LF_LIVE? */
  enc = qcAMF_EncodeString(enc, pend, &av_live);
  if (!enc)
    return FALSE;

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, TRUE);
}

SAVC(deleteStream);

static int
qcSendDeleteStream(qcRTMP *r, double dStreamId)
{
  qcRTMPPacket packet;
  char pbuf[256], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_deleteStream);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_NULL;
  enc = qcAMF_EncodeNumber(enc, pend, dStreamId);

  packet.m_nBodySize = enc - packet.m_body;

  /* no response expected */
  return qcRTMP_SendPacket(r, &packet, FALSE);
}

SAVC(pause);

int
qcRTMP_SendPause(qcRTMP *r, int DoPause, int iTime)
{
  qcRTMPPacket packet;
  char pbuf[256], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x08;	/* video channel */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_pause);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_NULL;
  enc = qcAMF_EncodeBoolean(enc, pend, DoPause);
  enc = qcAMF_EncodeNumber(enc, pend, (double)iTime);

  packet.m_nBodySize = enc - packet.m_body;

  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, %d, pauseTime=%d", __FUNCTION__, DoPause, iTime);
  return qcRTMP_SendPacket(r, &packet, TRUE);
}

int qcRTMP_Pause(qcRTMP *r, int DoPause)
{
  if (DoPause)
    r->m_pauseStamp = r->m_mediaChannel < r->m_channelsAllocatedIn ?
                      r->m_channelTimestamp[r->m_mediaChannel] : 0;
  return qcRTMP_SendPause(r, DoPause, r->m_pauseStamp);
}

SAVC(seek);

int
qcRTMP_SendSeek(qcRTMP *r, int iTime)
{
  qcRTMPPacket packet;
  char pbuf[256], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x08;	/* video channel */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_seek);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_NULL;
  enc = qcAMF_EncodeNumber(enc, pend, (double)iTime);

  packet.m_nBodySize = enc - packet.m_body;

  r->m_read.flags |= qcRTMP_READ_SEEKING;
  r->m_read.nResumeTS = 0;

  return qcRTMP_SendPacket(r, &packet, TRUE);
}

int
qcRTMP_SendServerBW(qcRTMP *r)
{
  qcRTMPPacket packet;
  char pbuf[256], *pend = pbuf + sizeof(pbuf);

  packet.m_nChannel = 0x02;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_LARGE;
  packet.m_packetType = qcRTMP_PACKET_TYPE_SERVER_BW;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  packet.m_nBodySize = 4;

  qcAMF_EncodeInt32(packet.m_body, pend, r->m_nServerBW);
  return qcRTMP_SendPacket(r, &packet, FALSE);
}

int
qcRTMP_SendClientBW(qcRTMP *r)
{
  qcRTMPPacket packet;
  char pbuf[256], *pend = pbuf + sizeof(pbuf);

  packet.m_nChannel = 0x02;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_LARGE;
  packet.m_packetType = qcRTMP_PACKET_TYPE_CLIENT_BW;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  packet.m_nBodySize = 5;

  qcAMF_EncodeInt32(packet.m_body, pend, r->m_nClientBW);
  packet.m_body[4] = r->m_nClientBW2;
  return qcRTMP_SendPacket(r, &packet, FALSE);
}

static int
qcSendBytesReceived(qcRTMP *r)
{
  qcRTMPPacket packet;
  char pbuf[256], *pend = pbuf + sizeof(pbuf);

  packet.m_nChannel = 0x02;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_BYTES_READ_REPORT;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  packet.m_nBodySize = 4;

  qcAMF_EncodeInt32(packet.m_body, pend, r->m_nBytesIn);	/* hard coded for now */
  r->m_nBytesInSent = r->m_nBytesIn;

  /*qcRTMP_Log(qcRTMP_LOGDEBUG, "Send bytes report. 0x%x (%d bytes)", (unsigned int)m_nBytesIn, m_nBytesIn); */
  return qcRTMP_SendPacket(r, &packet, FALSE);
}

SAVC(_checkbw);

static int
qcSendCheckBW(qcRTMP *r)
{
  qcRTMPPacket packet;
  char pbuf[256], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_LARGE;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;	/* qcRTMP_GetTime(); */
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av__checkbw);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_NULL;

  packet.m_nBodySize = enc - packet.m_body;

  /* triggers _onbwcheck and eventually results in _onbwdone */
  return qcRTMP_SendPacket(r, &packet, FALSE);
}

SAVC(_result);

static int
qcSendCheckBWResult(qcRTMP *r, double txn)
{
  qcRTMPPacket packet;
  char pbuf[256], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0x16 * r->m_nBWCheckCounter;	/* temp inc value. till we figure it out. */
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av__result);
  enc = qcAMF_EncodeNumber(enc, pend, txn);
  *enc++ = qcAMF_NULL;
  enc = qcAMF_EncodeNumber(enc, pend, (double)r->m_nBWCheckCounter++);

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, FALSE);
}

SAVC(ping);
SAVC(pong);

static int
qcSendPong(qcRTMP *r, double txn)
{
  qcRTMPPacket packet;
  char pbuf[256], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0x16 * r->m_nBWCheckCounter;	/* temp inc value. till we figure it out. */
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_pong);
  enc = qcAMF_EncodeNumber(enc, pend, txn);
  *enc++ = qcAMF_NULL;

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, FALSE);
}

SAVC(play);

static int
qcSendPlay(qcRTMP *r)
{
  qcRTMPPacket packet;
  char pbuf[1024], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x08;	/* we make 8 our stream channel */
  packet.m_headerType = qcRTMP_PACKET_SIZE_LARGE;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = r->m_stream_id;	/*0x01000000; */
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_play);
  enc = qcAMF_EncodeNumber(enc, pend, ++r->m_numInvokes);
  *enc++ = qcAMF_NULL;

  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, seekTime=%d, stopTime=%d, sending play: %s",
      __FUNCTION__, r->Link.seekTime, r->Link.stopTime,
      r->Link.playpath.av_val);
  enc = qcAMF_EncodeString(enc, pend, &r->Link.playpath);
  if (!enc)
    return FALSE;

  /* Optional parameters start and len.
   *
   * start: -2, -1, 0, positive number
   *  -2: looks for a live stream, then a recorded stream,
   *      if not found any open a live stream
   *  -1: plays a live stream
   * >=0: plays a recorded streams from 'start' milliseconds
   */
  if (r->Link.lFlags & qcRTMP_LF_LIVE)
    enc = qcAMF_EncodeNumber(enc, pend, -1000.0);
  else
    {
      if (r->Link.seekTime > 0.0)
	enc = qcAMF_EncodeNumber(enc, pend, r->Link.seekTime);	/* resume from here */
      else
	enc = qcAMF_EncodeNumber(enc, pend, 0.0);	/*-2000.0);*/ /* recorded as default, -2000.0 is not reliable since that freezes the player if the stream is not found */
    }
  if (!enc)
    return FALSE;

  /* len: -1, 0, positive number
   *  -1: plays live or recorded stream to the end (default)
   *   0: plays a frame 'start' ms away from the beginning
   *  >0: plays a live or recoded stream for 'len' milliseconds
   */
  /*enc += EncodeNumber(enc, -1.0); */ /* len */
  if (r->Link.stopTime)
    {
      enc = qcAMF_EncodeNumber(enc, pend, r->Link.stopTime - r->Link.seekTime);
      if (!enc)
	return FALSE;
    }

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, TRUE);
}

SAVC(set_playlist);
SAVC(0);

static int
qcSendPlaylist(qcRTMP *r)
{
  qcRTMPPacket packet;
  char pbuf[1024], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x08;	/* we make 8 our stream channel */
  packet.m_headerType = qcRTMP_PACKET_SIZE_LARGE;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = r->m_stream_id;	/*0x01000000; */
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_set_playlist);
  enc = qcAMF_EncodeNumber(enc, pend, 0);
  *enc++ = qcAMF_NULL;
  *enc++ = qcAMF_ECMA_ARRAY;
  *enc++ = 0;
  *enc++ = 0;
  *enc++ = 0;
  *enc++ = qcAMF_OBJECT;
  enc = qcAMF_EncodeNamedString(enc, pend, &av_0, &r->Link.playpath);
  if (!enc)
    return FALSE;
  if (enc + 3 >= pend)
    return FALSE;
  *enc++ = 0;
  *enc++ = 0;
  *enc++ = qcAMF_OBJECT_END;

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, TRUE);
}

static int
qcSendSecureTokenResponse(qcRTMP *r, AVal *resp)
{
  qcRTMPPacket packet;
  char pbuf[1024], *pend = pbuf + sizeof(pbuf);
  char *enc;

  packet.m_nChannel = 0x03;	/* control channel (invoke) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = qcAMF_EncodeString(enc, pend, &av_secureTokenResponse);
  enc = qcAMF_EncodeNumber(enc, pend, 0.0);
  *enc++ = qcAMF_NULL;
  enc = qcAMF_EncodeString(enc, pend, resp);
  if (!enc)
    return FALSE;

  packet.m_nBodySize = enc - packet.m_body;

  return qcRTMP_SendPacket(r, &packet, FALSE);
}

/*
from http://jira.red5.org/confluence/display/docs/Ping:

Ping is the most mysterious message in RTMP and till now we haven't fully interpreted it yet. In summary, Ping message is used as a special command that are exchanged between client and server. This page aims to document all known Ping messages. Expect the list to grow.

The type of Ping packet is 0x4 and contains two mandatory parameters and two optional parameters. The first parameter is the type of Ping and in short integer. The second parameter is the target of the ping. As Ping is always sent in Channel 2 (control channel) and the target object in RTMP header is always 0 which means the Connection object, it's necessary to put an extra parameter to indicate the exact target object the Ping is sent to. The second parameter takes this responsibility. The value has the same meaning as the target object field in RTMP header. (The second value could also be used as other purposes, like RTT Ping/Pong. It is used as the timestamp.) The third and fourth parameters are optional and could be looked upon as the parameter of the Ping packet. Below is an unexhausted list of Ping messages.

    * type 0: Clear the stream. No third and fourth parameters. The second parameter could be 0. After the connection is established, a Ping 0,0 will be sent from server to client. The message will also be sent to client on the start of Play and in response of a Seek or Pause/Resume request. This Ping tells client to re-calibrate the clock with the timestamp of the next packet server sends.
    * type 1: Tell the stream to clear the playing buffer.
    * type 3: Buffer time of the client. The third parameter is the buffer time in millisecond.
    * type 4: Reset a stream. Used together with type 0 in the case of VOD. Often sent before type 0.
    * type 6: Ping the client from server. The second parameter is the current time.
    * type 7: Pong reply from client. The second parameter is the time the server sent with his ping request.
    * type 26: SWFVerification request
    * type 27: SWFVerification response
*/
int
qcRTMP_SendCtrl(qcRTMP *r, short nType, unsigned int nObject, unsigned int nTime)
{
  qcRTMPPacket packet;
  char pbuf[256], *pend = pbuf + sizeof(pbuf);
  int nSize;
  char *buf;

  qcRTMP_Log(qcRTMP_LOGDEBUG, "sending ctrl. type: 0x%04x", (unsigned short)nType);

  packet.m_nChannel = 0x02;	/* control channel (ping) */
  packet.m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = qcRTMP_PACKET_TYPE_CONTROL;
  packet.m_nTimeStamp = 0;	/* qcRTMP_GetTime(); */
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + qcRTMP_MAX_HEADER_SIZE;

  switch(nType) {
  case 0x03: nSize = 10; break;	/* buffer time */
  case 0x1A: nSize = 3; break;	/* SWF verify request */
  case 0x1B: nSize = 44; break;	/* SWF verify response */
  default: nSize = 6; break;
  }

  packet.m_nBodySize = nSize;

  buf = packet.m_body;
  buf = qcAMF_EncodeInt16(buf, pend, nType);

  if (nType == 0x1B)
    {
#ifdef CRYPTO
      memcpy(buf, r->Link.SWFVerificationResponse, 42);
      qcRTMP_Log(qcRTMP_LOGDEBUG, "Sending SWFVerification response: ");
      qcRTMP_LogHex(qcRTMP_LOGDEBUG, (uint8_t *)packet.m_body, packet.m_nBodySize);
#endif
    }
  else if (nType == 0x1A)
    {
	  *buf = nObject & 0xff;
	}
  else
    {
      if (nSize > 2)
	buf = qcAMF_EncodeInt32(buf, pend, nObject);

      if (nSize > 6)
	buf = qcAMF_EncodeInt32(buf, pend, nTime);
    }

  return qcRTMP_SendPacket(r, &packet, FALSE);
}

static void
qcAV_erase(qcRTMP_METHOD *vals, int *num, int i, int freeit)
{
  if (freeit)
    free(vals[i].name.av_val);
  (*num)--;
  for (; i < *num; i++)
    {
      vals[i] = vals[i + 1];
    }
  vals[i].name.av_val = NULL;
  vals[i].name.av_len = 0;
  vals[i].num = 0;
}

void
qcRTMP_DropRequest(qcRTMP *r, int i, int freeit)
{
  qcAV_erase(r->m_methodCalls, &r->m_numCalls, i, freeit);
}

static void
qcAV_queue(qcRTMP_METHOD **vals, int *num, AVal *av, int txn)
{
  char *tmp;
  if (!(*num & 0x0f))
    *vals = realloc(*vals, (*num + 16) * sizeof(qcRTMP_METHOD));
  tmp = malloc(av->av_len + 1);
  memcpy(tmp, av->av_val, av->av_len);
  tmp[av->av_len] = '\0';
  (*vals)[*num].num = txn;
  (*vals)[*num].name.av_len = av->av_len;
  (*vals)[(*num)++].name.av_val = tmp;
}

static void
qcAV_clear(qcRTMP_METHOD *vals, int num)
{
  int i;
  for (i = 0; i < num; i++)
    free(vals[i].name.av_val);
  free(vals);
}


#ifdef CRYPTO
static int
qc_b64enc(const unsigned char *input, int length, char *output, int maxsize)
{
#ifdef USE_POLARSSL
  size_t buf_size = maxsize;
  if(base64_encode((unsigned char *) output, &buf_size, input, length) == 0)
    {
      output[buf_size] = '\0';
      return 1;
    }
  else
    {
      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, error", __FUNCTION__);
      return 0;
    }
#elif defined(USE_GNUTLS)
  if (BASE64_ENCODE_RAW_LENGTH(length) <= maxsize)
    base64_encode_raw((uint8_t*) output, length, input);
  else
    {
      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, error", __FUNCTION__);
      return 0;
    }
#else   /* USE_OPENSSL */
  BIO *bmem, *b64;
  BUF_MEM *bptr;

  b64 = BIO_new(BIO_f_base64());
  bmem = BIO_new(BIO_s_mem());
  b64 = BIO_push(b64, bmem);
  BIO_write(b64, input, length);
  if (BIO_flush(b64) == 1)
    {
      BIO_get_mem_ptr(b64, &bptr);
      memcpy(output, bptr->data, bptr->length-1);
      output[bptr->length-1] = '\0';
    }
  else
    {
      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, error", __FUNCTION__);
      return 0;
    }
  BIO_free_all(b64);
#endif
  return 1;
}

#ifdef USE_POLARSSL
#define MD5_CTX	md5_context
#define MD5_Init(ctx)	md5_starts(ctx)
#define MD5_Update(ctx,data,len)	md5_update(ctx,(unsigned char *)data,len)
#define MD5_Final(dig,ctx)	md5_finish(ctx,dig)
#elif defined(USE_GNUTLS)
typedef struct md5_ctx	MD5_CTX;
#define MD5_Init(ctx)	md5_init(ctx)
#define MD5_Update(ctx,data,len)	md5_update(ctx,len,data)
#define MD5_Final(dig,ctx)	md5_digest(ctx,MD5_DIGEST_LENGTH,dig)
#else
#endif

static const AVal qc_av_authmod_adobe = AVC("authmod=adobe");
static const AVal qc_av_authmod_llnw  = AVC("authmod=llnw");

static void qc_hexenc(unsigned char *inbuf, int len, char *dst)
{
    char *ptr = dst;
    while(len--) {
        sprintf(ptr, "%02x", *inbuf++);
        ptr += 2;
    }
    *ptr = '\0';
}

static char *
qcAValChr(AVal *av, char c)
{
  int i;
  for (i = 0; i < av->av_len; i++)
    if (av->av_val[i] == c)
      return &av->av_val[i];
  return NULL;
}

static int
qcPublisherAuth(qcRTMP *r, AVal *description)
{
  char *token_in = NULL;
  char *ptr;
  unsigned char md5sum_val[MD5_DIGEST_LENGTH+1];
  MD5_CTX md5ctx;
  int challenge2_data;
#define RESPONSE_LEN 32
#define CHALLENGE2_LEN 16
#define SALTED2_LEN (32+8+8+8)
#define B64DIGEST_LEN	24	/* 16 byte digest => 22 b64 chars + 2 chars padding */
#define B64INT_LEN	8	/* 4 byte int => 6 b64 chars + 2 chars padding */
#define HEXHASH_LEN	(2*MD5_DIGEST_LENGTH)
  char response[RESPONSE_LEN];
  char challenge2[CHALLENGE2_LEN];
  char salted2[SALTED2_LEN];
  AVal pubToken;

  if (strstr(description->av_val, av_authmod_adobe.av_val) != NULL)
    {
      if(strstr(description->av_val, "code=403 need auth") != NULL)
        {
            if (strstr(r->Link.app.av_val, av_authmod_adobe.av_val) != NULL) {
              qcRTMP_Log(qcRTMP_LOGERROR, "%s, wrong pubUser & pubPasswd for publisher auth", __FUNCTION__);
              return 0;
            } else if(r->Link.pubUser.av_len && r->Link.pubPasswd.av_len) {
              pubToken.av_val = malloc(r->Link.pubUser.av_len + av_authmod_adobe.av_len + 8);
              pubToken.av_len = sprintf(pubToken.av_val, "?%s&user=%s",
                      av_authmod_adobe.av_val,
                      r->Link.pubUser.av_val);
              qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, pubToken1: %s", __FUNCTION__, pubToken.av_val);
            } else {
              qcRTMP_Log(qcRTMP_LOGERROR, "%s, need to set pubUser & pubPasswd for publisher auth", __FUNCTION__);
              return 0;
            }
        }
      else if((token_in = strstr(description->av_val, "?reason=needauth")) != NULL)
        {
          char *par, *val = NULL, *orig_ptr;
	  AVal user, salt, opaque, challenge, *aptr = NULL;
	  opaque.av_len = 0;
	  challenge.av_len = 0;

          ptr = orig_ptr = strdup(token_in);
          while (ptr)
            {
              par = ptr;
              ptr = strchr(par, '&');
              if(ptr)
                  *ptr++ = '\0';

              val =  strchr(par, '=');
              if(val)
                  *val++ = '\0';

	      if (aptr) {
		aptr->av_len = par - aptr->av_val - 1;
		aptr = NULL;
	      }
              if (strcmp(par, "user") == 0){
                  user.av_val = val;
		  aptr = &user;
              } else if (strcmp(par, "salt") == 0){
                  salt.av_val = val;
		  aptr = &salt;
              } else if (strcmp(par, "opaque") == 0){
                  opaque.av_val = val;
		  aptr = &opaque;
              } else if (strcmp(par, "challenge") == 0){
                  challenge.av_val = val;
		  aptr = &challenge;
              }

              qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, par:\"%s\" = val:\"%s\"", __FUNCTION__, par, val);
            }
	  if (aptr)
	    aptr->av_len = strlen(aptr->av_val);

	  /* hash1 = base64enc(md5(user + _aodbeAuthSalt + password)) */
	  MD5_Init(&md5ctx);
	  MD5_Update(&md5ctx, user.av_val, user.av_len);
	  MD5_Update(&md5ctx, salt.av_val, salt.av_len);
	  MD5_Update(&md5ctx, r->Link.pubPasswd.av_val, r->Link.pubPasswd.av_len);
	  MD5_Final(md5sum_val, &md5ctx);
          qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, md5(%s%s%s) =>", __FUNCTION__,
	    user.av_val, salt.av_val, r->Link.pubPasswd.av_val);
          qcRTMP_LogHexString(qcRTMP_LOGDEBUG, md5sum_val, MD5_DIGEST_LENGTH);

          b64enc(md5sum_val, MD5_DIGEST_LENGTH, salted2, SALTED2_LEN);
          qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, b64(md5_1) = %s", __FUNCTION__, salted2);

            challenge2_data = rand();

            b64enc((unsigned char *) &challenge2_data, sizeof(int), challenge2, CHALLENGE2_LEN);
            qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, b64(%d) = %s", __FUNCTION__, challenge2_data, challenge2);

	  MD5_Init(&md5ctx);
	  MD5_Update(&md5ctx, salted2, B64DIGEST_LEN);
            /* response = base64enc(md5(hash1 + opaque + challenge2)) */
	  if (opaque.av_len)
	    MD5_Update(&md5ctx, opaque.av_val, opaque.av_len);
	  else if (challenge.av_len)
	    MD5_Update(&md5ctx, challenge.av_val, challenge.av_len);
	  MD5_Update(&md5ctx, challenge2, B64INT_LEN);
	  MD5_Final(md5sum_val, &md5ctx);

          qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, md5(%s%s%s) =>", __FUNCTION__,
	    salted2, opaque.av_len ? opaque.av_val : "", challenge2);
          qcRTMP_LogHexString(qcRTMP_LOGDEBUG, md5sum_val, MD5_DIGEST_LENGTH);

          b64enc(md5sum_val, MD5_DIGEST_LENGTH, response, RESPONSE_LEN);
          qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, b64(md5_2) = %s", __FUNCTION__, response);

            /* have all hashes, create auth token for the end of app */
            pubToken.av_val = malloc(32 + B64INT_LEN + B64DIGEST_LEN + opaque.av_len);
            pubToken.av_len = sprintf(pubToken.av_val,
                    "&challenge=%s&response=%s&opaque=%s",
                    challenge2,
                    response,
                    opaque.av_len ? opaque.av_val : "");
            qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, pubToken2: %s", __FUNCTION__, pubToken.av_val);
            free(orig_ptr);
        }
      else if(strstr(description->av_val, "?reason=authfailed") != NULL)
        {
          qcRTMP_Log(qcRTMP_LOGERROR, "%s, Authentication failed: wrong password", __FUNCTION__);
          return 0;
        }
      else if(strstr(description->av_val, "?reason=nosuchuser") != NULL)
        {
          qcRTMP_Log(qcRTMP_LOGERROR, "%s, Authentication failed: no such user", __FUNCTION__);
          return 0;
        }
      else
        {
          qcRTMP_Log(qcRTMP_LOGERROR, "%s, Authentication failed: unknown auth mode: %s",
                  __FUNCTION__, description->av_val);
          return 0;
        }

      ptr = malloc(r->Link.app.av_len + pubToken.av_len);
      strncpy(ptr, r->Link.app.av_val, r->Link.app.av_len);
      strncpy(ptr + r->Link.app.av_len, pubToken.av_val, pubToken.av_len);
      r->Link.app.av_len += pubToken.av_len;
      if(r->Link.lFlags & qcRTMP_LF_FAPU)
          free(r->Link.app.av_val);
      r->Link.app.av_val = ptr;

      ptr = malloc(r->Link.tcUrl.av_len + pubToken.av_len);
      strncpy(ptr, r->Link.tcUrl.av_val, r->Link.tcUrl.av_len);
      strncpy(ptr + r->Link.tcUrl.av_len, pubToken.av_val, pubToken.av_len);
      r->Link.tcUrl.av_len += pubToken.av_len;
      if(r->Link.lFlags & qcRTMP_LF_FTCU)
          free(r->Link.tcUrl.av_val);
      r->Link.tcUrl.av_val = ptr;

      free(pubToken.av_val);
      r->Link.lFlags |= qcRTMP_LF_FTCU | qcRTMP_LF_FAPU;

      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, new app: %.*s tcUrl: %.*s playpath: %s", __FUNCTION__,
              r->Link.app.av_len, r->Link.app.av_val,
              r->Link.tcUrl.av_len, r->Link.tcUrl.av_val,
              r->Link.playpath.av_val);
    }
  else if (strstr(description->av_val, av_authmod_llnw.av_val) != NULL)
    {
      if(strstr(description->av_val, "code=403 need auth") != NULL)
        {
            /* This part seems to be the same for llnw and adobe */

            if (strstr(r->Link.app.av_val, av_authmod_llnw.av_val) != NULL) {
              qcRTMP_Log(qcRTMP_LOGERROR, "%s, wrong pubUser & pubPasswd for publisher auth", __FUNCTION__);
              return 0;
            } else if(r->Link.pubUser.av_len && r->Link.pubPasswd.av_len) {
              pubToken.av_val = malloc(r->Link.pubUser.av_len + av_authmod_llnw.av_len + 8);
              pubToken.av_len = sprintf(pubToken.av_val, "?%s&user=%s",
                      av_authmod_llnw.av_val,
                      r->Link.pubUser.av_val);
              qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, pubToken1: %s", __FUNCTION__, pubToken.av_val);
            } else {
              qcRTMP_Log(qcRTMP_LOGERROR, "%s, need to set pubUser & pubPasswd for publisher auth", __FUNCTION__);
              return 0;
            }
        }
      else if((token_in = strstr(description->av_val, "?reason=needauth")) != NULL)
        {
          char *orig_ptr;
          char *par, *val = NULL;
	  char hash1[HEXHASH_LEN+1], hash2[HEXHASH_LEN+1], hash3[HEXHASH_LEN+1];
	  AVal user, nonce, *aptr = NULL;
	  AVal apptmp;

          /* llnw auth method
           * Seems to be closely based on HTTP Digest Auth:
           *    http://tools.ietf.org/html/rfc2617
           *    http://en.wikipedia.org/wiki/Digest_access_authentication
           */

          const char authmod[] = "llnw";
          const char realm[] = "live";
          const char method[] = "publish";
          const char qop[] = "auth";
          /* nc = 1..connection count (or rather, number of times cnonce has been reused) */
          int nc = 1;
          /* nchex = hexenc(nc) (8 hex digits according to RFC 2617) */
          char nchex[9];
          /* cnonce = hexenc(4 random bytes) (initialized on first connection) */
          char cnonce[9];

          ptr = orig_ptr = strdup(token_in);
          /* Extract parameters (we need user and nonce) */
          while (ptr)
            {
              par = ptr;
              ptr = strchr(par, '&');
              if(ptr)
                  *ptr++ = '\0';

              val =  strchr(par, '=');
              if(val)
                  *val++ = '\0';

	      if (aptr) {
		aptr->av_len = par - aptr->av_val - 1;
		aptr = NULL;
	      }
              if (strcmp(par, "user") == 0){
                user.av_val = val;
		aptr = &user;
              } else if (strcmp(par, "nonce") == 0){
                nonce.av_val = val;
		aptr = &nonce;
              }

              qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, par:\"%s\" = val:\"%s\"", __FUNCTION__, par, val);
            }
	  if (aptr)
	    aptr->av_len = strlen(aptr->av_val);

          /* FIXME: handle case where user==NULL or nonce==NULL */

          sprintf(nchex, "%08x", nc);
          sprintf(cnonce, "%08x", rand());

          /* hash1 = hexenc(md5(user + ":" + realm + ":" + password)) */
	  MD5_Init(&md5ctx);
	  MD5_Update(&md5ctx, user.av_val, user.av_len);
	  MD5_Update(&md5ctx, ":", 1);
	  MD5_Update(&md5ctx, realm, sizeof(realm)-1);
	  MD5_Update(&md5ctx, ":", 1);
	  MD5_Update(&md5ctx, r->Link.pubPasswd.av_val, r->Link.pubPasswd.av_len);
	  MD5_Final(md5sum_val, &md5ctx);
          qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, md5(%s:%s:%s) =>", __FUNCTION__,
	    user.av_val, realm, r->Link.pubPasswd.av_val);
          qcRTMP_LogHexString(qcRTMP_LOGDEBUG, md5sum_val, MD5_DIGEST_LENGTH);
          hexenc(md5sum_val, MD5_DIGEST_LENGTH, hash1);

          /* hash2 = hexenc(md5(method + ":/" + app + "/" + appInstance)) */
          /* Extract appname + appinstance without query parameters */
	  apptmp = r->Link.app;
	  ptr = AValChr(&apptmp, '?');
	  if (ptr)
	    apptmp.av_len = ptr - apptmp.av_val;

	  MD5_Init(&md5ctx);
	  MD5_Update(&md5ctx, method, sizeof(method)-1);
	  MD5_Update(&md5ctx, ":/", 2);
	  MD5_Update(&md5ctx, apptmp.av_val, apptmp.av_len);
	  if (!AValChr(&apptmp, '/'))
	    MD5_Update(&md5ctx, "/_definst_", sizeof("/_definst_") - 1);
	  MD5_Final(md5sum_val, &md5ctx);
          qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, md5(%s:/%.*s) =>", __FUNCTION__,
	    method, apptmp.av_len, apptmp.av_val);
          qcRTMP_LogHexString(qcRTMP_LOGDEBUG, md5sum_val, MD5_DIGEST_LENGTH);
          hexenc(md5sum_val, MD5_DIGEST_LENGTH, hash2);

          /* hash3 = hexenc(md5(hash1 + ":" + nonce + ":" + nchex + ":" + cnonce + ":" + qop + ":" + hash2)) */
	  MD5_Init(&md5ctx);
	  MD5_Update(&md5ctx, hash1, HEXHASH_LEN);
	  MD5_Update(&md5ctx, ":", 1);
	  MD5_Update(&md5ctx, nonce.av_val, nonce.av_len);
	  MD5_Update(&md5ctx, ":", 1);
	  MD5_Update(&md5ctx, nchex, sizeof(nchex)-1);
	  MD5_Update(&md5ctx, ":", 1);
	  MD5_Update(&md5ctx, cnonce, sizeof(cnonce)-1);
	  MD5_Update(&md5ctx, ":", 1);
	  MD5_Update(&md5ctx, qop, sizeof(qop)-1);
	  MD5_Update(&md5ctx, ":", 1);
	  MD5_Update(&md5ctx, hash2, HEXHASH_LEN);
	  MD5_Final(md5sum_val, &md5ctx);
          qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, md5(%s:%s:%s:%s:%s:%s) =>", __FUNCTION__,
	    hash1, nonce.av_val, nchex, cnonce, qop, hash2);
          qcRTMP_LogHexString(qcRTMP_LOGDEBUG, md5sum_val, MD5_DIGEST_LENGTH);
          hexenc(md5sum_val, MD5_DIGEST_LENGTH, hash3);

          /* pubToken = &authmod=<authmod>&user=<username>&nonce=<nonce>&cnonce=<cnonce>&nc=<nchex>&response=<hash3> */
          /* Append nonces and response to query string which already contains
           * user + authmod */
          pubToken.av_val = malloc(64 + sizeof(authmod)-1 + user.av_len + nonce.av_len + sizeof(cnonce)-1 + sizeof(nchex)-1 + HEXHASH_LEN);
          sprintf(pubToken.av_val,
                  "&nonce=%s&cnonce=%s&nc=%s&response=%s",
                  nonce.av_val, cnonce, nchex, hash3);
          pubToken.av_len = strlen(pubToken.av_val);
          qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, pubToken2: %s", __FUNCTION__, pubToken.av_val);

          free(orig_ptr);
        }
      else if(strstr(description->av_val, "?reason=authfail") != NULL)
        {
          qcRTMP_Log(qcRTMP_LOGERROR, "%s, Authentication failed", __FUNCTION__);
          return 0;
        }
      else if(strstr(description->av_val, "?reason=nosuchuser") != NULL)
        {
          qcRTMP_Log(qcRTMP_LOGERROR, "%s, Authentication failed: no such user", __FUNCTION__);
          return 0;
        }
      else
        {
          qcRTMP_Log(qcRTMP_LOGERROR, "%s, Authentication failed: unknown auth mode: %s",
                  __FUNCTION__, description->av_val);
          return 0;
        }

      ptr = malloc(r->Link.app.av_len + pubToken.av_len);
      strncpy(ptr, r->Link.app.av_val, r->Link.app.av_len);
      strncpy(ptr + r->Link.app.av_len, pubToken.av_val, pubToken.av_len);
      r->Link.app.av_len += pubToken.av_len;
      if(r->Link.lFlags & qcRTMP_LF_FAPU)
          free(r->Link.app.av_val);
      r->Link.app.av_val = ptr;

      ptr = malloc(r->Link.tcUrl.av_len + pubToken.av_len);
      strncpy(ptr, r->Link.tcUrl.av_val, r->Link.tcUrl.av_len);
      strncpy(ptr + r->Link.tcUrl.av_len, pubToken.av_val, pubToken.av_len);
      r->Link.tcUrl.av_len += pubToken.av_len;
      if(r->Link.lFlags & qcRTMP_LF_FTCU)
          free(r->Link.tcUrl.av_val);
      r->Link.tcUrl.av_val = ptr;

      free(pubToken.av_val);
      r->Link.lFlags |= qcRTMP_LF_FTCU | qcRTMP_LF_FAPU;

      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, new app: %.*s tcUrl: %.*s playpath: %s", __FUNCTION__,
              r->Link.app.av_len, r->Link.app.av_val,
              r->Link.tcUrl.av_len, r->Link.tcUrl.av_val,
              r->Link.playpath.av_val);
    }
  else
    {
      return 0;
    }
  return 1;
}
#endif


SAVC(onBWDone);
SAVC(onFCSubscribe);
SAVC(onFCUnsubscribe);
SAVC(_onbwcheck);
SAVC(_onbwdone);
SAVC(_error);
SAVC(close);
SAVC(code);
SAVC(level);
SAVC(description);
SAVC(onStatus);
SAVC(playlist_ready);
static const AVal qc_av_NetStream_Failed = AVC("NetStream.Failed");
static const AVal qc_av_NetStream_Play_Failed = AVC("NetStream.Play.Failed");
static const AVal qc_av_NetStream_Play_StreamNotFound =
AVC("NetStream.Play.StreamNotFound");
static const AVal qc_av_NetConnection_Connect_InvalidApp =
AVC("NetConnection.Connect.InvalidApp");
static const AVal qc_av_NetStream_Play_Start = AVC("NetStream.Play.Start");
static const AVal qc_av_NetStream_Play_Complete = AVC("NetStream.Play.Complete");
static const AVal qc_av_NetStream_Play_Stop = AVC("NetStream.Play.Stop");
static const AVal qc_av_NetStream_Seek_Notify = AVC("NetStream.Seek.Notify");
static const AVal qc_av_NetStream_Pause_Notify = AVC("NetStream.Pause.Notify");
static const AVal qc_av_NetStream_Play_PublishNotify =
AVC("NetStream.Play.PublishNotify");
static const AVal qc_av_NetStream_Play_UnpublishNotify =
AVC("NetStream.Play.UnpublishNotify");
static const AVal qc_av_NetStream_Publish_Start = AVC("NetStream.Publish.Start");
static const AVal qc_av_NetConnection_Connect_Rejected =
AVC("NetConnection.Connect.Rejected");

/* Returns 0 for OK/Failed/error, 1 for 'Stop or Complete' */
static int
qcHandleInvoke(qcRTMP *r, const char *body, unsigned int nBodySize)
{
  qcAMFObject obj;
  AVal method;
  double txn;
  int ret = 0, nRes;
  if (body[0] != 0x02)		/* make sure it is a string method name we start with */
    {
      qcRTMP_Log(qcRTMP_LOGWARNING, "%s, Sanity failed. no string method in invoke packet",
	  __FUNCTION__);
      return 0;
    }

  nRes = qcAMF_Decode(&obj, body, nBodySize, FALSE);
  if (nRes < 0)
    {
      qcRTMP_Log(qcRTMP_LOGERROR, "%s, error decoding invoke packet", __FUNCTION__);
      return 0;
    }

  qcAMF_Dump(&obj);
  qcAMFProp_GetString(qcAMF_GetProp(&obj, NULL, 0), &method);
  txn = qcAMFProp_GetNumber(qcAMF_GetProp(&obj, NULL, 1));
  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, server invoking <%s>", __FUNCTION__, method.av_val);

  if (AVMATCH(&method, &av__result))
    {
      AVal methodInvoked = {0};
      int i;

      for (i=0; i<r->m_numCalls; i++) {
	if (r->m_methodCalls[i].num == (int)txn) {
	  methodInvoked = r->m_methodCalls[i].name;
	  qcAV_erase(r->m_methodCalls, &r->m_numCalls, i, FALSE);
	  break;
	}
      }
      if (!methodInvoked.av_val) {
        qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, received result id %f without matching request",
	  __FUNCTION__, txn);
	goto leave;
      }

      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, received result for method call <%s>", __FUNCTION__,
	  methodInvoked.av_val);

      if (AVMATCH(&methodInvoked, &av_connect))
	{
	  if (r->Link.token.av_len)
	    {
	      qcAMFObjectProperty p;
	      if (qcRTMP_FindFirstMatchingProperty(&obj, &av_secureToken, &p))
		{
		  qcDecodeTEA(&r->Link.token, &p.p_vu.p_aval);
		  qcSendSecureTokenResponse(r, &p.p_vu.p_aval);
		}
	    }
	  if (r->Link.protocol & qcRTMP_FEATURE_WRITE)
	    {
	      qcSendReleaseStream(r);
	      qcSendFCPublish(r);
	    }
	  else
	    {
	      qcRTMP_SendServerBW(r);
	      qcRTMP_SendCtrl(r, 3, 0, 300);
	    }
	  qcRTMP_SendCreateStream(r);

	  if (!(r->Link.protocol & qcRTMP_FEATURE_WRITE))
	    {
	      /* Authenticate on Justin.tv legacy servers before sending FCSubscribe */
	      if (r->Link.usherToken.av_len)
	        qcSendUsherToken(r, &r->Link.usherToken);
	      /* Send the FCSubscribe if live stream or if subscribepath is set */
	      if (r->Link.subscribepath.av_len)
	        qcSendFCSubscribe(r, &r->Link.subscribepath);
	      else if (r->Link.lFlags & qcRTMP_LF_LIVE)
	        qcSendFCSubscribe(r, &r->Link.playpath);
	    }
	}
      else if (AVMATCH(&methodInvoked, &av_createStream))
	{
	  r->m_stream_id = (int)qcAMFProp_GetNumber(qcAMF_GetProp(&obj, NULL, 3));

	  if (r->Link.protocol & qcRTMP_FEATURE_WRITE)
	    {
	      qcSendPublish(r);
	    }
	  else
	    {
	      if (r->Link.lFlags & qcRTMP_LF_PLST)
	        qcSendPlaylist(r);
	      qcSendPlay(r);
	      qcRTMP_SendCtrl(r, 3, r->m_stream_id, r->m_nBufferMS);
	    }
	}
      else if (AVMATCH(&methodInvoked, &av_play) ||
      	AVMATCH(&methodInvoked, &av_publish))
	{
	  r->m_bPlaying = TRUE;
	}
      free(methodInvoked.av_val);
    }
  else if (AVMATCH(&method, &av_onBWDone))
    {
	  if (!r->m_nBWCheckCounter)
        qcSendCheckBW(r);
    }
  else if (AVMATCH(&method, &av_onFCSubscribe))
    {
      /* SendOnFCSubscribe(); */
    }
  else if (AVMATCH(&method, &av_onFCUnsubscribe))
    {
      qcRTMP_Close(r);
      ret = 1;
    }
  else if (AVMATCH(&method, &av_ping))
    {
      qcSendPong(r, txn);
    }
  else if (AVMATCH(&method, &av__onbwcheck))
    {
      qcSendCheckBWResult(r, txn);
    }
  else if (AVMATCH(&method, &av__onbwdone))
    {
      int i;
      for (i = 0; i < r->m_numCalls; i++)
	if (AVMATCH(&r->m_methodCalls[i].name, &av__checkbw))
	  {
	    qcAV_erase(r->m_methodCalls, &r->m_numCalls, i, TRUE);
	    break;
	  }
    }
  else if (AVMATCH(&method, &av__error))
    {
#ifdef CRYPTO
      AVal methodInvoked = {0};
      int i;

      if (r->Link.protocol & qcRTMP_FEATURE_WRITE)
        {
          for (i=0; i<r->m_numCalls; i++)
            {
              if (r->m_methodCalls[i].num == txn)
                {
                  methodInvoked = r->m_methodCalls[i].name;
                  AV_erase(r->m_methodCalls, &r->m_numCalls, i, FALSE);
                  break;
                }
            }
          if (!methodInvoked.av_val)
            {
              qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, received result id %f without matching request",
                    __FUNCTION__, txn);
              goto leave;
            }

          qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, received error for method call <%s>", __FUNCTION__,
          methodInvoked.av_val);

          if (AVMATCH(&methodInvoked, &av_connect))
            {
              qcAMFObject obj2;
              AVal code, level, description;
              qcAMFProp_GetObject(qcAMF_GetProp(&obj, NULL, 3), &obj2);
              qcAMFProp_GetString(qcAMF_GetProp(&obj2, &av_code, -1), &code);
              qcAMFProp_GetString(qcAMF_GetProp(&obj2, &av_level, -1), &level);
              qcAMFProp_GetString(qcAMF_GetProp(&obj2, &av_description, -1), &description);
              qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, error description: %s", __FUNCTION__, description.av_val);
              /* if PublisherAuth returns 1, then reconnect */
              if (PublisherAuth(r, &description) == 1)
              {
                CloseInternal(r, 1);
                if (!qcRTMP_Connect(r, NULL) || !qcRTMP_ConnectStream(r, 0))
                  goto leave;
              }
            }
        }
      else
        {
          qcRTMP_Log(qcRTMP_LOGERROR, "rtmp server sent error");
        }
      free(methodInvoked.av_val);
#else
      qcRTMP_Log(qcRTMP_LOGERROR, "rtmp server sent error");
#endif
    }
  else if (AVMATCH(&method, &av_close))
    {
      qcRTMP_Log(qcRTMP_LOGERROR, "rtmp server requested close");
      qcRTMP_Close(r);
    }
  else if (AVMATCH(&method, &av_onStatus))
    {
      qcAMFObject obj2;
      AVal code, level;
      qcAMFProp_GetObject(qcAMF_GetProp(&obj, NULL, 3), &obj2);
      qcAMFProp_GetString(qcAMF_GetProp(&obj2, &av_code, -1), &code);
      qcAMFProp_GetString(qcAMF_GetProp(&obj2, &av_level, -1), &level);

      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, onStatus: %s", __FUNCTION__, code.av_val);
      if (AVMATCH(&code, &qc_av_NetStream_Failed)
	  || AVMATCH(&code, &qc_av_NetStream_Play_Failed)
	  || AVMATCH(&code, &qc_av_NetStream_Play_StreamNotFound)
	  || AVMATCH(&code, &qc_av_NetConnection_Connect_InvalidApp))
	{
	  r->m_stream_id = -1;
	  qcRTMP_Close(r);
	  qcRTMP_Log(qcRTMP_LOGERROR, "Closing connection: %s", code.av_val);
	}

      else if (AVMATCH(&code, &qc_av_NetStream_Play_Start)
           || AVMATCH(&code, &qc_av_NetStream_Play_PublishNotify))
	{
	  int i;
	  r->m_bPlaying = TRUE;
	  for (i = 0; i < r->m_numCalls; i++)
	    {
	      if (AVMATCH(&r->m_methodCalls[i].name, &av_play))
		{
		  qcAV_erase(r->m_methodCalls, &r->m_numCalls, i, TRUE);
		  break;
		}
	    }
	}

      else if (AVMATCH(&code, &qc_av_NetStream_Publish_Start))
	{
	  int i;
	  r->m_bPlaying = TRUE;
	  for (i = 0; i < r->m_numCalls; i++)
	    {
	      if (AVMATCH(&r->m_methodCalls[i].name, &av_publish))
		{
		  qcAV_erase(r->m_methodCalls, &r->m_numCalls, i, TRUE);
		  break;
		}
	    }
	}

      /* Return 1 if this is a Play.Complete or Play.Stop */
      else if (AVMATCH(&code, &qc_av_NetStream_Play_Complete)
	  || AVMATCH(&code, &qc_av_NetStream_Play_Stop)
	  || AVMATCH(&code, &qc_av_NetStream_Play_UnpublishNotify))
	{
	  qcRTMP_Close(r);
	  ret = 1;
	}

      else if (AVMATCH(&code, &qc_av_NetStream_Seek_Notify))
        {
	  r->m_read.flags &= ~qcRTMP_READ_SEEKING;
	}

      else if (AVMATCH(&code, &qc_av_NetStream_Pause_Notify))
        {
	  if (r->m_pausing == 1 || r->m_pausing == 2)
	  {
	    qcRTMP_SendPause(r, FALSE, r->m_pauseStamp);
	    r->m_pausing = 3;
	  }
	}
    }
  else if (AVMATCH(&method, &av_playlist_ready))
    {
      int i;
      for (i = 0; i < r->m_numCalls; i++)
        {
          if (AVMATCH(&r->m_methodCalls[i].name, &av_set_playlist))
	    {
	      qcAV_erase(r->m_methodCalls, &r->m_numCalls, i, TRUE);
	      break;
	    }
        }
    }
  else
    {

    }
leave:
  qcAMF_Reset(&obj);
  return ret;
}

int
qcRTMP_FindFirstMatchingProperty(qcAMFObject *obj, const AVal *name,
			       qcAMFObjectProperty * p)
{
  int n;
  /* this is a small object search to locate the "duration" property */
  for (n = 0; n < obj->o_num; n++)
    {
      qcAMFObjectProperty *prop = qcAMF_GetProp(obj, NULL, n);

      if (AVMATCH(&prop->p_name, name))
	{
	  memcpy(p, prop, sizeof(*prop));
	  return TRUE;
	}

      if (prop->p_type == qcAMF_OBJECT || prop->p_type == qcAMF_ECMA_ARRAY)
	{
	  if (qcRTMP_FindFirstMatchingProperty(&prop->p_vu.p_object, name, p))
	    return TRUE;
	}
    }
  return FALSE;
}

/* Like above, but only check if name is a prefix of property */
int
qcRTMP_FindPrefixProperty(qcAMFObject *obj, const AVal *name,
			       qcAMFObjectProperty * p)
{
  int n;
  for (n = 0; n < obj->o_num; n++)
    {
      qcAMFObjectProperty *prop = qcAMF_GetProp(obj, NULL, n);

      if (prop->p_name.av_len > name->av_len &&
      	  !memcmp(prop->p_name.av_val, name->av_val, name->av_len))
	{
	  memcpy(p, prop, sizeof(*prop));
	  return TRUE;
	}

      if (prop->p_type == qcAMF_OBJECT)
	{
	  if (qcRTMP_FindPrefixProperty(&prop->p_vu.p_object, name, p))
	    return TRUE;
	}
    }
  return FALSE;
}

static int
qcDumpMetaData(qcAMFObject *obj)
{
  qcAMFObjectProperty *prop;
  int n, len;
  for (n = 0; n < obj->o_num; n++)
    {
      char str[256] = "";
      prop = qcAMF_GetProp(obj, NULL, n);
      switch (prop->p_type)
	{
	case qcAMF_OBJECT:
	case qcAMF_ECMA_ARRAY:
	case qcAMF_STRICT_ARRAY:
	  if (prop->p_name.av_len)
	    qcRTMP_Log(qcRTMP_LOGINFO, "%.*s:", prop->p_name.av_len, prop->p_name.av_val);
	  qcDumpMetaData(&prop->p_vu.p_object);
	  break;
	case qcAMF_NUMBER:
	  snprintf(str, 255, "%.2f", prop->p_vu.p_number);
	  break;
	case qcAMF_BOOLEAN:
	  snprintf(str, 255, "%s",
		   prop->p_vu.p_number != 0. ? "TRUE" : "FALSE");
	  break;
	case qcAMF_STRING:
	  len = snprintf(str, 255, "%.*s", prop->p_vu.p_aval.av_len,
		   prop->p_vu.p_aval.av_val);
	  if (len >= 1 && str[len-1] == '\n')
	    str[len-1] = '\0';
	  break;
	case qcAMF_DATE:
	  snprintf(str, 255, "timestamp:%.2f", prop->p_vu.p_number);
	  break;
	default:
	  snprintf(str, 255, "INVALID TYPE 0x%02x",
		   (unsigned char)prop->p_type);
	}
      if (str[0] && prop->p_name.av_len)
	{
	  qcRTMP_Log(qcRTMP_LOGINFO, "  %-22.*s%s", prop->p_name.av_len,
		    prop->p_name.av_val, str);
	}
    }
  return FALSE;
}

SAVC(onMetaData);
SAVC(duration);
SAVC(video);
SAVC(audio);

static int
qcHandleMetadata(qcRTMP *r, char *body, unsigned int len)
{
  /* allright we get some info here, so parse it and print it */
  /* also keep duration or filesize to make a nice progress bar */

  qcAMFObject obj;
  AVal metastring;
  int ret = FALSE;

  int nRes = qcAMF_Decode(&obj, body, len, FALSE);
  if (nRes < 0)
    {
      qcRTMP_Log(qcRTMP_LOGERROR, "%s, error decoding meta data packet", __FUNCTION__);
      return FALSE;
    }

  qcAMF_Dump(&obj);
  qcAMFProp_GetString(qcAMF_GetProp(&obj, NULL, 0), &metastring);

  if (AVMATCH(&metastring, &av_onMetaData))
    {
      qcAMFObjectProperty prop;
      /* Show metadata */
      qcRTMP_Log(qcRTMP_LOGINFO, "Metadata:");
      qcDumpMetaData(&obj);
      if (qcRTMP_FindFirstMatchingProperty(&obj, &av_duration, &prop))
	{
	  r->m_fDuration = prop.p_vu.p_number;
	  /*qcRTMP_Log(qcRTMP_LOGDEBUG, "Set duration: %.2f", m_fDuration); */
	}
      /* Search for audio or video tags */
      if (qcRTMP_FindPrefixProperty(&obj, &av_video, &prop))
        r->m_read.dataType |= 1;
      if (qcRTMP_FindPrefixProperty(&obj, &av_audio, &prop))
        r->m_read.dataType |= 4;
      ret = TRUE;
    }
  qcAMF_Reset(&obj);
  return ret;
}

static void
qcHandleChangeChunkSize(qcRTMP *r, const qcRTMPPacket *packet)
{
  if (packet->m_nBodySize >= 4)
    {
      r->m_inChunkSize = qcAMF_DecodeInt32(packet->m_body);
      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, received: chunk size change to %d", __FUNCTION__,
	  r->m_inChunkSize);
    }
}

static void
qcHandleAudio(qcRTMP *r, const qcRTMPPacket *packet)
{
}

static void
qcHandleVideo(qcRTMP *r, const qcRTMPPacket *packet)
{
}

static void
qcHandleCtrl(qcRTMP *r, const qcRTMPPacket *packet)
{
  short nType = -1;
  unsigned int tmp;
  if (packet->m_body && packet->m_nBodySize >= 2)
    nType = qcAMF_DecodeInt16(packet->m_body);
  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, received ctrl. type: %d, len: %d", __FUNCTION__, nType,
      packet->m_nBodySize);
  /*qcRTMP_LogHex(packet.m_body, packet.m_nBodySize); */

  if (packet->m_nBodySize >= 6)
    {
      switch (nType)
	{
	case 0:
	  tmp = qcAMF_DecodeInt32(packet->m_body + 2);
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, Stream Begin %d", __FUNCTION__, tmp);
	  break;

	case 1:
	  tmp = qcAMF_DecodeInt32(packet->m_body + 2);
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, Stream EOF %d", __FUNCTION__, tmp);
	  if (r->m_pausing == 1)
	    r->m_pausing = 2;
	  break;

	case 2:
	  tmp = qcAMF_DecodeInt32(packet->m_body + 2);
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, Stream Dry %d", __FUNCTION__, tmp);
	  break;

	case 4:
	  tmp = qcAMF_DecodeInt32(packet->m_body + 2);
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, Stream IsRecorded %d", __FUNCTION__, tmp);
	  break;

	case 6:		/* server ping. reply with pong. */
	  tmp = qcAMF_DecodeInt32(packet->m_body + 2);
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, Ping %d", __FUNCTION__, tmp);
	  qcRTMP_SendCtrl(r, 0x07, tmp, 0);
	  break;

	/* FMS 3.5 servers send the following two controls to let the client
	 * know when the server has sent a complete buffer. I.e., when the
	 * server has sent an amount of data equal to m_nBufferMS in duration.
	 * The server meters its output so that data arrives at the client
	 * in realtime and no faster.
	 *
	 * The rtmpdump program tries to set m_nBufferMS as large as
	 * possible, to force the server to send data as fast as possible.
	 * In practice, the server appears to cap this at about 1 hour's
	 * worth of data. After the server has sent a complete buffer, and
	 * sends this BufferEmpty message, it will wait until the play
	 * duration of that buffer has passed before sending a new buffer.
	 * The BufferReady message will be sent when the new buffer starts.
	 * (There is no BufferReady message for the very first buffer;
	 * presumably the Stream Begin message is sufficient for that
	 * purpose.)
	 *
	 * If the network speed is much faster than the data bitrate, then
	 * there may be long delays between the end of one buffer and the
	 * start of the next.
	 *
	 * Since usually the network allows data to be sent at
	 * faster than realtime, and rtmpdump wants to download the data
	 * as fast as possible, we use this RTMP_LF_BUFX hack: when we
	 * get the BufferEmpty message, we send a Pause followed by an
	 * Unpause. This causes the server to send the next buffer immediately
	 * instead of waiting for the full duration to elapse. (That's
	 * also the purpose of the ToggleStream function, which rtmpdump
	 * calls if we get a read timeout.)
	 *
	 * Media player apps don't need this hack since they are just
	 * going to play the data in realtime anyway. It also doesn't work
	 * for live streams since they obviously can only be sent in
	 * realtime. And it's all moot if the network speed is actually
	 * slower than the media bitrate.
	 */
	case 31:
	  tmp = qcAMF_DecodeInt32(packet->m_body + 2);
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, Stream BufferEmpty %d", __FUNCTION__, tmp);
	  if (!(r->Link.lFlags & qcRTMP_LF_BUFX))
	    break;
	  if (!r->m_pausing)
	    {
	      r->m_pauseStamp = r->m_mediaChannel < r->m_channelsAllocatedIn ?
	                        r->m_channelTimestamp[r->m_mediaChannel] : 0;
	      qcRTMP_SendPause(r, TRUE, r->m_pauseStamp);
	      r->m_pausing = 1;
	    }
	  else if (r->m_pausing == 2)
	    {
	      qcRTMP_SendPause(r, FALSE, r->m_pauseStamp);
	      r->m_pausing = 3;
	    }
	  break;

	case 32:
	  tmp = qcAMF_DecodeInt32(packet->m_body + 2);
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, Stream BufferReady %d", __FUNCTION__, tmp);
	  break;

	default:
	  tmp = qcAMF_DecodeInt32(packet->m_body + 2);
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, Stream xx %d", __FUNCTION__, tmp);
	  break;
	}

    }

  if (nType == 0x1A)
    {
      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, SWFVerification ping received: ", __FUNCTION__);
      if (packet->m_nBodySize > 2 && packet->m_body[2] > 0x01)
	{
	  qcRTMP_Log(qcRTMP_LOGERROR,
            "%s: SWFVerification Type %d request not supported! Patches welcome...",
	    __FUNCTION__, packet->m_body[2]);
	}
#ifdef CRYPTO
      /*qcRTMP_LogHex(packet.m_body, packet.m_nBodySize); */

      /* respond with HMAC SHA256 of decompressed SWF, key is the 30byte player key, also the last 30 bytes of the server handshake are applied */
      else if (r->Link.SWFSize)
	{
	  qcRTMP_SendCtrl(r, 0x1B, 0, 0);
	}
      else
	{
	  qcRTMP_Log(qcRTMP_LOGERROR,
	      "%s: Ignoring SWFVerification request, use --swfVfy!",
	      __FUNCTION__);
	}
#else
      qcRTMP_Log(qcRTMP_LOGERROR,
	  "%s: Ignoring SWFVerification request, no CRYPTO support!",
	  __FUNCTION__);
#endif
    }
}

static void
qcHandleServerBW(qcRTMP *r, const qcRTMPPacket *packet)
{
  r->m_nServerBW = qcAMF_DecodeInt32(packet->m_body);
  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s: server BW = %d", __FUNCTION__, r->m_nServerBW);
}

static void
qcHandleClientBW(qcRTMP *r, const qcRTMPPacket *packet)
{
  r->m_nClientBW = qcAMF_DecodeInt32(packet->m_body);
  if (packet->m_nBodySize > 4)
    r->m_nClientBW2 = packet->m_body[4];
  else
    r->m_nClientBW2 = -1;
  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s: client BW = %d %d", __FUNCTION__, r->m_nClientBW,
      r->m_nClientBW2);
}

static int
qcDecodeInt32LE(const char *data)
{
  unsigned char *c = (unsigned char *)data;
  unsigned int val;

  val = (c[3] << 24) | (c[2] << 16) | (c[1] << 8) | c[0];
  return val;
}

static int
qcEncodeInt32LE(char *output, int nVal)
{
  output[0] = nVal;
  nVal >>= 8;
  output[1] = nVal;
  nVal >>= 8;
  output[2] = nVal;
  nVal >>= 8;
  output[3] = nVal;
  return 4;
}

int
qcRTMP_ReadPacket(qcRTMP *r, qcRTMPPacket *packet)
{
  uint8_t hbuf[qcRTMP_MAX_HEADER_SIZE] = { 0 };
  char *header = (char *)hbuf;
  int nSize, hSize, nToRead, nChunk;
  int didAlloc = FALSE;
  int extendedTimestamp;

  qcRTMP_Log(qcRTMP_LOGDEBUG2, "%s: fd=%d", __FUNCTION__, r->m_sb.sb_socket);

  if (qcReadN(r, (char *)hbuf, 1) == 0)
    {
      qcRTMP_Log(qcRTMP_LOGERROR, "%s, failed to read RTMP packet header, %d", __FUNCTION__, qcRTMP_GetTime());
      return FALSE;
    }

  packet->m_headerType = (hbuf[0] & 0xc0) >> 6;
  packet->m_nChannel = (hbuf[0] & 0x3f);
  header++;
  if (packet->m_nChannel == 0)
    {
      if (qcReadN(r, (char *)&hbuf[1], 1) != 1)
	{
	  qcRTMP_Log(qcRTMP_LOGERROR, "%s, failed to read RTMP packet header 2nd byte",
	      __FUNCTION__);
	  return FALSE;
	}
      packet->m_nChannel = hbuf[1];
      packet->m_nChannel += 64;
      header++;
    }
  else if (packet->m_nChannel == 1)
    {
      int tmp;
      if (qcReadN(r, (char *)&hbuf[1], 2) != 2)
	{
	  qcRTMP_Log(qcRTMP_LOGERROR, "%s, failed to read RTMP packet header 3nd byte",
	      __FUNCTION__);
	  return FALSE;
	}
      tmp = (hbuf[2] << 8) + hbuf[1];
      packet->m_nChannel = tmp + 64;
      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, m_nChannel: %0x", __FUNCTION__, packet->m_nChannel);
      header += 2;
    }

  nSize = qc_packetSize[packet->m_headerType];

  if (packet->m_nChannel >= r->m_channelsAllocatedIn)
    {
      int n = packet->m_nChannel + 10;
      int *timestamp = realloc(r->m_channelTimestamp, sizeof(int) * n);
      qcRTMPPacket **packets = realloc(r->m_vecChannelsIn, sizeof(qcRTMPPacket*) * n);
      if (!timestamp)
        free(r->m_channelTimestamp);
      if (!packets)
        free(r->m_vecChannelsIn);
      r->m_channelTimestamp = timestamp;
      r->m_vecChannelsIn = packets;
      if (!timestamp || !packets) {
        r->m_channelsAllocatedIn = 0;
        return FALSE;
      }
      memset(r->m_channelTimestamp + r->m_channelsAllocatedIn, 0, sizeof(int) * (n - r->m_channelsAllocatedIn));
      memset(r->m_vecChannelsIn + r->m_channelsAllocatedIn, 0, sizeof(qcRTMPPacket*) * (n - r->m_channelsAllocatedIn));
      r->m_channelsAllocatedIn = n;
    }

  if (nSize == qcRTMP_LARGE_HEADER_SIZE)	/* if we get a full header the timestamp is absolute */
    packet->m_hasAbsTimestamp = TRUE;

  else if (nSize < qcRTMP_LARGE_HEADER_SIZE)
    {				/* using values from the last message of this channel */
      if (r->m_vecChannelsIn[packet->m_nChannel])
	memcpy(packet, r->m_vecChannelsIn[packet->m_nChannel],
	       sizeof(qcRTMPPacket));
    }

  nSize--;

  if (nSize > 0 && qcReadN(r, header, nSize) != nSize)
    {
      qcRTMP_Log(qcRTMP_LOGERROR, "%s, failed to read RTMP packet header. type: %x",
	  __FUNCTION__, (unsigned int)hbuf[0]);
      return FALSE;
    }

  hSize = nSize + (header - (char *)hbuf);

  if (nSize >= 3)
    {
      packet->m_nTimeStamp = qcAMF_DecodeInt24(header);

      /*qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, reading RTMP packet chunk on channel %x, headersz %i, timestamp %i, abs timestamp %i", __FUNCTION__, packet.m_nChannel, nSize, packet.m_nTimeStamp, packet.m_hasAbsTimestamp); */

      if (nSize >= 6)
	{
	  packet->m_nBodySize = qcAMF_DecodeInt24(header + 3);
	  packet->m_nBytesRead = 0;

	  if (nSize > 6)
	    {
	      packet->m_packetType = header[6];

	      if (nSize == 11)
		packet->m_nInfoField2 = qcDecodeInt32LE(header + 7);
	    }
	}
    }

  extendedTimestamp = packet->m_nTimeStamp == 0xffffff;
  if (extendedTimestamp)
    {
      if (qcReadN(r, header + nSize, 4) != 4)
	{
	  qcRTMP_Log(qcRTMP_LOGERROR, "%s, failed to read extended timestamp",
	      __FUNCTION__);
	  return FALSE;
	}
      packet->m_nTimeStamp = qcAMF_DecodeInt32(header + nSize);
      hSize += 4;
    }

  qcRTMP_LogHexString(qcRTMP_LOGDEBUG2, (uint8_t *)hbuf, hSize);

  if (packet->m_nBodySize > 0 && packet->m_body == NULL)
    {
      if (!qcRTMPPacket_Alloc(packet, packet->m_nBodySize))
	{
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, failed to allocate packet", __FUNCTION__);
	  return FALSE;
	}
      didAlloc = TRUE;
      packet->m_headerType = (hbuf[0] & 0xc0) >> 6;
    }

  nToRead = packet->m_nBodySize - packet->m_nBytesRead;
  nChunk = r->m_inChunkSize;
  if (nToRead < nChunk)
    nChunk = nToRead;

  /* Does the caller want the raw chunk? */
  if (packet->m_chunk)
    {
      packet->m_chunk->c_headerSize = hSize;
      memcpy(packet->m_chunk->c_header, hbuf, hSize);
      packet->m_chunk->c_chunk = packet->m_body + packet->m_nBytesRead;
      packet->m_chunk->c_chunkSize = nChunk;
    }

  if (qcReadN(r, packet->m_body + packet->m_nBytesRead, nChunk) != nChunk)
    {
      qcRTMP_Log(qcRTMP_LOGERROR, "%s, failed to read RTMP packet body. len: %u",
	  __FUNCTION__, packet->m_nBodySize);
      return FALSE;
    }

  qcRTMP_LogHexString(qcRTMP_LOGDEBUG2, (uint8_t *)packet->m_body + packet->m_nBytesRead, nChunk);

  packet->m_nBytesRead += nChunk;

  /* keep the packet as ref for other packets on this channel */
  if (!r->m_vecChannelsIn[packet->m_nChannel])
    r->m_vecChannelsIn[packet->m_nChannel] = malloc(sizeof(qcRTMPPacket));
  memcpy(r->m_vecChannelsIn[packet->m_nChannel], packet, sizeof(qcRTMPPacket));
  if (extendedTimestamp)
    {
      r->m_vecChannelsIn[packet->m_nChannel]->m_nTimeStamp = 0xffffff;
    }

  if (qcRTMPPacket_IsReady(packet))
    {
      /* make packet's timestamp absolute */
      if (!packet->m_hasAbsTimestamp)
	packet->m_nTimeStamp += r->m_channelTimestamp[packet->m_nChannel];	/* timestamps seem to be always relative!! */

      r->m_channelTimestamp[packet->m_nChannel] = packet->m_nTimeStamp;

      /* reset the data from the stored packet. we keep the header since we may use it later if a new packet for this channel */
      /* arrives and requests to re-use some info (small packet header) */
      r->m_vecChannelsIn[packet->m_nChannel]->m_body = NULL;
      r->m_vecChannelsIn[packet->m_nChannel]->m_nBytesRead = 0;
      r->m_vecChannelsIn[packet->m_nChannel]->m_hasAbsTimestamp = FALSE;	/* can only be false if we reuse header */
    }
  else
    {
      packet->m_body = NULL;	/* so it won't be erased on free */
    }

  return TRUE;
}

#ifndef CRYPTO
static int
qcHandShake(qcRTMP *r, int FP9HandShake)
{
  int i;
  uint32_t uptime, suptime;
  int bMatch;
  char type;
  char clientbuf[qcRTMP_SIG_SIZE + 1], *clientsig = clientbuf + 1;
  char serversig[qcRTMP_SIG_SIZE];

  clientbuf[0] = 0x03;		/* not encrypted */

  uptime = htonl(qcRTMP_GetTime());
  memcpy(clientsig, &uptime, 4);

  memset(&clientsig[4], 0, 4);

#ifdef _DEBUG
  for (i = 8; i < qcRTMP_SIG_SIZE; i++)
    clientsig[i] = 0xff;
#else
  for (i = 8; i < qcRTMP_SIG_SIZE; i++)
    clientsig[i] = (char)(rand() % 256);
#endif

  if (!qcWriteN(r, clientbuf, qcRTMP_SIG_SIZE + 1))
    return FALSE;

  if (qcReadN(r, &type, 1) != 1)	/* 0x03 or 0x06 */
    return FALSE;

  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s: Type Answer   : %02X", __FUNCTION__, type);

  if (type != clientbuf[0])
    qcRTMP_Log(qcRTMP_LOGWARNING, "%s: Type mismatch: client sent %d, server answered %d",
	__FUNCTION__, clientbuf[0], type);

  if (qcReadN(r, serversig, qcRTMP_SIG_SIZE) != qcRTMP_SIG_SIZE)
    return FALSE;

  /* decode server response */

  memcpy(&suptime, serversig, 4);
  suptime = ntohl(suptime);

  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s: Server Uptime : %d", __FUNCTION__, suptime);
  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s: FMS Version   : %d.%d.%d.%d", __FUNCTION__,
      serversig[4], serversig[5], serversig[6], serversig[7]);

  /* 2nd part of handshake */
  if (!qcWriteN(r, serversig, qcRTMP_SIG_SIZE))
    return FALSE;

  if (qcReadN(r, serversig, qcRTMP_SIG_SIZE) != qcRTMP_SIG_SIZE)
    return FALSE;

  bMatch = (memcmp(serversig, clientsig, qcRTMP_SIG_SIZE) == 0);
  if (!bMatch)
    {
      qcRTMP_Log(qcRTMP_LOGWARNING, "%s, client signature does not match!", __FUNCTION__);
    }
  return TRUE;
}

static int
qcSHandShake(qcRTMP *r)
{
  int i;
  char serverbuf[qcRTMP_SIG_SIZE + 1], *serversig = serverbuf + 1;
  char clientsig[qcRTMP_SIG_SIZE];
  uint32_t uptime;
  int bMatch;

  if (qcReadN(r, serverbuf, 1) != 1)	/* 0x03 or 0x06 */
    return FALSE;

  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s: Type Request  : %02X", __FUNCTION__, serverbuf[0]);

  if (serverbuf[0] != 3)
    {
      qcRTMP_Log(qcRTMP_LOGERROR, "%s: Type unknown: client sent %02X",
	  __FUNCTION__, serverbuf[0]);
      return FALSE;
    }

  uptime = htonl(qcRTMP_GetTime());
  memcpy(serversig, &uptime, 4);

  memset(&serversig[4], 0, 4);
#ifdef _DEBUG
  for (i = 8; i < qcRTMP_SIG_SIZE; i++)
    serversig[i] = 0xff;
#else
  for (i = 8; i < qcRTMP_SIG_SIZE; i++)
    serversig[i] = (char)(rand() % 256);
#endif

  if (!qcWriteN(r, serverbuf, qcRTMP_SIG_SIZE + 1))
    return FALSE;

  if (qcReadN(r, clientsig, qcRTMP_SIG_SIZE) != qcRTMP_SIG_SIZE)
    return FALSE;

  /* decode client response */

  memcpy(&uptime, clientsig, 4);
  uptime = ntohl(uptime);

  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s: Client Uptime : %d", __FUNCTION__, uptime);
  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s: Player Version: %d.%d.%d.%d", __FUNCTION__,
      clientsig[4], clientsig[5], clientsig[6], clientsig[7]);

  /* 2nd part of handshake */
  if (!qcWriteN(r, clientsig, qcRTMP_SIG_SIZE))
    return FALSE;

  if (qcReadN(r, clientsig, qcRTMP_SIG_SIZE) != qcRTMP_SIG_SIZE)
    return FALSE;

  bMatch = (memcmp(serversig, clientsig, qcRTMP_SIG_SIZE) == 0);
  if (!bMatch)
    {
      qcRTMP_Log(qcRTMP_LOGWARNING, "%s, client signature does not match!", __FUNCTION__);
    }
  return TRUE;
}
#endif

int
qcRTMP_SendChunk(qcRTMP *r, qcRTMPChunk *chunk)
{
  int wrote;
  char hbuf[qcRTMP_MAX_HEADER_SIZE];

  qcRTMP_Log(qcRTMP_LOGDEBUG2, "%s: fd=%d, size=%d", __FUNCTION__, r->m_sb.sb_socket,
      chunk->c_chunkSize);
  qcRTMP_LogHexString(qcRTMP_LOGDEBUG2, (uint8_t *)chunk->c_header, chunk->c_headerSize);
  if (chunk->c_chunkSize)
    {
      char *ptr = chunk->c_chunk - chunk->c_headerSize;
      qcRTMP_LogHexString(qcRTMP_LOGDEBUG2, (uint8_t *)chunk->c_chunk, chunk->c_chunkSize);
      /* save header bytes we're about to overwrite */
      memcpy(hbuf, ptr, chunk->c_headerSize);
      memcpy(ptr, chunk->c_header, chunk->c_headerSize);
      wrote = qcWriteN(r, ptr, chunk->c_headerSize + chunk->c_chunkSize);
      memcpy(ptr, hbuf, chunk->c_headerSize);
    }
  else
    wrote = qcWriteN(r, chunk->c_header, chunk->c_headerSize);
  return wrote;
}

int
qcRTMP_SendPacket(qcRTMP *r, qcRTMPPacket *packet, int queue)
{
  if(r->forceClose == 1)
     return FALSE;
  const qcRTMPPacket *prevPacket;
  uint32_t last = 0;
  int nSize;
  int hSize, cSize;
  char *header, *hptr, *hend, hbuf[qcRTMP_MAX_HEADER_SIZE], c;
  uint32_t t;
  char *buffer, *tbuf = NULL, *toff = NULL;
  int nChunkSize;
  int tlen;

  if (packet->m_nChannel >= r->m_channelsAllocatedOut)
    {
      int n = packet->m_nChannel + 10;
      qcRTMPPacket **packets = realloc(r->m_vecChannelsOut, sizeof(qcRTMPPacket*) * n);
      if (!packets) {
        free(r->m_vecChannelsOut);
        r->m_vecChannelsOut = NULL;
        r->m_channelsAllocatedOut = 0;
        return FALSE;
      }
      r->m_vecChannelsOut = packets;
      memset(r->m_vecChannelsOut + r->m_channelsAllocatedOut, 0, sizeof(qcRTMPPacket*) * (n - r->m_channelsAllocatedOut));
      r->m_channelsAllocatedOut = n;
    }

  prevPacket = r->m_vecChannelsOut[packet->m_nChannel];
  if (prevPacket && packet->m_headerType != qcRTMP_PACKET_SIZE_LARGE)
    {
      /* compress a bit by using the prev packet's attributes */
      if (prevPacket->m_nBodySize == packet->m_nBodySize
	  && prevPacket->m_packetType == packet->m_packetType
	  && packet->m_headerType == qcRTMP_PACKET_SIZE_MEDIUM)
	packet->m_headerType = qcRTMP_PACKET_SIZE_SMALL;

      if (prevPacket->m_nTimeStamp == packet->m_nTimeStamp
	  && packet->m_headerType == qcRTMP_PACKET_SIZE_SMALL)
	packet->m_headerType = qcRTMP_PACKET_SIZE_MINIMUM;
      last = prevPacket->m_nTimeStamp;
    }

  if (packet->m_headerType > 3)	/* sanity */
    {
      qcRTMP_Log(qcRTMP_LOGERROR, "sanity failed!! trying to send header of type: 0x%02x.",
	  (unsigned char)packet->m_headerType);
      return FALSE;
    }

  nSize = qc_packetSize[packet->m_headerType];
  hSize = nSize; cSize = 0;
  t = packet->m_nTimeStamp - last;

  if (packet->m_body)
    {
      header = packet->m_body - nSize;
      hend = packet->m_body;
    }
  else
    {
      header = hbuf + 6;
      hend = hbuf + sizeof(hbuf);
    }

  if (packet->m_nChannel > 319)
    cSize = 2;
  else if (packet->m_nChannel > 63)
    cSize = 1;
  if (cSize)
    {
      header -= cSize;
      hSize += cSize;
    }

  if (t >= 0xffffff)
    {
      header -= 4;
      hSize += 4;
      qcRTMP_Log(qcRTMP_LOGWARNING, "Larger timestamp than 24-bit: 0x%x", t);
    }

  hptr = header;
  c = packet->m_headerType << 6;
  switch (cSize)
    {
    case 0:
      c |= packet->m_nChannel;
      break;
    case 1:
      break;
    case 2:
      c |= 1;
      break;
    }
  *hptr++ = c;
  if (cSize)
    {
      int tmp = packet->m_nChannel - 64;
      *hptr++ = tmp & 0xff;
      if (cSize == 2)
	*hptr++ = tmp >> 8;
    }

  if (nSize > 1)
    {
      hptr = qcAMF_EncodeInt24(hptr, hend, t > 0xffffff ? 0xffffff : t);
    }

  if (nSize > 4)
    {
      hptr = qcAMF_EncodeInt24(hptr, hend, packet->m_nBodySize);
      *hptr++ = packet->m_packetType;
    }

  if (nSize > 8)
    hptr += qcEncodeInt32LE(hptr, packet->m_nInfoField2);

  if (t >= 0xffffff)
    hptr = qcAMF_EncodeInt32(hptr, hend, t);

  nSize = packet->m_nBodySize;
  buffer = packet->m_body;
  nChunkSize = r->m_outChunkSize;

  qcRTMP_Log(qcRTMP_LOGDEBUG2, "%s: fd=%d, size=%d", __FUNCTION__, r->m_sb.sb_socket,
      nSize);
  /* send all chunks in one HTTP request */
  if (r->Link.protocol & qcRTMP_FEATURE_HTTP)
    {
      int chunks = (nSize+nChunkSize-1) / nChunkSize;
      if (chunks > 1)
        {
	  tlen = chunks * (cSize + 1) + nSize + hSize;
	  tbuf = malloc(tlen);
	  if (!tbuf)
	    return FALSE;
	  toff = tbuf;
	}
    }
  while (nSize + hSize)
    {
      int wrote;

      if (nSize < nChunkSize)
	nChunkSize = nSize;

      qcRTMP_LogHexString(qcRTMP_LOGDEBUG2, (uint8_t *)header, hSize);
      qcRTMP_LogHexString(qcRTMP_LOGDEBUG2, (uint8_t *)buffer, nChunkSize);
      if (tbuf)
        {
	  memcpy(toff, header, nChunkSize + hSize);
	  toff += nChunkSize + hSize;
	}
      else
        {
	  wrote = qcWriteN(r, header, nChunkSize + hSize);
	  if (!wrote)
	    return FALSE;
	}
      nSize -= nChunkSize;
      buffer += nChunkSize;
      hSize = 0;

      if (nSize > 0)
	{
	  header = buffer - 1;
	  hSize = 1;
	  if (cSize)
	    {
	      header -= cSize;
	      hSize += cSize;
	    }
          if (t >= 0xffffff)
            {
              header -= 4;
              hSize += 4;
            }
	  *header = (0xc0 | c);
	  if (cSize)
	    {
	      int tmp = packet->m_nChannel - 64;
	      header[1] = tmp & 0xff;
	      if (cSize == 2)
		header[2] = tmp >> 8;
	    }
          if (t >= 0xffffff)
            {
              char* extendedTimestamp = header + 1 + cSize;
              qcAMF_EncodeInt32(extendedTimestamp, extendedTimestamp + 4, t);
            }
	}
    }
  if (tbuf)
    {
      int wrote = qcWriteN(r, tbuf, toff-tbuf);
      free(tbuf);
      tbuf = NULL;
      if (!wrote)
        return FALSE;
    }

  /* we invoked a remote method */
  if (packet->m_packetType == qcRTMP_PACKET_TYPE_INVOKE)
    {
      AVal method;
      char *ptr;
      ptr = packet->m_body + 1;
      qcAMF_DecodeString(ptr, &method);
      qcRTMP_Log(qcRTMP_LOGDEBUG, "Invoking %s", method.av_val);
      /* keep it in call queue till result arrives */
      if (queue) {
        int txn;
        ptr += 3 + method.av_len;
        txn = (int)qcAMF_DecodeNumber(ptr);
	qcAV_queue(&r->m_methodCalls, &r->m_numCalls, &method, txn);
      }
    }

  if (!r->m_vecChannelsOut[packet->m_nChannel])
    r->m_vecChannelsOut[packet->m_nChannel] = malloc(sizeof(qcRTMPPacket));
  memcpy(r->m_vecChannelsOut[packet->m_nChannel], packet, sizeof(qcRTMPPacket));
  return TRUE;
}

int
qcRTMP_Serve(qcRTMP *r)
{
  return qcSHandShake(r);
}

void
qcRTMP_Close(qcRTMP *r)
{
  qcCloseInternal(r, 0);
}

static void
qcCloseInternal(qcRTMP *r, int reconnect)
{
  int i;

  if (qcRTMP_IsConnected(r))
    {
      if (r->m_stream_id > 0)
        {
	  i = r->m_stream_id;
	  r->m_stream_id = 0;
          if ((r->Link.protocol & qcRTMP_FEATURE_WRITE))
	    qcSendFCUnpublish(r);
	  qcSendDeleteStream(r, i);
	}
      if (r->m_clientID.av_val)
        {
	  qcHTTP_Post(r, qcRTMPT_CLOSE, "", 1);
	  free(r->m_clientID.av_val);
	  r->m_clientID.av_val = NULL;
	  r->m_clientID.av_len = 0;
	}
      qcRTMPSockBuf_Close(&r->m_sb);
    }

  r->m_stream_id = -1;
  r->m_sb.sb_socket = -1;
  r->m_nBWCheckCounter = 0;
  r->m_nBytesIn = 0;
  r->m_nBytesInSent = 0;

  if (r->m_read.flags & qcRTMP_READ_HEADER) {
    free(r->m_read.buf);
    r->m_read.buf = NULL;
  }
  r->m_read.dataType = 0;
  r->m_read.flags = 0;
  r->m_read.status = 0;
  r->m_read.nResumeTS = 0;
  r->m_read.nIgnoredFrameCounter = 0;
  r->m_read.nIgnoredFlvFrameCounter = 0;

  r->m_write.m_nBytesRead = 0;
  qcRTMPPacket_Free(&r->m_write);

  for (i = 0; i < r->m_channelsAllocatedIn; i++)
    {
      if (r->m_vecChannelsIn[i])
	{
	  qcRTMPPacket_Free(r->m_vecChannelsIn[i]);
	  free(r->m_vecChannelsIn[i]);
	  r->m_vecChannelsIn[i] = NULL;
	}
    }
  free(r->m_vecChannelsIn);
  r->m_vecChannelsIn = NULL;
  free(r->m_channelTimestamp);
  r->m_channelTimestamp = NULL;
  r->m_channelsAllocatedIn = 0;
  for (i = 0; i < r->m_channelsAllocatedOut; i++)
    {
      if (r->m_vecChannelsOut[i])
	{
	  free(r->m_vecChannelsOut[i]);
	  r->m_vecChannelsOut[i] = NULL;
	}
    }
  free(r->m_vecChannelsOut);
  r->m_vecChannelsOut = NULL;
  r->m_channelsAllocatedOut = 0;
  qcAV_clear(r->m_methodCalls, r->m_numCalls);
  r->m_methodCalls = NULL;
  r->m_numCalls = 0;
  r->m_numInvokes = 0;

  r->m_bPlaying = FALSE;
  r->m_sb.sb_size = 0;

  r->m_msgCounter = 0;
  r->m_resplen = 0;
  r->m_unackd = 0;
    
    if(r->Link.sockAddr)
    {
        free(r->Link.sockAddr);
        r->Link.sockAddr = NULL;
    }

  if (r->Link.lFlags & qcRTMP_LF_FTCU && !reconnect)
    {
      free(r->Link.tcUrl.av_val);
      r->Link.tcUrl.av_val = NULL;
      r->Link.lFlags ^= qcRTMP_LF_FTCU;
    }
  if (r->Link.lFlags & qcRTMP_LF_FAPU && !reconnect)
    {
      free(r->Link.app.av_val);
      r->Link.app.av_val = NULL;
      r->Link.lFlags ^= qcRTMP_LF_FAPU;
    }

  if (!reconnect)
    {
      free(r->Link.playpath0.av_val);
      r->Link.playpath0.av_val = NULL;
    }
#ifdef CRYPTO
  if (r->Link.dh)
    {
      MDH_free(r->Link.dh);
      r->Link.dh = NULL;
    }
  if (r->Link.rc4keyIn)
    {
      RC4_free(r->Link.rc4keyIn);
      r->Link.rc4keyIn = NULL;
    }
  if (r->Link.rc4keyOut)
    {
      RC4_free(r->Link.rc4keyOut);
      r->Link.rc4keyOut = NULL;
    }
#endif
}

int
qcRTMPSockBuf_Fill(qcRTMP* r, qcRTMPSockBuf *sb)
{
  int nBytes;
    int readTimeoutCount = 0;
  int useTime = qcRTMP_GetTime();

  if (!sb->sb_size)
    sb->sb_start = sb->sb_buf;

    while (1 && (r?(r->forceClose==0):1))
    {
      nBytes = sizeof(sb->sb_buf) - 1 - sb->sb_size - (sb->sb_start - sb->sb_buf);
#if defined(CRYPTO) && !defined(NO_SSL)
      if (sb->sb_ssl)
	{
	  nBytes = TLS_read(sb->sb_ssl, sb->sb_start + sb->sb_size, nBytes);
	}
      else
#endif
	{
	  nBytes = recv(sb->sb_socket, sb->sb_start + sb->sb_size, nBytes, 0);
	}
      if (nBytes != -1)
	{
        readTimeoutCount = 0;
	  sb->sb_size += nBytes;
	}
      else
	{
	  int sockerr = GetSockError();
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, recv returned %d. GetSockError(): %d (%s), forceclose %d",
	      __FUNCTION__, nBytes, sockerr, strerror(sockerr), r->forceClose);
	  if (sockerr == EINTR && !qcRTMP_ctrlC)
	    continue;
#ifdef __QC_OS_WIN32__
	  if (sockerr == WSAETIMEDOUT)
		  continue;
	  else if (sockerr == qcRTMP_ERR_NOTREADY)
	  {
		  msleep(100);
		  continue;
	  }
#elif __QC_OS_NDK__
	  if (sockerr == ETIMEDOUT)
		  continue;
#elif __QC_OS_IOS__
	  if (sockerr == ETIMEDOUT)
      {
          qcRTMP_Log(qcRTMP_LOGDEBUG, "Continue try to read...");
          continue;
      }
		  
#endif // __QC_OS_WIN32__

	  if (sockerr == EWOULDBLOCK || sockerr == EAGAIN)
	    {
            nBytes = 0;
            readTimeoutCount++;
            //if( (qcRTMP_GetTime()-useTime) >= r->Link.timeout)
            if(readTimeoutCount > 50)
            {
                qcRTMP_Log(qcRTMP_LOGERROR, "%s, Read fail. try time %d", __FUNCTION__, qcRTMP_GetTime()-useTime);
                
                // EAGAIN, we think network connection disconnect, so exit recv loop
                sb->sb_timedout = TRUE;
                break;
            }
            
		  continue;
	    }
	}
      break;
    }

  return nBytes;
}

int
qcRTMPSockBuf_Send(qcRTMPSockBuf *sb, const char *buf, int len)
{
  int rc;

#if defined(CRYPTO) && !defined(NO_SSL)
  if (sb->sb_ssl)
    {
      rc = TLS_write(sb->sb_ssl, buf, len);
    }
  else
#endif
    {
      rc = send(sb->sb_socket, buf, len, 0);
    }
  return rc;
}

int
qcRTMPSockBuf_Close(qcRTMPSockBuf *sb)
{
#if defined(CRYPTO) && !defined(NO_SSL)
  if (sb->sb_ssl)
    {
      TLS_shutdown(sb->sb_ssl);
      TLS_close(sb->sb_ssl);
      sb->sb_ssl = NULL;
    }
#endif
  if (sb->sb_socket != -1)
      return closesocket(sb->sb_socket);
  return 0;
}

#define HEX2BIN(a)	(((a)&0x40)?((a)&0xf)+9:((a)&0xf))

static void
qcDecodeTEA(AVal *key, AVal *text)
{
  uint32_t *v, k[4] = { 0 }, u;
  uint32_t z, y, sum = 0, e, DELTA = 0x9e3779b9;
  int32_t p, q;
  int i, n;
  unsigned char *ptr, *out;

  /* prep key: pack 1st 16 chars into 4 LittleEndian ints */
  ptr = (unsigned char *)key->av_val;
  u = 0;
  n = 0;
  v = k;
  p = key->av_len > 16 ? 16 : key->av_len;
  for (i = 0; i < p; i++)
    {
      u |= ptr[i] << (n * 8);
      if (n == 3)
	{
	  *v++ = u;
	  u = 0;
	  n = 0;
	}
      else
	{
	  n++;
	}
    }
  /* any trailing chars */
  if (u)
    *v = u;

  /* prep text: hex2bin, multiples of 4 */
  n = (text->av_len + 7) / 8;
  out = malloc(n * 8);
  ptr = (unsigned char *)text->av_val;
  v = (uint32_t *) out;
  for (i = 0; i < n; i++)
    {
      u = (HEX2BIN(ptr[0]) << 4) + HEX2BIN(ptr[1]);
      u |= ((HEX2BIN(ptr[2]) << 4) + HEX2BIN(ptr[3])) << 8;
      u |= ((HEX2BIN(ptr[4]) << 4) + HEX2BIN(ptr[5])) << 16;
      u |= ((HEX2BIN(ptr[6]) << 4) + HEX2BIN(ptr[7])) << 24;
      *v++ = u;
      ptr += 8;
    }
  v = (uint32_t *) out;

  /* http://www.movable-type.co.uk/scripts/tea-block.html */
#define MX (((z>>5)^(y<<2)) + ((y>>3)^(z<<4))) ^ ((sum^y) + (k[(p&3)^e]^z));
  z = v[n - 1];
  y = v[0];
  q = 6 + 52 / n;
  sum = q * DELTA;
  while (sum != 0)
    {
      e = sum >> 2 & 3;
      for (p = n - 1; p > 0; p--)
	z = v[p - 1], y = v[p] -= MX;
      z = v[n - 1];
      y = v[0] -= MX;
      sum -= DELTA;
    }

  text->av_len /= 2;
  memcpy(text->av_val, out, text->av_len);
  free(out);
}

static int
qcHTTP_Post(qcRTMP *r, qcRTMPTCmd cmd, const char *buf, int len)
{
  char hbuf[512];
  int hlen = snprintf(hbuf, sizeof(hbuf), "POST /%s%s/%d HTTP/1.1\r\n"
    "Host: %.*s:%d\r\n"
    "Accept: */*\r\n"
    "User-Agent: Shockwave Flash\r\n"
    "Connection: Keep-Alive\r\n"
    "Cache-Control: no-cache\r\n"
    "Content-type: application/x-fcs\r\n"
    "Content-length: %d\r\n\r\n", qcRTMPT_cmds[cmd],
    r->m_clientID.av_val ? r->m_clientID.av_val : "",
    r->m_msgCounter, r->Link.hostname.av_len, r->Link.hostname.av_val,
    r->Link.port, len);
  qcRTMPSockBuf_Send(&r->m_sb, hbuf, hlen);
  hlen = qcRTMPSockBuf_Send(&r->m_sb, buf, len);
  r->m_msgCounter++;
  r->m_unackd++;
  return hlen;
}

static int
qcHTTP_read(qcRTMP *r, int fill)
{
  char *ptr;
  int hlen;

restart:
  if (fill)
    qcRTMPSockBuf_Fill(r, &r->m_sb);
  if (r->m_sb.sb_size < 13) {
    if (fill && r->forceClose==0)
      goto restart;
    return -2;
  }
  if (strncmp(r->m_sb.sb_start, "HTTP/1.1 200 ", 13))
    return -1;
  r->m_sb.sb_start[r->m_sb.sb_size] = '\0';
  if (!strstr(r->m_sb.sb_start, "\r\n\r\n")) {
    if (fill && r->forceClose==0)
      goto restart;
    return -2;
  }

  ptr = r->m_sb.sb_start + sizeof("HTTP/1.1 200");
  while ((ptr = strstr(ptr, "Content-"))) {
    if (!strncasecmp(ptr+8, "length:", 7)) break;
    ptr += 8;
  }
  if (!ptr)
    return -1;
  hlen = atoi(ptr+16);
  ptr = strstr(ptr+16, "\r\n\r\n");
  if (!ptr)
    return -1;
  ptr += 4;
  if (ptr + (r->m_clientID.av_val ? 1 : hlen) > r->m_sb.sb_start + r->m_sb.sb_size)
    {
      if (fill)
        goto restart;
      return -2;
    }
  r->m_sb.sb_size -= ptr - r->m_sb.sb_start;
  r->m_sb.sb_start = ptr;
  r->m_unackd--;

  if (!r->m_clientID.av_val)
    {
      r->m_clientID.av_len = hlen;
      r->m_clientID.av_val = malloc(hlen+1);
      if (!r->m_clientID.av_val)
        return -1;
      r->m_clientID.av_val[0] = '/';
      memcpy(r->m_clientID.av_val+1, ptr, hlen-1);
      r->m_clientID.av_val[hlen] = 0;
      r->m_sb.sb_size = 0;
    }
  else
    {
      r->m_polling = *ptr++;
      r->m_resplen = hlen - 1;
      r->m_sb.sb_start++;
      r->m_sb.sb_size--;
    }
  return 0;
}

#define MAX_IGNORED_FRAMES	50

/* Read from the stream until we get a media packet.
 * Returns -3 if Play.Close/Stop, -2 if fatal error, -1 if no more media
 * packets, 0 if ignorable error, >0 if there is a media packet
 */
static int
qcRead_1_Packet(qcRTMP *r, char *buf, unsigned int buflen)
{
  uint32_t prevTagSize = 0;
  int rtnGetNextMediaPacket = 0, ret = qcRTMP_READ_EOF;
  qcRTMPPacket packet = { 0 };
  int recopy = FALSE;
  unsigned int size;
  char *ptr, *pend;
  uint32_t nTimeStamp = 0;
  unsigned int len;

  rtnGetNextMediaPacket = qcRTMP_GetNextMediaPacket(r, &packet);
  while (rtnGetNextMediaPacket)
    {
      char *packetBody = packet.m_body;
      unsigned int nPacketLen = packet.m_nBodySize;

      /* Return qcRTMP_READ_COMPLETE if this was completed nicely with
       * invoke message Play.Stop or Play.Complete
       */
      if (rtnGetNextMediaPacket == 2)
	{
	  qcRTMP_Log(qcRTMP_LOGDEBUG,
	      "Got Play.Complete or Play.Stop from server. "
	      "Assuming stream is complete");
	  ret = qcRTMP_READ_COMPLETE;
	  break;
	}

      r->m_read.dataType |= (((packet.m_packetType == qcRTMP_PACKET_TYPE_AUDIO) << 2) |
			     (packet.m_packetType == qcRTMP_PACKET_TYPE_VIDEO));

      if (packet.m_packetType == qcRTMP_PACKET_TYPE_VIDEO && nPacketLen <= 5)
	{
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "ignoring too small video packet: size: %d",
	      nPacketLen);
	  ret = qcRTMP_READ_IGNORE;
	  break;
	}
      if (packet.m_packetType == qcRTMP_PACKET_TYPE_AUDIO && nPacketLen <= 1)
	{
	  qcRTMP_Log(qcRTMP_LOGDEBUG, "ignoring too small audio packet: size: %d",
	      nPacketLen);
	  ret = qcRTMP_READ_IGNORE;
	  break;
	}

      if (r->m_read.flags & qcRTMP_READ_SEEKING)
	{
	  ret = qcRTMP_READ_IGNORE;
	  break;
	}
#ifdef _DEBUG
      qcRTMP_Log(qcRTMP_LOGDEBUG, "type: %02X, size: %d, TS: %d ms, abs TS: %d",
	  packet.m_packetType, nPacketLen, packet.m_nTimeStamp,
	  packet.m_hasAbsTimestamp);
      if (packet.m_packetType == qcRTMP_PACKET_TYPE_VIDEO)
	qcRTMP_Log(qcRTMP_LOGDEBUG, "frametype: %02X", (*packetBody & 0xf0));
#endif

      if (r->m_read.flags & qcRTMP_READ_RESUME)
	{
	  /* check the header if we get one */
	  if (packet.m_nTimeStamp == 0)
	    {
	      if (r->m_read.nMetaHeaderSize > 0
		  && packet.m_packetType == qcRTMP_PACKET_TYPE_INFO)
		{
		  qcAMFObject metaObj;
		  int nRes =
		    qcAMF_Decode(&metaObj, packetBody, nPacketLen, FALSE);
		  if (nRes >= 0)
		    {
		      AVal metastring;
		      qcAMFProp_GetString(qcAMF_GetProp(&metaObj, NULL, 0),
					&metastring);

		      if (AVMATCH(&metastring, &av_onMetaData))
			{
			  /* compare */
			  if ((r->m_read.nMetaHeaderSize != nPacketLen) ||
			      (memcmp
			       (r->m_read.metaHeader, packetBody,
				r->m_read.nMetaHeaderSize) != 0))
			    {
			      ret = qcRTMP_READ_ERROR;
			    }
			}
		      qcAMF_Reset(&metaObj);
		      if (ret == qcRTMP_READ_ERROR)
			break;
		    }
		}

	      /* check first keyframe to make sure we got the right position
	       * in the stream! (the first non ignored frame)
	       */
	      if (r->m_read.nInitialFrameSize > 0)
		{
		  /* video or audio data */
		  if (packet.m_packetType == r->m_read.initialFrameType
		      && r->m_read.nInitialFrameSize == nPacketLen)
		    {
		      /* we don't compare the sizes since the packet can
		       * contain several FLV packets, just make sure the
		       * first frame is our keyframe (which we are going
		       * to rewrite)
		       */
		      if (memcmp
			  (r->m_read.initialFrame, packetBody,
			   r->m_read.nInitialFrameSize) == 0)
			{
			  qcRTMP_Log(qcRTMP_LOGDEBUG, "Checked keyframe successfully!");
			  r->m_read.flags |= qcRTMP_READ_GOTKF;
			  /* ignore it! (what about audio data after it? it is
			   * handled by ignoring all 0ms frames, see below)
			   */
			  ret = qcRTMP_READ_IGNORE;
			  break;
			}
		    }

		  /* hande FLV streams, even though the server resends the
		   * keyframe as an extra video packet it is also included
		   * in the first FLV stream chunk and we have to compare
		   * it and filter it out !!
		   */
		  if (packet.m_packetType == qcRTMP_PACKET_TYPE_FLASH_VIDEO)
		    {
		      /* basically we have to find the keyframe with the
		       * correct TS being nResumeTS
		       */
		      unsigned int pos = 0;
		      uint32_t ts = 0;

		      while (pos + 11 < nPacketLen)
			{
			  /* size without header (11) and prevTagSize (4) */
			  uint32_t dataSize =
			    qcAMF_DecodeInt24(packetBody + pos + 1);
			  ts = qcAMF_DecodeInt24(packetBody + pos + 4);
			  ts |= (packetBody[pos + 7] << 24);

#ifdef _DEBUG
			  qcRTMP_Log(qcRTMP_LOGDEBUG,
			      "keyframe search: FLV Packet: type %02X, dataSize: %d, timeStamp: %d ms",
			      packetBody[pos], dataSize, ts);
#endif
			  /* ok, is it a keyframe?:
			   * well doesn't work for audio!
			   */
			  if (packetBody[pos /*6928, test 0 */ ] ==
			      r->m_read.initialFrameType
			      /* && (packetBody[11]&0xf0) == 0x10 */ )
			    {
			      if (ts == r->m_read.nResumeTS)
				{
				  qcRTMP_Log(qcRTMP_LOGDEBUG,
				      "Found keyframe with resume-keyframe timestamp!");
				  if (r->m_read.nInitialFrameSize != dataSize
				      || memcmp(r->m_read.initialFrame,
						packetBody + pos + 11,
						r->m_read.
						nInitialFrameSize) != 0)
				    {
				      qcRTMP_Log(qcRTMP_LOGERROR,
					  "FLV Stream: Keyframe doesn't match!");
				      ret = qcRTMP_READ_ERROR;
				      break;
				    }
				  r->m_read.flags |= qcRTMP_READ_GOTFLVK;

				  /* skip this packet?
				   * check whether skippable:
				   */
				  if (pos + 11 + dataSize + 4 > nPacketLen)
				    {
				      qcRTMP_Log(qcRTMP_LOGWARNING,
					  "Non skipable packet since it doesn't end with chunk, stream corrupt!");
				      ret = qcRTMP_READ_ERROR;
				      break;
				    }
				  packetBody += (pos + 11 + dataSize + 4);
				  nPacketLen -= (pos + 11 + dataSize + 4);

				  goto stopKeyframeSearch;

				}
			      else if (r->m_read.nResumeTS < ts)
				{
				  /* the timestamp ts will only increase with
				   * further packets, wait for seek
				   */
				  goto stopKeyframeSearch;
				}
			    }
			  pos += (11 + dataSize + 4);
			}
		      if (ts < r->m_read.nResumeTS)
			{
			  qcRTMP_Log(qcRTMP_LOGERROR,
			      "First packet does not contain keyframe, all "
			      "timestamps are smaller than the keyframe "
			      "timestamp; probably the resume seek failed?");
			}
		    stopKeyframeSearch:
		      ;
		      if (!(r->m_read.flags & qcRTMP_READ_GOTFLVK))
			{
			  qcRTMP_Log(qcRTMP_LOGERROR,
			      "Couldn't find the seeked keyframe in this chunk!");
			  ret = qcRTMP_READ_IGNORE;
			  break;
			}
		    }
		}
	    }

	  if (packet.m_nTimeStamp > 0
	      && (r->m_read.flags & (qcRTMP_READ_GOTKF|qcRTMP_READ_GOTFLVK)))
	    {
	      /* another problem is that the server can actually change from
	       * 09/08 video/audio packets to an FLV stream or vice versa and
	       * our keyframe check will prevent us from going along with the
	       * new stream if we resumed.
	       *
	       * in this case set the 'found keyframe' variables to true.
	       * We assume that if we found one keyframe somewhere and were
	       * already beyond TS > 0 we have written data to the output
	       * which means we can accept all forthcoming data including the
	       * change between 08/09 <-> FLV packets
	       */
	      r->m_read.flags |= (qcRTMP_READ_GOTKF|qcRTMP_READ_GOTFLVK);
	    }

	  /* skip till we find our keyframe
	   * (seeking might put us somewhere before it)
	   */
	  if (!(r->m_read.flags & qcRTMP_READ_GOTKF) &&
	  	packet.m_packetType != qcRTMP_PACKET_TYPE_FLASH_VIDEO)
	    {
	      qcRTMP_Log(qcRTMP_LOGWARNING,
		  "Stream does not start with requested frame, ignoring data... ");
	      r->m_read.nIgnoredFrameCounter++;
	      if (r->m_read.nIgnoredFrameCounter > MAX_IGNORED_FRAMES)
		ret = qcRTMP_READ_ERROR;	/* fatal error, couldn't continue stream */
	      else
		ret = qcRTMP_READ_IGNORE;
	      break;
	    }
	  /* ok, do the same for FLV streams */
	  if (!(r->m_read.flags & qcRTMP_READ_GOTFLVK) &&
	  	packet.m_packetType == qcRTMP_PACKET_TYPE_FLASH_VIDEO)
	    {
	      qcRTMP_Log(qcRTMP_LOGWARNING,
		  "Stream does not start with requested FLV frame, ignoring data... ");
	      r->m_read.nIgnoredFlvFrameCounter++;
	      if (r->m_read.nIgnoredFlvFrameCounter > MAX_IGNORED_FRAMES)
		ret = qcRTMP_READ_ERROR;
	      else
		ret = qcRTMP_READ_IGNORE;
	      break;
	    }

	  /* we have to ignore the 0ms frames since these are the first
	   * keyframes; we've got these so don't mess around with multiple
	   * copies sent by the server to us! (if the keyframe is found at a
	   * later position there is only one copy and it will be ignored by
	   * the preceding if clause)
	   */
	  if (!(r->m_read.flags & qcRTMP_READ_NO_IGNORE) &&
	  	packet.m_packetType != qcRTMP_PACKET_TYPE_FLASH_VIDEO)
	    {
              /* exclude type qcRTMP_PACKET_TYPE_FLASH_VIDEO since it can
               * contain several FLV packets
               */
	      if (packet.m_nTimeStamp == 0)
		{
		  ret = qcRTMP_READ_IGNORE;
		  break;
		}
	      else
		{
		  /* stop ignoring packets */
		  r->m_read.flags |= qcRTMP_READ_NO_IGNORE;
		}
	    }
	}

      /* calculate packet size and allocate slop buffer if necessary */
      size = nPacketLen +
	((packet.m_packetType == qcRTMP_PACKET_TYPE_AUDIO
          || packet.m_packetType == qcRTMP_PACKET_TYPE_VIDEO
	  || packet.m_packetType == qcRTMP_PACKET_TYPE_INFO) ? 11 : 0) +
	(packet.m_packetType != qcRTMP_PACKET_TYPE_FLASH_VIDEO ? 4 : 0);

      if (size + 4 > buflen)
	{
	  /* the extra 4 is for the case of an FLV stream without a last
	   * prevTagSize (we need extra 4 bytes to append it) */
	  r->m_read.buf = malloc(size + 4);
	  if (r->m_read.buf == 0)
	    {
	      qcRTMP_Log(qcRTMP_LOGERROR, "Couldn't allocate memory!");
	      ret = qcRTMP_READ_ERROR;		/* fatal error */
	      break;
	    }
	  recopy = TRUE;
	  ptr = r->m_read.buf;
	}
      else
	{
	  ptr = buf;
	}
      pend = ptr + size + 4;

      /* use to return timestamp of last processed packet */

      /* audio (0x08), video (0x09) or metadata (0x12) packets :
       * construct 11 byte header then add rtmp packet's data */
      if (packet.m_packetType == qcRTMP_PACKET_TYPE_AUDIO
          || packet.m_packetType == qcRTMP_PACKET_TYPE_VIDEO
	  || packet.m_packetType == qcRTMP_PACKET_TYPE_INFO)
	{
	  nTimeStamp = r->m_read.nResumeTS + packet.m_nTimeStamp;
	  prevTagSize = 11 + nPacketLen;

	  *ptr = packet.m_packetType;
	  ptr++;
	  ptr = qcAMF_EncodeInt24(ptr, pend, nPacketLen);

#if 0
	    if(packet.m_packetType == qcRTMP_PACKET_TYPE_VIDEO) {

	     /* H264 fix: */
	     if((packetBody[0] & 0x0f) == 7) { /* CodecId = H264 */
	     uint8_t packetType = *(packetBody+1);

	     uint32_t ts = qcAMF_DecodeInt24(packetBody+2); /* composition time */
	     int32_t cts = (ts+0xff800000)^0xff800000;
	     qcRTMP_Log(qcRTMP_LOGDEBUG, "cts  : %d\n", cts);

	     nTimeStamp -= cts;
	     /* get rid of the composition time */
	     CRTMP::EncodeInt24(packetBody+2, 0);
	     }
	     qcRTMP_Log(qcRTMP_LOGDEBUG, "VIDEO: nTimeStamp: 0x%08X (%d)\n", nTimeStamp, nTimeStamp);
	     }
#endif

	  ptr = qcAMF_EncodeInt24(ptr, pend, nTimeStamp);
	  *ptr = (char)((nTimeStamp & 0xFF000000) >> 24);
	  ptr++;

	  /* stream id */
	  ptr = qcAMF_EncodeInt24(ptr, pend, 0);
	}

      memcpy(ptr, packetBody, nPacketLen);
      len = nPacketLen;

      /* correct tagSize and obtain timestamp if we have an FLV stream */
      if (packet.m_packetType == qcRTMP_PACKET_TYPE_FLASH_VIDEO)
	{
	  unsigned int pos = 0;
	  int delta;

	  /* grab first timestamp and see if it needs fixing */
	  nTimeStamp = qcAMF_DecodeInt24(packetBody + 4);
	  nTimeStamp |= (packetBody[7] << 24);
	  delta = packet.m_nTimeStamp - nTimeStamp + r->m_read.nResumeTS;

	  while (pos + 11 < nPacketLen)
	    {
	      /* size without header (11) and without prevTagSize (4) */
	      uint32_t dataSize = qcAMF_DecodeInt24(packetBody + pos + 1);
	      nTimeStamp = qcAMF_DecodeInt24(packetBody + pos + 4);
	      nTimeStamp |= (packetBody[pos + 7] << 24);

	      if (delta)
		{
		  nTimeStamp += delta;
		  qcAMF_EncodeInt24(ptr+pos+4, pend, nTimeStamp);
		  ptr[pos+7] = nTimeStamp>>24;
		}

	      /* set data type */
	      r->m_read.dataType |= (((*(packetBody + pos) == 0x08) << 2) |
				     (*(packetBody + pos) == 0x09));

	      if (pos + 11 + dataSize + 4 > nPacketLen)
		{
		  if (pos + 11 + dataSize > nPacketLen)
		    {
		      qcRTMP_Log(qcRTMP_LOGERROR,
			  "Wrong data size (%u), stream corrupted, aborting!",
			  dataSize);
		      ret = qcRTMP_READ_ERROR;
		      break;
		    }
		  qcRTMP_Log(qcRTMP_LOGWARNING, "No tagSize found, appending!");

		  /* we have to append a last tagSize! */
		  prevTagSize = dataSize + 11;
		  qcAMF_EncodeInt32(ptr + pos + 11 + dataSize, pend,
				  prevTagSize);
		  size += 4;
		  len += 4;
		}
	      else
		{
		  prevTagSize =
		    qcAMF_DecodeInt32(packetBody + pos + 11 + dataSize);

#ifdef _DEBUG
		  qcRTMP_Log(qcRTMP_LOGDEBUG,
		      "FLV Packet: type %02X, dataSize: %lu, tagSize: %lu, timeStamp: %lu ms",
		      (unsigned char)packetBody[pos], dataSize, prevTagSize,
		      nTimeStamp);
#endif

		  if (prevTagSize != (dataSize + 11))
		    {
#ifdef _DEBUG
		      qcRTMP_Log(qcRTMP_LOGWARNING,
			  "Tag and data size are not consitent, writing tag size according to dataSize+11: %d",
			  dataSize + 11);
#endif

		      prevTagSize = dataSize + 11;
		      qcAMF_EncodeInt32(ptr + pos + 11 + dataSize, pend,
				      prevTagSize);
		    }
		}

	      pos += prevTagSize + 4;	/*(11+dataSize+4); */
	    }
	}
      ptr += len;

      if (packet.m_packetType != qcRTMP_PACKET_TYPE_FLASH_VIDEO)
	{
	  /* FLV tag packets contain their own prevTagSize */
	  qcAMF_EncodeInt32(ptr, pend, prevTagSize);
	}

      /* In non-live this nTimeStamp can contain an absolute TS.
       * Update ext timestamp with this absolute offset in non-live mode
       * otherwise report the relative one
       */
      /* qcRTMP_Log(qcRTMP_LOGDEBUG, "type: %02X, size: %d, pktTS: %dms, TS: %dms, bLiveStream: %d", packet.m_packetType, nPacketLen, packet.m_nTimeStamp, nTimeStamp, r->Link.lFlags & qcRTMP_LF_LIVE); */
      r->m_read.timestamp = (r->Link.lFlags & qcRTMP_LF_LIVE) ? packet.m_nTimeStamp : nTimeStamp;
      r->m_read.nCurrPacketType = packet.m_packetType;

      ret = size;
      break;
    }

  if (rtnGetNextMediaPacket)
    qcRTMPPacket_Free(&packet);

  if (recopy)
    {
      len = ret > buflen ? buflen : ret;
      memcpy(buf, r->m_read.buf, len);
      r->m_read.bufpos = r->m_read.buf + len;
      r->m_read.buflen = ret - len;
    }
  return ret;
}

static const char qc_flvHeader[] = { 'F', 'L', 'V', 0x01,
  0x00,				/* 0x04 == audio, 0x01 == video */
  0x00, 0x00, 0x00, 0x09,
  0x00, 0x00, 0x00, 0x00
};

#define HEADERBUF	(128*1024)
int
qcRTMP_Read(qcRTMP *r, char *buf, int size)
{
  int nRead = 0, total = 0;

  /* can't continue */
fail:
  switch (r->m_read.status) {
  case qcRTMP_READ_EOF:
  case qcRTMP_READ_COMPLETE:
    return 0;
  case qcRTMP_READ_ERROR:  /* corrupted stream, resume failed */
    SetSockError(EINVAL);
    return -1;
  default:
    break;
  }

  /* first time thru */
  if (!(r->m_read.flags & qcRTMP_READ_HEADER))
    {
      if (!(r->m_read.flags & qcRTMP_READ_RESUME))
	{
	  char *mybuf = malloc(HEADERBUF), *end = mybuf + HEADERBUF;
	  int cnt = 0;
	  r->m_read.buf = mybuf;
	  r->m_read.buflen = HEADERBUF;

	  memcpy(mybuf, qc_flvHeader, sizeof(qc_flvHeader));
	  r->m_read.buf += sizeof(qc_flvHeader);
	  r->m_read.buflen -= sizeof(qc_flvHeader);
	  cnt += sizeof(qc_flvHeader);

	  while (r->m_read.timestamp == 0)
	    {
	      nRead = qcRead_1_Packet(r, r->m_read.buf, r->m_read.buflen);
	      if (nRead < 0)
		{
		  free(mybuf);
		  r->m_read.buf = NULL;
		  r->m_read.buflen = 0;
		  r->m_read.status = nRead;
		  goto fail;
		}
	      /* buffer overflow, fix buffer and give up */
	      if (r->m_read.buf < mybuf || r->m_read.buf > end) {
	      	mybuf = realloc(mybuf, cnt + nRead);
		memcpy(mybuf+cnt, r->m_read.buf, nRead);
		free(r->m_read.buf);
		r->m_read.buf = mybuf+cnt+nRead;
	        break;
	      }
	      cnt += nRead;
	      r->m_read.buf += nRead;
	      r->m_read.buflen -= nRead;
	      if (r->m_read.dataType == 5)
	        break;
	    }
	  mybuf[4] = r->m_read.dataType;
	  r->m_read.buflen = r->m_read.buf - mybuf;
	  r->m_read.buf = mybuf;
	  r->m_read.bufpos = mybuf;
	}
      r->m_read.flags |= qcRTMP_READ_HEADER;
    }

  if ((r->m_read.flags & qcRTMP_READ_SEEKING) && r->m_read.buf)
    {
      /* drop whatever's here */
      free(r->m_read.buf);
      r->m_read.buf = NULL;
      r->m_read.bufpos = NULL;
      r->m_read.buflen = 0;
    }

  /* If there's leftover data buffered, use it up */
  if (r->m_read.buf)
    {
      nRead = r->m_read.buflen;
      if (nRead > size)
	nRead = size;
      memcpy(buf, r->m_read.bufpos, nRead);
      r->m_read.buflen -= nRead;
      if (!r->m_read.buflen)
	{
	  free(r->m_read.buf);
	  r->m_read.buf = NULL;
	  r->m_read.bufpos = NULL;
	}
      else
	{
	  r->m_read.bufpos += nRead;
	}
      buf += nRead;
      total += nRead;
      size -= nRead;
    }

  while (size > 0 && (nRead = qcRead_1_Packet(r, buf, size)) >= 0)
    {
      if (!nRead) continue;
      buf += nRead;
      total += nRead;
      size -= nRead;
      break;
    }
  if (nRead < 0)
    r->m_read.status = nRead;

  if (size < 0)
    total += size;
  return total;
}

static const AVal qc_av_setDataFrame = AVC("@setDataFrame");

int
qcRTMP_Write(qcRTMP *r, const char *buf, int size)
{
  qcRTMPPacket *pkt = &r->m_write;
  char *pend, *enc;
  int s2 = size, ret, num;

  pkt->m_nChannel = 0x04;	/* source channel */
  pkt->m_nInfoField2 = r->m_stream_id;

  while (s2)
    {
      if (!pkt->m_nBytesRead)
	{
	  if (size < 11) {
	    /* FLV pkt too small */
	    return 0;
	  }

	  if (buf[0] == 'F' && buf[1] == 'L' && buf[2] == 'V')
	    {
	      buf += 13;
	      s2 -= 13;
	    }

	  pkt->m_packetType = *buf++;
	  pkt->m_nBodySize = qcAMF_DecodeInt24(buf);
	  buf += 3;
	  pkt->m_nTimeStamp = qcAMF_DecodeInt24(buf);
	  buf += 3;
	  pkt->m_nTimeStamp |= *buf++ << 24;
	  buf += 3;
	  s2 -= 11;

	  if (((pkt->m_packetType == qcRTMP_PACKET_TYPE_AUDIO
                || pkt->m_packetType == qcRTMP_PACKET_TYPE_VIDEO) &&
            !pkt->m_nTimeStamp) || pkt->m_packetType == qcRTMP_PACKET_TYPE_INFO)
	    {
	      pkt->m_headerType = qcRTMP_PACKET_SIZE_LARGE;
	      if (pkt->m_packetType == qcRTMP_PACKET_TYPE_INFO)
		pkt->m_nBodySize += 16;
	    }
	  else
	    {
	      pkt->m_headerType = qcRTMP_PACKET_SIZE_MEDIUM;
	    }

	  if (!qcRTMPPacket_Alloc(pkt, pkt->m_nBodySize))
	    {
	      qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, failed to allocate packet", __FUNCTION__);
	      return FALSE;
	    }
	  enc = pkt->m_body;
	  pend = enc + pkt->m_nBodySize;
	  if (pkt->m_packetType == qcRTMP_PACKET_TYPE_INFO)
	    {
	      enc = qcAMF_EncodeString(enc, pend, &qc_av_setDataFrame);
	      pkt->m_nBytesRead = enc - pkt->m_body;
	    }
	}
      else
	{
	  enc = pkt->m_body + pkt->m_nBytesRead;
	}
      num = pkt->m_nBodySize - pkt->m_nBytesRead;
      if (num > s2)
	num = s2;
      memcpy(enc, buf, num);
      pkt->m_nBytesRead += num;
      s2 -= num;
      buf += num;
      if (pkt->m_nBytesRead == pkt->m_nBodySize)
	{
	  ret = qcRTMP_SendPacket(r, pkt, FALSE);
	  qcRTMPPacket_Free(pkt);
	  pkt->m_nBytesRead = 0;
	  if (!ret)
	    return -1;
	  buf += 4;
	  s2 -= 4;
	  if (s2 < 0)
	    break;
	}
    }
  return size+s2;
}

void qcRTMP_SetSocketBlock(qcRTMP *r)
{
#ifdef __QC_OS_WIN32__
    u_long non_blk = 0;
    ioctlsocket(r->m_sb.sb_socket, FIONBIO, &non_blk);
#else
    int flags = fcntl(r->m_sb.sb_socket, F_GETFL, 0);
    flags &= (~O_NONBLOCK);
    fcntl(r->m_sb.sb_socket, F_SETFL, flags);
#endif
}

void qcRTMP_SetSocketNonBlock(qcRTMP *r)
{
#ifdef __QC_OS_WIN32__
    u_long non_blk = 1;
    ioctlsocket(r->m_sb.sb_socket, FIONBIO, &non_blk);
#else
    int flags = fcntl(r->m_sb.sb_socket, F_GETFL, 0);
    fcntl(r->m_sb.sb_socket, F_SETFL, flags | O_NONBLOCK);
#endif
}

int qcRTMP_WaitSocketWriteBuffer(qcRTMP* r)
{
    fd_set        	fds;
    //struct timeval 	tmWait = { 1, 0 };
    struct timeval     tmWait = { 0, 100000 };
    int sec = r->Link.timeoutConnect / 1000;
    int milli = (r->Link.timeoutConnect % 1000) * 1000;
    
    if(sec == 0 && milli < tmWait.tv_usec)
        tmWait.tv_usec = milli;

    int            	ret = 0;
    int             tryCount = 0;
    int            	nStartTime = qcRTMP_GetTime();
    while (ret == 0)
    {
        if (qcRTMP_GetTime() - nStartTime > r->Link.timeoutConnect)
            break;
        if (r->forceClose)
        {
            qcRTMP_Log(qcRTMP_LOGDEBUG, "%s, force to disconnect, time %d, try count %d", __FUNCTION__, qcRTMP_GetTime() - nStartTime, tryCount);
            return -1;
        }
        
        FD_ZERO(&fds);
        FD_SET(r->m_sb.sb_socket, &fds);
        tryCount++;
        ret = select(r->m_sb.sb_socket + 1, NULL, &fds, NULL, &tmWait);
    }
    int err = 0;
    int errLength = sizeof(err);
    
    if (ret > 0 && FD_ISSET(r->m_sb.sb_socket, &fds))
    {
        getsockopt(r->m_sb.sb_socket, SOL_SOCKET, SO_ERROR, (char *)&err, (socklen_t*)&errLength);
        if (err != 0)
        {
            ret = -1;
        }
    }
    
    // 0 timeout, 1 success, -1 connect fail
    return ret;
}
