/*******************************************************************************
	File:		CHTTPIO2.h

	Contains:	http io header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-06		Bangfei			Create file

*******************************************************************************/
#ifndef __CHTTPIO2_H__
#define __CHTTPIO2_H__

#include "CBaseIO.h"
#include "CDNSCache.h"

#include "CHTTPClient.h"
#include "CMemFile.h"
#include "CThreadWork.h"

#include "CFileIO.h"
#include "CNodeList.h"

class CSpeedItem
{
public:
	CSpeedItem (void);
	virtual ~CSpeedItem (void);

public:
	int		m_nStartTime;
	int		m_nUsedTime;
	int		m_nSize;
};

class CHTTPIO2 : public CBaseIO, public CThreadFunc
{
public:
	CHTTPIO2(CBaseInst * pBaseInst);
    virtual ~CHTTPIO2(void);

	virtual int 		Open (const char * pURL, long long llOffset, int nFlag);
	virtual int 		Reconnect (const char * pNewURL, long long llOffset);
	virtual int 		Close (void);

	virtual int 		Run (void);
	virtual int 		Pause (void);
	virtual int 		Stop (void);

	virtual int			Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual int			ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual int			Write (unsigned char * pBuff, int nSize, long long llPos = -1);
	virtual int			GetSpeed (int nLastSecs);
	virtual long long 	SetPos (long long llPos, int nFlag);

	virtual QCIOType	GetType (void);

	virtual int 		GetParam (int nID, void * pParam);
	virtual int 		SetParam (int nID, void * pParam);

protected:
	virtual int			OnWorkItem (void);
	CSpeedItem *		GetLastSpeedItem (void);

	virtual int			OpenURL(void);
	virtual int			CopyOtherMem(void * pHTTPIO);

protected:
	CDNSCache *				m_pDNSCache;
	CHTTPClient *			m_pHttpData;
	CMemFile *				m_pMemData;
	char *					m_pBuffData;
	int						m_nBuffSize;

	bool					m_bNotifyMsg;
	bool					m_bOpenCache;
	bool					m_bConnected;
	bool					m_bReconnect;
	int						m_nRecntTime;
	bool					m_bConnectFailed;
	int						m_nMaxBuffSize;

	CMutexLock				m_mtLockFunc;
	CMutexLock				m_mtLock;
	CMutexLock				m_mtLockHttp;
	CThreadWork *			m_pThreadWork;
	
	CMutexLock				m_mtSpeed;
	CObjectList<CSpeedItem>	m_lstSpeed;
	CSpeedItem *			m_pSpeedItem;
	int						m_nSpeedNotify[32];

	int						m_nDLPercent;

	CHTTPIO2 *				m_pIOCache;
	unsigned char *			m_pHeadBuff;
	long long				m_llHeadSize;
	unsigned char *			m_pMoovBuff;
	long long				m_llMoovPos;
	int						m_nMoovSize;

	// for IO cache
	int						m_nCacheSize;

	// For test dump
	CFileIO *				m_pFile;
};

#endif // __CHTTPIO2_H__