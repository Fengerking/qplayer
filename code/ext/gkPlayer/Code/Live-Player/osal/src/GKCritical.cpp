/**
* File : GKCritical.cpp
* Created on : 2011-3-1
* Description : RGKCritical 实现文件
*/

// INCLUDES
#include "GKMacrodef.h"
#include "GKCritical.h"

RGKCritical::RGKCritical()
: iAlreadyExisted(ETTFalse)
{

}

RGKCritical::~RGKCritical()
{
	Destroy();
}

TTInt RGKCritical::Create()
{
	TTInt nErr = TTKErrNone;

	pthread_mutexattr_t mAttr; 
    pthread_mutexattr_init(&mAttr); 
    pthread_mutexattr_settype(&mAttr, PTHREAD_MUTEX_RECURSIVE); 
  	
	if (iAlreadyExisted)
	{
		nErr = TTKErrAlreadyExists;
	}
	else if ((nErr = pthread_mutex_init(&iMutex, &mAttr)) == TTKErrNone)
	{
        iAlreadyExisted = ETTTrue;		
	}

	pthread_mutexattr_destroy(&mAttr);

	return nErr;
}

TTInt RGKCritical::Lock()
{
	GKASSERT(iAlreadyExisted);
	return pthread_mutex_lock(&iMutex);
}

TTInt RGKCritical::TryLock()
{
	GKASSERT(iAlreadyExisted);
	return pthread_mutex_trylock(&iMutex);
}

TTInt RGKCritical::UnLock()
{	
	GKASSERT(iAlreadyExisted);
	return pthread_mutex_unlock(&iMutex);
}

TTInt RGKCritical::Destroy()
{
	TTInt nErr = TTKErrNone;
	if (iAlreadyExisted)
	{
		nErr = pthread_mutex_destroy(&iMutex);
		if (nErr == TTKErrNone)
		{
			iAlreadyExisted = ETTFalse;
		}
		else
		{
			GKASSERT(ETTFalse);
		}
	}

	return nErr;
}

pthread_mutex_t* RGKCritical::GetMutex()
{
	if (iAlreadyExisted)
	{
		return &iMutex;
	}

	return NULL;
}

//end of file
