/*******************************************************************************
	File:		CAudioTrack.cpp

	Contains:	The audio track implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2019-04-12		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "CAudioTrack.h"
#include "CJniEnvUtil.h"
#include "CMsgMng.h"

#include "ULogFunc.h"
#include "USystemFunc.h"

CAudioTrack::CAudioTrack(CBaseInst * pBaseInst, void * hInst)
	: CBaseAudioRnd (pBaseInst, hInst)
{
	SetObjectName ("CAudioTrack");
    memset (&m_fmtAudio, 0, sizeof (m_fmtAudio));
	ResetParam ();
}

CAudioTrack::~CAudioTrack(void) {
}

int CAudioTrack::SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer) {
	m_pjVM = jvm;
	m_pjCls = clsPlayer;
	m_pjObj = objPlayer;

	return QC_ERR_NONE;
}

int CAudioTrack::Init (QC_AUDIO_FORMAT * pFmt, bool bAudioOnly) {
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
	QCLOGI ("Init audio format % 8d X % 8d, size = %d", pFmt->nSampleRate, pFmt->nChannels, m_nSizeBySec);
    return QC_ERR_NONE;
}

int CAudioTrack::Render (QC_DATA_BUFF * pBuff) {
	if (pBuff == NULL || pBuff->pBuff == NULL)
		return QC_ERR_ARG;
	CBaseAudioRnd::Render (pBuff);

	if (m_pEnv == NULL)
		m_pjVM->AttachCurrentThread (&m_pEnv, NULL);    

	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT || m_fmtAudio.nSampleRate == 0) {			
		QC_AUDIO_FORMAT * pFmtAudio = (QC_AUDIO_FORMAT *)pBuff->pFormat;
		Init (pFmtAudio, m_bAudioOnly);
        ReleaseTrack (m_pEnv);
	}    

    if (m_objTrack == NULL)
        CreateTrack (m_pEnv);

    m_pEnv->SetByteArrayRegion (m_buffer, 0, pBuff->uSize, (jbyte*)(pBuff->pBuff));
    m_pEnv->CallIntMethod (m_objTrack, m_write, m_buffer, 0, pBuff->uSize);

	if (m_pClock != NULL)
		m_pClock->SetTime (pBuff->llTime - (m_nBuffSize * 1000) / m_nSizeBySec);

	m_nRndCount++;

    return QC_ERR_NONE;
}

int	CAudioTrack::OnStart (void) {
	if (m_pEnv == NULL)
		m_pjVM->AttachCurrentThread (&m_pEnv, NULL);    
    if (m_objTrack == NULL)
        CreateTrack (m_pEnv);         
    return QC_ERR_NONE;
}

int	CAudioTrack::OnStop (void) {
	QCLOGI ("OnStop!");
	if (m_pjVM == NULL)
		return QC_ERR_NONE;

	if (m_pEnv == NULL)
		m_pjVM->AttachCurrentThread (&m_pEnv, NULL);
    ReleaseTrack (m_pEnv);
	m_pjVM->DetachCurrentThread ();		
	m_pEnv = NULL;

	return QC_ERR_NONE;
}

int CAudioTrack::CreateTrack (JNIEnv * pEnv) {
	if (m_objTrack != NULL)
		return QC_ERR_NONE;
	QCLOGI("Try to Create the audio track."); 
	jclass lclass = pEnv->FindClass("android/media/AudioTrack");
	if(lclass == NULL){
		QCLOGI("can not find the android/media/AudioTrack class");
		if (m_pEnv->ExceptionOccurred()) {				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	m_clsTrack = (jclass)pEnv->NewGlobalRef(lclass);
	pEnv->DeleteLocalRef(lclass);
    int nChannelConfig = m_fmtAudio.nChannels == 1 ? 2 : 3;
    m_nBuffSize = 0;
    m_getMinBufferSize = GetMethod (pEnv, m_clsTrack, "getMinBufferSize", "(III)I", 1);
    if (m_getMinBufferSize != NULL) {
        m_nBuffSize = pEnv->CallStaticIntMethod (m_clsTrack, m_getMinBufferSize, m_fmtAudio.nSampleRate, nChannelConfig, 2);
        if (m_nBuffSize < 8192)
            m_nBuffSize = 8192;
    }

    m_initTrack = GetMethod (pEnv, m_clsTrack, "<init>", "(IIIIII)V", 0);
    if (m_initTrack == NULL)
        return QC_ERR_FAILED;

    m_objTrack = pEnv->NewObject (m_clsTrack, m_initTrack, 3, m_fmtAudio.nSampleRate, nChannelConfig, 2, m_nBuffSize, 1);
    if (m_objTrack == NULL)
        return QC_ERR_FAILED;

    m_play = GetMethod (pEnv, m_clsTrack, "play", "()V", 0);
    m_pause = GetMethod (pEnv, m_clsTrack, "pause", "()V", 0);
    m_stop = GetMethod (pEnv, m_clsTrack, "stop", "()V", 0);
    m_flush = GetMethod (pEnv, m_clsTrack, "flush", "()V", 0);
    m_release = GetMethod (pEnv, m_clsTrack, "release", "()V", 0);
    m_write = GetMethod (pEnv, m_clsTrack, "write", "([BII)I", 0);

    m_buffer = pEnv->NewByteArray(m_nBuffSize);

    if (m_play != NULL) 
        pEnv->CallVoidMethod (m_objTrack, m_play);

	QCLOGI("Create the audio track! BufferSize = %d", m_nBuffSize); 

    return QC_ERR_NONE;
}

int CAudioTrack::ReleaseTrack (JNIEnv * pEnv) {
    if (m_objTrack == NULL)
        return QC_ERR_NONE;

    if (m_stop!= NULL) 
        pEnv->CallVoidMethod (m_objTrack, m_stop);
    if (m_release!= NULL) 
        pEnv->CallVoidMethod (m_objTrack, m_release);
    pEnv->DeleteLocalRef (m_objTrack);
    m_objTrack = NULL;

	if (m_buffer != NULL)
		pEnv->DeleteLocalRef(m_buffer);
	m_buffer = NULL;

	QCLOGI("Release the audio track!");     
    return QC_ERR_NONE;
}

jmethodID CAudioTrack::GetMethod (JNIEnv * pEnv, jclass cls, const char * pName, const char * pParam, int nStatic) {
    jmethodID jMethod = NULL;
    if (nStatic == 0)
        jMethod = pEnv->GetMethodID(cls, pName, pParam);
    else 
        jMethod = pEnv->GetStaticMethodID(cls, pName, pParam);    
	if(jMethod == NULL) 
    {
		QCLOGI("It can not find method %s, with %s", pName, pParam);
		if (pEnv->ExceptionOccurred()) 	{				
			pEnv->ExceptionDescribe();
			pEnv->ExceptionClear();
		}
	}
    return jMethod;
}

int CAudioTrack::ResetParam (void) {
    m_pjVM = NULL;
    m_pEnv = NULL;
    m_pjCls = NULL;
    m_pjObj = NULL;

    m_clsTrack = NULL;
    m_objTrack = NULL;

    m_initTrack = NULL;
    m_getMinBufferSize = NULL;

    m_play = NULL;
    m_pause = NULL;
    m_stop = NULL;
    m_flush = NULL;
    m_release = NULL;
    m_write = NULL;
    m_buffer = NULL;
    m_nBuffSize = 0;
    return QC_ERR_NONE;
}
