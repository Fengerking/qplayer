#ifdef __QC_OS_WIN32__
#include "winsock2.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <signal.h> 
#include <ctype.h>
#include <exception>
#include <typeinfo>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include<signal.h>
#endif // __QC_OS_WIN32__
#include "qcErr.h"

#include "CDNSLookUp.h"
#include "CDNSCache.h"

#include "USystemFunc.h"
#include "USocketFunc.h"
#include "ULogFunc.h"

CDNSLookup::CDNSLookup(CBaseInst * pBaseInst)
	: CBaseObject (pBaseInst)
	, m_pExtDNSServer(NULL)
	, m_bIsInitOK(false)
    , m_sock(-1)
    , m_szDNSPacket(NULL)
{
	SetObjectName ("CDNSLookup");
	// 114 DNS  114.114.114.114  114.114.115.115
	//m_ulDNSServerIP = inet_addr("114.114.114.114"); 

	// ALI DNS  223.5.5.5   223.6.6.6
	m_ulDNSServerIP = inet_addr("223.5.5.5");
	strcpy(m_szDNSServer, "223.5.5.5");

	// Baidu DNS  180.76.76.76
	// m_ulDNSServerIP = inet_addr("180.76.76.76");

	// Google DNS  8.8.8.8  8.8.4.4
	// m_ulDNSServerIP = inet_addr("8.8.8.8");

	char * pDNSServer = m_pBaseInst->m_pSetting->g_qcs_szDNSServerName;
	if (strlen(pDNSServer) > 0)
	{
		if (strcmp(pDNSServer, "0.0.0.0") && strcmp(pDNSServer, "127.0.0.1"))
		{
			m_pExtDNSServer = pDNSServer;
			strcpy(m_szDNSServer, m_pExtDNSServer);
			m_ulDNSServerIP = inet_addr(m_pExtDNSServer);
		}
	}

	m_bIsInitOK = Init();
}

CDNSLookup::~CDNSLookup(void)
{
	UnInit();
}

int	CDNSLookup::UpdateDNSServer(void)
{
	char * pDNSServer = m_pBaseInst->m_pSetting->g_qcs_szDNSServerName;
	if (strlen(pDNSServer) > 0)
	{
		if (strcmp(pDNSServer, "0.0.0.0") && strcmp(pDNSServer, "127.0.0.1"))
		{
			m_pExtDNSServer = pDNSServer;
			strcpy(m_szDNSServer, m_pExtDNSServer);
			m_ulDNSServerIP = inet_addr(m_pExtDNSServer);
		}
	}
	return QC_ERR_NONE;
}

int CDNSLookup::GetDNSAddrInfo(char *szDomainName, void * pDevice, void * pInt, void ** ppResult, unsigned long ulTimeout)
{
	CAutoLock lock(&m_mtLock);
	if (szDomainName == NULL || ppResult == NULL)
		return QC_ERR_ARG;
	*ppResult = NULL;

	int				nRC = QC_ERR_NONE;
	unsigned long	uIPAddr = 0;
	if (!qcHostIsIPAddr(szDomainName))
	{
		nRC = DNSLookup(szDomainName, ulTimeout);
		if (nRC != QC_ERR_NONE)
			return nRC;
		uIPAddr = GetIPAdr(0);

		if (strstr(szDomainName, "report.qiniuapi.com") == NULL)
		{
			unsigned long *	pIPNum = 0;
			NODEPOS pos = m_lstIPNum.GetHeadPosition();
			while (pos != NULL)
			{
				pIPNum = m_lstIPNum.GetNext(pos);
				m_pBaseInst->m_pDNSCache->AddCheckIP(szDomainName, *pIPNum, false);
			}
			m_pBaseInst->m_pDNSCache->Start();
		}	
	}
	else
	{
		uIPAddr = qcGetIPAddrFromString(szDomainName);
	}

	qcGetIPAddrFromValue(uIPAddr, ppResult);

	return QC_ERR_NONE;
}

int CDNSLookup::FreeDNSAddrInfo(void * pAddr)
{
	CAutoLock lock(&m_mtLock);

	if (pAddr == NULL)
		return QC_ERR_ARG;

	struct addrinfo * hInfo = (addrinfo *)pAddr;
	if (hInfo->ai_addr != NULL)
		delete hInfo->ai_addr;
	delete hInfo;

	return QC_ERR_NONE;
}

