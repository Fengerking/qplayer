/*******************************************************************************
	File:		CNDKVideoRnd.h

	Contains:	The ndk Video render header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-12		Bangfei			Create file

*******************************************************************************/
#ifndef __CNDKVideoRnd_H__
#define __CNDKVideoRnd_H__
#include "./android/native_window_jni.h"
#include "./android/native_window.h"

#include "CNDKSendBuff.h"
#include "CBaseVideoRnd.h"

#define	WINDOW_FORMAT_YUV_YV12		0x32315659
#define	WINDOW_FORMAT_YUV_NV16		0x10
#define	WINDOW_FORMAT_YUV_NV12		0x11
#define	WINDOW_FORMAT_YUV_YUY2		0x14

typedef void* (*ANativeWindow_fromSurface_t)		(JNIEnv *env, jobject surface);
typedef void  (*ANativeWindow_release_t)			(void *window);
typedef int   (*ANativeWindow_setBuffersGeometry_t)	(void *window, int width, int height, int format);
typedef int   (* ANativeWindow_lock_t)				(void *window, void *outBuffer, void *inOutDirtyBounds);
typedef int   (* ANativeWindow_unlockAndPost_t)		(void *window);

class CNDKVideoRnd : public CBaseVideoRnd
{
public:
	CNDKVideoRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~CNDKVideoRnd(void);

	virtual int		SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer);
	virtual int		SetSurface (JNIEnv* pEnv, jobject pSurface);
	virtual void	SetEventDone (bool bEventDone);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Render (QC_DATA_BUFF * pBuff);

	virtual int		RecvEvent(int nEventID);
	virtual int		ReleaseRnd (void);
	
	virtual int		OnStart(void);
	virtual int		OnStop(void);
	virtual int 	SetParam(JNIEnv * pEnv, int nID, void * pParam);

protected:
	virtual void	UpdateVideoSize (QC_VIDEO_FORMAT * pFmtVideo);
	virtual void	UpdateVideoView (void);
	
	virtual int		ReleaseSurface (void);

protected:
	JavaVM *							m_pjVM;
	jclass     							m_pjCls;
	jobject								m_pjObj;
	jmethodID							m_fPostEvent;

	void * 								m_hAndroidDll;	
	ANativeWindow *						m_pNativeWnd;
	
	ANativeWindow_fromSurface_t			m_fANativeWindow_fromSurface;
	ANativeWindow_release_t 			m_fANativeWindow_release;
	ANativeWindow_setBuffersGeometry_t 	m_fANativeWindow_setBuffersGeometry;
	ANativeWindow_lock_t 				m_fANativeWindow_lock;
	ANativeWindow_unlockAndPost_t 		m_fANativeWindow_unlockAndPost;	
	
	ANativeWindow_Buffer 				m_buffer;	
	
	int									m_nFormat;
	int									m_nWidth;
	int									m_nHeight;
	bool								m_bEventDone;
	int									m_nFormatTime;

	CNDKSendBuff *						m_pSendBuff;
	int									m_nInRender;

	QC_VIDEO_BUFF *						m_pLastVideo;
	int									m_nJavaThread;
};

#endif // __CNDKVideoRnd_H__
