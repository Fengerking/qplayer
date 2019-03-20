/*******************************************************************************
	File:		CHTTPClient.cpp

	Contains:	http client implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-06		Bangfei			Create file

*******************************************************************************/
#ifdef __QC_OS_WIN32__
#include <winsock2.h>
#include "Ws2tcpip.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <signal.h> 
#include <ctype.h>
#include "GKMacrodef.h"
#include <exception>
#include <typeinfo>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include<signal.h>
#endif // __QC_OS_WIN32__

#include "qcErr.h"
#include "qcIO.h"
#include "CHTTPClient.h"
#include "CMsgMng.h"

#include "UUrlParser.h"
#include "USourceFormat.h"
#include "USystemFunc.h"
#include "ULogFunc.h"

#define  CONNECTION_TIMEOUT_IN_SECOND			30	//unit: second
#define  HTTP_HEADER_RECV_TIMEOUT_IN_SECOND		30 //unit: second
#define  HTTP_HEADER_RECV_MAXTIME				10000 //

#define  CONNECT_ERROR_BASE		600		//connect error base value
#define  REQUEST_ERROR_BASE		1000	//request error base value
#define  RESPONSE_ERROR_BASE	1300	//response error base value
#define  DNS_ERROR_BASE			2000	//dns resolve error base value

#define HTTPRESPONSE_INTERUPTER 1304
#define ERROR_CODE_TEST         902
#define INET_ADDR_EXCEPTION     16
#define DNS_TIME_OUT            17
#define DNS_UNKNOWN             18

#define TIME_KEEP_ALIVE  1  // ��̽��
#define TIME_KEEP_IDLE   10  // ��ʼ̽��ǰ�Ŀ��еȴ�ʱ��10s
#define TIME_KEEP_INTVL  2  // ����̽��ֽڵ�ʱ���� 2s
#define TIME_KEEP_CNT    3  // ����̽��ֽڵĴ��� 3 times

static const char KContentType[] = {'C', 'o', 'n', 't', 'e', 'n', 't', '-', 'T', 'y', 'p', 'e', '\0'};
static const char KContentRangeKey[] = {'C', 'o', 'n', 't', 'e', 'n', 't', '-', 'R', 'a', 'n', 'g', 'e', '\0'};
static const char KContentLengthKey[] = {'C', 'o', 'n', 't', 'e', 'n', 't', '-', 'L', 'e', 'n', 'g', 't', 'h', '\0'};
static const char KLocationKey[] = {'L', 'o', 'c', 'a', 't', 'i', 'o', 'n', '\0'};
static const char KTransferEncodingKey[] = {'T', 'r', 'a', 'n', 's', 'f', 'e', 'r', '-', 'E', 'n', 'c', 'o', 'd',  'i', 'n', 'g','\0'};

static const int STATUS_OK = 0;
static const int Status_Error_ServerConnectTimeOut = 905;
static const int Status_Error_HttpResponseTimeOut = 1556;
static const int Status_Error_HttpResponseBadDescriptor = 1557;
static const int Status_Error_HttpResponseArgumentError= 1558;
static const int Status_Error_NoUsefulSocket = 1559;

#define	_ETIMEDOUT	60

unsigned int	g_ProxyHostIP = 0;
QCIPAddr		g_ProxyHostIPV6 = NULL;
int				g_ProxyHostPort = 0;
char*			g_AutherKey = NULL;
char*			g_Domain = NULL;

#define ONESECOND			1000
#define WAIT_DNS_INTERNAL	50
#define WAIT_DNS_MAX_TIME	600

void HTTP_SignalHandle(int avalue)
{
  //  gCancle = true;
}

CHTTPClient::CHTTPClient(CBaseInst * pBaseInst, CDNSCache * pDNSCache)
	: CBaseObject (pBaseInst)
	, m_sState (DISCONNECTED)
	, m_bNotifyMsg (true)
	, m_bIsSSL (false)
	, m_pOpenSSL (NULL)
	, m_nSocketHandle (KInvalidSocketHandler)
	, m_llContentLength (0)
	, m_nWSAStartup (0)
	, m_pHostMetaData (NULL)
	, m_pDNSCache (pDNSCache)
	, m_pDNSLookup(NULL)
	, m_sHostIP (NULL)
	, m_nStatusCode (0)
	, m_nHostIP (0)
	, m_pRespBuff (NULL)
	, m_pChunkHeadBuff (NULL)
	, m_nChunkHeadSize (64)
	, m_bFirstByte(true)
	, m_nWaitTimeoutCount(0)
	, m_llConnectOffset(0)
	, m_llHadReadSize(0)
	, m_pDumpFile(NULL)
	, m_llSendLength(0)
{
	SetObjectName ("CHTTPClient");
#ifdef __QC_OS_WIN32__
	m_pInetNtop = NULL;
   	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD( 2, 2 );
	m_nWSAStartup = WSAStartup( wVersionRequested, &wsaData );
#else
    struct sigaction act, oldact;
    act.sa_handler = HTTP_SignalHandle;
    act.sa_flags = SA_NODEFER;
    //sigaddset(&act.sa_mask, SIGALRM);
    sigaction(SIGALRM, &act, &oldact);
	//signal(SIGPIPE, SIG_IGN);
#endif

	if (m_pDNSCache == NULL)
		m_pDNSCache = m_pBaseInst->m_pDNSCache;
	m_pDNSLookup = m_pBaseInst->m_pDNSLookup;// new CDNSLookup(m_pBaseInst);

	memset(m_szRedirectUrl,0,sizeof(m_szRedirectUrl));
	ResetParam ();
}

CHTTPClient::~CHTTPClient(void)
{
	//if (m_sState == CONNECTED)
		Disconnect();

	QC_DEL_P(m_pOpenSSL);

#ifdef __QC_OS_WIN32__
	QC_DEL_P(m_pInetNtop);
	WSACleanup();
#endif

    if (m_nHostIP != 0) 
	{
        struct sockaddr_storage* tmp = (struct sockaddr_storage*)m_nHostIP;
        //b need free?  free(tmp);
        m_nHostIP = 0;
    }

    QC_FREE_P(m_sHostIP);
	QC_DEL_A (m_pHostMetaData);
	QC_DEL_A (m_pChunkHeadBuff);
	QC_DEL_A (m_pRespBuff);

	QC_DEL_P (m_pDumpFile);
}

int CHTTPClient::Read(char* aDstBuffer, int aSize)
{
	if(m_sState == DISCONNECTED)
		return QC_ERR_Disconnected;

	struct timeval tTimeout = { 0, m_pBaseInst->m_pSetting->g_qcs_nTimeOutRead * 1000 };
	int nRC = Receive(m_nSocketHandle, tTimeout, aDstBuffer, aSize);
	if (nRC > 0)
		m_llHadReadSize += nRC;
	if (m_bEOS)
		return QC_ERR_HTTP_EOS;
	return nRC;
}

