/*******************************************************************************
	File:		CMediaCodecDec.h

	Contains:	The media codec dec header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-03-01		Bangfei			Create file

*******************************************************************************/
#ifndef __CMediaCodecDec_H__
#define __CMediaCodecDec_H__
#include "qcData.h"

#include "CBaseClock.h"
#include "CMutexLock.h"
#include "CBaseObject.h"

#include "jni.h"

class CMediaCodecDec : public CBaseObject
{
public:
	CMediaCodecDec(CBaseInst * pBaseInst, void * hInst, int nOSVer);
	virtual ~CMediaCodecDec(void);

	virtual int			SetSurface (JavaVM * pJVM, JNIEnv* pEnv, jobject pSurface);	
	virtual int			Init (QC_VIDEO_FORMAT * pFmt);

	virtual int			SetInputBuff (QC_DATA_BUFF * pBuff);
	virtual int			SetInputEmpty (void);	
	virtual int			RenderOutput (long long * pOutTime, bool bRender);
	virtual int			RenderRestBuff (JNIEnv* pEnv);

	virtual int			OnStart (JNIEnv * pEnv);
	virtual int			OnStop (JNIEnv * pEnv);

	QC_VIDEO_FORMAT * 	GetVideoFormat (void) {return &m_fmtVideo;}

protected:
	virtual int						CreateVideoDec (void);
	virtual int						ReleaseVideoDec (JNIEnv* pEnv);

	virtual int						UpdateBufferFunc();
	virtual int						UpdateBuffers();
	virtual int						SetHeadData(unsigned char *pBuf, int nLen);
	virtual int						SetHeadDataJava(unsigned char *pBuf, int nLen, int index);

	virtual	int						Start();
	virtual	int						Stop(JNIEnv* pEnv);
	virtual	int						Flush();

	virtual int						IsSupportAdpater(jstring jstr);
	virtual int						ReleaseSurface (JNIEnv* pEnv);
	virtual int						ResetParam (void);

protected:
	CMutexLock							m_mtDec;
	QC_VIDEO_FORMAT						m_fmtVideo;
	bool								m_bCreateDecFailed;

	long long							m_llLastTime;
	int									m_nFrameTime;

	bool								m_bStarted;
	bool								m_bNewStart;
	bool								m_bAdaptivePlayback;
	int									m_nOSVer;

	JavaVM *							m_pJVM;
	JNIEnv *							m_pEnv;	
	jobject								m_objSurface;
	jobject								m_objMediaCodec;
    jobject								m_objBufferInfo;
	jobject								m_objVideoFormat;
    jobjectArray						m_objInputBuffers;
	jobjectArray						m_objOutputBuffers;


    jclass								m_clsMediaCodec;
    jclass								m_clsMediaFormat;
    jclass								m_clsBufferInfo; 
	jclass								m_clsByteBuffer;
    
	jmethodID							m_mhdToString;
    jmethodID							m_mhdCreateByCodecType;
	jmethodID							m_mhdConfigure;
	jmethodID							m_mhdStart;
	jmethodID							m_mhdStop;
	jmethodID							m_mhdFlush;
	jmethodID							m_mhdRelease;
    jmethodID							m_mhdGetOutputFormat;
    jmethodID							m_mhdGetInputBuffers;
    jmethodID							m_mhdGetOutputBuffers;
    jmethodID							m_mhdDequeueInputBuffer;
	jmethodID							m_mhdDequeueOutputBuffer;
	jmethodID							m_mhdQueueInputBuffer;
    jmethodID							m_mhdReleaseOutputBuffer;
    jmethodID							m_mhdCreateVideoFormat;
	jmethodID							m_mhdSetInteger;
	jmethodID							m_mhdSetByteBuffer;
	jmethodID							m_mhdGetInteger;
    jmethodID							m_mhdBufferInfoConstructor;
    jfieldID							m_mhdSizeField;
	jfieldID							m_mhdOffsetField;
	jfieldID							m_mhdPtsField;		
};

#endif // __CmediaCodecDec_H__
