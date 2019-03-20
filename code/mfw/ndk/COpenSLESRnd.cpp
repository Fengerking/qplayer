/*******************************************************************************
	File:		COpenSLESRnd.cpp

	Contains:	The ndk audio render implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-12		Bangfei			Create file

*******************************************************************************/
#include "dlfcn.h"

#include "qcErr.h"
#include "qcMsg.h"
#include "qcPlayer.h"

#include "COpenSLESRnd.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

#define	 MAX_RND_BUFFS	3

COpenSLESRnd::COpenSLESRnd(CBaseInst * pBaseInst, void * hInst)
	: CBaseAudioRnd (pBaseInst, hInst)
	, m_pObject (NULL)
	, m_pEngine (NULL)
	, m_pOutputMix (NULL)
	, m_pPlayer (NULL)
	, m_pPlay (NULL)
	, m_pVolume (NULL)
	, m_pBuffQueue (NULL)
	, m_pCurBuff (NULL)
	, m_nVolume (100)
	, m_pSendBuff (NULL)
	, m_nInRender (0)
	, m_nSysTime (0)
{
	SetObjectName ("COpenSLESRnd");
}

COpenSLESRnd::~COpenSLESRnd(void)
{
	Uninit ();
	QC_DEL_P (m_pSendBuff);	
}

int COpenSLESRnd::Init (QC_AUDIO_FORMAT * pFmt, bool bAudioOnly)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;

	if (pFmt->nBits == 0)
		pFmt->nBits = 16;
	m_fmtAudio.nChannels = pFmt->nChannels;
	m_fmtAudio.nSampleRate = pFmt->nSampleRate;
	m_fmtAudio.nBits = pFmt->nBits;
	if (m_fmtAudio.nChannels > 2)
		m_fmtAudio.nChannels = 2;

	CBaseAudioRnd::Init (pFmt, bAudioOnly);

	if (m_fmtAudio.nSampleRate <= 0)
		return QC_ERR_FAILED;

	m_nSizeBySec = m_fmtAudio.nSampleRate * m_fmtAudio.nChannels * m_fmtAudio.nBits / 8;
	GetSampleRate ();

	QCLOGI ("Init format %d  %d", m_fmtAudio.nSampleRate, m_fmtAudio.nChannels);
	if (CreateEngine () != QC_ERR_NONE)
	{
		QCLOGW ("Create OpenSLES failed!");
		DestroyEngine ();
		return QC_ERR_FAILED;
	}

	ReleaseBuffer ();

	OPENSLES_RNDBUFF * pRndBuff = NULL;
	for (int i = 0; i < MAX_RND_BUFFS; i++)
	{
		pRndBuff = new OPENSLES_RNDBUFF ();
		pRndBuff->pBuff = new unsigned char[m_nSizeBySec];
		pRndBuff->nSize = 0;
		pRndBuff->llTime = 0;
		m_lstEmpty.AddTail (pRndBuff);
	}
	return QC_ERR_NONE;
}

int COpenSLESRnd::Uninit (void)
{
	QCLOGI ("Uninit");
	CBaseAudioRnd::Uninit ();

	DestroyEngine ();

	ReleaseBuffer ();

	return QC_ERR_NONE;
}