int CHTTPClient::Recv(char* aDstBuffer, int aSize)
{
	struct timeval tTimeout = { 0, m_pBaseInst->m_pSetting->g_qcs_nTimeOutRead * 1000 };//{HTTP_HEADER_RECV_TIMEOUT_IN_SECOND, 0};
    int nErr;
   // int retryCnt = HTTP_HEADER_RECV_MAXTIMES;// 6*5s = 30s, total wait time is 30s;
	long long nStartTime = qcGetSysTime();
	long long nOffset = 0;
	do{
        nErr = Receive(m_nSocketHandle, tTimeout, aDstBuffer, aSize);
        nOffset = qcGetSysTime() - nStartTime;
        if (nOffset > HTTP_HEADER_RECV_MAXTIME || m_bCancel) {
            break;
        }
		if (m_pBaseInst->m_bForceClose == true)
			break;
    }while (nErr == 0);
    return nErr;
}

int CHTTPClient::Send(const char* aSendBuffer, int aSize)
{
	if(m_sState == DISCONNECTED)
		return QC_ERR_Disconnected;

	int nSend = 0;
	int nTotalsend = 0;
	while(nTotalsend < aSize)
	{
		nSend = SocketSend (m_nSocketHandle , aSendBuffer + nTotalsend, aSize - nTotalsend , 0 );
		if(nSend < 0)
		{
			SetStatusCode(errno + REQUEST_ERROR_BASE);
			QCLOGE("send error!%s/n", strerror(errno));
		    return QC_ERR_CANNOT_CONNECT;
		}

        m_llSendLength += nSend;
		nTotalsend += nSend;
	}

	return QC_ERR_NONE;
}

int CHTTPClient::Connect(const char* aUrl, long long aOffset, int nTimeOut)
{
	int nErr = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nErr, m_pBaseInst, (int)aOffset);	
	if (m_bNotifyMsg)
	{
		//m_pDumpFile = new CFileIO(m_pBaseInst);
		if (m_pDumpFile != NULL)
			m_pDumpFile->Open("c:\\temp\\http.dat", 0, QCIO_FLAG_WRITE);
	}

	if(m_nWSAStartup)
		return QC_ERR_CANNOT_CONNECT;

	if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_CONNECT_START, 0, 0, aUrl);

	if (!strncmp(aUrl, "http", 4))
	{
		if (strstr(aUrl, "https://") == aUrl)
		{
			if (m_pOpenSSL == NULL)
			{
				m_pOpenSSL = new COpenSSL(m_pBaseInst, NULL);
				if (m_pOpenSSL->Init() != QC_ERR_NONE)
					return QC_ERR_FAILED;
			}
			m_bIsSSL = true;
		}
		else
			m_bIsSSL = false;
	}

	int nPort;
	strcpy(m_szURL, aUrl);

	qcUrlParseUrl(m_szURL, m_szHostAddr, m_szHostFileName, nPort, m_szDomainAddr);
	if (strlen(m_szDomainAddr) > 0)
	{
		QC_DEL_A(m_pHostMetaData);
		m_pHostMetaData = new char[strlen(m_szDomainAddr) + 32];
		sprintf(m_pHostMetaData, "Host:%s", m_szDomainAddr);
	}

	ResetParam ();

    if (m_sHostIP == NULL) {
        m_sHostIP = (QCIPAddr)malloc(sizeof(struct sockaddr_storage));
    }
    else{
        memset(m_sHostIP, 0, sizeof(struct sockaddr_storage));
    }
    m_nHostIP = 0;
  
	if (m_pBaseInst->m_bForceClose == true)
		return QC_ERR_STATUS;

	int nStartTime = qcGetSysTime();
	int nTryTimes = 0;
	while (nTryTimes < 3)
	{
		nStartTime = qcGetSysTime();
		nErr = ResolveDNS(m_szHostAddr, m_sHostIP);
		if (nErr == QC_ERR_NONE)
			break;
		if (qcGetSysTime() - nStartTime > 1000)
			break;
		QCLOGW("ResolveDNS failed. error = %d", nErr);
        qcSleepEx(200000, &m_pBaseInst->m_bForceClose);
		nTryTimes++;
		if (m_pBaseInst->m_bForceClose || m_pBaseInst->m_bCheckReopn)
			return QC_ERR_STATUS;
	}
	if (nErr != QC_ERR_NONE)
		return nErr;
	int nDNSUsedTime = qcGetSysTime() - nStartTime;
	QCLOGI("Parse DNS used time = %d", nDNSUsedTime);

	if (m_pBaseInst->m_bForceClose == true)
		return QC_ERR_STATUS;

	if (nTimeOut < 0)
		nTimeOut = m_pBaseInst->m_pSetting->g_qcs_nTimeOutConnect;
	nErr = ConnectServer(m_sHostIP, nPort, nTimeOut);
	if (nErr != QC_ERR_NONE)
	{
		m_pDNSCache->Del(m_szHostAddr, m_sHostIP, sizeof(struct sockaddr));
		return nErr;
	}
    
#ifdef __QC_OS_IOS__
    int set = 1;
    setsockopt(m_nSocketHandle, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif

	if (m_bIsSSL)
		nPort = 80;

	nErr = SendRequestAndParseResponse(&CHTTPClient::Connect, aUrl, nPort, aOffset, nTimeOut);
	if (m_llContentLength < QCIO_MAX_CONTENT_LEN)
		m_bIsStreaming = false;

	m_bReadChunkHead = false;
	m_llConnectOffset = aOffset;
	m_llHadReadSize = 0;

	return nErr;
}

int CHTTPClient::ConnectViaProxy(const char* aUrl, long long aOffset, int nTimeOut)
{
	if(m_nWSAStartup)
		return QC_ERR_CANNOT_CONNECT;
 
	if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_CONNECT_START, 0, 0, aUrl);

	if (strstr (aUrl, "https://") == aUrl)
		m_bIsSSL = true;
	else
		m_bIsSSL = false;

    char	tLine[3] = {0};
    int		nPort;
    int		nErr;

	ResetParam ();

    if (g_Domain != NULL) {
        //memset(&g_ProxyHostIP, 0, sizeof(QCIPAddr));
        if (g_ProxyHostIPV6 == NULL) {
            g_ProxyHostIPV6 = (QCIPAddr)malloc(sizeof(struct sockaddr_storage));
        }
        else{
            memset(g_ProxyHostIPV6, 0, sizeof(struct sockaddr_storage));
        }
        
        int nErr = ResolveDNS(g_Domain, g_ProxyHostIPV6);
        if( nErr != QC_ERR_NONE)
        {
            return nErr;
        }
		if (nTimeOut < 0)
			nTimeOut = m_pBaseInst->m_pSetting->g_qcs_nTimeOutConnect;
		nErr = ConnectServer(g_ProxyHostIPV6, g_ProxyHostPort, nTimeOut);
    }
	else
	{
		if (nTimeOut < 0)
			nTimeOut = m_pBaseInst->m_pSetting->g_qcs_nTimeOutConnect;
		nErr = ConnectServerIPV4Proxy(g_ProxyHostIP, g_ProxyHostPort, nTimeOut);
	}
    
    if( nErr != QC_ERR_NONE)
		return nErr;
    
	qcUrlParseUrl(aUrl, m_szHostAddr, m_szHostFileName, nPort, m_szDomainAddr);
    
	m_nStatusCode = STATUS_OK;
    char strRequest[2048] = {0};
    
    // sprintf(strRequest, "CONNECT %s:%d HTTP/1.1\r\nHost: %s:%d\r\nProxy-Connection: Keep-Alive\r\nContent-Length: 0\r\nProxy-Authorization: Basic MzAwMDAwNDU1MDpCREFBQUQ5QjczOUQzQjNG\r\n\r\n",g_pHostAddr, nPort, g_pHostAddr, nPort);MzAwMDAwNDU1MDpCREFBQUQ5QjczOUQzQjNG
    sprintf(strRequest, "CONNECT %s:%d HTTP/1.1\r\nProxy-Authorization: Basic %s\r\n\r\n",m_szHostAddr, nPort, g_AutherKey);
    
    //send proxyserver connect request
	nErr = Send(strRequest, strlen(strRequest));
    if (nErr != QC_ERR_NONE)
        return nErr;
    
    unsigned int nStatusCode;
    //wait for proxyserver connect response
    //response: HTTP/1.1 200 Connection established\r\n
    nErr = ParseResponseHeader(nStatusCode);
	if (nStatusCode != 200)
        return nErr;
    
    //read \r\n
    Recv(tLine, 2);
    
    nErr = SendRequestAndParseResponse(&CHTTPClient::ConnectViaProxy, aUrl, nPort, aOffset, nTimeOut);
	m_bReadChunkHead = false;
	return nErr;
}

