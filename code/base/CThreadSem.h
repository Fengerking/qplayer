/*******************************************************************************
	File:		CThreadSem.h

	Contains:	Thread Semaphore header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#ifndef __CThreadSem_H__
#define __CThreadSem_H__

#ifdef __QC_OS_WIN32__
#include <windows.h>
#elif defined __QC_OS_NDK || defined __QC_OS_IOS__
#include <pthread.h>
#include <time.h>
#endif // __QC_OS_WIN32__

#include "CBaseObject.h"

#define QC_SEM_TIMEOUT		0X80000001
#define QC_SEM_MAXTIME		0XFFFFFFFF
#define QC_SEM_OK			0X00000000

// wrapper for whatever critical section we have
class CThreadSem : public CBaseObject
{
public:
	CThreadSem(void);
    virtual ~CThreadSem(void);

    virtual int		Down (int nWaitTime = QC_SEM_MAXTIME);
    virtual int		Up (void);
    virtual int		Reset(void);
    virtual int		Wait (int nWaitTime = QC_SEM_MAXTIME);
    virtual int		Signal (void);
    virtual int		Broadcast (void);
	virtual int		Count (void);
	virtual bool	Waiting (void);

#ifdef __QC_OS_WIN32__
	CRITICAL_SECTION	m_CritSec;
	HANDLE				m_hEvent;
#elif defined __QC_OS_NDK || defined __QC_OS_IOS__
  pthread_cond_t		m_hCondition;
  pthread_mutex_t		m_hMutex;	
#endif // __QC_OS_WIN32__

public:
	int					m_nSemCount;
	bool				m_bWaiting;
};

#endif //__CThreadSem_H__
