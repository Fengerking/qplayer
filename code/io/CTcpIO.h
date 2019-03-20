/*******************************************************************************
	File:		CTcpIO.h
 
	Contains:
 
	Written by:	liangliang
 
	Change History (most recent first):
	12/13/16		liangliang			Create file
 
 *******************************************************************************/

#ifndef __CTcpIO_H__
#define __CTcpIO_H__

#include "CBaseIO.h"
#include "CUrl.h"
#include "CThreadWork.h"
#include "CMutexLock.h"

class CTcpIO : public CBaseIO, protected CThreadFunc
{
public:
    CTcpIO (void);
    virtual ~CTcpIO (void);
    
    virtual int 		Open (const char * pURL, int nFlag);
    virtual int 		Reconnect (const char * pNewURL);
    virtual int 		Close (void);
    
    virtual int			Read (unsigned char * pBuff, int nSize);
    virtual int			ReadAt (long long llPos, unsigned char * pBuff, int nSize);
    virtual int			Write (unsigned char * pBuff, int nSize);
    virtual int			GetSpeed (void);
    
    virtual int 		Run (void);
    virtual int 		Pause (void);
    virtual int 		Stop (void);
    
    virtual long long 	GetPos (void);
    
    virtual int 		GetParam (int nID, void * pParam);
    virtual int 		SetParam (int nID, void * pParam);
    
protected:
    int		            OnWorkItem (void * pParam);
    
private:
    int                 ReadableSize();
    int                 WriteableSize();
    
    QCIO_STATUS         m_status;
    int                 m_nError;
    int                 m_nFD;
    CUrl                m_url;
    CMutexLock *        m_pMutexLock;
    CThreadWork         m_ThreadWork;
    
    static const int    kBufferSize = 1024 * 1024;
    unsigned char       m_pBuffer[kBufferSize];
    int                 m_nRead;
    int                 m_nWrite;
    long long           m_llPos;
    
    long long           m_llBytesCount;
    int                 m_nPreTime;
    int                 m_nSpeed;
};

#endif /* __CTcpIO_H__ */