long long CHTTPClient::ContentLength(void)
{
	if (m_llContentLength != QCIO_MAX_CONTENT_LEN)
		return m_llContentLength;
	else
	{
		if (m_llChunkLength > 0)
			return m_llChunkLength;
		else
			return m_llContentLength;
	}
}

void  CHTTPClient::Interrupt()
{
	m_bCancel = true;
}

unsigned int CHTTPClient::HostIP()
{
    return  m_nHostIP;
}

void  CHTTPClient::SetSocketCheckForNetException(void)
{
#ifdef __QC_OS_IOS__
    int keepalive = TIME_KEEP_ALIVE;
    int keepidle = TIME_KEEP_IDLE;
    int keepintvl = TIME_KEEP_INTVL;
    int keepcnt = TIME_KEEP_CNT;
    if(m_nSocketHandle != KInvalidSocketHandler){
        setsockopt(m_nSocketHandle, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof (keepalive));
        setsockopt(m_nSocketHandle, IPPROTO_TCP, TCP_KEEPALIVE, (void *) &keepidle, sizeof (keepidle));
        setsockopt(m_nSocketHandle, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepintvl, sizeof (keepintvl));
        setsockopt(m_nSocketHandle, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepcnt, sizeof (keepcnt));
    }
#endif
}

int CHTTPClient::RequireContentLength()
{
	int nErr = QC_ERR_ARG;

	if(!m_bTransferBlock)
		return nErr;
	while (true)
	{
		nErr = ReceiveLine(m_szLineBuffer, sizeof(char) * KMaxLineLength);
		if (nErr != QC_ERR_NONE)
		{
			QCLOGE("CHTTPClient RecHeader Error:%d", nErr);
			break;
		}
		if (m_szLineBuffer[0] == '\0')
			continue;
		
		int a= ConvertToValue(m_szLineBuffer);
		return a;
	}
	return nErr;
}

int CHTTPClient::ConvertToValue(char * aBuffer)
{
	int size = strlen(aBuffer);
	 int i=0;
	 int value = 0;
	 while(i<size)
	 {
		 if(aBuffer[i] >= '0' && aBuffer[i] <= '9')
		 {
			 value = value* 16 +(aBuffer[i]-'0');
		 }
		 else  if(aBuffer[i] >= 'a' && aBuffer[i] <= 'f')
		 {
			 value = value* 16 +(aBuffer[i]-'a' + 10);
		 }
		 else  if(aBuffer[i] >= 'A' && aBuffer[i] <= 'F')
		 {
			 value = value* 16 +(aBuffer[i]-'A' + 10);
		 }
		 else
			 return -1;

		 i++;
	  }
	 return value;
}

char* CHTTPClient::GetRedirectUrl(void)
{
	if(strlen(m_szRedirectUrl) == 0)
		return NULL;
	else
		return m_szRedirectUrl;
}

int CHTTPClient::Disconnect()
{
	if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DISCONNECT_START, 0, 0);

	if ((m_sState == CONNECTED || m_sState == CONNECTING) && (m_nSocketHandle != KInvalidSocketHandler))
	{
		// for http ssl
		SocketClose (m_nSocketHandle);

		m_nSocketHandle = KInvalidSocketHandler;
		m_sState = DISCONNECTED;
	}
	m_bTransferBlock = false;
	m_bTransforChunk = false;
	m_bMediaType = false;
	m_bIsStreaming = false;
	memset(m_szRedirectUrl,0,sizeof(m_szRedirectUrl));
	m_bCancel = false;

	m_llConnectOffset = 0;
	m_llHadReadSize = 0;

	if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DISCONNECT_DONE, 0, 0);

	return 0;
}

bool CHTTPClient::IsCancel()
{
    return m_bCancel;
}

int CHTTPClient::HttpStatus(void)
{
	return (int)m_sState;
}

unsigned int CHTTPClient::StatusCode(void)
{
	return m_nStatusCode;
}

void CHTTPClient::SetStatusCode(unsigned int aCode)
{
    //just for test, will delete later!
    if (aCode == ERROR_CODE_TEST || aCode == ERROR_CODE_TEST + 2) 
        aCode = aCode<<1;
	m_nStatusCode = aCode;
}

int CHTTPClient::ConnectServer(const QCIPAddr aHostIP, int& nPortNum, int nTimeOut)
{
	int nErr = 0;
	QCLOG_CHECK_FUNC(&nErr, m_pBaseInst, nTimeOut);

	if((m_nSocketHandle = socket(aHostIP->sa_family, SOCK_STREAM, 0)) == KInvalidSocketHandler)//
	{
		QCLOGE("socket return error, %d(%s)", errno, strerror(errno));
		m_nStatusCode = Status_Error_NoUsefulSocket;
		return QC_ERR_CANNOT_CONNECT;
	}
	m_sState = CONNECTING;

	SetSocketNonBlock(m_nSocketHandle); //aIP = ((struct sockaddr_in*)(res->ai_addr))->sin_addr;
    
    if (aHostIP->sa_family==AF_INET6) {
        ((struct sockaddr_in6*)aHostIP)->sin6_port = htons(nPortNum);
    } else {
        ((struct sockaddr_in*)aHostIP)->sin_port   = htons(nPortNum);
    }
#ifdef __QC_OS_IOS__
	nErr = connect(m_nSocketHandle, aHostIP, aHostIP->sa_len);
#else
    nErr = connect(m_nSocketHandle, aHostIP, sizeof(struct sockaddr));
#endif
    
	if (nErr < 0)
	{
		m_nStatusCode = errno + CONNECT_ERROR_BASE;
#ifdef __QC_OS_WIN32__
		if(nErr == -1)
#else
		if (errno == EINPROGRESS)
#endif
		{
			timeval timeout = { nTimeOut / 1000, (nTimeOut % 1000) * 1000 };
			nErr = WaitSocketWriteBuffer(m_nSocketHandle, timeout);
		}

		if (nErr < 0)
		{
			if (nErr == QC_ERR_TIMEOUT)
			{
				m_nStatusCode = Status_Error_ServerConnectTimeOut;
			}

			QCLOGE("connect error. nErr: %d, errorno: %d(%s)", nErr, errno, strerror(errno));
			Disconnect();
			SetSocketBlock(m_nSocketHandle);
			return QC_ERR_CANNOT_CONNECT;
		}
	}

	// for http ssl
	if (SoeketConnect (m_nSocketHandle, nTimeOut) != QC_ERR_NONE)
		return QC_ERR_FAILED;

    SetSocketBlock(m_nSocketHandle);
	m_sState = CONNECTED;

	if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_CONNECT_SUCESS, 0, 0);

	return QC_ERR_NONE;
}