int CDNSLookup::DNSLookup (char *szDomainName, unsigned long ulTimeout)
{
	CAutoLock lock(&m_mtLock);

	int nStartTime = qcGetSysTime();
	int nRC = QC_ERR_FAILED;

	if (m_pExtDNSServer != NULL)
		nRC = DNSLookupCore(szDomainName, ulTimeout);

	if (nRC != QC_ERR_NONE)
	{
		int nTryTimes = 0;
		int nOneTimeout = ulTimeout / 3;
		if (nOneTimeout < 3000)
			nOneTimeout = 3000;

		while (nTryTimes < 3)
		{
			if (nTryTimes == 0)
			{
				m_ulDNSServerIP = inet_addr("223.5.5.5"); // ALI
				strcpy(m_szDNSServer, "223.5.5.5");
			}
			else if (nTryTimes == 1)
			{
				m_ulDNSServerIP = inet_addr("8.8.8.8"); // google
				strcpy(m_szDNSServer, "8.8.8.8");
			}
			else
			{
				m_ulDNSServerIP = inet_addr("114.114.114.114"); // google
				strcpy(m_szDNSServer, "114.114.114.114");
			}

			nRC = DNSLookupCore(szDomainName, nOneTimeout);
			if (nRC == QC_ERR_NONE)
				break;

			nTryTimes++;
			if ((qcGetSysTime() - nStartTime > (int)ulTimeout) || m_pBaseInst->m_bForceClose)
				return QC_ERR_TIMEOUT;
		}
	}

	if (nRC != QC_ERR_NONE)
		return nRC;

	char *			pIPTxt = NULL;
	unsigned char *	pbyIPSegment = NULL;
	unsigned long *	pIPNum = 0;
	NODEPOS pos = m_lstIPNum.GetHeadPosition();
	while (pos != NULL)
	{
		pIPNum = m_lstIPNum.GetNext(pos);
		unsigned char *pbyIPSegment = (unsigned char*)pIPNum;
		pIPTxt = new char[16];
		sprintf(pIPTxt, "%d.%d.%d.%d", pbyIPSegment[0], pbyIPSegment[1], pbyIPSegment[2], pbyIPSegment[3]);
		m_lstIPTxt.AddTail(pIPTxt);
		QCLOGI("The IP is %s", pIPTxt);
	}

	QCLOGI("DNS Server %s lookup domain %s used time = %d", m_szDNSServer, szDomainName, qcGetSysTime() - nStartTime);

	return QC_ERR_NONE;
}

int CDNSLookup::GetCount(void)
{
	return m_lstIPNum.GetCount();
}

unsigned long CDNSLookup::GetIPAdr(int nIndex)
{
	if (nIndex < 0 || nIndex >= GetCount())
		return 0;
	int				nPos = 0;
	unsigned long *	pIPNum = NULL;
	NODEPOS pos = m_lstIPNum.GetHeadPosition();
	while (pos != NULL)
	{
		pIPNum = m_lstIPNum.GetNext(pos);
		if (nPos == nIndex)
			return *pIPNum;
		nPos++;
	}
	return 0;
}

const char * CDNSLookup::GetIPStr(int nIndex)
{
	if (nIndex < 0 || nIndex >= GetCount())
		return NULL;
		
	int			nPos = 0;
	char *		pIPTxt = NULL;
	NODEPOS pos = m_lstIPTxt.GetHeadPosition();
	while (pos != NULL)
	{
		pIPTxt = m_lstIPTxt.GetNext(pos);
		if (nPos == nIndex)
			return pIPTxt;
		nPos++;
	}
	return NULL;
}

const char * CDNSLookup::GetIPName(int nIndex)
{
	if (nIndex < 0 || nIndex >= GetCount())
		return NULL;

	int			nPos = 0;
	char *		pName = NULL;
	NODEPOS pos = m_lstName.GetHeadPosition();
	while (pos != NULL)
	{
		pName = m_lstName.GetNext(pos);
		if (nPos == nIndex)
			return pName;
		nPos++;
	}
	return NULL;
}

int CDNSLookup::DNSLookupCore (char *szDomainName, unsigned long ulTimeout)
{
	if (m_bIsInitOK == false || szDomainName == NULL)
		return QC_ERR_ARG;

	//config SOCKET
	sockaddr_in sockAddrDNSServer;
	sockAddrDNSServer.sin_family = AF_INET;
	sockAddrDNSServer.sin_addr.s_addr = m_ulDNSServerIP;
	sockAddrDNSServer.sin_port = htons(DNS_PORT);

	//DNS search and parser
	int nRC = SendDNSRequest(sockAddrDNSServer, szDomainName);
	if (nRC != QC_ERR_NONE)
		return nRC;

	ulTimeout = ulTimeout / 2;
	nRC = RecvDNSResponse(sockAddrDNSServer, ulTimeout);
	if (nRC == QC_ERR_TIMEOUT)
	{
		nRC = SendDNSRequest(sockAddrDNSServer, szDomainName);
		if (nRC != QC_ERR_NONE)
			return nRC;
		nRC = RecvDNSResponse(sockAddrDNSServer, ulTimeout);
	}

	return nRC;
}

