/*******************************************************************************
	File:		CNDKVideoRnd.cpp

	Contains:	The ndk Video render implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-12		Bangfei			Create file

*******************************************************************************/
#include <dlfcn.h>

#include "qcErr.h"
#include "qcMsg.h"
#include "qcPlayer.h"

#include "CNDKVideoRnd.h"
#include "ClConv.h"
#include "libyuv.h"

#include "ULogFunc.h"
#include "USystemFunc.h"

CNDKVideoRnd::CNDKVideoRnd(CBaseInst * pBaseInst, void * hInst)
	: CBaseVideoRnd (pBaseInst, hInst)
	, m_pjVM (NULL)
	, m_pjCls (NULL)
	, m_pjObj (NULL)
	, m_fPostEvent (NULL)
	, m_hAndroidDll (NULL)
	, m_pNativeWnd (NULL)		
	, m_fANativeWindow_fromSurface (NULL)
	, m_fANativeWindow_release (NULL)
	, m_fANativeWindow_setBuffersGeometry (NULL)
	, m_fANativeWindow_lock (NULL)
	, m_fANativeWindow_unlockAndPost (NULL)
	, m_nFormat (WINDOW_FORMAT_RGBA_8888)
	, m_nWidth (0)
	, m_nHeight (0)
	, m_bEventDone (false)
	, m_nFormatTime (0)
	, m_pSendBuff (NULL)
	, m_nInRender (0)
	, m_pLastVideo (NULL)
{
	SetObjectName ("CNDKVideoRnd");

	if (m_pBaseInst != NULL)
		m_pBaseInst->AddListener (this);
	m_bufRender.nType = QC_VDT_ARGB;
}

CNDKVideoRnd::~CNDKVideoRnd(void)
{
	Uninit ();

	ReleaseSurface ();

	QC_DEL_P (m_pSendBuff);	
}

int CNDKVideoRnd::SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer)
{
	m_pjVM = jvm;
	m_pjCls = clsPlayer;
	m_pjObj = objPlayer;

	if (m_pjCls != NULL && m_pjObj != NULL)
	{
		JNIEnv * pEnv = env;
		if (pEnv == NULL)
			m_pjVM->AttachCurrentThread(&pEnv, NULL);
		m_fPostEvent = pEnv->GetStaticMethodID(m_pjCls, "postEventFromNative", "(Ljava/lang/Object;IIILjava/lang/Object;)V");
		if (env == NULL)
			m_pjVM->DetachCurrentThread();
	}

	return QC_ERR_NONE;
}

int CNDKVideoRnd::SetSurface (JNIEnv* pEnv, jobject pSurface)
{
	CAutoLock lock (&m_mtDraw);
	QCLOGI ("the surface is %p", pSurface);
	if (pSurface == NULL)
	{
		ReleaseSurface ();
		return QC_ERR_NONE;
	}
	ReleaseSurface ();
	if (m_hAndroidDll == NULL)
	{
		m_hAndroidDll = dlopen("libandroid.so", RTLD_NOW);
		if (m_hAndroidDll != NULL) 
		{
			m_fANativeWindow_fromSurface = (ANativeWindow_fromSurface_t)dlsym(m_hAndroidDll, "ANativeWindow_fromSurface");
			m_fANativeWindow_release = (ANativeWindow_release_t)dlsym(m_hAndroidDll, "ANativeWindow_release");
			m_fANativeWindow_setBuffersGeometry = (ANativeWindow_setBuffersGeometry_t) dlsym(m_hAndroidDll, "ANativeWindow_setBuffersGeometry");
			m_fANativeWindow_lock = (ANativeWindow_lock_t) dlsym(m_hAndroidDll, "ANativeWindow_lock");
			m_fANativeWindow_unlockAndPost = (ANativeWindow_unlockAndPost_t)dlsym(m_hAndroidDll, "ANativeWindow_unlockAndPost");
			if (!m_fANativeWindow_fromSurface || !m_fANativeWindow_release || !m_fANativeWindow_setBuffersGeometry
				|| !m_fANativeWindow_lock || !m_fANativeWindow_unlockAndPost)
			{
				dlclose (m_hAndroidDll);
				m_hAndroidDll = NULL;
			}
		}
		else
		{
			QCLOGE ("The libandroid.so could not be loaded!");
		}
	}
	QCLOGI ("Set Surface: env %p, surface %p", pEnv, pSurface);
	if (m_pNativeWnd != NULL)
		m_fANativeWindow_release(m_pNativeWnd);
	m_pNativeWnd = (ANativeWindow *)m_fANativeWindow_fromSurface(pEnv, pSurface);
	if (m_pNativeWnd == NULL)
	{
		QCLOGE ("CNativeWndRender::ANativeWindow m_pNativeWnd = %p", m_pNativeWnd);
		return QC_ERR_FAILED;
	}
	if (m_nWidth > 0 && m_nHeight > 0)
		m_fANativeWindow_setBuffersGeometry(m_pNativeWnd, m_nWidth, m_nHeight, m_nFormat);		

	UpdateVideoView ();

	return QC_ERR_NONE;
}