int CHTTPClient::ConnectServerIPV4Proxy(unsigned int nHostIP, int& nPortNum, int nTimeOut)
{
    if((m_nSocketHandle = socket(AF_INET, SOCK_STREAM, 0)) == KInvalidSocketHandler)
    {
        QCLOGE("socket return error");
        m_nStatusCode = Status_Error_NoUsefulSocket;
        return QC_ERR_CANNOT_CONNECT;
    }
    m_sState = CONNECTING;
    
    SetSocketNonBlock(m_nSocketHandle);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(nPortNum);
    server_addr.sin_addr = *(struct in_addr *)(&nHostIP);
    int nErr = connect(m_nSocketHandle, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr));
    if (nErr < 0)
    {
        m_nStatusCode = errno + CONNECT_ERROR_BASE;
#ifdef __QC_OS_WIN32__
        if(nErr == -1)
#else
            if (errno == EINPROGRESS)
#endif
            {
				timeval timeout = { nTimeOut / 1000, (nTimeOut % 1000) * 1000 };
                nErr = WaitSocketWriteBuffer(m_nSocketHandle, timeout);
            }
        
        if (nErr < 0)
        {
            if (nErr == QC_ERR_TIMEOUT)
            {
                m_nStatusCode = Status_Error_ServerConnectTimeOut;
            }
            
            QCLOGE("connect error. nErr: %d, errorno: %d", nErr, errno);
            Disconnect();
            SetSocketBlock(m_nSocketHandle);
            return QC_ERR_CANNOT_CONNECT;
        }
    }

	// for http ssl
	if (SoeketConnect (m_nSocketHandle, nTimeOut) != QC_ERR_NONE)
		return QC_ERR_FAILED;
    
    SetSocketBlock(m_nSocketHandle);
    
    m_sState = CONNECTED;
    
    return QC_ERR_NONE;
}

int CHTTPClient::ResolveDNS(char* aHostAddr, QCIPAddr aHostIP)
{
    char 		pHostIP[INET6_ADDRSTRLEN];
    void*		numericAddress = NULL;
    QCIPAddr *	cachedHostIp = NULL;
    int			loopWaitCnt = 0;
    bool		parseRet;
	int			nRC = 0;
  
    int time = qcGetSysTime();
	if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DNS_START, 0, 0, aHostAddr);

	nRC = m_pDNSCache->Get(aHostAddr, aHostIP);
	if (nRC == QC_ERR_NONE)
    {
        numericAddress = (void*)&(((struct sockaddr_in*)aHostIP)->sin_addr);
#ifdef __QC_OS_WIN32__
		if (m_pInetNtop == NULL)
			m_pInetNtop = new CQCInetNtop();
		m_pInetNtop->qcInetNtop(aHostIP->sa_family, numericAddress, pHostIP, INET6_ADDRSTRLEN);
#else
        inet_ntop(aHostIP->sa_family, numericAddress, pHostIP, INET6_ADDRSTRLEN);
#endif // __QC_OS_WIN32__
		if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DNS_GET_CACHE, 0, 0, pHostIP);
        if (m_bNotifyMsg)
            QCLOGI("The connect IP is %s", pHostIP);
        return QC_ERR_NONE;
    }
	if (m_bNotifyMsg && (m_bCancel || m_pBaseInst->m_bForceClose))
        return QC_ERR_CANNOT_CONNECT;

    parseRet = false;
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    int ret;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 & IPv6 */
    hints.ai_socktype = SOCK_STREAM;

	if (!qcIsIPv6() && strcmp(m_pBaseInst->m_pSetting->g_qcs_szDNSServerName, "127.0.0.1"))
	{
		ret = m_pDNSLookup->GetDNSAddrInfo(aHostAddr, NULL, &hints, (void **)&res);
		if (ret == QC_ERR_NONE && res != NULL)
		{
			memcpy(aHostIP, res->ai_addr, res->ai_addrlen);
			m_pDNSLookup->FreeDNSAddrInfo(res);
			parseRet = true;
		}
	}
	else
	{
		ret = getaddrinfo(aHostAddr, NULL, &hints, &res);
		if (ret == 0 && res != NULL)
		{
			memcpy(aHostIP, res->ai_addr, res->ai_addrlen);
			freeaddrinfo(res);
			parseRet = true;
		}
	}
        
    if (parseRet) 
	{
        if (aHostIP->sa_family == AF_INET6) 
		{
            m_nHostIP = 0xffffffff;
            //numericAddress = (void*)&(((struct sockaddr_in6*)&aHostIP)->sin6_addr);
        } 
		else 
		{
            numericAddress = (void*)&(((struct sockaddr_in*)aHostIP)->sin_addr);
#ifdef __QC_OS_WIN32__
			if (m_pInetNtop == NULL)
				m_pInetNtop = new CQCInetNtop();
			if (m_pInetNtop->qcInetNtop(aHostIP->sa_family, numericAddress, pHostIP, INET6_ADDRSTRLEN) != NULL)
#else
            if (inet_ntop(aHostIP->sa_family, numericAddress, pHostIP, INET6_ADDRSTRLEN) != NULL)
#endif // __QC_OS_WIN32__
			{
                m_nHostIP = inet_addr(pHostIP);
                if (strcmp(pHostIP, aHostAddr) != 0)
                {
					if (m_pDNSCache != NULL)
						m_pDNSCache->Add(aHostAddr, (void *)aHostIP, sizeof(struct sockaddr), 999999);
                }
            }
            if (m_bNotifyMsg)
                QCLOGI ("The connect IP is %s", pHostIP);
        }
		if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DNS_GET_IPADDR, 0, 0, pHostIP);
        return QC_ERR_NONE;
    }    
	else
	{
		m_nStatusCode = DNS_ERROR_BASE + ret;
		QCLOGE("getaddrinfo return err: %d", ret);
	}
    //parse fail
    return QC_ERR_CANNOT_CONNECT;
}

int CHTTPClient::SendRequestAndParseResponse(_pFunConnect pFunConnect, const char* aUrl, int aPort, long long aOffset, int nIimeOut)
{
	//send get file request
    int nErr = QC_ERR_NONE;
	if (aOffset >= 0)
		nErr = SendRequest (aPort, aOffset);
	else
		nErr = SendPostPut (aPort, aOffset);
    if (nErr == QC_ERR_NONE)
    {
		unsigned int nStatusCode = STATUS_OK;
        nErr = ParseResponseHeader(nStatusCode);
        if (nErr == QC_ERR_NONE)
        {
            if (IsRedirectStatusCode(nStatusCode))
            {
                return Redirect(pFunConnect, aOffset);
            }
            else if (nStatusCode == 200 || nStatusCode == 206)
            {
                nErr = ParseContentLength(nStatusCode);
            }
            else
			{
				m_nStatusCode = nStatusCode;
				nErr = QC_ERR_CANNOT_CONNECT;
			}
		}
        
        if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL && m_llSendLength > 0)
        {
            m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_SEND_BYTE, (int)m_llSendLength, 0);
            m_llSendLength = 0;
        }
	}
    
    if ((nErr != QC_ERR_NONE) && (m_sState == CONNECTED))
	{
        nErr = QC_ERR_CANNOT_CONNECT;
		QCLOGE("It can't get response data. Connection is going to be closed");
		Disconnect();
	}
	
	struct timeval nRecvDataTimeOut = {0, 500000};
	SetSocketTimeOut(m_nSocketHandle, nRecvDataTimeOut);

	return nErr;
}