int CDNSLookup::SendDNSRequest(sockaddr_in sockAddrDNSServer, char *szDomainName)
{
	char *pWriteDNSPacket = m_szDNSPacket;
	memset(pWriteDNSPacket, 0, DNS_PACKET_MAX_SIZE);

	DNSHeader *pDNSHeader = (DNSHeader*)pWriteDNSPacket;
	pDNSHeader->usTransID = m_usCurrentProcID;
	pDNSHeader->usFlags = htons(0x0100);
	pDNSHeader->usQuestionCount = htons(0x0001);
	pDNSHeader->usAnswerCount = 0x0000;
	pDNSHeader->usAuthorityCount = 0x0000;
	pDNSHeader->usAdditionalCount = 0x0000;

	unsigned short usQType = htons(0x0001);
	unsigned short usQClass = htons(0x0001);
	unsigned short nDomainNameLen = strlen(szDomainName);
	char *szEncodedDomainName = (char *)malloc(nDomainNameLen + 2);
	if (szEncodedDomainName == NULL)
		return QC_ERR_MEMORY;

	bool bRC = EncodeDotStr(szDomainName, szEncodedDomainName, nDomainNameLen + 2);
	if (!bRC)
		return QC_ERR_FAILED;

	unsigned short nEncodedDomainNameLen = strlen(szEncodedDomainName) + 1;
	memcpy(pWriteDNSPacket += sizeof(DNSHeader), szEncodedDomainName, nEncodedDomainNameLen);
	memcpy(pWriteDNSPacket += nEncodedDomainNameLen, (char*)(&usQType), DNS_TYPE_SIZE);
	memcpy(pWriteDNSPacket += DNS_TYPE_SIZE, (char*)(&usQClass), DNS_CLASS_SIZE);
	free(szEncodedDomainName);

	unsigned short nDNSPacketSize = sizeof(DNSHeader) + nEncodedDomainNameLen + DNS_TYPE_SIZE + DNS_CLASS_SIZE;
	int nSendNum = sendto(m_sock, m_szDNSPacket, nDNSPacketSize, 0, (sockaddr*)&sockAddrDNSServer, sizeof(sockAddrDNSServer));
	if (nSendNum < 0)
	{
		QCLOGW("Send data failed! return = %d", nSendNum);
		return QC_ERR_FAILED;
	}

	return QC_ERR_NONE;
}

