/*******************************************************************************
	File:		CNDKVDecRnd.cpp

	Contains:	The ndk video decoder and render implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-04		Bangfei			Create file

*******************************************************************************/
#include <stdlib.h>
#include "qcErr.h"
#include "qcMsg.h"

#include "CNDKVDecRnd.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

CNDKVDecRnd::CNDKVDecRnd(CBaseInst * pBaseInst, void * hInst)
	: CBaseVideoRnd (pBaseInst, hInst)
#ifdef __QC_OS_NDK__
	, m_pJVM (NULL)
	, m_pjCls (NULL)
	, m_pjObj (NULL)
	, m_fPostEvent (NULL)
	, m_fPushVideo (NULL)
	, m_pEnvUtil (NULL)	
	, m_pEnv (NULL)
	, m_pDataBuff (NULL)
	, m_pDec (NULL)
	, m_pSurface (NULL)
#endif // __QC_OS_NDK__
	, m_nVer (1)
	, m_fSend (NULL)
	, m_pUser (NULL)
	, m_nDataSize (0)
	, m_nBuffSize (0)
	, m_bEventDone (false)
{
	SetObjectName ("CNDKVDecRnd");
}

CNDKVDecRnd::~CNDKVDecRnd(void)
{
	Uninit ();
#ifdef __QC_OS_NDK__
	QC_DEL_P (m_pDec);
#endif // __QC_OS_NDK__
}

#ifdef __QC_OS_NDK__
int CNDKVDecRnd::SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer, int nVer)
{
	m_pJVM = jvm;
	m_pjCls = clsPlayer;
	m_pjObj = objPlayer;
	m_nVer = nVer;
	
	m_fPostEvent = env->GetStaticMethodID (m_pjCls, "postEventFromNative", "(Ljava/lang/Object;IIILjava/lang/Object;)V");
	m_fPushVideo = env->GetStaticMethodID (m_pjCls, "videoDataFromNative", "(Ljava/lang/Object;[BIJI)V");
	QCLOGI ("m_fPostEvent = %p, m_fPushVideo = %p", m_fPostEvent, m_fPushVideo);

	m_pDec = new CMediaCodecDec (m_pBaseInst, m_hInst, nVer);

	return QC_ERR_NONE;
}

int CNDKVDecRnd::SetSurface (JNIEnv* pEnv, jobject pSurface)
{
	m_pSurface = pSurface;
	if (m_pDec != NULL)
		m_pDec->SetSurface (m_pJVM, pEnv, pSurface);
	return QC_ERR_NONE;
}
#endif // __QC_OS_NDK__

int CNDKVDecRnd::SetSendFunc (QCPlayerDataSendOut fSend, void * pUserData)
{
	m_fSend = fSend;
	m_pUser = pUserData;
	return QC_ERR_NONE;
}

int CNDKVDecRnd::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;

	QCLOGI ("Init format %d X %d m_fmtVideo.nWidth = %d", pFmt->nWidth, pFmt->nHeight, m_fmtVideo.nWidth);
	if (pFmt->nWidth == 0 || pFmt->nHeight == 0)
		return QC_ERR_NONE;
	
	if (pFmt->nCodecID == QC_CODEC_ID_H265 && m_nVer < 5)
		return QC_ERR_FAILED;

#ifdef __QC_OS_NDK__
	if (m_pDec != NULL)
		m_pDec->Init (pFmt);
#endif // __QC_OS_NDK__

	CBaseVideoRnd::Init (pFmt);
		
	UpdateVideoSize (pFmt);		

	return QC_ERR_NONE;
}

void CNDKVDecRnd::SetNewSource (bool bNewSource)
{
	m_bNewSource = bNewSource;
}

int CNDKVDecRnd::Render (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || pBuff->pBuff == NULL)
		return QC_ERR_ARG;

	if (m_fmtVideo.nWidth == 0 && ((pBuff->uFlag & QCPLAY_BUFF_NEW_FORMAT) == QCPLAY_BUFF_NEW_FORMAT))
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		QCLOGI ("Render new format %d X %d ", pFmt->nWidth, pFmt->nHeight);		
		Init (pFmt);
	}		
	int nRC = CBaseVideoRnd::Render (pBuff);