int CHTTPClient::SendRequest(int aPort, long long aOffset)
{
	char	szItemText[4096];
	memset(m_szRequset, 0, sizeof(m_szRequset));
	sprintf(m_szRequset, "GET /%s HTTP/1.1\r\n", m_szHostFileName);
	memset(szItemText, 0, sizeof(szItemText));
	if (m_pHostMetaData != NULL)
	{
		if (strstr(m_pHostMetaData, "Host:") == NULL)
			sprintf(szItemText, "%sHost: %s", m_pHostMetaData, m_szHostAddr);
		else
			sprintf(szItemText, "%s", m_pHostMetaData);
	}
	else
	{
		sprintf(szItemText, "Host: %s", m_szHostAddr);
	}
	if (aPort != 80)
		sprintf(szItemText, "%s:%d", szItemText, aPort);
	strcat(szItemText, "\r\n");
	strcat(m_szRequset, szItemText);

	if (aOffset > 0)
	{
		memset(szItemText, 0, sizeof(szItemText));
		sprintf(szItemText, "Range: bytes=%lld-\r\n", aOffset);
		strcat(m_szRequset, szItemText);
	}
	if (strlen(m_pBaseInst->m_pSetting->g_qcs_szHttpHeadReferer) > 0)
	{
		strcat(m_szRequset, m_pBaseInst->m_pSetting->g_qcs_szHttpHeadReferer);
		strcat(m_szRequset, "\r\n");
	}
    if (strlen(m_pBaseInst->m_pSetting->g_qcs_szHttpHeadUserAgent) > 0)
    {
        QCLOGI("user-agent -> %s", m_pBaseInst->m_pSetting->g_qcs_szHttpHeadUserAgent);
        strcat(m_szRequset, m_pBaseInst->m_pSetting->g_qcs_szHttpHeadUserAgent);
        strcat(m_szRequset, "\r\n");
    }
    else
    	strcat(m_szRequset, "User-Agent: QPlayer Engine\r\n");
	strcat(m_szRequset, "Connection: keep - alive\r\n\r\n");
//	strcat(m_szRequset, "Connection: close\r\n\r\n");
	return Send(m_szRequset, strlen(m_szRequset));
}

int CHTTPClient::SendPostPut (int aPort, long long aOffset)
{
	return QC_ERR_FAILED;
}

int CHTTPClient::ParseResponseHeader(unsigned int& aStatusCode)
{
	int		nSize = 1024 * 32;
	int		nRest = nSize;
	if (m_pRespBuff == NULL)
		m_pRespBuff = new char[nSize];
	memset (m_pRespBuff, 0, nSize);
	m_pRespPos = m_pRespBuff;
	m_nRespSize = 0;
	m_nRespLen = 0;
	m_nRespRead = 0;
	m_pRespData = NULL;
	m_nRespSize = 0;
	char *	pBuff = m_pRespBuff;
	int		nStartTime = qcGetSysTime();
	int		nRead = Recv(pBuff, nRest);
	while (true)
	{
		if (nRead > 0)
		{
			m_nRespSize += nRead;
			pBuff += nRead;
			nRest -= nRead;
		}
		m_pRespData = strstr (m_pRespBuff, "\r\n\r\n");
		if (m_pRespData != NULL)
		{
			m_pRespData += 4;
			m_nRespLen = m_pRespData - m_pRespBuff;
			m_nRespRead = 0;
			if (m_nRespLen == m_nRespSize)
				m_pRespData = NULL;
			break;
		}
		nRead = Recv(pBuff, nRest);
		if (nRead < 0)
			qcSleep(10000);
		if (m_pBaseInst->m_bForceClose)
			return QC_ERR_STATUS;
		if (qcGetSysTime() - nStartTime > m_pBaseInst->m_pSetting->g_qcs_nTimeOutConnect)
			return QC_ERR_TIMEOUT;
	}

	int nErr = ParseHeader(aStatusCode);
	if(nErr == QC_ERR_BadDescriptor)
	{
		m_nStatusCode = Status_Error_HttpResponseBadDescriptor;
		QCLOGW("ParseResponseHeader return %d, %u", nErr, aStatusCode);
	}
	return nErr;
}

int CHTTPClient::ParseHeader(unsigned int& aStatusCode)
{
	char tLine[KMaxLineLength];

	int nErr = ReceiveLine(tLine, sizeof(tLine));
	if (nErr != QC_ERR_NONE)
	{
		QCLOGE("Receive Response Error!");
		return nErr;
	}

	char* pSpaceStart = strchr(tLine, ' ');
	if (pSpaceStart == NULL) 
	{
		QCLOGE("Receive Response content Error!");
		return QC_ERR_BadDescriptor;
	}

	char* pResponseStatusStart = pSpaceStart + 1;
	char* pResponseStatusEnd = pResponseStatusStart;
	while (isdigit(*pResponseStatusEnd)) 
	{
		++pResponseStatusEnd;
	}

	if (pResponseStatusStart == pResponseStatusEnd) 
	{
		return QC_ERR_BadDescriptor;
	}

	memmove(tLine, pResponseStatusStart, pResponseStatusEnd - pResponseStatusStart);
	tLine[pResponseStatusEnd - pResponseStatusStart] = '\0';

	int nResponseNum = strtol(tLine, NULL, 10);
	if ((nResponseNum < 0) || (nResponseNum > 999))
	{
		QCLOGE("Receive Invalid ResponseNum!");
		if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_RETURN_CODE, nResponseNum, 0);
		return QC_ERR_BadDescriptor;
	}
	else if (nResponseNum > 400)
	{
		if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_RETURN_CODE, nResponseNum, 0);
	}

	aStatusCode = nResponseNum;
	if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_GET_HEADDATA, 0, 0, tLine);

	return QC_ERR_NONE;
}

bool CHTTPClient::IsRedirectStatusCode(unsigned int aStatusCode)
{
	return aStatusCode == 301 || aStatusCode == 302
		|| aStatusCode == 303 || aStatusCode == 307;
}