int CDNSLookup::RecvDNSResponse(sockaddr_in sockAddrDNSServer, unsigned long ulTimeout)
{
	unsigned long ulSendTimestamp = qcGetSysTime ();

	FreeList();

	char recvbuf[1024] = { '\0' };
	char szDotName[128] = { '\0' };

	unsigned short	nEncodedNameLen = 0;
	unsigned long	ulRecvTimestamp = qcGetSysTime();
	int				nSockaddrDestSize = sizeof(sockAddrDNSServer);

	int				nStatus = 0;
	struct timeval	nRecvDataTimeOut = { 0, 100000 };
	while (true)
	{
		if (qcGetSysTime() - ulSendTimestamp > ulTimeout)
		{
			m_uTimeSpent = ulTimeout + 1;
			QCLOGW("Recv data from DSN server %s is timeout  = %d", m_szDNSServer, (int)m_uTimeSpent);
			return QC_ERR_TIMEOUT;
		}

		nStatus = WaitSocketReadBuffer(nRecvDataTimeOut);
		if (nStatus < 0)
			return QC_ERR_FAILED;
		else if (m_pBaseInst->m_bForceClose)
			return QC_ERR_STATUS;
		else if (nStatus == 0)
			continue;
		int nRecvSize = 0;
#ifdef __QC_OS_WIN32__		
		nRecvSize = recvfrom(m_sock, recvbuf, 1024, 0, (struct sockaddr*)&sockAddrDNSServer, &nSockaddrDestSize);
#else
		nRecvSize = recvfrom(m_sock, recvbuf, 1024, 0, (struct sockaddr*)&sockAddrDNSServer, (socklen_t*)&nSockaddrDestSize);
#endif // __QC_OS_WIN32__
		if (nRecvSize > 0)
		{
			DNSHeader *pDNSHeader = (DNSHeader*)recvbuf;
			unsigned short usQuestionCount = 0;
			unsigned short usAnswerCount = 0;
		
			if (pDNSHeader->usTransID == m_usCurrentProcID
				&& (ntohs(pDNSHeader->usFlags) & 0xfb7f) == 0x8100 //RFC1035 4.1.1(Header section format)
				&& (usQuestionCount = ntohs(pDNSHeader->usQuestionCount)) >= 0
				&& (usAnswerCount = ntohs(pDNSHeader->usAnswerCount)) > 0)
			{
				char *pDNSData = recvbuf + sizeof(DNSHeader);
		
				for (int q = 0; q != usQuestionCount; ++q)
				{
					if (!DecodeDotStr(pDNSData, &nEncodedNameLen, szDotName, sizeof(szDotName)))
					{
						return QC_ERR_FAILED;
					}
					pDNSData += (nEncodedNameLen + DNS_TYPE_SIZE + DNS_CLASS_SIZE);
				}
				
				for (int a = 0; a != usAnswerCount; ++a)
				{
					if (!DecodeDotStr(pDNSData, &nEncodedNameLen, szDotName, sizeof(szDotName), recvbuf))
					{
						return QC_ERR_FAILED;
					}
					pDNSData += nEncodedNameLen;
				
					unsigned short usAnswerType = ntohs(*(unsigned short*)(pDNSData));
					unsigned short usAnswerClass = ntohs(*(unsigned short*)(pDNSData + DNS_TYPE_SIZE));
					unsigned long usAnswerTTL = ntohl(*(unsigned long*)(pDNSData + DNS_TYPE_SIZE + DNS_CLASS_SIZE));
					unsigned short usAnswerDataLen = ntohs(*(unsigned short*)(pDNSData + DNS_TYPE_SIZE + DNS_CLASS_SIZE + DNS_TTL_SIZE));
					pDNSData += (DNS_TYPE_SIZE + DNS_CLASS_SIZE + DNS_TTL_SIZE + DNS_DATALEN_SIZE);
				
					if (usAnswerType == DNS_TYPE_A)
					{
						unsigned long * pIPNum = new unsigned long;
						*pIPNum = *(unsigned long*)(pDNSData);
						m_lstIPNum.AddTail(pIPNum);
					}
					else if (usAnswerType == DNS_TYPE_CNAME)
					{
						if (!DecodeDotStr(pDNSData, &nEncodedNameLen, szDotName, sizeof(szDotName), recvbuf))
							return false;
						char * pName = new char[strlen(szDotName)+1];
						strcpy(pName, szDotName);
						m_lstName.AddTail(pName);
					}
				
					pDNSData += (usAnswerDataLen);
				}
				
				m_uTimeSpent = ulRecvTimestamp - ulSendTimestamp;
				break;
			}
		}
	}

	return QC_ERR_NONE;
}

/*
* convert "www.baidu.com" to "\x03www\x05baidu\x03com"
* 0x0000 03 77 77 77 05 62 61 69 64 75 03 63 6f 6d 00 ff
*/
 bool CDNSLookup::EncodeDotStr(char *szDotStr, char *szEncodedStr, unsigned short nEncodedStrSize)
 {
	unsigned short nDotStrLen = strlen(szDotStr);

	if (szDotStr == NULL || szEncodedStr == NULL || nEncodedStrSize < nDotStrLen + 2)
		return false;

	char *szDotStrCopy = new char[nDotStrLen + 1];
	//strcpy_s(szDotStrCopy, nDotStrLen + 1, szDotStr);
	strcpy(szDotStrCopy, szDotStr);

	char *pNextToken = NULL;
	//char *pLabel = strtok_s(szDotStrCopy, ".", &pNextToken);
	char *pLabel = strtok(szDotStrCopy, ".");
	unsigned short nLabelLen = 0;
	unsigned short nEncodedStrLen = 0;
	while (pLabel != NULL)
	{
		if ((nLabelLen = strlen(pLabel)) != 0)
		{
			//sprintf_s(szEncodedStr + nEncodedStrLen, nEncodedStrSize - nEncodedStrLen, "%c%s", nLabelLen, pLabel);
			sprintf(szEncodedStr + nEncodedStrLen, "%c%s", nLabelLen, pLabel);
			nEncodedStrLen += (nLabelLen + 1);
		}
		//pLabel = strtok_s(NULL, ".", &pNextToken);
		pLabel = strtok(NULL, ".");
	}

	delete[] szDotStrCopy;

	return true;
}