void CNDKVideoRnd::SetEventDone (bool bEventDone) 
{
	m_bEventDone = bEventDone;
	QCLOGI ("Set event done %d", m_bEventDone);
}

int CNDKVideoRnd::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (CBaseVideoRnd::Init (pFmt) != QC_ERR_NONE)
		return QC_ERR_STATUS;

	m_nWidth = pFmt->nWidth;
	m_nHeight = pFmt->nHeight;
	if (m_nWidth == 0 || m_nHeight == 0)
		return QC_ERR_NONE;

	QCLOGI ("Init WXH %d %d,    %p", m_nWidth, m_nHeight, m_pNativeWnd);

	UpdateVideoSize (pFmt);

	if (m_pNativeWnd != NULL)
		m_fANativeWindow_setBuffersGeometry(m_pNativeWnd, m_nWidth, m_nHeight, m_nFormat);	

	return QC_ERR_NONE;
}

int CNDKVideoRnd::Uninit (void)
{
	CBaseVideoRnd::Uninit ();
	return QC_ERR_NONE;
}

int	CNDKVideoRnd::ReleaseSurface (void)
{
	CAutoLock lock (&m_mtDraw);	
	if (m_pNativeWnd != NULL)
		m_fANativeWindow_release(m_pNativeWnd);
	m_pNativeWnd = NULL;
	if (m_hAndroidDll != NULL)
		dlclose (m_hAndroidDll);
	m_hAndroidDll = NULL;	
	
	return QC_ERR_NONE;
}

