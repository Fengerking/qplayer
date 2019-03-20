/*******************************************************************************
	File:		CNDKSendBuff.h

	Contains:	The ndk send data header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-07-17		Bangfei			Create file

*******************************************************************************/
#ifndef __CNDKSendBuff_H__
#define __CNDKSendBuff_H__

#include "CBaseObject.h"

#include "jni.h"

class CNDKSendBuff : public CBaseObject
{
public:
	CNDKSendBuff (CBaseInst * pBaseInst);
	virtual ~CNDKSendBuff(void);

	virtual int		SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer);

	virtual int		OnStop (void);

	virtual int		SendBuff (QC_DATA_BUFF * pBuff);

protected:
	JavaVM *			m_pjVM;
	jclass     			m_pjCls;
	jobject				m_pjObj;
	jmethodID			m_fPushAudio;
	jmethodID			m_fPushVideo;

	JNIEnv *			m_pEnv;
	jbyteArray			m_pDataBuff;
	int					m_nDataSize;
	int					m_nBuffSize;

	QC_VIDEO_BUFF *     m_pVideoBuff;
};

#endif // __CNDKSendBuff_H__