/*
* convert "\x03www\x05baidu\x03com\x00" to "www.baidu.com"
* 0x0000 03 77 77 77 05 62 61 69 64 75 03 63 6f 6d 00 ff
* convert "\x03www\x05baidu\xc0\x13" to "www.baidu.com"
* 0x0000 03 77 77 77 05 62 61 69 64 75 c0 13 ff ff ff ff
* 0x0010 ff ff ff 03 63 6f 6d 00 ff ff ff ff ff ff ff ff
*/
bool CDNSLookup::DecodeDotStr(char *szEncodedStr, unsigned short *pusEncodedStrLen, char *szDotStr, unsigned short nDotStrSize, char *szPacketStartPos)
{
	if (szEncodedStr == NULL || pusEncodedStrLen == NULL || szDotStr == NULL)
		return false;

	char *pDecodePos = szEncodedStr;
	unsigned short usPlainStrLen = 0;
	unsigned char nLabelDataLen = 0;
	* pusEncodedStrLen = 0;

	while ((nLabelDataLen = *pDecodePos) != 0x00)
	{
		if ((nLabelDataLen & 0xc0) == 0) 
		{
			if (usPlainStrLen + nLabelDataLen + 1 > nDotStrSize)
				return false;

			memcpy(szDotStr + usPlainStrLen, pDecodePos + 1, nLabelDataLen);
			memcpy(szDotStr + usPlainStrLen + nLabelDataLen, ".", 1);
			pDecodePos += (nLabelDataLen + 1);
			usPlainStrLen += (nLabelDataLen + 1);
			* pusEncodedStrLen += (nLabelDataLen + 1);
		}
		else
		{
			if (szPacketStartPos == NULL)
				return false;

			unsigned short usJumpPos = ntohs(*(unsigned short*)(pDecodePos)) & 0x3fff;
			unsigned short nEncodeStrLen = 0;
			if (!DecodeDotStr(szPacketStartPos + usJumpPos, &nEncodeStrLen, szDotStr + usPlainStrLen, nDotStrSize - usPlainStrLen, szPacketStartPos))
			{
				return false;
			}
			else
			{
				*pusEncodedStrLen += 2;
				return true;
			}
		}
	}

	szDotStr[usPlainStrLen - 1] = '\0';
	*pusEncodedStrLen += 1;

	return true;
}


bool CDNSLookup::Init()
{
#ifdef __QC_OS_WIN32__
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR)
		return false;
#endif //__QC_OS_WIN32__
	if ((m_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return false;

#ifdef __QC_OS_WIN32__
	u_long non_blk = 1;
	ioctlsocket(m_sock, FIONBIO, &non_blk);
#else
	int flags = fcntl(m_sock, F_GETFL, 0);
	fcntl(m_sock, F_SETFL, flags | O_NONBLOCK);
#endif

	struct timeval nRecvDataTimeOut = { 0, 100000 };
	setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&nRecvDataTimeOut, sizeof(struct timeval));

	//	m_event = WSACreateEvent();
	//	WSAEventSelect(m_sock, m_event, FD_READ);

	m_szDNSPacket = new char[DNS_PACKET_MAX_SIZE];
	if (m_szDNSPacket == NULL)
		return false;

	m_usCurrentProcID = (unsigned short)qcGetProcessID();

	return true;
}

bool CDNSLookup::UnInit()
{
	if (m_sock != -1)
	{
#ifdef __QC_OS_WIN32__
		closesocket(m_sock);
#else
		close(m_sock);
#endif
	}
	m_sock = -1;
#ifdef __QC_OS_WIN32__
	WSACleanup();
#endif // __QC_OS_WIN32__

	if (m_szDNSPacket != NULL)
	{
		delete[] m_szDNSPacket;
		m_szDNSPacket = NULL;
	}
	m_bIsInitOK = false;

	FreeList();

	return true;
}

int CDNSLookup::WaitSocketReadBuffer(timeval& aTimeOut)
{
	fd_set fds;
	int nRet;

	FD_ZERO(&fds);
	FD_SET(m_sock, &fds);

	//select : if error happens, select return -1, detail errorcode is in error
	nRet = select(m_sock + 1, &fds, NULL, NULL, &aTimeOut);

	return nRet;
}

void CDNSLookup::FreeList(void)
{
	unsigned long * pIPNum = m_lstIPNum.RemoveHead();
	while (pIPNum != NULL)
	{
		delete pIPNum;
		pIPNum = m_lstIPNum.RemoveHead();
	}

	char * pIPTxt = m_lstIPTxt.RemoveHead();
	while (pIPTxt != NULL)
	{
		delete []pIPTxt;
		pIPTxt = m_lstIPTxt.RemoveHead();
	}

	char * pName = m_lstName.RemoveHead();
	while (pName != NULL)
	{
		delete[]pName;
		pName = m_lstName.RemoveHead();
	}
}