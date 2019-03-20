/*******************************************************************************
	File:		CDNSCache.h

	Contains:	DNS cache header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-06		Bangfei			Create file

*******************************************************************************/
#ifndef __CDNSCache_H__
#define __CDNSCache_H__
#include "CBaseObject.h"

#include "CMutexLock.h"
#include "CNodeList.h"
#include "CThreadWork.h"

#ifdef __QC_NAMESPACE__
namespace __QC_NAMESPACE__ {
#endif

class CHTTPClient;
class CHTTPDNSLookup;

typedef struct DNSNode
{
    char*		pHostName;
    char *      pAddress; //适配IPv4 & IPv6地址，指向struct sockaddr
	int			nAddrSize;
	int			nConnectTime;
	int			nUpdateTime;
    DNSNode()
    {
        pHostName = NULL;
        pAddress = NULL;
		nAddrSize = 0;
		nConnectTime = -1;
		nUpdateTime = 0;
    }
} DNSNode, *PDNSNode;

typedef struct DNSHostIP
{
	char *			pHostName;
	unsigned long	nIPAddr;
	int				nAddTime;
	int				nCheckTime;
	DNSHostIP()
	{
		pHostName = NULL;
		nIPAddr = 0;
		nAddTime = 0;
		nCheckTime = 0;
	}
} DNSHostIP, *PDNSHostIP;

class CDNSCache : public CBaseObject, public CThreadFunc	
{
public:
	CDNSCache(CBaseInst * pBaseInst);
	virtual ~CDNSCache();

	virtual int		Add (char* pHostName, void * pAddress, unsigned int pAddressSize, int nConnectTime);
	virtual int		Del (char* pHostName, void * pAddress, unsigned int pAddressSize);
	virtual int		Get (char* pHostName, void * pHostIP);
	virtual void	Clear(void);
	virtual void	Release(void);

	virtual int		RecvEvent(int nEventID);

	virtual int		AddCheckIP (char * pHostName, int nIP, bool bUpdateNow);
	virtual int		Start(void);
	virtual int		DetectHost(char * pHostName);

protected:
	virtual int		OnWorkItem(void);
	CThreadWork *	m_pThreadWork;

	virtual int		UpdateHTTPDNS_IP(char * pHostName, PDNSHostIP pNode);
	virtual int		UpdateUDPDNS_IP(char * pHostName, PDNSHostIP pNode);
	virtual int		UpdateSYSDNS_IP(char * pHostName, PDNSHostIP pNode);
	virtual int		UpdateIP_Time(PDNSHostIP pNode);

protected:
	CMutexLock				m_mtLock;
	CObjectList<DNSNode>	m_lstNode;
	CObjectList<DNSNode>	m_lstNodeFree;
	CObjectList<DNSHostIP>	m_lstHostIP;
	CObjectList<DNSHostIP>	m_lstHostIPFree;

	CHTTPClient *			m_pHTTPClient;
	CHTTPDNSLookup *		m_pDNSLookup;
};

int		qcFillSockAddr(void * pAddrInfo, unsigned long nIP);

#ifdef __QC_NAMESPACE__
}// end of namespace __QC_NAMESPACE__
#endif

#endif // __CDNSCache_H__