//	if (pBuff->uFlag != 0)
//		QCLOGI ("Render Size = % 8d, Time = % 8d   uFlag = % 8x", pBuff->uSize, (int)pBuff->llTime, pBuff->uFlag);
	if (m_fSend != NULL)
		m_fSend (m_pUser, pBuff->pBuff, pBuff->uSize, pBuff->llTime, pBuff->uFlag);
#ifdef __QC_OS_NDK__
	if (m_pDec != NULL)
	{
		if (m_pSurface == NULL)
		{
			nRC = m_pDec->SetInputBuff (pBuff);
			return WaitRendTime (pBuff->llTime);	
		}	
		long long llTime = -1;
		if (m_nRndCount > 0 && ((pBuff->uFlag & QCPLAY_BUFF_NEW_FORMAT) == QCPLAY_BUFF_NEW_FORMAT))
		{
			if (m_pDec->SetInputEmpty () == QC_ERR_NONE)
			{
				while (true)
				{
					llTime = -1;
					nRC = m_pDec->RenderOutput (&llTime, true);
					QCLOGI ("Render the rest output buffers when end of stream. Return % 8d, Time: % 8lld", nRC, llTime);
					if (llTime > 0)
						WaitRendTime (llTime);
					else
						break;
					if (m_pBaseInst->m_bForceClose)
						break;
				}
			}
		}		

		if (m_bNewSource && ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS))
			m_bNewSource = false;
	
		nRC = m_pDec->SetInputBuff (pBuff);
		while (nRC != QC_ERR_NONE)
		{
			if (m_pBaseInst->m_bForceClose)
				break;
			qcSleep (5000);
			if (!m_bPlay)
				return QC_ERR_STATUS;			
			nRC = m_pDec->SetInputBuff (pBuff);		
		}
		nRC = m_pDec->RenderOutput (&llTime, true);
		if (nRC == QC_ERR_FORMAT)
		{
			QC_VIDEO_FORMAT * pFmtNew = m_pDec->GetVideoFormat ();
			if (pFmtNew->nWidth != m_fmtVideo.nWidth || pFmtNew->nHeight != m_fmtVideo.nHeight)
			{
				m_fmtVideo.nWidth = pFmtNew->nWidth;
				m_fmtVideo.nHeight = pFmtNew->nHeight;
				UpdateVideoSize (pFmtNew);	
			}
		}
		//QCLOGI ("Render output return % 8d, Time: % 8lld", nRC, llTime);
		if (llTime > 0)
			WaitRendTime (llTime);	
		// try render all output buffers
		if ((pBuff->uFlag & QCBUFF_EOS) == QCBUFF_EOS)
		{
			while (true)
			{
				llTime = -1;
				nRC = m_pDec->RenderOutput (&llTime, true);
				QCLOGI ("Render the rest output buffers when end of stream. Return % 8d, Time: % 8lld", nRC, llTime);
				if (llTime > 0)
					WaitRendTime (llTime);
				else
					break;
			}
		}
		return QC_ERR_NONE;
	}

	if (m_pEnv == NULL)
		return QC_ERR_STATUS;

	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{			
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		CBaseVideoRnd::Init (pFmt);
		UpdateVideoSize (pFmt);		
	}
	if (m_fmtVideo.nWidth * m_fmtVideo.nHeight > m_nBuffSize || m_pDataBuff == NULL)
	{
		if (m_pDataBuff != NULL)
			m_pEnv->DeleteLocalRef(m_pDataBuff);
		m_nBuffSize = m_fmtVideo.nWidth * m_fmtVideo.nHeight;
		m_nDataSize = 0;
		m_pDataBuff = m_pEnv->NewByteArray(m_nBuffSize);	
	}	
	int nBuffFlag = pBuff->uFlag;
	if ((pBuff->uFlag & QCBUFF_HEADDATA) == QCBUFF_HEADDATA)
	{
		if (m_fmtVideo.nCodecID == QC_CODEC_ID_H265)
		{
			nBuffFlag = nBuffFlag | 0X1000;
			pBuff->llTime = 0;
		}
		else
		{
			int nWorkNal = 0X01000000;
			for (int i = 8; i < pBuff->uSize; i++)
			{
				if (!memcmp (pBuff->pBuff + i, &nWorkNal, 4))
				{
					QCLOGI ("Buff size = % 8d,  Offset: % 8d", pBuff->uSize, i);
					pBuff->llTime = i;
					break;
				}
			}
		}
	}
	jbyte* pData = m_pEnv->GetByteArrayElements(m_pDataBuff, NULL);
	memcpy (pData + m_nDataSize, pBuff->pBuff, pBuff->uSize); 
	m_nDataSize = m_nDataSize + pBuff->uSize;
	m_pEnv->CallStaticVoidMethod(m_pjCls, m_fPushVideo, m_pjObj, m_pDataBuff, m_nDataSize, (int)pBuff->llTime, nBuffFlag);	
	m_nDataSize = 0;
	m_pEnv->ReleaseByteArrayElements(m_pDataBuff, pData, 0);
