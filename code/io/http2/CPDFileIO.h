/*******************************************************************************
	File:		CPDFileIO.h

	Contains:	pd file io header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-19		Bangfei			Create file

*******************************************************************************/
#ifndef __CPDFileIO_H__
#define __CPDFileIO_H__

#include "CBaseIO.h"
#include "CDNSCache.h"

#include "CHTTPClient.h"
#include "CThreadWork.h"

#include "CFileIO.h"
#include "CHTTPIO2.h"
#include "CPDData.h"
#include "CNodeList.h"

class CPDFileIO : public CBaseIO, public CThreadFunc
{
public:
	CPDFileIO(CBaseInst * pBaseInst);
    virtual ~CPDFileIO(void);

	virtual int 		Open (const char * pURL, long long llOffset, int nFlag);
	virtual int 		Close (void);

	virtual int 		Run (void);
	virtual int 		Pause (void);
	virtual int 		Stop (void);

	virtual int			Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual int			ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual int			Write (unsigned char * pBuff, int nSize, long long llPos = -1);
	virtual int			GetSpeed (int nLastSecs);
	virtual long long 	SetPos (long long llPos, int nFlag);
	virtual long long 	GetDownPos(void);


	virtual QCIOType	GetType (void);

	virtual int 		GetParam (int nID, void * pParam);
	virtual int 		SetParam (int nID, void * pParam);

protected:
	virtual int			DoOpen (void);

	virtual int			OnWorkItem (void);
	virtual int			OnStartFunc(void);

	CSpeedItem *		GetLastSpeedItem (void);

protected:
	CDNSCache *				m_pDNSCache;
	CHTTPClient *			m_pHttpData;
	CPDData *				m_pPDData;
	int						m_nReadSize;

	char *					m_pBuffData;
	int						m_nBuffSize;

	bool					m_bSetNewPos;
	bool					m_bConnected;
	int						m_nRecntTime;

	CMutexLock				m_mtLock;
	CMutexLock				m_mtLockHttp;
	CThreadWork *			m_pThreadWork;
	
	CMutexLock				m_mtSpeed;
	CObjectList<CSpeedItem>	m_lstSpeed;
	CSpeedItem *			m_pSpeedItem;
	int						m_nSpeedNotify[32];

	int						m_nDLPercent;

	// For test dump
	CFileIO *				m_pFile;
};

#endif // __CPDFileIO_H__