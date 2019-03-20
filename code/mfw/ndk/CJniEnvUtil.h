/*******************************************************************************
	File:		CJniEnvUtil.h

	Contains:	The jni utility header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-03-01		Bangfei			Create file

*******************************************************************************/
#ifndef __CJniEnvUtil_H__
#define __CJniEnvUtil_H__

#include <jni.h>

class CJniEnvUtil
{
public:
	CJniEnvUtil(JavaVM *pvm);
	~CJniEnvUtil();
	
	JNIEnv* getEnv() { return m_pEnv; } 

protected:
	bool 			m_fNeedDetach;
	JavaVM 			*mJavaVM;
	JNIEnv 			*m_pEnv;
	
	CJniEnvUtil(const CJniEnvUtil&);
	CJniEnvUtil& operator=(const CJniEnvUtil&);	
};

#endif // __CJniEnvUtil_H__
