/**
* File : TTCondition.h  
* Created on : 2014-9-1
* Author : yongping.lin
* Description : TTCondition定义文件
*/
#ifndef __TT__CONDITION__H__
#define __TT__CONDITION__H__

#include <pthread.h>
#include "GKTypedef.h"
#include "GKMacrodef.h"
#include "GKCritical.h"

class RTTCondition
{
public:
	/**
	* \fn				            RTTCondition()
	* \brief				        构造函数
	*/
	RTTCondition();

	/**
	* \fn						    ~RTTCondition()
	* \brief						析构函数
	*/
	virtual ~RTTCondition();

	/**
	* \fn							TTInt Create();
	* \brief						创建环境变量
	* \return						状态
	*/
	TTInt							Create();
	
	/**
	* \fn							TTInt Wait();
	* \brief						等待操作
	* \return						状态
	*/
	TTInt							Wait(RGKCritical& Mutex);

	/**
	* \fn							TTInt Wait(TTUint32 aTimeOutUs);
	* \brief						延时一段时间();
	* \param[in]	aTimeOut_Msec	延时数，毫秒
	* \return						状态
	*/
	TTInt							Wait(RGKCritical& Mutex, TTUint32 aTimeOutMs);

	/**
	* \fn							TTInt Signal();
	* \brief						增加量加一操作
	* \return						状态
	*/
	TTInt							Signal();
	
	/**
	* \fn							TTInt Destroy();
	* \brief						关闭信号量
	* \return						状态
	*/
	TTInt							Destroy();

private:
	void							GetAbsTime(struct timespec &aAbsTime, TTUint32 aTimeoutUs);

private:
	TTBool							iAlreadyExisted;
	pthread_cond_t					iCondition;
	pthread_mutex_t					iMutex;
};


#endif  //__TT__CONDITION__H__
