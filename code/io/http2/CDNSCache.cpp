/*******************************************************************************
	File:		CDNSCache.cpp

	Contains:	DNS cache implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-06		Bangfei			Create file

*******************************************************************************/
#include "stdio.h"
#include "string.h"
#ifdef __QC_OS_WIN32__
#include "winsock2.h"
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
#include <exception>
#include <typeinfo>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include<signal.h>
#endif // __QC_OS_WIN32__

#include "qcErr.h"

#include "CDNSCache.h"
#include "CHTTPClient.h"

#include "USystemFunc.h"
#include "USocketFunc.h"

//#define QCDNS_USE_THREAD

#ifdef __QC_NAMESPACE__
using namespace __QC_NAMESPACE__;
#endif

class CHTTPDNSLookup : public CHTTPClient
{
public:
	CHTTPDNSLookup(CBaseInst * pBaseInst)
		: CHTTPClient (pBaseInst, NULL)
	{
		m_bNotifyMsg = false;
	}

	virtual ~CHTTPDNSLookup(void)
	{
	}

	virtual int GetIPAddress(char * pHostName, unsigned long ** ppIPAddr, int * nSize)
	{
		if (ppIPAddr == NULL || nSize == NULL)
			return QC_ERR_ARG;
		*ppIPAddr = NULL;
		*nSize = 0;

		if (qcHostIsIPAddr(pHostName))
			return QC_ERR_ARG;

		int nPort = 80;
		unsigned long	dnsServer = 0X1D1D1D77; //119.29.29.29
		struct sockaddr ipAddr;
		qcFillSockAddr(&ipAddr, dnsServer);

		int nStartTime = qcGetSysTime();
		int nRC = ConnectServer((QCIPAddr)&ipAddr, nPort, 2000);
		if (nRC != QC_ERR_NONE)
			return nRC;
        
        struct timeval nTimeOut = {0, 100000};
        setsockopt(m_nSocketHandle, SOL_SOCKET, SO_SNDTIMEO, (char *)&nTimeOut, sizeof(struct timeval));
        setsockopt(m_nSocketHandle, SOL_SOCKET, SO_RCVTIMEO, (char *)&nTimeOut, sizeof(struct timeval));
        
		memset(m_szRequset, 0, sizeof(m_szRequset));
		sprintf(m_szRequset, "GET /%s%s HTTP/1.1\r\n", "d?dn=", pHostName);
		strcat(m_szRequset, "\r\n");
		nRC = Send(m_szRequset, strlen(m_szRequset));
		if (nRC != QC_ERR_NONE)
			return nRC;

		unsigned int nStatus = 0;
		nRC = ParseResponseHeader(nStatus);
		if (nRC == QC_ERR_NONE && m_nCount > 0)
		{
			*nSize = m_nCount;
			*ppIPAddr = m_ipAddr;
		}
		Disconnect();

		int nUsedTime = qcGetSysTime() - nStartTime;
		return QC_ERR_NONE;
	}

protected:
	virtual int	ParseHeader(unsigned int& aStatusCode)
	{
		if (m_pRespData == NULL)
			return QC_ERR_FAILED;

		m_nCount = 0;

		int	   nNum = 0;
		char * pDot = NULL;
		char * pIP = m_pRespData;
		char * pPos = strstr(m_pRespData, ";");
		while (pIP != NULL)
		{
			pDot = strchr(pIP, '.');
			if (pDot == NULL)
				return QC_ERR_FAILED;
			*pDot = 0;
			nNum = atoi(pIP);
			m_ipAddr[m_nCount] = nNum;

			pIP = pDot + 1;
			pDot = strchr(pIP, '.');
			if (pDot == NULL)
				return QC_ERR_FAILED;
			*pDot = 0;
			nNum = atoi(pIP);
			m_ipAddr[m_nCount] += (nNum << 8);

			pIP = pDot + 1;
			pDot = strchr(pIP, '.');
			if (pDot == NULL)
				return QC_ERR_FAILED;
			*pDot = 0;
			nNum = atoi(pIP);
			m_ipAddr[m_nCount] += (nNum << 16);

			pIP = pDot + 1;
			if (pPos != NULL)
				*pPos = 0;
			nNum = atoi(pIP);
			m_ipAddr[m_nCount] += (nNum << 24);

			m_nCount++;
			if (m_nCount >= 256)
				break;
			if (pPos != NULL)
			{
				pIP = pPos + 1;
				pPos = strstr(pIP, ";");
			}
			else
			{
				break;
			}
		}

		return QC_ERR_NONE;
	}