int COpenSLESRnd::Render (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || pBuff->pBuff == NULL || pBuff->uSize <= 0)
		return QC_ERR_ARG;

	CBaseAudioRnd::Render (pBuff);

	CAutoLock lock (&m_mtList);
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT || m_fmtAudio.nSampleRate == 0)
	{			
		QC_AUDIO_FORMAT * pFmtAudio = (QC_AUDIO_FORMAT *)pBuff->pFormat;
		Init (pFmtAudio, m_bAudioOnly);
	}	

	if (m_lstEmpty.GetCount () <= 1)
	{
		qcSleep (5000);
		return QC_ERR_RETRY;
	}
	if (m_pCurBuff == NULL)
	{
	 	m_pCurBuff = m_lstEmpty.GetHead ();
		if (m_pCurBuff == NULL)
			return QC_ERR_RETRY;
		m_pCurBuff->nSize = 0;
	}

	if (m_pSendBuff != NULL)
	{
		pBuff->nMediaType = QC_MEDIA_Audio;
		m_pSendBuff->SendBuff (pBuff);	
		if (m_nInRender == 1)
		{
			m_nRndCount++;		
			return QC_ERR_NONE;
		}
	}
	
	if (m_pCurBuff->nSize == 0)
		m_pCurBuff->llTime = pBuff->llTime;
	if (m_nSizeBySec - m_pCurBuff->nSize > pBuff->uSize)
	{
		memcpy (m_pCurBuff->pBuff + m_pCurBuff->nSize, pBuff->pBuff, pBuff->uSize);	
		m_pCurBuff->nSize += pBuff->uSize;
	}
	else
	{
		memcpy (m_pCurBuff->pBuff + m_pCurBuff->nSize, pBuff->pBuff, m_nSizeBySec - m_pCurBuff->nSize);	
		m_pCurBuff->nSize = m_nSizeBySec;
	}
		
	if (m_pCurBuff->nSize >= m_nSizeBySec / 5)
	{
		if (m_nSysTime == 0)
			m_nSysTime = qcGetSysTime ();
		m_pCurBuff = m_lstEmpty.RemoveHead ();	
		m_lstPlay.AddTail (m_pCurBuff);
		(*m_pBuffQueue)->Enqueue (m_pBuffQueue, m_pCurBuff->pBuff, m_pCurBuff->nSize);
		m_nRndCount++;
		m_pCurBuff = NULL;
	}
	return QC_ERR_NONE;
}

void COpenSLESRnd::RenderCallback (SLAndroidSimpleBufferQueueItf buffQueue, void * pContext)
{
	COpenSLESRnd * pRnd = (COpenSLESRnd *)pContext;
	CAutoLock lock (&pRnd->m_mtList);
	OPENSLES_RNDBUFF * pRndBuff  = pRnd->m_lstPlay.RemoveHead ();
	if (pRndBuff != NULL)
	{
		//int nSysTime = qcGetSysTime() - pRnd->m_nSysTime;
		//QCLOGT ("COpenSLESRnd", "CallBack  Sys = % 8d    Audio % 8lld, Diff = % 8d   Size = % 8d", nSysTime, pRndBuff->llTime, (int)(nSysTime - pRndBuff->llTime), pRndBuff->nSize);
		//pRnd->m_nSysTime = qcGetSysTime ();
		pRnd->m_lstEmpty.AddTail (pRndBuff);
		if (pRnd->m_pClock != NULL &&  pRnd->m_nSizeBySec != 0)
			pRnd->m_pClock->SetTime (pRndBuff->llTime + pRndBuff->nSize * 1000 / pRnd->m_nSizeBySec);
	}
	else
	{
		QCLOGT ("COpenSLESRnd", "it can't get buffer from play list!");
	}
}

int COpenSLESRnd::Flush (void)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, 0);

	CBaseAudioRnd::Flush ();

	while (m_lstEmpty.GetCount() < MAX_RND_BUFFS)
	{
		if (m_lstPlay.GetCount () == 0)
			break;	
		if (m_pBaseInst->m_bForceClose == true)
			break;	
		qcSleep(5000);
	}
	if (m_pBuffQueue != NULL)
		(*m_pBuffQueue)->Clear (m_pBuffQueue);
	m_pCurBuff = NULL;

	return QC_ERR_NONE;
}

int COpenSLESRnd::SetVolume (int nVolume)
{
	if (m_pVolume == NULL)
		return QC_ERR_FAILED;
	int nNewVolume = (nVolume - 100) * 0X0FFF / 100;
	if (nVolume == 0)
		nNewVolume = -0X7FFF;
	SLresult nResult = (*m_pVolume)->SetVolumeLevel (m_pVolume, (SLmillibel)nNewVolume);
//	QCLOGI ("COpenSLESRnd  new volume = %d nResult = %d", nNewVolume, nResult);
//	nResult = (*m_pVolume)->GetMaxVolumeLevel (m_pVolume, &nNewVolume);	
//	QCLOGI ("COpenSLESRnd  GetMaxVolumeLevel = %d nResult = %d", nNewVolume, nResult);
//	nResult = (*m_pVolume)->SetMute (m_pVolume, 1);	
//	QCLOGI ("COpenSLESRnd  SetMute = %d nResult = %d", nNewVolume, nResult);

	if (nResult != SL_RESULT_SUCCESS)
		return QC_ERR_FAILED;
	m_nVolume = nVolume;
	return QC_ERR_NONE;
}