int CHTTPClient::Redirect(_pFunConnect pFunConnect, long long aOffset)
{
	int nErr = GetHeaderValueByKey(KLocationKey, m_szHeaderValueBuffer, sizeof(char)*KMaxLineLength);
	Disconnect();
	if (QC_ERR_NONE == nErr)
	{
		if (strncmp(m_szHeaderValueBuffer, "http", 4))
		{
			if (m_bIsSSL)
				strcpy(m_szRedirectUrl, "https://");
			else
				strcpy(m_szRedirectUrl, "http://");
			strcat(m_szRedirectUrl, m_szHostAddr);
			strcat(m_szRedirectUrl, m_szHeaderValueBuffer);
		}
		else
		{
			memcpy(m_szRedirectUrl, m_szHeaderValueBuffer, sizeof(m_szRedirectUrl));
		}
		if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_REDIRECT, 0, 0, m_szRedirectUrl);
		return (this->*pFunConnect)(m_szRedirectUrl, aOffset, -1);
	}
	else
	{
		nErr = QC_ERR_CANNOT_CONNECT;
	}
	return nErr;
}

int CHTTPClient::ParseContentLength(unsigned int aStatusCode)
{
	const char* pKey = (aStatusCode == 206) ? KContentRangeKey : KContentLengthKey;
	memset(m_szHeaderValueBuffer, 0, KMaxLineLength);
	int nErr = GetHeaderValueByKey(pKey, m_szHeaderValueBuffer, sizeof(char)*KMaxLineLength);
	if(m_bTransferBlock) 
		return QC_ERR_NONE;

	if(QC_ERR_FINISH == nErr && m_bMediaType) 
	{
		m_llContentLength = 0;
		return QC_ERR_NONE;
	}

	if (QC_ERR_NONE == nErr)
	{
		char *pStart = (aStatusCode == 206) ? strchr(m_szHeaderValueBuffer, '/') + 1 : m_szHeaderValueBuffer;
		char* pEnd = NULL;
		long long nContentLen = strtoll(pStart, &pEnd, 10);

		if ((pEnd == m_szHeaderValueBuffer) || (*pEnd != '\0'))
		{
			QCLOGE("CHTTPClient Get ContentLength Error!");
			m_nStatusCode = Status_Error_HttpResponseArgumentError;
			nErr = QC_ERR_ARG;
		}
		else
		{
			m_llContentLength = nContentLen;
			if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_CONTENT_LEN, 0, m_llContentLength);
		}
	}
	else
	{
		m_llContentLength = QCIO_MAX_CONTENT_LEN;
	}
	return QC_ERR_NONE;
}

int CHTTPClient::GetHeaderValueByKey(const char* aKey, char* aBuffer, int aBufferLen)
{
	int nErr = QC_ERR_ARG;
    bool bIsKeyFound = false;
	bool bIsKeyChange = false;

	if(0 == qcStrComp(aKey, KContentLengthKey, -1, false))
		bIsKeyChange = true;

	while (true)
	{
		nErr = ReceiveLine(m_szLineBuffer, sizeof(char) * KMaxLineLength);

		if (nErr != QC_ERR_NONE)
		{
			if (!m_bTransferBlock)
				QCLOGI("CHTTPClient RecHeader Error:%d", nErr);
			break;
		}

		if(m_bTransferBlock)
		{
			if (m_szLineBuffer[0] == '\0')
			{
				nErr = QC_ERR_NONE;
				break;
			}
			else
				continue;
		}

		if (m_szLineBuffer[0] == '\0')
		{
			nErr = bIsKeyFound ? QC_ERR_NONE : QC_ERR_FINISH;
			break;
		}

		char* pColonStart = strchr(m_szLineBuffer, ':');
		if (pColonStart == NULL) 
		{
			continue;			
		} 

		char* pEndofkey = pColonStart;

		while ((pEndofkey > m_szLineBuffer) && isspace(pEndofkey[-1])) 
		{
			--pEndofkey;
		}

		char* pStartofValue = pColonStart + 1;
		while (isspace(*pStartofValue)) 
		{
			++pStartofValue;
		}

		*pEndofkey = '\0';

		if (qcStrComp(m_szLineBuffer, aKey, strlen(aKey), false) != 0)
		{
			if(bIsKeyChange){
				if (qcStrComp(m_szLineBuffer, KTransferEncodingKey, strlen(KTransferEncodingKey), false) == 0)
				{
					 m_bTransferBlock = true;
					 if (!qcStrComp (pStartofValue,"chunked", -1, false))
						 m_bTransforChunk = true;
					 m_llContentLength = QCIO_MAX_CONTENT_LEN;
				}

				if (qcStrComp(m_szLineBuffer, KContentType, strlen(KContentType), false) == 0)
				{
					char* pSrc = m_szLineBuffer + strlen(KContentType) + 1;
					while (pSrc != NULL && *pSrc == ' ')
						pSrc++;
					if (strlen(pSrc) < sizeof(m_szContentType))
						strcpy(m_szContentType, pSrc);
					else
						strncpy(m_szContentType, pSrc, sizeof(m_szContentType) - 1);
					if(strstr(pSrc, "audio") != NULL || strstr(pSrc, "video") != NULL)  {
						m_bMediaType = true;
					}
					if(strstr(pSrc, "octet-stream") != NULL || strstr(pSrc, "video/x-flv") != NULL)  {
						m_bIsStreaming = true;
					}
                    if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
                        m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_CONTENT_TYPE, 0, 0, m_szContentType);
				}
			}
			
			continue;
		}		

		if (aBufferLen > (int)strlen(pStartofValue))
		{
            bIsKeyFound = true;
			strcpy(aBuffer, pStartofValue);
			return QC_ERR_NONE;
		}		
	}

	return nErr;
}

int CHTTPClient::ReceiveLine(char* aLine, int aSize)
{
	if (m_sState != CONNECTED) 
		return QC_ERR_RETRY;

	bool bSawCR = false;
	int nLength = 0;
	char log[2048];
	memset(log, 0, sizeof(char) * 2048);

	while (true) 
	{
	//	char c;
	//	int n = Recv(&c, 1);
	//	if (n <= 0) 
		int n = 1;
		char c = *m_pRespPos++;
		if (m_pRespPos - m_pRespBuff >= m_nRespLen)
		{
			n = 0;
			strncpy(log, aLine, nLength);
			if (n == 0)
			{
				m_nStatusCode = Status_Error_HttpResponseTimeOut;
				return QC_ERR_TIMEOUT;
			}
			else
			{
				return QC_ERR_CANNOT_CONNECT;
			}
		} 

		if (bSawCR && c == '\n') 
		{
			aLine[nLength - 1] = '\0';
			//strncpy(log, aLine, nLength);
			//LOGI("log: %s, logLength: %d", log, nLength);
			return QC_ERR_NONE;
		}

		bSawCR = (c == '\r');

		if (nLength + 1 >= aSize) 
		{
			return QC_ERR_Overflow;
		}

		aLine[nLength++] = c;
	}

	return QC_ERR_NONE;
}

