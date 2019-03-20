/*******************************************************************************
	File:		CJniEnvUtil.cpp

	Contains:	The jni utility class file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-03-01		Bangfei			Create file

*******************************************************************************/
#include <stdio.h>

#include "CJniEnvUtil.h"
#include "ULogFunc.h"

CJniEnvUtil::CJniEnvUtil(JavaVM *pvm)
	: m_fNeedDetach(false)
	, mJavaVM(pvm)
	, m_pEnv(NULL)
{
	switch (mJavaVM->GetEnv((void**)&m_pEnv, JNI_VERSION_1_6))
	{ 
		case JNI_OK: 
			break; 
		case JNI_EDETACHED: 
			m_fNeedDetach = true;
			if (mJavaVM->AttachCurrentThread(&m_pEnv, NULL) != 0) { 
				QCLOGT ("CJniEnvUtil", "callback_handler: failed to attach current thread");
				break;
			} 			
			break; 
		case JNI_EVERSION: 
			QCLOGT ("CJniEnvUtil", "Invalid java version"); 
			break;
		}
}

CJniEnvUtil::~CJniEnvUtil()
{
	if (m_fNeedDetach) 
		 mJavaVM->DetachCurrentThread(); 
}
 
