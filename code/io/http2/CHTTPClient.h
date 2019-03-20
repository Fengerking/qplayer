/*******************************************************************************
	File:		CHTTPClient.h

	Contains:	http client header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-06		Bangfei			Create file

*******************************************************************************/
#ifndef __CHTTPClient_H__
#define __CHTTPClient_H__
#ifndef __QC_OS_WIN32__
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "pthread.h"
#endif //__QC_OS_WIN32__

#include "CBaseObject.h"
#include "CDNSCache.h"
#include "COpenSSL.h"
#include "CDNSLookUp.h"
#include "CFileIO.h"

#include "USocketFunc.h"

#ifdef __QC_NAMESPACE__
using namespace __QC_NAMESPACE__;
#endif

#define MAXDOMAINNAME		256
#define QCPOST_DATA			-1
#define QCPUT_DATA			-2

static const int KInvalidSocketHandler = -1;
static const int KInvalidContentLength = -1;

static const int KMaxHostAddrLen		= 256;
static const int KMaxHostFileNameLen	= 2048;
static const int KMaxRequestLen			= 2048;
static const int KMaxLineLength			= 4096;

typedef struct _DNSParam{
    char			domainName[MAXDOMAINNAME];
    unsigned int	ip;
    int				errorcode;
    _DNSParam(){
        memset(domainName, 0, MAXDOMAINNAME);
        errorcode = -1;
        ip = 0;
    }
} DNSParam;

typedef struct sockaddr* QCIPAddr; //For IPv4 and IPv6
typedef int (CHTTPClient::*_pFunConnect)(const char* aUrl, long long aOffset, int nTimeOut);

class CHTTPClient : public CBaseObject
{
public:
	CHTTPClient(CBaseInst * pBaseInst, CDNSCache * pDNSCache);
    virtual ~CHTTPClient(void);

	virtual int				Read(char* aDstBuffer, int aSize);
	virtual int				Recv(char* aDstBuffer, int aSize);
	virtual int				Send(const char* aSendBuffer, int aSize);
	// if aoffset is -1 to post data, -2 to put data.
	virtual int				Connect(const char* aUrl, long long aOffset, int nTimeOut);
	virtual int				ConnectViaProxy(const char* aUrl, long long aOffset, int nTimeOut);

	virtual long long		ContentLength(void);
	virtual void			Interrupt(void);
	virtual unsigned int	HostIP(void);

 	virtual int				Disconnect(void);

	virtual int				HttpStatus(void);
	virtual unsigned int	StatusCode(void);
	virtual void			SetStatusCode(unsigned int aCode);

    virtual bool			IsCancel(void);
    virtual void            SetSocketCheckForNetException(void);
	virtual bool			IsTtransferBlock(){return m_bTransferBlock;}
	virtual bool			IsTtransferChunk(){return m_bTransforChunk;}
	virtual bool			IsStreaming () {return m_bIsStreaming||m_bTransforChunk||(m_llContentLength==QCIO_MAX_CONTENT_LEN);}
	virtual int				RequireContentLength();
	virtual int				ConvertToValue(char * aBuffer);
	virtual char*			GetRedirectUrl(void);
	virtual char *			GetContentType(void) { return m_szContentType; }

	virtual void			SetNotifyMsg(bool bNotifyMsg) { m_bNotifyMsg = bNotifyMsg; }
	virtual int				ConnectServer(const QCIPAddr aHostIP, int& nPortNum, int nTimeOut = 10000);
	virtual long long		GetReadPos(void) { return m_llConnectOffset + m_llHadReadSize; }
	virtual bool			IsConnected(void) { return m_sState == CONNECTED; }

protected:
	virtual int 	ConnectServerIPV4Proxy(unsigned int nHostIP, int& nPortNum, int nTimeout = 10000);
	virtual int		ResolveDNS(char* aHostAddr, QCIPAddr aHostIP);
    
	virtual int		SendRequestAndParseResponse(_pFunConnect pFunConnect, const char* aUrl, int aPort, long long aOffset, int nTimeOut);
	virtual int		SendRequest(int aPort, long long aOffset);
	virtual int		SendPostPut(int aPort, long long aOffset);

