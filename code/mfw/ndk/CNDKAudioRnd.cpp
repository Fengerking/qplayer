/*******************************************************************************
	File:		CNDKAudioRnd.cpp

	Contains:	The ndk audio render implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-12		Bangfei			Create file

*******************************************************************************/
#include "dlfcn.h"

#include "qcErr.h"
#include "qcMsg.h"
#include "CNDKAudioRnd.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

CNDKAudioRnd::CNDKAudioRnd(CBaseInst * pBaseInst, void * hInst)
	: CBaseAudioRnd (pBaseInst, hInst)
	, m_pjVM (NULL)
	, m_pjCls (NULL)
	, m_pjObj (NULL)
	, m_fPostEvent (NULL)
	, m_fPushAudio (NULL)
	, m_pEnv (NULL)
	, m_pDataBuff (NULL)
	, m_nDataSize (0)
	, m_nBuffSize (0)
	, m_bFormatUpdate (false)
{
	SetObjectName ("CNDKAudioRnd");
}

CNDKAudioRnd::~CNDKAudioRnd(void)
{
	Uninit ();
}

int CNDKAudioRnd::SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer)
{
	m_pjVM = jvm;
	m_pjCls = clsPlayer;
	m_pjObj = objPlayer;
	
	m_fPostEvent = env->GetStaticMethodID (m_pjCls, "postEventFromNative", "(Ljava/lang/Object;IIILjava/lang/Object;)V");
	m_fPushAudio = env->GetStaticMethodID (m_pjCls, "audioDataFromNative", "(Ljava/lang/Object;[BIJ)V");

	QCLOGI ("m_fPostEvent = %p, m_fPushAudio = %p", m_fPostEvent, m_fPushAudio);

	return QC_ERR_NONE;
}

int CNDKAudioRnd::Init (QC_AUDIO_FORMAT * pFmt, bool bAudioOnly)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;
	CBaseAudioRnd::Init (pFmt, bAudioOnly);

	if (m_fmtAudio.nChannels == pFmt->nChannels && m_fmtAudio.nSampleRate == pFmt->nSampleRate)
		return QC_ERR_NONE;

	if (pFmt->nBits == 0)
		pFmt->nBits = 16;
	m_fmtAudio.nChannels = pFmt->nChannels;
	m_fmtAudio.nSampleRate = pFmt->nSampleRate;
	m_fmtAudio.nBits = pFmt->nBits;
	if (m_fmtAudio.nChannels > 2)
		m_fmtAudio.nChannels = 2;

	m_nSizeBySec = m_fmtAudio.nSampleRate * m_fmtAudio.nChannels * m_fmtAudio.nBits / 8;
	m_bFormatUpdate = false;

	QCLOGI ("Init audio format % 8d X % 8d, size = %d", pFmt->nSampleRate, pFmt->nChannels, m_nSizeBySec);	

	return QC_ERR_NONE;
}

int	CNDKAudioRnd::OnStop (void)
{
	QCLOGI ("OnStop!");
	if (m_pjVM == NULL)
		return QC_ERR_NONE;

	if (m_pEnv == NULL)
		m_pjVM->AttachCurrentThread (&m_pEnv, NULL);
//	UpdateFormat (m_pEnv, NULL);
	if (m_pDataBuff != NULL)
		m_pEnv->DeleteLocalRef(m_pDataBuff);
	m_pDataBuff = NULL;		
	m_nBuffSize = 0;
	m_pjVM->DetachCurrentThread ();		
	m_pEnv = NULL;

	return QC_ERR_NONE;
}

int CNDKAudioRnd::Render (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || pBuff->pBuff == NULL)
		return QC_ERR_ARG;
	CBaseAudioRnd::Render (pBuff);

	if (m_pEnv == NULL)
		m_pjVM->AttachCurrentThread (&m_pEnv, NULL);
	
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT || m_fmtAudio.nSampleRate == 0)
	{			
		QC_AUDIO_FORMAT * pFmtAudio = (QC_AUDIO_FORMAT *)pBuff->pFormat;
		Init (pFmtAudio, m_bAudioOnly);
		UpdateFormat (m_pEnv, pFmtAudio);
	}
	else if (m_pDataBuff == NULL)
	{
		UpdateFormat (m_pEnv, &m_fmtAudio);
	}

	if (m_nSizeBySec > m_nBuffSize * 2 || m_pDataBuff == NULL)
	{
		if (m_pDataBuff != NULL)
			m_pEnv->DeleteLocalRef(m_pDataBuff);
		m_nBuffSize = m_nSizeBySec/ 2;
		m_nDataSize = 0;
		m_pDataBuff = m_pEnv->NewByteArray(m_nBuffSize);	
	}	
	if (pBuff->uSize > m_nBuffSize)
	{
		QCLOGW ("The buffer size is too large. %d", pBuff->uSize);
		return QC_ERR_ARG;
	}

	jbyte* pData = m_pEnv->GetByteArrayElements(m_pDataBuff, NULL);
	m_nDataSize = pBuff->uSize;
	memcpy (pData, pBuff->pBuff, m_nDataSize); 
	m_pEnv->CallStaticVoidMethod(m_pjCls, m_fPushAudio, m_pjObj, m_pDataBuff, m_nDataSize, (int)pBuff->llTime);	
	
	m_pEnv->ReleaseByteArrayElements(m_pDataBuff, pData, 0);
	
	if (m_pClock != NULL)	
		m_pClock->SetTime (pBuff->llTime);

	m_nRndCount++;

	return QC_ERR_NONE;
}

int CNDKAudioRnd::UpdateFormat (JNIEnv * pEnv, QC_AUDIO_FORMAT * pFmt)
{
	if (pEnv == NULL || m_fPostEvent == NULL)
		return QC_ERR_ARG;

	if (m_bFormatUpdate)
		return QC_ERR_NONE;

	if (pFmt == NULL)
	{
		pEnv->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, 
									QC_MSG_SNKA_NEW_FORMAT, 0, 0, NULL);
		m_bFormatUpdate = false;	
		return QC_ERR_NONE;
	}

	QCLOGI ("New audio format % 8d X % 8d", pFmt->nSampleRate, pFmt->nChannels);
	pEnv->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, 
								QC_MSG_SNKA_NEW_FORMAT, pFmt->nSampleRate , pFmt->nChannels, NULL);	
	m_bFormatUpdate = true;

	return QC_ERR_NONE;
}