int COpenSLESRnd::GetVolume (void)
{
	return m_nVolume;
}

int	COpenSLESRnd::OnStart(void)
{
	int nRC = CBaseAudioRnd::OnStart();

	return nRC;
}

int	COpenSLESRnd::OnStop(void)
{
	CBaseAudioRnd::OnStop();
	if (m_pSendBuff != NULL)
		m_pSendBuff->OnStop ();

	while (m_lstEmpty.GetCount() < MAX_RND_BUFFS)
	{
		if (m_lstPlay.GetCount () == 0)
			break;	
		if (m_pBaseInst->m_bForceClose == true)
			break;	
		qcSleep(3000);
	}
	m_pCurBuff = NULL;

	return QC_ERR_NONE;
}

int COpenSLESRnd::SetParam(int nID, void * pParam)
{
	if (nID == QCPLAY_PID_SendOut_AudioBuff)
	{
		if (m_pSendBuff == NULL)
		{
			m_pSendBuff = new CNDKSendBuff (m_pBaseInst);
		}
		m_nInRender = *(int*)pParam;
		return QC_ERR_NONE; 
	}
	return QC_ERR_FAILED;
}

int	COpenSLESRnd::SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer)
{
	if (m_pSendBuff != NULL)
	{
		m_pSendBuff->SetNDK (jvm, env, clsPlayer, objPlayer);
		return QC_ERR_NONE;
	}
	return QC_ERR_FAILED;
}

int COpenSLESRnd::CreateEngine (void)
{
	DestroyEngine ();

	SLresult	nResult;
	// create engine
	nResult = slCreateEngine (&m_pObject, 0, NULL, 0, NULL, NULL);
	if(nResult != SL_RESULT_SUCCESS) 
		return QC_ERR_FAILED;
	// realize the engine
	nResult = (*m_pObject)->Realize (m_pObject, SL_BOOLEAN_FALSE);
	if(nResult != SL_RESULT_SUCCESS)
		return QC_ERR_FAILED;
	// get the engine interface, which is needed in order to create other objects
	nResult = (*m_pObject)->GetInterface (m_pObject, SL_IID_ENGINE, &m_pEngine);
	if(nResult != SL_RESULT_SUCCESS) 
		return QC_ERR_FAILED;

	const SLInterfaceID ids[]	= {SL_IID_VOLUME};
	const SLboolean req[]		= {SL_BOOLEAN_FALSE};
	nResult = (*m_pEngine)->CreateOutputMix (m_pEngine, &m_pOutputMix, 1, ids, req);
	if(nResult != SL_RESULT_SUCCESS) 
		return QC_ERR_FAILED;
	// realize the output mix
	nResult = (*m_pOutputMix)->Realize (m_pOutputMix, SL_BOOLEAN_FALSE);
	if(nResult != SL_RESULT_SUCCESS) 
		return QC_ERR_FAILED;

	int nSpeakers = SL_SPEAKER_FRONT_CENTER;
	if(m_fmtAudio.nChannels > 1) 
		nSpeakers = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	int nSampleRate = GetSampleRate ();
	SLDataFormat_PCM pcmFormat = {SL_DATAFORMAT_PCM, m_fmtAudio.nChannels, nSampleRate, SL_PCMSAMPLEFORMAT_FIXED_16, 
									SL_PCMSAMPLEFORMAT_FIXED_16, nSpeakers, SL_BYTEORDER_LITTLEENDIAN};
    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue bufQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, MAX_RND_BUFFS};
	SLDataSource audioSrc = {&bufQueue, &pcmFormat};
	// configure audio sink
	SLDataLocator_OutputMix outMix = {SL_DATALOCATOR_OUTPUTMIX, m_pOutputMix};
	SLDataSink audioSnk = {&outMix, NULL};
	// create audio player
	const SLInterfaceID ids1[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME};
	const SLboolean req1[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
	nResult = (*m_pEngine)->CreateAudioPlayer (m_pEngine, &m_pPlayer, &audioSrc, &audioSnk, 2, ids1, req1);
	if(nResult != SL_RESULT_SUCCESS)
		return QC_ERR_FAILED;
	// realize the player
	nResult = (*m_pPlayer)->Realize (m_pPlayer, SL_BOOLEAN_FALSE);
	if(nResult != SL_RESULT_SUCCESS)
		return QC_ERR_FAILED;

	// get the play interface
	nResult = (*m_pPlayer)->GetInterface(m_pPlayer, SL_IID_PLAY, &m_pPlay);
	if(nResult != SL_RESULT_SUCCESS)
		return QC_ERR_FAILED;
	nResult = (*m_pPlayer)->GetInterface(m_pPlayer, SL_IID_VOLUME, &m_pVolume);	
	QCLOGI ("m_pVolume = %p", m_pVolume);	

	// get the buffer queue interface
	nResult = (*m_pPlayer)->GetInterface(m_pPlayer, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &m_pBuffQueue);
	if(nResult != SL_RESULT_SUCCESS)
		return QC_ERR_FAILED;

	// register callback on the buffer queue
	nResult = (*m_pBuffQueue)->RegisterCallback (m_pBuffQueue, RenderCallback, this);
	if(nResult != SL_RESULT_SUCCESS)
		return QC_ERR_FAILED;

	// set the player's state to playing
	nResult = (*m_pPlay)->SetPlayState (m_pPlay, SL_PLAYSTATE_PLAYING);

	return QC_ERR_NONE;
}

