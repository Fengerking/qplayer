/*******************************************************************************
	File:		USocketFunc.cpp

	Contains:	The utility for socket implement file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-18		Bangfei			Create file

*******************************************************************************/
#ifdef __QC_OS_WIN32__
#include <iostream>
#include <winsock2.h>
#else
#include "unistd.h"
#include "netdb.h"
#include "fcntl.h"
#include "stdlib.h"
#endif // _WIN32
#include "qcErr.h"

#include "USocketFunc.h"
#include "ULibFunc.h"
#include "ULogFunc.h"

bool qcSocketInit (void)
{
#ifdef __QC_OS_WIN32__
	WSADATA wsaData;
	WORD	wVersionRequested = MAKEWORD( 2, 2 );
	int nErr = WSAStartup (wVersionRequested, &wsaData);
	if( nErr != 0 )
		return false;
#else
	signal(SIGPIPE, SIG_IGN);
#endif
	return true;
}

void qcSocketUninit (void)
{
#ifdef __QC_OS_WIN32__
	WSACleanup();
#endif
}

bool qcHostIsIPAddr(char * pHostName)
{
	char *	pChar = pHostName;
	bool	bIsIP = true;
	while (*pChar != 0)
	{
		if ((*pChar < '0' || *pChar > '9') && *pChar != '.')
		{
			bIsIP = false;
			break;
		}
		pChar++;
	}
	return bIsIP;
}

int	qcGetDNSType(char * pDNSServer)
{
	int nDNSType = QC_DNS_FROM_HTTP;
	if (pDNSServer == NULL || strlen (pDNSServer) == 0)
		return QC_DNS_FROM_HTTP;

	if (!strcmp(pDNSServer, "0.0.0.0"))
		return QC_DNS_FROM_HTTP;
		
	if (!strcmp(pDNSServer, "127.0.0.1"))
		return QC_DNS_FROM_SYS;

	return QC_DNS_FROM_UDP;
}

unsigned long qcGetIPAddrFromString(char * pIPAddr)
{
	if (!qcHostIsIPAddr(pIPAddr))
		return QC_ERR_FAILED;

	char szAddr[64];
	strcpy(szAddr, pIPAddr);

	unsigned long uIPAddr = 0;
	int	   nStep = 0;
	bool   bEnd = false;
	int	   nNum = 0;
	char * pPos = szAddr;
	char * pNum = pPos;
	while (*pPos != 0)
	{
		if (*pPos != '.')
		{
			pPos++;
			if (*pPos != 0)
				continue;
			else
				bEnd = true;
		}
		*pPos = 0;

		nNum = atoi(pNum) << nStep;
		nStep += 8;
		uIPAddr = uIPAddr + nNum;

		if (bEnd)
			break;
		pPos++;
		pNum = pPos;
	}

	return uIPAddr;
}

int	qcGetIPAddrFromValue(unsigned long uIPAddr, void ** ppAddr)
{
	if (ppAddr == NULL)
		return QC_ERR_ARG;
	*ppAddr = NULL;

	struct addrinfo * hResult = new addrinfo();
	memset(hResult, 0, sizeof(addrinfo));
	hResult->ai_addrlen = sizeof(sockaddr); //16;
	hResult->ai_addr = new sockaddr();
	memset(hResult->ai_addr, 0, sizeof(sockaddr));

	hResult->ai_family = AF_INET;
	hResult->ai_socktype = 1;
#ifdef __QC_OS_IOS__
	hResult->ai_addr->sa_len = sizeof(sockaddr);
#endif // __QC_OS_WIN32__
	hResult->ai_addr->sa_family = AF_INET;
	hResult->ai_addr->sa_data[2] = (uIPAddr & 0XFF);
	hResult->ai_addr->sa_data[3] = (uIPAddr >> 8) & 0XFF;
	hResult->ai_addr->sa_data[4] = (uIPAddr >> 16) & 0XFF;
	hResult->ai_addr->sa_data[5] = (uIPAddr >> 24) & 0XFF;

	*ppAddr = hResult;

	return QC_ERR_NONE;
}

int	qcFreeIPAddr(void * pIPAddr)
{
	if (pIPAddr == NULL)
		return QC_ERR_ARG;

	struct addrinfo * hInfo = (addrinfo *)pIPAddr;
	if (hInfo->ai_addr != NULL)
		delete hInfo->ai_addr;
	delete hInfo;

	return QC_ERR_NONE;
}


#ifdef __QC_OS_WIN32__
CQCInetNtop::CQCInetNtop(void)
{
	m_hWSDll = qcLibLoad("ws2_32", 0);
	if (m_hWSDll != NULL)
		m_fInetNtop = (QCINETNTOP)qcLibGetAddr(m_hWSDll, "inet_ntop", 0);
}

CQCInetNtop::~CQCInetNtop()
{
	if (m_hWSDll != NULL)
		qcLibFree(m_hWSDll, 0);
}

char * CQCInetNtop::qcInetNtop(int nFamily, void * pAddr, char * pStringBuf, int nStringBufSize)
{
	if (m_fInetNtop != NULL)
		return m_fInetNtop(nFamily, pAddr, pStringBuf, nStringBufSize);
	return NULL;
}
#endif // __QC_OS_WIN32__