	int Recv(char* aDstBuffer, int aSize)
	{
		struct timeval tTimeout = { 0, m_pBaseInst->m_pSetting->g_qcs_nTimeOutRead * 1000 };//{HTTP_HEADER_RECV_TIMEOUT_IN_SECOND, 0};
		int nErr = 0;
		char *	pBuff = aDstBuffer;
		int		nRest = aSize;
		do{
			nErr = Receive(m_nSocketHandle, tTimeout, pBuff, nRest);
			if (m_bCancel) {
				break;
			}
			if (m_pBaseInst->m_bForceClose == true)
				break;
			if (nErr > 0)
			{
				pBuff += nErr;
				nRest -= nErr;
			}
		} while (nErr > 0);

		return aSize - nRest;
	}

protected:
	unsigned long	m_ipAddr[256];
	int				m_nCount;
};

CDNSCache::CDNSCache(CBaseInst * pBaseInst)
	: CBaseObject (pBaseInst)
	, m_pThreadWork(NULL)
	, m_pHTTPClient(NULL)
	, m_pDNSLookup(NULL)
{
	SetObjectName("CDNSCache");

	if (m_pBaseInst != NULL)
		m_pBaseInst->AddListener(this);
}

CDNSCache::~CDNSCache()
{
	Release ();
	if (m_pThreadWork != NULL)
	{
		m_pThreadWork->Stop();
		delete m_pThreadWork;
		m_pThreadWork = NULL;
	}
	QC_DEL_P(m_pHTTPClient);
	QC_DEL_P(m_pDNSLookup);
	if (m_pBaseInst != NULL)
		m_pBaseInst->RemListener(this);
}

int CDNSCache::Get(char* pHostName, void * pHostIP)
{
	PDNSNode	pFind = NULL;
	PDNSNode	pNode = NULL;

	CAutoLock	lock(&m_mtLock);
	NODEPOS pos = m_lstNode.GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_lstNode.GetNext(pos);
		if (pNode->pHostName && strcmp(pNode->pHostName, pHostName) == 0)
		{
			pFind = pNode;
			break;
		}
	}

	if (pFind == NULL)
	{
		void * pIPAddr = NULL;
		if (qcHostIsIPAddr(pHostName))
		{
			unsigned long uIPAddr = qcGetIPAddrFromString(pHostName);
			if (uIPAddr > 0)
			{
				int nSize = qcGetIPAddrFromValue(uIPAddr, &pIPAddr);
				if (pIPAddr != NULL)
				{
					struct addrinfo * pAddrInfo = (addrinfo *)pIPAddr;
					Add(pHostName, pAddrInfo->ai_addr, pAddrInfo->ai_addrlen, 999999);
					qcFreeIPAddr(pIPAddr);
				}
			}
		}
		if (pIPAddr == NULL)
		{
            if(qcIsIPv6())
            {
                QCLOGI("Device works on IPv6");
                return QC_ERR_UNSUPPORT;
            }
            
			int nRC = QC_ERR_NONE;
			int nDNSType = qcGetDNSType(m_pBaseInst->m_pSetting->g_qcs_szDNSServerName);
			if (nDNSType == QC_DNS_FROM_HTTP)
			{
                nRC = UpdateHTTPDNS_IP(pHostName, NULL);
                if (nRC != QC_ERR_NONE)
                    nRC = UpdateUDPDNS_IP(pHostName, NULL);
                //if (nRC != QC_ERR_NONE)
                //    nRC = UpdateSYSDNS_IP(pHostName, NULL);
			}
			else if (nDNSType == QC_DNS_FROM_UDP)
			{
				nRC = UpdateUDPDNS_IP(pHostName, NULL);
				if (nRC != QC_ERR_NONE)
					//nRC = UpdateSYSDNS_IP(pHostName, NULL);
					nRC = UpdateHTTPDNS_IP(pHostName, NULL);

			}
			else
			{
				nRC = UpdateSYSDNS_IP(pHostName, NULL);
				if (nRC != QC_ERR_NONE)
					nRC = UpdateUDPDNS_IP(pHostName, NULL);
				if (nRC != QC_ERR_NONE)
					nRC = UpdateHTTPDNS_IP(pHostName, NULL);
			}
            
            if(m_pBaseInst->m_bForceClose)
                return QC_ERR_STATUS;
#ifdef QCDNS_USE_THREAD
			AddCheckIP(pHostName, nDNSType, false);
			Start();
#endif // QCDNS_USE_THREAD
		}
	}

	pFind = NULL;
	pos = m_lstNode.GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_lstNode.GetNext(pos);
		if (pNode->pHostName && strcmp(pNode->pHostName, pHostName) == 0)
		{
			if (pFind == NULL)
				pFind = pNode;
			else if (pFind->nConnectTime > pNode->nConnectTime)
				pFind = pNode;
		}
	}
	if (pFind == NULL)
		return QC_ERR_FAILED;
	