int CNDKVideoRnd::Render (QC_DATA_BUFF * pBuff)
{
	CAutoLock lock (&m_mtDraw);
#ifdef __QC_LIB_ONE__
	m_fColorCvtR = qcColorCvtRotate;
#else
	if (m_fColorCvtR == NULL && m_pBaseInst != NULL && m_pBaseInst->m_hLibCodec != NULL)
		m_fColorCvtR = (QCCOLORCVTROTATE)qcLibGetAddr(m_pBaseInst->m_hLibCodec, "qcColorCvtRotate", 0);
#endif // __QC_LIB_ONE__
	CBaseVideoRnd::Render (pBuff);
	if (m_pNativeWnd == NULL)
		return QC_ERR_STATUS;
	
	if (pBuff->uBuffType != QC_BUFF_TYPE_Video)
		return QC_ERR_UNSUPPORT;

	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		if (pFmt != NULL && (m_nWidth != pFmt->nWidth || m_nHeight != pFmt->nHeight || m_fmtVideo.nNum != pFmt->nNum || m_fmtVideo.nDen != pFmt->nDen))
		{
			m_nFormatTime = qcGetSysTime ();
			Init (pFmt);
		}
	}

	if (m_nFormatTime > 0 && m_nRndCount == 0)
	{
		m_nFormatTime = qcGetSysTime () - m_nFormatTime;
		if (m_nFormatTime < 80)
		{
			m_nFormatTime = 80 - m_nFormatTime;
			qcSleep (m_nFormatTime * 1000);
			QCLOGI ("TestRender sleep %d", m_nFormatTime);
		}	
		m_nFormatTime = 0;
	}

	if (m_pSendBuff != NULL)
	{
		pBuff->nMediaType = QC_MEDIA_Video;
		m_pSendBuff->SendBuff (pBuff);	
		if (m_nInRender == 1)
		{
			m_nRndCount++;		
			return QC_ERR_NONE;
		}
	}

	int nRndWidth = m_nWidth;
	int nRndHeight = m_nHeight;
	int nRC = m_fANativeWindow_lock(m_pNativeWnd, (void*)&m_buffer, NULL);			
	if (nRC == 0) 
	{
		if (nRndWidth > m_buffer.width)
			nRndWidth = m_buffer.width;
		if (nRndHeight > m_buffer.height)
			nRndHeight = m_buffer.height;

		QC_VIDEO_BUFF *  pVideoBuff = (QC_VIDEO_BUFF *)pBuff->pBuffPtr;
		if (pVideoBuff->nType != QC_VDT_YUV420_P)
		{
			pVideoBuff = &m_bufVideo;
			if (pVideoBuff->nType != QC_VDT_YUV420_P)
			{
				nRC = m_fANativeWindow_unlockAndPost (m_pNativeWnd);				
				return QC_ERR_STATUS;
			}
		}	
		m_pLastVideo = pVideoBuff;	

		m_bufRender.nWidth = nRndWidth;
		m_bufRender.nHeight = nRndHeight;
		m_bufRender.pBuff[0] = (unsigned char *)m_buffer.bits;
		m_bufRender.nStride[0] = m_buffer.stride * 4;
		if (m_fColorCvtR != NULL)
			m_fColorCvtR(pVideoBuff, &m_bufRender, 0);
/*		
#ifdef _ARM_ARCH_NEON_
		colorConNxN_neon (pVideoBuff->pBuff[0], pVideoBuff->pBuff[1], pVideoBuff->pBuff[2], pVideoBuff->nStride[0],
							(unsigned char *)m_buffer.bits, m_buffer.stride * 4, nRndWidth, nRndHeight, pVideoBuff->nStride[1],pVideoBuff->nStride[2]);
#else
		colorConNxN_c (pVideoBuff->pBuff[0], pVideoBuff->pBuff[1], pVideoBuff->pBuff[2], pVideoBuff->nStride[0],
						(unsigned char *)m_buffer.bits, m_buffer.stride * 4, nRndWidth, nRndHeight, pVideoBuff->nStride[1],pVideoBuff->nStride[2]);
#endif // _ARM_ARCH_NEON_

		nRC = libyuv::I420ToARGB(m_pLastVideo->pBuff[0], m_pLastVideo->nStride[0], m_pLastVideo->pBuff[2], m_pLastVideo->nStride[2], m_pLastVideo->pBuff[1], m_pLastVideo->nStride[1],
								(unsigned char *)m_buffer.bits, m_buffer.stride * 4, nRndWidth, nRndHeight);
*/
	}	
	else
	{
		QCLOGI ("Lock window buffer failed! return %08X", nRC);
	}	

	nRC = m_fANativeWindow_unlockAndPost (m_pNativeWnd);
	m_nRndCount++;

	return QC_ERR_NONE;
}

void CNDKVideoRnd::UpdateVideoSize (QC_VIDEO_FORMAT * pFmtVideo)
{
	if (m_fPostEvent == NULL || pFmtVideo == NULL)
		return;

	m_rcView.top = 0;
	m_rcView.left = 0;
	m_rcView.right = m_fmtVideo.nWidth;
	m_rcView.bottom = m_fmtVideo.nHeight;	

	CBaseVideoRnd::UpdateRenderSize ();
	int nWidth = m_rcRender.right - m_rcRender.left;
	int nHeight = m_rcRender.bottom - m_rcRender.top;

	QCLOGI ("Update Video Size: %d X %d  Ratio: %d : %d", pFmtVideo->nWidth, pFmtVideo->nHeight, nWidth, nHeight);

	if (m_fPostEvent != NULL)
	{
		m_bEventDone = false;

		JNIEnv * pEnv = NULL;
		m_pjVM->AttachCurrentThread(&pEnv, NULL);
		pEnv->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, QC_MSG_SNKV_NEW_FORMAT, nWidth, nHeight, NULL);
		m_pjVM->DetachCurrentThread();

		int nTryTimes = 0;
		while (!m_bEventDone)
		{
			qcSleep(2000);
			nTryTimes++;
			if (nTryTimes >= 100)
				break;
		}
		QCLOGI("The m_bEventDone is %d", m_bEventDone);
	}
}

int	CNDKVideoRnd::OnStart(void)
{
	int nRC = CBaseVideoRnd::OnStart();
	return nRC;
}

int	CNDKVideoRnd::OnStop(void)
{
	int nRC = CBaseVideoRnd::OnStop();
	if (m_pSendBuff != NULL)
		m_pSendBuff->OnStop ();
	m_pLastVideo = NULL;
	return nRC;
}