int CHTTPClient::Receive(int& aSocketHandle, timeval& aTimeOut, char* aDstBuffer, int aSize)
{
	int nRead = 1;
	if (m_pRespData == NULL)
	{
		if (m_bEOS)
			return QC_ERR_HTTP_EOS;
		nRead = WaitSocketReadBuffer(aSocketHandle, aTimeOut);
	}
	if (nRead > 0)
	{
		if (m_bTransforChunk && !m_bReadChunkHead)
		{
			nRead = ReadChunkBuff(aSocketHandle, aDstBuffer, aSize);
			if (nRead > 0)
				m_llChunkLength += nRead;
		}
		else
		{
			nRead = SocketRead (aSocketHandle, aDstBuffer, aSize, 0);
		}

		if (nRead == 0)
		{
			//server close socket
			nRead = QC_ERR_ServerTerminated;
			QCLOGW ("server closed socket!");
		}
        if (nRead == -1)// && errno == _ETIMEDOUT) 
		{
            //network abnormal disconnected
            nRead = QC_ERR_NTAbnormallDisconneted;
 			QCLOGW ("network abnormal disconnected!");
       }
	}
    else
    {
//        QCLOGI("[EVT]Selelct return %d, %d, sys time %d", nRead, errno, qcGetSysTime());
    }

    if(m_bFirstByte && nRead>0)
    {
        m_bFirstByte = false;
		if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_IO_FIRST_BYTE_DONE, 0, 0);
    }

	return nRead;
}

int	CHTTPClient::ReadChunkBuff(int nSocketHandle, char * pDestBuff, int nDestSize)
{
	int		nErr = 0;
	int		nReadSize = 0;
	int		nHeadSize = 0;

	char *	pOutBuff = pDestBuff;
	int		nCpySize = 0;
	
	if (m_nChunkSize == 0)
	{
		m_nChunkHeadSize = 64;
		if (m_pChunkHeadBuff == NULL)
			m_pChunkHeadBuff = new char[m_nChunkHeadSize];
		bool	bFound = false;
		char *	pChunkBuff = m_pChunkHeadBuff;
		while (pChunkBuff - m_pChunkHeadBuff < m_nChunkHeadSize)
		{
			nErr = SocketRead(nSocketHandle, pChunkBuff, 1, 0);
			if (nErr <= 0)
				return nErr;
			if (pChunkBuff > m_pChunkHeadBuff)
			{
				if (*(pChunkBuff - 1) == '\r' && *pChunkBuff == '\n')
				{
					pChunkBuff++;
					bFound = true;
					break;
				}
			}
			pChunkBuff++;
		}
		if (!bFound)
			return 0;

		m_nChunkRead = 0;
		m_nChunkSize = GetChunkSize(m_pChunkHeadBuff, pChunkBuff - m_pChunkHeadBuff, nHeadSize);
		if (m_nChunkSize <= 0)
		{
			if (m_nChunkSize == 0)
				m_bEOS = true;
			return m_nChunkSize;
		}
	}

	nCpySize = nDestSize;
	if (nCpySize > m_nChunkSize - m_nChunkRead)
		nCpySize = m_nChunkSize - m_nChunkRead;

	nReadSize = SocketRead(nSocketHandle, pDestBuff, nCpySize, 0);
	if (nReadSize <= 0)
		return nReadSize;

	m_nChunkRead += nReadSize;
	if (m_nChunkRead >= m_nChunkSize)
	{
		m_nChunkRead = 0;
		m_nChunkSize = 0;
		// read end chars \r\n
		nErr = SocketRead(nSocketHandle, m_pChunkHeadBuff, 2, 0);
		if (nErr == 1)
			nErr = SocketRead(nSocketHandle, m_pChunkHeadBuff, 1, 0);
	}

	return nReadSize;
}

int	CHTTPClient::GetChunkSize(char * pBuff, int nSize, int & nHeadSize)
{
	if (pBuff == NULL || nSize < 3)
		return -1;

	nHeadSize = 0;
	char *	pEnd = pBuff;
	while (pEnd - pBuff < nSize - 1)
	{
		if (*pEnd == '\r' && *(pEnd + 1) == '\n')
		{
			nHeadSize = pEnd - pBuff + 2;
			break;
		}
		pEnd++;
	}
	if (nHeadSize == 0)
		return -1;

	int		nNum = 0;
	int		nLen = 0;
	int		nMove = 4;
	char *	pPos = pBuff;
	while (*pPos != '\r')
	{
		if (*pPos >= '0' && *pPos <= '9')
			nNum = *pPos - '0';
		else if (*pPos >= 'a' && *pPos <= 'f')
			nNum = *pPos - 'a' + 10;
		else if (*pPos >= 'A' && *pPos <= 'F')
			nNum = *pPos - 'A' + 10;
		else
			break;

		nLen = nLen << nMove;
		nLen = nLen + nNum;
		pPos++;
	}

	return nLen;
}

int CHTTPClient::WaitSocketWriteBuffer(int& aSocketHandle, timeval& aTimeOut)
{
	fd_set		fds;
	//timeval		tmWait = { 1, 0 };
    timeval        tmWait = { 0, 100000 };
    if(aTimeOut.tv_sec == 0 && aTimeOut.tv_usec < tmWait.tv_usec)
        tmWait.tv_usec = aTimeOut.tv_usec;
    
	int			ret = 0;
	int			nStartTime = qcGetSysTime();
	while (ret == 0)
	{
		if (qcGetSysTime() - nStartTime > (aTimeOut.tv_sec * 1000 + aTimeOut.tv_usec/1000))
			break;
		if (m_pBaseInst->m_bForceClose == true)
			return QC_ERR_TIMEOUT;

		FD_ZERO(&fds);
		FD_SET(aSocketHandle, &fds);

		ret = select(aSocketHandle + 1, NULL, &fds, NULL, &tmWait);
		if (ret == 0)
			qcSleep(1000);
	}
	int err = 0;
	int errLength = sizeof(err);

	if (ret > 0 && FD_ISSET(aSocketHandle, &fds))
	{
        getsockopt(aSocketHandle, SOL_SOCKET, SO_ERROR, (char *)&err, (socklen_t*)&errLength);
		if (err != 0)
		{
			SetStatusCode(err + CONNECT_ERROR_BASE);
			ret = -1;
		}
	}
	else if(ret < 0)
	{
		SetStatusCode(errno + CONNECT_ERROR_BASE);
	}

	return (ret > 0) ? QC_ERR_NONE : ((ret == 0) ? QC_ERR_TIMEOUT : QC_ERR_CANNOT_CONNECT);
}

int CHTTPClient::WaitSocketReadBuffer(int& aSocketHandle, timeval& aTimeOut)
{
	fd_set fds;
    int nRet = 0;
    int tryCnt =0;
retry:
	//select : if error happens, select return -1, detail errorcode is in error
    SetStatusCode(0);
    
    timeval tmWait = { 0, 100000 };
    if(aTimeOut.tv_sec == 0 && aTimeOut.tv_usec < tmWait.tv_usec)
        tmWait.tv_usec = aTimeOut.tv_usec;
    int     nStartTime = qcGetSysTime();
    while (nRet == 0)
    {
        if (qcGetSysTime() - nStartTime > (aTimeOut.tv_sec * 1000 + aTimeOut.tv_usec/1000))
            break;
        if (m_pBaseInst->m_bForceClose == true)
            return QC_ERR_TIMEOUT;
        FD_ZERO(&fds);
        FD_SET(aSocketHandle, &fds);
        
        nRet = select(aSocketHandle + 1, &fds, NULL, NULL, &tmWait);
    }
    
	if (nRet > 0 && !FD_ISSET(aSocketHandle, &fds))
	{
		m_nWaitTimeoutCount = 0;
		nRet = 0;
	}
	else if(nRet < 0)
	{
		if (m_pBaseInst->m_bForceClose)
			return nRet;

		SetStatusCode(errno + RESPONSE_ERROR_BASE);
        if (StatusCode() == HTTPRESPONSE_INTERUPTER && tryCnt < 20 && IsCancel() == false)
        {
            tryCnt++;
            goto retry;
        }
	}
    else if(nRet == 0)
    {
        m_nWaitTimeoutCount++;
        if(m_nWaitTimeoutCount > 50) // 50 * m_pSetting->g_qcs_nTimeOutRead
        {
            QCLOGW("select read buffer is timeout count %d, socket maybe disconnect", m_nWaitTimeoutCount);
            m_nWaitTimeoutCount = 0;
            nRet = QC_ERR_NTAbnormallDisconneted;
        }
    }
    else // select successfully
        m_nWaitTimeoutCount = 0;
    
	return nRet;
}