	virtual int		ParseResponseHeader(unsigned int& aStatusCode);
	virtual bool	IsRedirectStatusCode(unsigned int aStatusCode);
	virtual int		Redirect(_pFunConnect pFunConnect, long long aOffset);
	virtual int		ParseHeader(unsigned int& aStatusCode);

	virtual int		ParseContentLength(unsigned int aStatusCode);
	virtual int		GetHeaderValueByKey(const char* aKey, char* aBuffer, int aBufferLen);
	virtual int		ReceiveLine(char* aLine, int aSize);

	virtual int		Receive(int& aSocketHandle, timeval& aTimeOut, char* aDstBuffer, int aSize);
	virtual int		ReadChunkBuff(int nSocketHandle, char * pDestBuff, int nDestSize);
	virtual int		GetChunkSize(char * pBuff, int nSize, int & nHeadSize);

	virtual int		WaitSocketWriteBuffer(int& aSocketHandle, timeval& aTimeOut);
	virtual int		WaitSocketReadBuffer(int& aSocketHandle, timeval& aTimeOut);
	virtual int		SetSocketTimeOut(int& aSocketHandle, timeval aTimeOut);

	virtual void	SetSocketBlock(int& aSocketHandle);
	virtual void	SetSocketNonBlock(int& aSocketHandle);

	// For http ssl 
	virtual int		SocketRead (int nSocket, char * pBuff, int nSize, int nFlag);
	virtual int		SocketSend (int nSocket, const char * pBuff, int nSize, int nFlag);
	virtual int		SoeketConnect (int nSocket, int nTimeOut);
	virtual int		SocketClose (int nSocket);

	virtual int		RespDataRead(int nSocket, char * pBuff, int nSize, int nFlag);

	virtual void	ResetParam (void);

protected:
	enum State {
	        DISCONNECTED,
	        CONNECTING,
	        CONNECTED
	    };

	char			m_szURL[4096];
	State			m_sState;
	bool			m_bNotifyMsg;
	bool			m_bIsSSL;
	COpenSSL *		m_pOpenSSL;
	int				m_nSocketHandle;
	long long		m_llContentLength;
	char			m_szContentType[256];
	int				m_nWSAStartup;

	char			m_szLineBuffer[KMaxLineLength];
	char			m_szHeaderValueBuffer[KMaxLineLength];

	char			m_szHostAddr[KMaxHostAddrLen];
	char			m_szDomainAddr[KMaxHostAddrLen];
	char			m_szHostFileName[KMaxHostFileNameLen];
	char			m_szRequset[KMaxRequestLen];
	char*			m_pHostMetaData;

	CDNSCache*		m_pDNSCache;
	CDNSLookup *	m_pDNSLookup;
	QCIPAddr		m_sHostIP;
	unsigned int	m_nStatusCode;
    bool			m_bCancel;
    unsigned int    m_nHostIP;

	char *			m_pRespBuff;
	char *			m_pRespPos;
	char *			m_pRespData;
	int				m_nRespSize;
	int				m_nRespLen;
	int				m_nRespRead;

	bool			m_bMediaType;
	bool			m_bTransferBlock;
	bool			m_bTransforChunk;
	bool			m_bIsStreaming;
	char			m_szRedirectUrl[KMaxLineLength];

	bool			m_bReadChunkHead;
	char *			m_pChunkHeadBuff;
	int				m_nChunkHeadSize;
	int				m_nChunkSize;
	int				m_nChunkRead;
	bool			m_bEOS;
	long long		m_llChunkLength;

    bool			m_bFirstByte;

	long long		m_llConnectOffset;
	long long		m_llHadReadSize;
    
    int				m_nWaitTimeoutCount;
    
    long long       m_llSendLength;

#ifdef __QC_OS_WIN32__
	CQCInetNtop *	m_pInetNtop;
#endif // __QC_OS_WIN32__

	CFileIO *		m_pDumpFile;
};

#endif // __CHTTPClient_H__
