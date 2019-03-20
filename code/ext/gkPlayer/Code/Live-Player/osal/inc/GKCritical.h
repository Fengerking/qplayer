/**
* File : GKCritical.h
* Created on : 2011-3-1
* Description : RGKCritical定义文件
*/

#ifndef __GK_CRITICAL_H__
#define __GK_CRITICAL_H__

// INCLUDES
#include <pthread.h>
#include "GKTypedef.h"

// CLASSES DECLEARATION
class RGKCritical
{
public:

	/**
	* \fn                       RGKCritical()
	* \brief                    构造函数
	*/
	RGKCritical();

	/**
	* \fn                       ~RGKCritical()
	* \brief                    析构函数
	*/
	virtual ~RGKCritical();

public:

	/**
	* \fn                       TTInt Create()
	* \brief                    创造互斥量
	* \return					操作状态
	*/	
	TTInt						Create();

	/**
	* \fn                       void Lock()
	* \brief                    进入临界区函数
	* \return					操作状态
	*/	
	TTInt						Lock();

	/**
	* \fn                       void Lock()
	* \brief                    尝试进入临界区
	* \return					操作状态，0表示成功，需要Unlock
	*/
	TTInt						TryLock();

	/**
	* \fn                       void UnLock()
	* \brief                    释放临界区
	* \return					操作状态
	*/	
	TTInt						UnLock();

	/**
	* \fn                       TTInt Destroy()
	* \brief                    删除互斥量
	*/	
	TTInt					    Destroy();

	/**
	* \fn                       TTInt GetMutex()
	* \brief                    获得互斥量
	*/	
	pthread_mutex_t*			GetMutex();
	
private:

	pthread_mutex_t				iMutex;				/**< 互斥量*/
	TTBool						iAlreadyExisted;	/**< 互斥量是否已经存在*/
};


// locks a critical section, and unlocks it automatically
// when the lock goes out of scope
class GKCAutoLock
{
protected:
    RGKCritical * mLock;

public:
    GKCAutoLock(RGKCritical *plock)
    {
        mLock = plock;
        if (mLock) {
            mLock->Lock();
        }
    };

    ~GKCAutoLock()
	{
        if (mLock) {
            mLock->UnLock();
        }
    };
};

#endif