int	COpenSLESRnd::DestroyEngine (void)
{
    if (m_pPlayer != NULL) 
	{
        (*m_pPlayer)->Destroy (m_pPlayer);
        m_pPlayer = NULL;
        m_pPlay = NULL;
		m_pVolume = NULL;
        m_pBuffQueue = NULL;
    }

    // destroy output mix object, and invalidate all associated interfaces
    if (m_pOutputMix != NULL) 
	{
        (*m_pOutputMix)->Destroy(m_pOutputMix);
        m_pOutputMix = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (m_pObject != NULL) 
	{
        (*m_pObject)->Destroy(m_pObject);
        m_pObject = NULL;
        m_pEngine = NULL;
    }
	return QC_ERR_NONE;
}

int COpenSLESRnd::GetSampleRate (void)
{
	int nSampleRate = 0;
    switch(m_fmtAudio.nSampleRate)
	{
    case 8000:
        nSampleRate = SL_SAMPLINGRATE_8;
        break;
    case 11025:
        nSampleRate = SL_SAMPLINGRATE_11_025;
        break;
    case 16000:
        nSampleRate = SL_SAMPLINGRATE_16;
        break;
    case 22050:
        nSampleRate = SL_SAMPLINGRATE_22_05;
        break;
    case 24000:
        nSampleRate = SL_SAMPLINGRATE_24;
        break;
    case 32000:
        nSampleRate = SL_SAMPLINGRATE_32;
        break;
    case 44100:
        nSampleRate = SL_SAMPLINGRATE_44_1;
        break;
    case 48000:
        nSampleRate = SL_SAMPLINGRATE_48;
        break;
    case 64000:
        nSampleRate = SL_SAMPLINGRATE_64;
        break;
    case 88200:
        nSampleRate = SL_SAMPLINGRATE_88_2;
        break;
    case 96000:
        nSampleRate = SL_SAMPLINGRATE_96;
        break;
    case 192000:
        nSampleRate = SL_SAMPLINGRATE_192;
        break;
    default:
        nSampleRate = 0;
		break;
	}
	return nSampleRate;
}

int COpenSLESRnd::ReleaseBuffer (void)
{
	OPENSLES_RNDBUFF * pBuff = m_lstEmpty.RemoveHead ();
	while (pBuff != NULL)
	{
		delete []pBuff->pBuff;
		delete pBuff;
		pBuff = m_lstEmpty.RemoveHead ();
	}
	pBuff = m_lstPlay.RemoveHead ();
	while (pBuff != NULL)
	{
		delete []pBuff->pBuff;
		delete pBuff;
		pBuff = m_lstPlay.RemoveHead ();
	}
	m_pCurBuff = NULL;

	return QC_ERR_NONE;
}