int CHTTPClient::SetSocketTimeOut(int& aSocketHandle, timeval aTimeOut)
{	
	return setsockopt(aSocketHandle, SOL_SOCKET, SO_RCVTIMEO, (char *)&aTimeOut, sizeof(struct timeval));
}

void CHTTPClient::SetSocketBlock(int& aSocketHandle)
{
#ifdef __QC_OS_WIN32__
	u_long non_blk = 0;
	ioctlsocket(aSocketHandle, FIONBIO, &non_blk);
#else
	int flags = fcntl(aSocketHandle, F_GETFL, 0);
	flags &= (~O_NONBLOCK);
	fcntl(aSocketHandle, F_SETFL, flags);
#endif
}

void CHTTPClient::SetSocketNonBlock(int& aSocketHandle)
{
#ifdef __QC_OS_WIN32__
	u_long non_blk = 1;
	ioctlsocket(aSocketHandle, FIONBIO, &non_blk);
#else
	int flags = fcntl(aSocketHandle, F_GETFL, 0);
	fcntl(aSocketHandle, F_SETFL, flags | O_NONBLOCK);
#endif
}

int	CHTTPClient::SocketRead (int nSocket, char * pBuff, int nSize, int nFlag)
{
	int nErr = 0;
	if (m_pRespData != NULL)
		nErr = RespDataRead(nSocket, pBuff, nSize, 0);
	if (nErr > 0)
	{
		if (m_pDumpFile != NULL && nErr > 0 && !m_bReadChunkHead)
			m_pDumpFile->Write((unsigned char *)pBuff, nErr);
		return nErr;
	}

	if (m_bIsSSL)
	{
		if (m_pOpenSSL == NULL)
			return -1;
		nErr = m_pOpenSSL->Read(pBuff, nSize);
	}
	else
	{
		nErr = recv (nSocket, pBuff, nSize, nFlag);
	}

	if (m_pDumpFile != NULL && nErr > 0 && !m_bReadChunkHead)
		m_pDumpFile->Write((unsigned char *)pBuff, nErr);

	return nErr;
}

int CHTTPClient::RespDataRead(int nSocket, char * pBuff, int nSize, int nFlag)
{
	if (m_pRespData == NULL)
		return 0;

	int nCopySize = nSize;
	int nDataSize = m_nRespSize - m_nRespLen;
	if (nCopySize > nDataSize - m_nRespRead)
		nCopySize = nDataSize - m_nRespRead;
	if (nCopySize == 0)
		return 0;
	memcpy(pBuff, m_pRespData + m_nRespRead, nCopySize);
	m_nRespRead += nCopySize;
	if (m_nRespRead == nDataSize)
		m_pRespData = NULL;

	return nCopySize;
}

int	CHTTPClient::SocketSend (int nSocket, const char * pBuff, int nSize, int nFlag)
{
	int nSend = 0;
	if (m_bIsSSL)
	{
		if (m_pOpenSSL == NULL)
			return -1;
		nSend = m_pOpenSSL->Write(pBuff, nSize);
	}
	else
	{
#ifdef __QC_OS_WIN32__
		nSend = send (nSocket, pBuff, nSize, nFlag);
#else
		nSend = write (nSocket, pBuff, nSize);
		if (nSend < 0 && errno == EINTR) {
              nSend = 0;        // and call write() again 
		} 
#endif
	}
	return nSend;
}

int CHTTPClient::SoeketConnect (int nSocket, int nTimeOut)
{
	if (m_bIsSSL)
	{
        QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
		if (m_pOpenSSL == NULL)
			return -1;
        
        if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
            m_pBaseInst->m_pMsgMng->Notify(QC_MSG_IO_HANDSHAKE_START, 0, 0);

#if 0
		int nRet = m_pOpenSSL->Connect(nSocket);
#else
        int nRet = QC_ERR_RETRY;
        m_pOpenSSL->SetConnectState(nSocket);
        int nStartTime = qcGetSysTime();
        timeval timeout = { nTimeOut / 1000, (nTimeOut % 1000) * 1000 };
        while ((nRet = m_pOpenSSL->DoHandshake()) != QC_ERR_NONE)
        {
            if (nRet == SSL_ERROR_WANT_READ)
                nRet = WaitSocketReadBuffer(nSocket, timeout);
            else if (nRet == SSL_ERROR_WANT_WRITE)
                nRet = WaitSocketWriteBuffer(nSocket, timeout);
            else if (m_pBaseInst->m_bForceClose)
                nRet = QC_ERR_FORCECLOSE;
            else
                qcSleep(1000);
                
            if (nRet < 0 || (qcGetSysTime()-nStartTime > nTimeOut))
            {
                if (nRet == QC_ERR_TIMEOUT)
                    m_nStatusCode = Status_Error_ServerConnectTimeOut;
                QCLOGE("SSL connect error. nErr: %d, errorno: %d(%s)", nRet, errno, strerror(errno));
                if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
                    m_pBaseInst->m_pMsgMng->Notify((nRet==QC_ERR_NONE)?QC_MSG_IO_HANDSHAKE_SUCESS:QC_MSG_IO_HANDSHAKE_FAILED, 0, 0);
                Disconnect();
                SetSocketBlock(m_nSocketHandle);
                return QC_ERR_CANNOT_CONNECT;
            }
        }
#endif
        
        if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
            m_pBaseInst->m_pMsgMng->Notify((nRet==QC_ERR_NONE)?QC_MSG_IO_HANDSHAKE_SUCESS:QC_MSG_IO_HANDSHAKE_FAILED, 0, 0);
        
        return nRet;
	}
	return QC_ERR_NONE;
}

int CHTTPClient::SocketClose (int nSocket)
{
	int nRC = QC_ERR_NONE;
	if (m_bIsSSL)
	{
		if (m_pOpenSSL != NULL)
			m_pOpenSSL->Disconnect(nSocket);
	}
#ifdef __QC_OS_WIN32__
	closesocket(nSocket);
#else
	close(nSocket);
#endif
	return nRC;
}

void CHTTPClient::ResetParam (void)
{
	m_nStatusCode = STATUS_OK;
	m_bCancel = false;
	m_bTransferBlock = false;
	m_bTransforChunk = false;
	m_bMediaType = false;
	m_bIsStreaming = false;
	m_llContentLength = QCIO_MAX_CONTENT_LEN;// KInvalidContentLength;

	m_bReadChunkHead = true;
	m_nChunkSize = 0;
	m_nChunkRead = 0;
	m_bEOS = false;
	m_llChunkLength = 0;

	strcpy(m_szContentType, "");
}