int CNDKVideoRnd::SetParam(JNIEnv * pEnv, int nID, void * pParam)
{
	if (nID == QCPLAY_PID_SendOut_VideoBuff && m_pBaseInst != NULL)
	{
		if (m_pSendBuff == NULL)
		{
			m_pSendBuff = new CNDKSendBuff (m_pBaseInst);
			m_pSendBuff->SetNDK (m_pjVM, pEnv, m_pjCls, m_pjObj);
		}	
		m_nInRender = *(int*)pParam;
		return QC_ERR_NONE; 
	}
	else if (nID == QCPLAY_PID_EXT_SOURCE_DATA)
	{
		m_nJavaThread = qcThreadGetCurrentID ();
	}
	return QC_ERR_FAILED;
}

void CNDKVideoRnd::UpdateVideoView (void)
{
	if (m_bPlay || m_pLastVideo == NULL || m_pNativeWnd == NULL)
		return;
	
	int nRndWidth = m_nWidth;
	int nRndHeight = m_nHeight;
	int nRC = m_fANativeWindow_lock(m_pNativeWnd, (void*)&m_buffer, NULL);			
	if (nRC == 0) 
	{
		if (nRndWidth > m_buffer.width)
			nRndWidth = m_buffer.width;
		if (nRndHeight > m_buffer.height)
			nRndHeight = m_buffer.height;

		m_bufRender.nWidth = nRndWidth;
		m_bufRender.nHeight = nRndHeight;
		m_bufRender.pBuff[0] = (unsigned char *)m_buffer.bits;
		m_bufRender.nStride[0] = m_buffer.stride * 4;
		if (m_fColorCvtR != NULL)
			m_fColorCvtR(m_pLastVideo, &m_bufRender, 0);
/*
#ifdef _ARM_ARCH_NEON_
		colorConNxN_neon (m_pLastVideo->pBuff[0], m_pLastVideo->pBuff[1], m_pLastVideo->pBuff[2], m_pLastVideo->nStride[0],
							(unsigned char *)m_buffer.bits, m_buffer.stride * 4, nRndWidth, nRndHeight, m_pLastVideo->nStride[1],m_pLastVideo->nStride[2]);
#else
		colorConNxN_c (m_pLastVideo->pBuff[0], m_pLastVideo->pBuff[1], m_pLastVideo->pBuff[2], m_pLastVideo->nStride[0],
						(unsigned char *)m_buffer.bits, m_buffer.stride * 4, nRndWidth, nRndHeight, m_pLastVideo->nStride[1],m_pLastVideo->nStride[2]);
#endif // _ARM_ARCH_NEON_

		nRC = libyuv::I420ToARGB(m_pLastVideo->pBuff[0], m_pLastVideo->nStride[0], m_pLastVideo->pBuff[2], m_pLastVideo->nStride[2], m_pLastVideo->pBuff[1], m_pLastVideo->nStride[1],
								(unsigned char *)m_buffer.bits, m_buffer.stride * 4, nRndWidth, nRndHeight);
*/								
	}	
	else
	{
		QCLOGI ("Lock window buffer failed! return %08X", nRC);
	}	

	nRC = m_fANativeWindow_unlockAndPost (m_pNativeWnd);
}

int CNDKVideoRnd::RecvEvent(int nEventID)
{
	if (nEventID == QC_BASEINST_EVENT_NEWFORMAT_V && m_pBaseInst != NULL)
	{
		if (m_nRndCount > 0)
			return QC_ERR_NONE;
		if (qcThreadGetCurrentID () == m_nJavaThread)	
			return QC_ERR_NONE;		
		
		QC_VIDEO_FORMAT fmtVideo;
		memset (&fmtVideo, 0, sizeof (QC_VIDEO_FORMAT));
		fmtVideo.nWidth = m_pBaseInst->m_nVideoWidth;
		fmtVideo.nHeight = m_pBaseInst->m_nVideoHeight;
		m_nFormatTime = qcGetSysTime ();

		QCLOGI ("TestRender  %d X %d", fmtVideo.nWidth, fmtVideo.nHeight);
		Init (&fmtVideo);
	}
	return QC_ERR_NONE;
}

int	CNDKVideoRnd::ReleaseRnd (void)
{
	if (m_pBaseInst != NULL)
		m_pBaseInst->RemListener (this);
	return QC_ERR_NONE;
}