//	int nSize = sizeof(struct sockaddr);
	memcpy(pHostIP, pFind->pAddress, pFind->nAddrSize);// sizeof(struct sockaddr_storage));
//	memcpy(pHostIP, pFind->pAddress, sizeof(struct sockaddr_storage));

	return QC_ERR_NONE;
}

int CDNSCache::Add (char* pHostName, void * pAddress, unsigned int pAddressSize, int nConnectTime)
{
	CAutoLock	lock(&m_mtLock);
	if (pHostName == NULL || strlen(pHostName) == 0 || pAddress == NULL)
		return QC_ERR_FAILED;

	PDNSNode	pNode = NULL;
	NODEPOS		pos = m_lstNode.GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_lstNode.GetNext(pos);
		if (pNode->pHostName && strcmp(pNode->pHostName, pHostName) == 0)
		{
			if (pNode->nAddrSize == pAddressSize && memcmp(pNode->pAddress, pAddress, pAddressSize) == 0)
			{
				pNode->nConnectTime = nConnectTime;
				pNode->nUpdateTime = qcGetSysTime();
				return QC_ERR_NONE;
			}
		}
	}

	pNode = new DNSNode;
	pNode->pHostName = new char[strlen(pHostName) + 1];
	strcpy(pNode->pHostName, pHostName);
	pNode->pAddress = new char[pAddressSize];
	memcpy(pNode->pAddress, pAddress, pAddressSize);
	pNode->nAddrSize = pAddressSize;
	pNode->nConnectTime = nConnectTime;
	pNode->nUpdateTime = qcGetSysTime();
	m_lstNode.AddTail(pNode);

	return QC_ERR_NONE;
}

int CDNSCache::Del(char* pHostName, void * pAddress, unsigned int pAddressSize)
{
	CAutoLock	lock(&m_mtLock);
	PDNSNode	pNode = NULL;
	NODEPOS		pos = m_lstNode.GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_lstNode.GetNext(pos);
		if (pNode->pHostName && strcmp(pNode->pHostName, pHostName) == 0)
		{
			if (pNode->nAddrSize == pAddressSize && memcmp(pNode->pAddress, pAddress, pAddressSize) == 0)
			{
				m_lstNode.Remove(pNode);
				m_lstNodeFree.AddTail(pNode);
			}
		}
	}

	return QC_ERR_NONE;
}

void CDNSCache::Clear(void)
{
	CAutoLock	lock(&m_mtLock);
	PDNSNode pNode = m_lstNode.RemoveHead();
	while (pNode != NULL)
	{
		m_lstNodeFree.AddTail(pNode);
		pNode = m_lstNode.RemoveHead();
	}

	PDNSHostIP pHost = m_lstHostIP.RemoveHead();
	while (pHost != NULL)
	{
		m_lstHostIPFree.AddTail(pHost);
		pHost = m_lstHostIP.RemoveHead();
	}
}

void CDNSCache::Release(void)
{
	CAutoLock	lock(&m_mtLock);
	Clear();
	PDNSNode pNode = m_lstNodeFree.RemoveHead();
	while (pNode != NULL)
	{
		QC_DEL_A(pNode->pHostName);
		QC_DEL_A(pNode->pAddress);
		delete pNode;
		pNode = m_lstNodeFree.RemoveHead();
	}

	PDNSHostIP pHost = m_lstHostIPFree.RemoveHead();
	while (pHost != NULL)
	{
		QC_DEL_A(pHost->pHostName);
		delete pHost;
		pHost = m_lstHostIPFree.RemoveHead();
	}
}

int	CDNSCache::RecvEvent(int nEventID)
{
	if (nEventID == QC_BASEINST_EVENT_NETCHANGE)
		Clear();

	return QC_ERR_NONE;
}

