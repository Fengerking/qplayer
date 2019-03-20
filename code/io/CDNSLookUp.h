/*******************************************************************************
File:		CDNSLookUp.h

Contains:	http client header file.

Written by:	Bangfei Jin

Change History (most recent first):
2017-01-06		Bangfei			Create file

*******************************************************************************/
#ifndef __CDNSLookUp_H__
#define __CDNSLookUp_H__
#ifdef __QC_OS_WIN32__
#include <windows.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "pthread.h"
#endif // __QC_OS_WIN32__

#include "CBaseObject.h"
#include "CMutexLock.h"
#include "CNodeList.h"

#define MAX_DOMAINNAME_LEN  255
#define DNS_PORT            53
#define DNS_TYPE_SIZE       2
#define DNS_CLASS_SIZE      2
#define DNS_TTL_SIZE        4
#define DNS_DATALEN_SIZE    2
#define DNS_TYPE_A          0x0001 //1 a host address
#define DNS_TYPE_CNAME      0x0005 //5 the canonical name for an alias
#define DNS_PACKET_MAX_SIZE (sizeof(DNSHeader) + MAX_DOMAINNAME_LEN + DNS_TYPE_SIZE + DNS_CLASS_SIZE)

struct DNSHeader
{
	unsigned short usTransID;			//��ʶ��
	unsigned short usFlags;				//���ֱ�־λ
	unsigned short usQuestionCount;		//Question�ֶθ��� 
	unsigned short usAnswerCount;		//Answer�ֶθ���
	unsigned short usAuthorityCount;	//Authority�ֶθ���
	unsigned short usAdditionalCount;	//Additional�ֶθ���
};

class CDNSLookup : public CBaseObject
{
public:
	CDNSLookup(CBaseInst * pBaseInst);
	~CDNSLookup(void);

	int				UpdateDNSServer(void);

	int				GetDNSAddrInfo(char *szDomainName, void * pDevice, void * pInt, void ** ppResult, unsigned long ulTimeout = 10000);
	int				FreeDNSAddrInfo(void * pAddr);


private:
	int				DNSLookup(char *szDomainName, unsigned long ulTimeout = 10000);
	int				GetCount(void);
	unsigned long	GetIPAdr(int nIndex);
	const char *	GetIPStr(int nIndex);
	const char *	GetIPName(int nIndex);	
	
	int				DNSLookupCore (char *szDomainName, unsigned long ulTimeout);
	int				SendDNSRequest(sockaddr_in sockAddrDNSServer, char *szDomainName);
	int				RecvDNSResponse(sockaddr_in sockAddrDNSServer, unsigned long ulTimeout);
	bool			EncodeDotStr(char *szDotStr, char *szEncodedStr, unsigned short nEncodedStrSize);
	bool			DecodeDotStr(char *szEncodedStr, unsigned short *pusEncodedStrLen, char *szDotStr, unsigned short nDotStrSize, char *szPacketStartPos = NULL);

	bool			Init();
	bool			UnInit();
	int				WaitSocketReadBuffer(timeval& aTimeOut);

	void			FreeList(void);

private:
	unsigned long				m_ulDNSServerIP;
	char *						m_pExtDNSServer;
	char						m_szDNSServer[64];

	CObjectList<unsigned long>	m_lstIPNum;
	CObjectList<char>			m_lstName;
	CObjectList<char>			m_lstIPTxt;
/*	
	std::vector<unsigned long>	m_vIPList;
	std::vector<std::string>	m_vNameList;
	std::vector<std::string>	m_vIPStrList;
*/
	unsigned long				m_uTimeSpent;

	bool						m_bIsInitOK;
	int							m_sock;
	unsigned short				m_usCurrentProcID;
	char *						m_szDNSPacket;

	CMutexLock					m_mtLock;

};

#endif // __CDNSLookUp_H__
