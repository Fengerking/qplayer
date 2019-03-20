/*******************************************************************************
	File:		CWaveOutRnd.cpp

	Contains:	The wave out render implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-26		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CWaveOutRnd.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

CWaveOutRnd::CWaveOutRnd(CBaseInst * pBaseInst, void * hInst)
	: CBaseAudioRnd (pBaseInst, hInst)
	, m_hWaveOut (NULL)
	, m_nDeviceID (WAVE_MAPPER)
	, m_dwVolume (0XFF)
	, m_bReseted (true)
	, m_bFirstRnd (true)
	, m_nWriteCount (0)
	, m_nBufSize (0)
	, m_llBuffTime (0)
	, m_llRendTime (0)
{
	SetObjectName ("CWaveOutRnd");
	memset (&m_wavFormat, 0, sizeof (m_wavFormat));
}

CWaveOutRnd::~CWaveOutRnd(void)
{
	Uninit ();
}

int CWaveOutRnd::Init (QC_AUDIO_FORMAT * pFmt, bool bAudioOnly)
{
	CBaseAudioRnd::Init (pFmt, bAudioOnly);

	if (pFmt == NULL)
		return QC_ERR_ARG;
	if (pFmt->nSampleRate == 0 || pFmt->nChannels == 0)
		return QC_ERR_ARG;

	if (pFmt->nBits == 0)
		pFmt->nBits = 16;

	m_fmtAudio.nChannels = pFmt->nChannels;
	m_fmtAudio.nSampleRate = pFmt->nSampleRate;
	m_fmtAudio.nBits = pFmt->nBits;
	if (m_fmtAudio.nChannels > 2)
		m_fmtAudio.nChannels = 2;

	m_nSizeBySec = m_fmtAudio.nSampleRate * m_fmtAudio.nChannels * m_fmtAudio.nBits / 8;

	Uninit();

	return QC_ERR_NONE;
}

bool CWaveOutRnd::InitDevice (QC_AUDIO_FORMAT * pFmt)
{
	if (pFmt->nSampleRate == 0)
		return false;

	if (UpdateFormat (pFmt) == QC_ERR_NONE)
	{
		if (m_hWaveOut != NULL)
			return true;
	}

	Uninit ();

	if (m_hWaveOut != NULL)
	{
		QCLOGE ("The WaveOut device was not release before!");
		return false;
	}

	MMRESULT mr = waveOutOpen (&m_hWaveOut, m_nDeviceID, &m_wavFormat, (DWORD)QCWaveOutProc, (DWORD)this, CALLBACK_FUNCTION);
	if (mr != MMSYSERR_NOERROR)
	{
		m_hWaveOut = NULL;
		QCLOGE ("waveOutOpen return Error 0X%08X", mr);
		return false;
	}

	if (!AllocBuffer ())
		return false;

	if (m_dwVolume != 0XFF)
		waveOutSetVolume (m_hWaveOut, m_dwVolume);

	m_llBuffTime = 0;
	m_bFirstRnd = true;

	return true;
}

int CWaveOutRnd::Uninit (void)
{
	if (m_hWaveOut == NULL)
		return QC_ERR_NONE;

	Stop ();

	ReleaseBuffer();

	CAutoLock lock (&m_mtWaveOut);
	MMRESULT mr = 0;
	int nTryTimes = 0;
	do 
	{
		mr = waveOutClose (m_hWaveOut);
		if(mr == WAVERR_STILLPLAYING)
			qcSleep(10000);
		nTryTimes++;
		if (nTryTimes > 100)
		{
			QCLOGW ("waveOutClose return Error 0X%08X", mr);
			break;
		}
	} while (mr == WAVERR_STILLPLAYING);
	
	m_hWaveOut = NULL;

	if (mr != MMSYSERR_NOERROR)
		return QC_ERR_FAILED;
	else
		return QC_ERR_NONE;
}

int	CWaveOutRnd::Start (void)
{
	CBaseAudioRnd::Start ();

	if (m_hWaveOut == NULL)
		InitDevice (&m_fmtAudio);
	if (m_hWaveOut == NULL)
		return QC_ERR_STATUS;

	CAutoLock lock (&m_mtWaveOut);
	MMRESULT mr = waveOutRestart (m_hWaveOut);
	if (mr != MMSYSERR_NOERROR)
	{
		QCLOGE ("waveOutRestart return error 0X%08X", mr);
		return QC_ERR_STATUS;
	}
	m_bReseted = false;

	m_nRndCount = 0;
	m_nWriteCount = 0;

	return QC_ERR_NONE;
}

int	CWaveOutRnd::Stop (void)
{
	CBaseAudioRnd::Stop ();

	if (m_bReseted)
		return QC_ERR_NONE;
	if (m_hWaveOut == NULL)
	{
		QCLOGW ("The waveout device was closed!");
		return QC_ERR_STATUS;
	}

	WaitAllBufferDone (1200);

	CAutoLock lock (&m_mtWaveOut);
	MMRESULT mr = waveOutReset (m_hWaveOut);
	if (mr != MMSYSERR_NOERROR)
	{
		QCLOGE ("waveOutReset return Error 0X%08X", mr);
		return QC_ERR_STATUS;
	}
	m_bReseted = true;

	return QC_ERR_NONE;
}

int CWaveOutRnd::Flush (void)
{
	WaitAllBufferDone (1200);

	m_nPCMLen = 0;
	m_nRndCount = 0;
	m_nWriteCount = 0;

	CAutoLock lockList (&m_mtList);
	WAVEHDR *	pHead = NULL;
	NODEPOS	pos = m_lstFree.GetHeadPosition ();
	while (pos != NULL)
	{
		pHead = m_lstFree.GetNext (pos);
		if (pHead != NULL)
		{
			((WAVEHDRINFO *)pHead->dwUser)->llTime = -1;
			pHead->dwBufferLength = 0;
		}
	}
	m_bFirstRnd = true;

	return QC_ERR_NONE;
}

int CWaveOutRnd::Render (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || pBuff->pBuff == NULL)
		return QC_ERR_ARG;
	CBaseAudioRnd::Render (pBuff);

	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		Init ((QC_AUDIO_FORMAT *)pBuff->pFormat, m_bAudioOnly);
		Start ();
		pBuff->uFlag = 0;
	}
	if (pBuff->uSize == 0)
		return QC_ERR_NONE;

	if (m_lstFree.GetCount () <= 0)
	{
		qcSleep (5000);
		return QC_ERR_RETRY;
	}
	
	WAVEHDR *		pHead = m_lstFree.GetHead ();
	unsigned char *	pData = NULL;
	int				nSize = 0;

	// Render the PCM data first.
	if (m_pPCMBuff != NULL && m_nPCMLen > 0)
	{
		if (pHead->dwBufferLength + m_nPCMLen > m_nBufSize)
		{
			int nCopySize = m_nBufSize - pHead->dwBufferLength;
			memcpy (pHead->lpData + pHead->dwBufferLength, m_pPCMBuff, nCopySize);
			pHead->dwBufferLength += nCopySize;

			m_pPCMBuff += nCopySize;
			m_nPCMLen -= nCopySize;

			m_llBuffTime += (nCopySize * 1000) / m_wavFormat.nAvgBytesPerSec;
		}
		else
		{
			memcpy (pHead->lpData + pHead->dwBufferLength, m_pPCMBuff, m_nPCMLen);
			pHead->dwBufferLength += m_nPCMLen;
			m_pPCMBuff = m_pPCMData;
			m_nPCMLen = 0;
		}
	}	

	if (pHead->dwBufferLength < m_nBufSize)
	{
		m_llBuffTime = pBuff->llTime;

		pData = pBuff->pBuff;
		nSize = pBuff->uSize;

		if (pHead->dwBufferLength + nSize > m_nBufSize)
		{
			int nCopySize = m_nBufSize - pHead->dwBufferLength;
			memcpy (pHead->lpData + pHead->dwBufferLength, pData, nCopySize);

			if (m_nPCMSize < nSize)
			{
				m_nPCMSize = nSize;
				QC_DEL_A(m_pPCMData);
			}

			if (m_pPCMData == NULL)
			{
				m_nPCMSize = m_wavFormat.nSamplesPerSec;
				if (m_nPCMSize < nSize)
					m_nPCMSize = nSize;
				m_pPCMData = new unsigned char[m_nPCMSize];
				m_pPCMBuff = m_pPCMData;
				m_nPCMLen = 0;
			}

			if (m_nPCMLen == 0)
			{
				m_nPCMLen = pHead->dwBufferLength + nSize - m_nBufSize;
				memcpy (m_pPCMBuff, pData + nCopySize, m_nPCMLen);
			}
			else
			{
				m_pPCMBuff += nCopySize;
				m_nPCMLen -= nCopySize;
			}

			pHead->dwBufferLength = m_nBufSize;
		}
		else
		{
			memcpy (pHead->lpData + pHead->dwBufferLength, pData, nSize);
			pHead->dwBufferLength += nSize;
			if (m_nPCMLen > 0)
			{
				m_pPCMBuff += nSize;
				m_nPCMLen -= nSize;
			}
		}
	}

	if (((WAVEHDRINFO *)pHead->dwUser)->llTime == -1)
		((WAVEHDRINFO *)pHead->dwUser)->llTime = m_llBuffTime;

	if (pHead->dwBufferLength == m_nBufSize)
	{
		// lock in short time
		if (pHead != NULL)
		{
			CAutoLock lockList (&m_mtList);
			pHead = m_lstFree.RemoveHead ();
		}

//		FadeIn (pHead->lpData, pHead->dwBufferLength);

		CAutoLock lockWave (&m_mtWaveOut);
		//QCLOGI ("Time % 8d  % 8d", (int)((WAVEHDRINFO *)pHead->dwUser)->llTime, (int)pBuff->llTime);
		MMRESULT mr = waveOutWrite (m_hWaveOut, pHead, sizeof (WAVEHDR));
		if (mr != MMSYSERR_NOERROR)
		{
			m_lstFree.AddTail (pHead);
			QCLOGE ("Write data % 6d failed! return Error 0X%08X, pHead %p", pHead->dwBufferLength, mr, pHead);
			return QC_ERR_IMPLEMENT;
		}
		if (pHead != NULL)
		{
			CAutoLock lockList (&m_mtList);
			m_lstFull.AddTail (pHead);
		}
		m_bFirstRnd = false;
	}

	if (m_nPCMLen >= (int)m_nBufSize)
		return QC_ERR_RETRY;

	return QC_ERR_NONE;
}

bool CWaveOutRnd::AllocBuffer (void)
{
	ReleaseBuffer ();

	CAutoLock lock (&m_mtList);
	WAVEHDR * pWaveHeader = NULL;
	for (int i = 0; i < MAXINPUTBUFFERS; i++)
	{
		pWaveHeader = new WAVEHDR;
		memset (pWaveHeader, 0, sizeof (WAVEHDR));

		WAVEHDRINFO * pWavInfo = new WAVEHDRINFO ();
		pWaveHeader->dwUser = (DWORD_PTR)pWavInfo;

		m_lstFree.AddTail (pWaveHeader);
	}

	m_nBufSize = m_wavFormat.nAvgBytesPerSec / 10;
	if (m_nBufSize < 1600)
		m_nBufSize = 1600;
	m_nBufSize = (m_nBufSize + m_wavFormat.nBlockAlign - 1) / m_wavFormat.nBlockAlign * m_wavFormat.nBlockAlign;

	char *		pBuffer = NULL;
	NODEPOS pos = m_lstFree.GetHeadPosition ();
	while (pos != NULL)
	{
		pBuffer = new char[m_nBufSize];
		if (pBuffer == NULL)
			return false;
		memset (pBuffer, 0, m_nBufSize);

		pWaveHeader = m_lstFree.GetNext (pos);
		if(pWaveHeader->lpData != NULL)
			delete []pWaveHeader->lpData;
		pWaveHeader->lpData = pBuffer;
		pWaveHeader->dwBufferLength = m_nBufSize;

#ifdef _OS_WINCE
		pWaveHeader->dwFlags = 0;
#else
		pWaveHeader->dwFlags = WHDR_ENDLOOP;//WHDR_INQUEUE | WHDR_DONE;
#endif // _WIN32_WCE
		pWaveHeader->reserved = 0;

		MMRESULT mr = MMSYSERR_NOMEM;
		mr = waveOutPrepareHeader (m_hWaveOut, pWaveHeader, sizeof (WAVEHDR));
		if (mr != MMSYSERR_NOERROR)
		{
			QCLOGE ("waveOutPrepareHeader return Error 0X%08X", mr);
		}

		pWaveHeader->dwBufferLength = 0;
		((WAVEHDRINFO *)pWaveHeader->dwUser)->llTime = -1;
	}

	return true;
}

bool CWaveOutRnd::ReleaseBuffer (void)
{
	if (m_lstFull.GetCount () > 0 || m_lstFree.GetCount ())
		WaitAllBufferDone (1000);

	CAutoLock lock (&m_mtList);

	WAVEHDR * pWaveHeader = NULL;
	while (m_lstFull.GetCount () > 0)
	{
		pWaveHeader = m_lstFull.RemoveHead ();
		m_lstFree.AddTail (pWaveHeader);
	}

	NODEPOS pos = m_lstFree.GetHeadPosition ();
	while (pos != NULL)
	{
		pWaveHeader = m_lstFree.GetNext (pos);
		if (pWaveHeader->lpData != NULL)
		{
			MMRESULT mr = MMSYSERR_NOMEM;
			mr = waveOutUnprepareHeader(m_hWaveOut, pWaveHeader, sizeof (WAVEHDR));
			if (mr != MMSYSERR_NOERROR)
			{
				QCLOGE ("waveOutUnprepareHeader return Error 0X%08X", mr);
			}
			delete []pWaveHeader->lpData;
			pWaveHeader->lpData = NULL;
		}
		//memset (pWaveHeader, 0, sizeof(WAVEHDR));
	}

	while (m_lstFree.GetCount () > 0)
	{
		pWaveHeader = m_lstFree.RemoveHead ();
		WAVEHDRINFO * pWavInfo = (WAVEHDRINFO *)pWaveHeader->dwUser;
		delete pWavInfo;
		delete pWaveHeader;
	}

	return true;
}

int CWaveOutRnd::WaitAllBufferDone (int nWaitTime)
{
	int nStartTime = qcGetSysTime ();

	while (m_lstFree.GetCount () < MAXINPUTBUFFERS)
	{
		if (m_lstFull.GetCount () == 0)
			break;
		qcSleep (2000);
		if (qcGetSysTime () - nStartTime >= nWaitTime)
		{
			QCLOGW ("The count is %d", m_lstFree.GetCount ());
			return nWaitTime;
		}
	}

	return qcGetSysTime () - nStartTime;
}

int CWaveOutRnd::UpdateFormat (QC_AUDIO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;

	if(pFmt->nChannels == m_wavFormat.nChannels && 
		pFmt->nBits == m_wavFormat.wBitsPerSample && 
		pFmt->nSampleRate == m_wavFormat.nSamplesPerSec)
	{
		return QC_ERR_NONE;
	}

	memset(&m_wavFormat, 0, sizeof (WAVEFORMATEX));		
	// cbSize(extra information size) should be 0!!
	// m_wavFormat.cbSize = sizeof (WAVEFORMATEX);
	m_wavFormat.nSamplesPerSec = pFmt->nSampleRate;
	m_wavFormat.nChannels = (WORD)pFmt->nChannels;
	m_wavFormat.wBitsPerSample = (WORD)pFmt->nBits;
	if (m_wavFormat.wBitsPerSample == 0)
		m_wavFormat.wBitsPerSample = 16;

	if (m_wavFormat.nSamplesPerSec <= 8000)
		m_wavFormat.nSamplesPerSec = 8000;
	else if (m_wavFormat.nSamplesPerSec <= 11025)
		m_wavFormat.nSamplesPerSec = 11025;
	else if (m_wavFormat.nSamplesPerSec <= 12000)
		m_wavFormat.nSamplesPerSec = 12000;
	else if (m_wavFormat.nSamplesPerSec <= 16000)
		m_wavFormat.nSamplesPerSec = 16000;
	else if (m_wavFormat.nSamplesPerSec <= 22050)
		m_wavFormat.nSamplesPerSec = 22050;
	else if (m_wavFormat.nSamplesPerSec <= 24000)
		m_wavFormat.nSamplesPerSec = 24000;
	else if (m_wavFormat.nSamplesPerSec <= 32000)
		m_wavFormat.nSamplesPerSec = 32000;
	else if (m_wavFormat.nSamplesPerSec <= 44100)
		m_wavFormat.nSamplesPerSec = 44100;
	else if (m_wavFormat.nSamplesPerSec <= 48000)
		m_wavFormat.nSamplesPerSec = 48000;

	if (m_wavFormat.nChannels > 2)
		m_wavFormat.nChannels = 2;
	else if (m_wavFormat.nChannels <= 0)
		m_wavFormat.nChannels = 1;

	m_wavFormat.nBlockAlign = (WORD)(m_wavFormat.nChannels * pFmt->nBits / 8);
	m_wavFormat.nAvgBytesPerSec = m_wavFormat.nSamplesPerSec * m_wavFormat.nBlockAlign;
	m_wavFormat.wFormatTag = 1;

	QCLOGI ("Audio Format: %d    %d    %d", m_wavFormat.nSamplesPerSec, m_wavFormat.nChannels, m_wavFormat.wBitsPerSample);

	return QC_ERR_FORMAT;
}

bool CWaveOutRnd::AudioDone (WAVEHDR * pWaveHeader)
{
	if (pWaveHeader != NULL && m_pClock != NULL)
	{
		if (((WAVEHDRINFO *)pWaveHeader->dwUser)->llTime >= 0)
		{
			m_llRendTime = ((WAVEHDRINFO *)pWaveHeader->dwUser)->llTime;
			//m_llRendTime = m_llRendTime + pWaveHeader->dwBufferLength * 1000 / m_wavFormat.nAvgBytesPerSec;
			m_pClock->SetTime (m_llRendTime);
		}
	}

	CAutoLock lockList (&m_mtList);
	m_lstFull.Remove (pWaveHeader);
	pWaveHeader->dwBufferLength = 0;
	((WAVEHDRINFO *)pWaveHeader->dwUser)->llTime = -1;
	m_lstFree.AddTail (pWaveHeader);

	m_nRndCount++;

	return true;
}

bool CALLBACK CWaveOutRnd::QCWaveOutProc(HWAVEOUT hwo, UINT uMsg,  DWORD dwInstance, 
										  DWORD dwParam1, DWORD dwParam2)
{
	CWaveOutRnd * pRender = (CWaveOutRnd *)dwInstance;

	switch (uMsg)
	{
	case WOM_CLOSE:
		break;

	case WOM_OPEN:
		break;

	case WOM_DONE:
		pRender->AudioDone ((WAVEHDR *)dwParam1);
		break;

	default:
		break;
	}

	return true;
}

int CWaveOutRnd::SetVolume (int nVolume)
{
	CAutoLock lock (&m_mtWaveOut);
	m_dwVolume = nVolume * 0XFFFF / 100;
	m_dwVolume = ((m_dwVolume << 16) & 0XFFFF0000) + (nVolume * 0XFFFF / 100);
	if (m_hWaveOut != NULL)
	{
		MMRESULT mr = waveOutSetVolume (m_hWaveOut, m_dwVolume);
		return mr == MMSYSERR_NOERROR ? QC_ERR_NONE : QC_ERR_FAILED;
	}

	return QC_ERR_FAILED;
}

int CWaveOutRnd::GetVolume (void)
{
	if (m_hWaveOut == NULL)
		return -1;

	CAutoLock lock (&m_mtWaveOut);
	DWORD dwVolume = 0;
	waveOutGetVolume (m_hWaveOut, &dwVolume);

	return (dwVolume & 0XFFFF) * 100 / 0XFFFF;
}
