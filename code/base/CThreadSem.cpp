/*******************************************************************************
	File:		CThreadSem.cpp

	Contains:	Thread Semaphore implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#include "CThreadSem.h"

#include "string.h"

#if defined __QC_OS_NDK || defined __QC_OS_IOS__
#include "sys/time.h"
#endif

CThreadSem::CThreadSem()
	: CBaseObject(NULL)
	, m_nSemCount (0)
	, m_bWaiting (false)
{
	SetObjectName ("CThreadSem");

#ifdef __QC_OS_WIN32__
	InitializeCriticalSection(&m_CritSec);
	m_hEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
#elif defined __QC_OS_NDK || defined __QC_OS_IOS__
	pthread_cond_init (&m_hCondition, NULL);
	pthread_mutex_init (&m_hMutex, NULL);
#endif // __QC_OS_WIN32__

}

CThreadSem::~CThreadSem()
{	
#ifdef __QC_OS_WIN32__
	CloseHandle (m_hEvent);
	DeleteCriticalSection(&m_CritSec);
#elif defined __QC_OS_NDK || defined __QC_OS_IOS__
	pthread_cond_destroy (&m_hCondition);
	pthread_mutex_destroy (&m_hMutex);	 
#endif // __QC_OS_WIN32__
}

int CThreadSem::Down (int nWaitTime)
{
	m_bWaiting = true;

#ifdef __QC_OS_WIN32__
	int uRC = 0;
	if (m_nSemCount == 0)
	{
		uRC = WaitForSingleObject (m_hEvent, nWaitTime);
		if (uRC == WAIT_TIMEOUT)
			return QC_SEM_TIMEOUT;
	}

	EnterCriticalSection(&m_CritSec);
	ResetEvent (m_hEvent);

	if (m_nSemCount > 0)
		m_nSemCount--;
	
	LeaveCriticalSection(&m_CritSec);
#elif defined __QC_OS_NDK || defined __QC_OS_IOS__
	pthread_mutex_lock (&m_hMutex);
	while (m_nSemCount == 0)
	{
		pthread_cond_wait (&m_hCondition, &m_hMutex);
		pthread_mutex_unlock (&m_hMutex);				
	}
	m_nSemCount--;
	pthread_mutex_unlock (&m_hMutex);	 
#endif // __QC_OS_WIN32__

	m_bWaiting = false;

	return 0;
}

int CThreadSem::Up (void)
{
 #ifdef __QC_OS_WIN32__
	EnterCriticalSection(&m_CritSec);

	m_nSemCount++;
	SetEvent (m_hEvent);

	LeaveCriticalSection(&m_CritSec);
#elif defined __QC_OS_NDK || defined __QC_OS_IOS__
	pthread_mutex_lock (&m_hMutex);
	m_nSemCount++;
	pthread_cond_signal(&m_hCondition);
	pthread_mutex_unlock (&m_hMutex);
#endif // __QC_OS_WIN32__

	return 0;
}

int CThreadSem::Reset(void)
{
#ifdef __QC_OS_WIN32__
	EnterCriticalSection(&m_CritSec);

	m_nSemCount = 0;
	ResetEvent (m_hEvent);

	LeaveCriticalSection(&m_CritSec);
#elif defined __QC_OS_NDK || defined __QC_OS_IOS__
	pthread_mutex_lock (&m_hMutex);
	m_nSemCount = 0;
	pthread_mutex_unlock (&m_hMutex);	 
#endif // __QC_OS_WIN32__

	return 0;
}

int CThreadSem::Wait (int nWaitTime)
{
	int ret = QC_SEM_OK;

#ifdef __QC_OS_WIN32__
	if (WaitForSingleObject(m_hEvent, nWaitTime) == WAIT_TIMEOUT)
		return QC_SEM_TIMEOUT;
	ResetEvent (m_hEvent);
#elif defined __QC_OS_NDK || defined __QC_OS_IOS__
	pthread_mutex_lock (&m_hMutex);
	pthread_cond_wait (&m_hCondition, &m_hMutex);
	pthread_mutex_unlock (&m_hMutex);			
#endif // __QC_OS_WIN32__

	return ret;
}

int CThreadSem::Signal (void)
{
#ifdef __QC_OS_WIN32__
	SetEvent (m_hEvent);
#elif defined __QC_OS_NDK || defined __QC_OS_IOS__
	pthread_mutex_lock (&m_hMutex);
	pthread_cond_signal (&m_hCondition);
	pthread_mutex_unlock (&m_hMutex);	 
#endif // __QC_OS_WIN32__

	return 0;
}

int CThreadSem::Broadcast (void)
{
#ifdef __QC_OS_WIN32__
	SetEvent (m_hEvent);
#elif defined __QC_OS_NDK || defined __QC_OS_IOS__
	pthread_mutex_lock (&m_hMutex);
	pthread_cond_broadcast (&m_hCondition);
	pthread_mutex_unlock (&m_hMutex);	 
#endif // __QC_OS_WIN32__

	return 0;
}

int CThreadSem::Count (void)
{
	return m_nSemCount;
}

bool CThreadSem::Waiting (void)
{
	return (bool)(m_bWaiting&&m_nSemCount <= 0);
}