int CDNSCache::AddCheckIP(char * pHostName, int nIP, bool bUpdateNow)
{
	CAutoLock	lock(&m_mtLock);

	DNSHostIP * pNode = NULL;
	NODEPOS pos = m_lstHostIP.GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_lstHostIP.GetNext(pos);
		if (strcmp(pNode->pHostName, pHostName) == 0 && pNode->nIPAddr == nIP)
			return QC_ERR_NONE;
	}

	pNode = new DNSHostIP();
	pNode->pHostName = new char[strlen(pHostName) + 1];
	strcpy(pNode->pHostName, pHostName);
	pNode->nIPAddr = nIP;
	pNode->nAddTime = qcGetSysTime();
	if (!bUpdateNow)
		pNode->nCheckTime = qcGetSysTime();

	m_lstHostIP.AddTail(pNode);

#ifndef QCDNS_USE_THREAD
	sockaddr skAddr;
	qcFillSockAddr(&skAddr, nIP);
	Add(pHostName, &skAddr, sizeof(sockaddr), 100);
#endif // QCDNS_USE_THREAD

	return QC_ERR_NONE;
}

int	CDNSCache::Start(void)
{
#ifdef QCDNS_USE_THREAD
	if (m_pThreadWork == NULL)
	{
		m_pThreadWork = new CThreadWork(m_pBaseInst);
		m_pThreadWork->SetOwner(m_szObjName);
		m_pThreadWork->SetWorkProc(this, &CThreadFunc::OnWork);
		m_pThreadWork->SetStartStopFunc(&CThreadFunc::OnStart, &CThreadFunc::OnStop);
	}
	m_pThreadWork->Start();
#endif // QCDNS_USE_THREAD
	return QC_ERR_NONE;
}

int	CDNSCache::OnWorkItem(void)
{
	int			nKeepTime = 10 * 60 * 1000; // ten minutes
	DNSHostIP * pNode = NULL;

	m_mtLock.Lock();
	NODEPOS pos = m_lstHostIP.GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_lstHostIP.GetNext(pos);
		if (pNode->nCheckTime > 0 && qcGetSysTime() - pNode->nCheckTime < nKeepTime)
			continue;
		pNode->nCheckTime = qcGetSysTime();

		if (pNode->nIPAddr == QC_DNS_FROM_HTTP)
			UpdateHTTPDNS_IP(pNode->pHostName, pNode);
		else if (pNode->nIPAddr == QC_DNS_FROM_UDP)
			UpdateUDPDNS_IP(pNode->pHostName, pNode);
		//else if (pNode->nIPAddr == QC_DNS_FROM_SYS)
		//	UpdateSYSDNS_IP(pNode->pHostName, pNode);
		else if (pNode->nIPAddr != QC_DNS_FROM_NONE)
			UpdateIP_Time(pNode);
		break;
	}
	m_mtLock.Unlock();

	int nTryTimes = 0;
	while (nTryTimes < 100)
	{
		qcSleep(5000);
		if (m_pThreadWork->GetStatus() != QCWORK_Run || m_pBaseInst->m_bForceClose)
			break;
		nTryTimes++;
	}
	
	return QC_ERR_NONE;
}

int	CDNSCache::UpdateHTTPDNS_IP(char * pHostName, PDNSHostIP pNode)
{
    if(m_pBaseInst->m_bForceClose)
        return QC_ERR_STATUS;
	if (m_pDNSLookup == NULL)
		m_pDNSLookup = new CHTTPDNSLookup(m_pBaseInst);
	unsigned long * ipAddr = NULL;
	int				nSize = 0;
	int nRC = m_pDNSLookup->GetIPAddress(pHostName, &ipAddr, &nSize);
	if (nRC == QC_ERR_NONE && ipAddr != NULL)
	{
		sockaddr skAddr;
		int		 nConnectTime = 10;
		for (int i = 0; i < nSize; i++)
		{
			qcFillSockAddr(&skAddr, ipAddr[i]);
			Add(pHostName, &skAddr, sizeof(sockaddr), nConnectTime * (i + 1));
		}
	}
	else
	{
		if (pNode != NULL)
			pNode->nIPAddr = QC_DNS_FROM_UDP;
	}

	return nRC;
}