#endif //__QC_OS_NDK__
	return QC_ERR_NONE;
}

int CNDKVDecRnd::WaitRendTime (long long llTime)
{
	if (m_pExtClock == NULL)
		return QC_ERR_STATUS;
	m_nRndCount++;
	if (m_pExtClock->IsPaused ())
		m_pExtClock->Start ();	
	long long llPlayTime = m_pExtClock->GetTime ();
	while (llPlayTime < llTime)
	{
		if (abs ((int)(llTime - llPlayTime)) > 50000 && llPlayTime != 0)
		{
			qcSleep (30000);
			return QC_ERR_NONE;
		}
		qcSleep (2000);		
		llPlayTime = m_pExtClock->GetTime ();
		if (!m_bPlay)
			return -1;
		if (m_bNewSource)
			return -1;
		if (m_pBaseInst->m_bForceClose)
			break;
	}
	return 0;
}

void CNDKVDecRnd::SetEventDone (bool bEventDone) 
{
	m_bEventDone = bEventDone;
	QCLOGI ("Set event done %d", m_bEventDone);
}

void CNDKVDecRnd::UpdateVideoSize (QC_VIDEO_FORMAT * pFmtVideo)
{
#ifdef __QC_OS_NDK__
	if (m_fPostEvent == NULL || pFmtVideo == NULL)
		return;
		
	QCLOGI ("Update Video Size: %d X %d  Ratio: %d, %d", pFmtVideo->nWidth, pFmtVideo->nHeight, pFmtVideo->nNum, pFmtVideo->nDen);
	m_rcView.top = 0;
	m_rcView.left = 0;
	m_rcView.right = m_fmtVideo.nWidth;
	m_rcView.bottom = m_fmtVideo.nHeight;	

	CBaseVideoRnd::UpdateRenderSize ();
	int nWidth = m_rcRender.right - m_rcRender.left;
	int nHeight = m_rcRender.bottom - m_rcRender.top;

	QCLOGI ("Update display Size: %d X %d ", nWidth, nHeight);	
			
	m_bEventDone = false;

	JNIEnv * pEnv =	m_pEnv;
	if (m_pEnv == NULL)
		m_pJVM->AttachCurrentThread (&pEnv, NULL);
	pEnv->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, QC_MSG_SNKV_NEW_FORMAT, nWidth, nHeight, NULL);	
	if (m_pEnv == NULL)
		m_pJVM->DetachCurrentThread ();

	while (!m_bEventDone)
		qcSleep (2000);	
#endif // __QC_OS_NDK__
}

int	CNDKVDecRnd::OnStart (void)
{
	QCLOGI ("OnStart");
#ifdef __QC_OS_NDK__	
	if (m_pEnvUtil == NULL)
	{
		m_pEnvUtil = new CJniEnvUtil (m_pJVM);
		m_pEnv = m_pEnvUtil->getEnv ();
	}
	if (m_pDec != NULL)
		m_pDec->OnStart (m_pEnv);
#endif // __QC_OS_NDK__		
	return QC_ERR_NONE;
}

int	CNDKVDecRnd::OnStop (void)
{
	QCLOGI ("OnStop!");
#ifdef __QC_OS_NDK__
	if (m_pDec == NULL)
	{
		if (m_pEnv != NULL && m_pDataBuff != NULL)
			m_pEnv->DeleteLocalRef(m_pDataBuff);
		m_pDataBuff = NULL;		
	}
	else
	{
		m_pDec->OnStop (m_pEnv);		
	}
	QC_DEL_P (m_pEnvUtil);
	m_pEnv = NULL;
#endif // __QC_OS_NDK__
	return QC_ERR_NONE;
}
