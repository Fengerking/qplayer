/*******************************************************************************
	File:		CRTMPIO.h

	Contains:	RTMP io header file.

	Written by:	Jun Lin

	Change History (most recent first):
	2017-04-15		Jun Lin			Create file

*******************************************************************************/
#ifndef __CRTMPIO_H__
#define __CRTMPIO_H__

#include "CBaseIO.h"
#include "USocketFunc.h"
#include "CMutexLock.h"

struct qcRTMP;
struct qcRTMP_IPAddrInfo;
class CDNSLookup;

class CRTMPIO : public CBaseIO
{
public:
	CRTMPIO(CBaseInst * pBaseInst);
    virtual ~CRTMPIO(void);

	virtual int 		Open (const char * pURL, long long llOffset, int nFlag);
	virtual int 		Reconnect (const char * pNewURL, long long llOffset);
	virtual int 		Close (void);
    virtual int 		Stop (void);

	virtual int			Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual int			ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
    virtual int         GetParam (int nID, void * pParam);

	virtual QCIOType	GetType (void);
    virtual int			RecvEvent(int nEventID);
    
private:
    static int 			GetAddrInfo (void* pUserData, const char* pszHostName, const char* pszService,
                                   const struct addrinfo* pHints, struct addrinfo** ppResult);
    static int          FreeAddrInfo (void* pUserData, struct addrinfo* pResult);
    static void*        GetCache (void* pUserData, char* pszHostName);
    static int          AddCache (void* pUserData, char* pHostName, void * pAddress, unsigned int pAddressSize, int nConnectTime);
    int					ResolveIP(void* pAddr, bool bFromCache);
    bool                IsUseDNSLookup();

public:
    int            		doGetAddrInfo (const char* pszHostName, const char* pszService,
                                        const struct addrinfo* pHints, struct addrinfo** ppResult);
    int          		doFreeAddrInfo (struct addrinfo* pResult);
    void*        		doGetCache (char* pszHostName);
    int          		doAddCache (char* pHostName, void * pAddress, unsigned int pAddressSize, int nConnectTime);

private:
    qcRTMP*	m_hHandle;
    bool	m_bFirstByte;
    int		m_nReadTime;
    int 	m_nDownloadSize;
    bool	m_bReconnectting;
    bool	m_bConnected;
    long long m_llLastAudioMsgTime;
    long long m_llLastVideoMsgTime;
    
    qcRTMP_IPAddrInfo*  m_pAddrInfo;
    CDNSLookup* 		m_pDNSLookUp;
	struct sockaddr *	m_sHostIP;

#ifdef __QC_OS_WIN32__
    CQCInetNtop *    m_pInetNtop;
#endif // __QC_OS_WIN32__

    CMutexLock	m_mtLockFunc;
};

#endif // __CRTMPIO_H__