int	CDNSCache::UpdateUDPDNS_IP(char * pHostName, PDNSHostIP pNode)
{
    if(m_pBaseInst->m_bForceClose)
        return QC_ERR_STATUS;
	void * pResult = NULL;
	int nRC = m_pBaseInst->m_pDNSLookup->GetDNSAddrInfo(pHostName, NULL, NULL, &pResult, 2000);
	if (pResult != NULL)
	{
		m_pBaseInst->m_pDNSLookup->FreeDNSAddrInfo(pResult);
		if (pNode != NULL)
			pNode->nIPAddr = QC_DNS_FROM_NONE;
	}

	return nRC;
}

int	CDNSCache::UpdateSYSDNS_IP(char * pHostName, PDNSHostIP pNode)
{
    if(m_pBaseInst->m_bForceClose)
        return QC_ERR_STATUS;
	struct addrinfo hints;
	struct addrinfo *res = NULL;
	int ret;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC; /* Allow IPv4 & IPv6 */
	hints.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo(pHostName, NULL, &hints, &res);
	if (ret == 0 && res != NULL)
	{
        // use sockaddr_storage to support both IPv4 and IPv6
		Add(pHostName, (void *)res->ai_addr, sizeof(struct sockaddr_storage), 999999);
		freeaddrinfo(res);
        return QC_ERR_NONE;
	}

	return QC_ERR_FAILED;
}

int	CDNSCache::UpdateIP_Time(PDNSHostIP pNode)
{
	if (m_pHTTPClient == NULL)
	{
		m_pHTTPClient = new CHTTPClient(m_pBaseInst, this);
		m_pHTTPClient->SetNotifyMsg(false);
	}

	char szIP[16] = { '\0' };
	unsigned char *pbyIPSegment = (unsigned char*)(&pNode->nIPAddr);
	sprintf(szIP, "%d.%d.%d.%d", pbyIPSegment[0], pbyIPSegment[1], pbyIPSegment[2], pbyIPSegment[3]);

	sockaddr ipAddr;
	qcFillSockAddr(&ipAddr, pNode->nIPAddr);
	int nPort = 80;
	int nStartTime = qcGetSysTime();
	int nRC = m_pHTTPClient->ConnectServer((QCIPAddr)&ipAddr, nPort, 2000);
	if (nRC == QC_ERR_NONE)
	{
		int nConnectTime = qcGetSysTime() - nStartTime;
		Add(pNode->pHostName, &ipAddr, sizeof(sockaddr), nConnectTime);
		m_pHTTPClient->Disconnect();
		QCLOGI("The Host %s of IP %s connect time is %d", pNode->pHostName, szIP, nConnectTime);
	}
	else
	{
		QCLOGI("The Host %s of IP %s connect failed is 0X%08X", pNode->pHostName, szIP, nRC);
	}
	return nRC;
}

int	CDNSCache::DetectHost(char * pHostName)
{
	if (m_pBaseInst == NULL || pHostName == NULL)
		return QC_ERR_STATUS;

	if (qcHostIsIPAddr(pHostName))
		return QC_ERR_ARG;

	int nDNSType = qcGetDNSType(m_pBaseInst->m_pSetting->g_qcs_szDNSServerName);
#ifdef QCDNS_USE_THREAD
	AddCheckIP(pHostName, nDNSType, true);
	Start();
#else
	if (nDNSType == QC_DNS_FROM_HTTP)
		UpdateHTTPDNS_IP(pHostName, NULL);
	else if (nDNSType == QC_DNS_FROM_UDP)
		UpdateUDPDNS_IP(pHostName, NULL);
	else if (nDNSType == QC_DNS_FROM_SYS)
		UpdateSYSDNS_IP(pHostName, NULL);
#endif // QCDNS_USE_THREAD

	return QC_ERR_NONE;
}

int	qcFillSockAddr(void * pAddrInfo, unsigned long nIP)
{
	sockaddr * hResult = (sockaddr *)pAddrInfo;
	memset(hResult, 0, sizeof(sockaddr));

#ifdef __QC_OS_IOS__
	hResult->sa_len = sizeof(sockaddr);
#endif // __QC_OS_WIN32__
	hResult->sa_family = AF_INET;
	hResult->sa_data[2] = (nIP & 0XFF);
	hResult->sa_data[3] = (nIP >> 8) & 0XFF;
	hResult->sa_data[4] = (nIP >> 16) & 0XFF;
	hResult->sa_data[5] = (nIP >> 24) & 0XFF;

	return QC_ERR_NONE;
}
