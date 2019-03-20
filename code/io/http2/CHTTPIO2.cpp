/*******************************************************************************
	File:		CHTTPIO2.cpp

	Contains:	http io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-06		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "CHTTPIO2.h"
#include "CMsgMng.h"

#include "USystemFunc.h"
#include "UUrlParser.h"
#include "ULogFunc.h"
#include "USourceFormat.h"

CHTTPIO2::CHTTPIO2(CBaseInst * pBaseInst)
	: CBaseIO (pBaseInst)
	, m_pDNSCache (NULL)
	, m_pHttpData (NULL)
	, m_pMemData (NULL)
	, m_bNotifyMsg(true)
	, m_bOpenCache(false)
	, m_bConnected (false)
	, m_bReconnect (false)
	, m_nRecntTime (0)
	, m_bConnectFailed (false)
	, m_pThreadWork (NULL)
	, m_nCacheSize(0)
	, m_pFile (NULL)
{
	SetObjectName ("CHTTPIO2");
	m_nMaxBuffSize = 1024 * 1024 * 32;
	m_pDNSCache = m_pBaseInst->m_pDNSCache;

	m_llFileSize = QCIO_MAX_CONTENT_LEN;

	m_nBuffSize = MEM_BUFF_SIZE;
	m_pBuffData = new char[m_nBuffSize];

	m_pIOCache = NULL;
	m_pHeadBuff = NULL;
	m_llHeadSize = 0;
	m_pMoovBuff = NULL;
	m_llMoovPos = 0;
	m_nMoovSize = 0;
}

CHTTPIO2::~CHTTPIO2(void)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	Close();
    QC_DEL_P (m_pThreadWork);
	QC_DEL_P (m_pMemData);
	QC_DEL_A (m_pBuffData);
	QC_DEL_A (m_pHeadBuff);
	QC_DEL_A (m_pMoovBuff);
}

int CHTTPIO2::Open (const char * pURL, long long llOffset, int nFlag)
{
	int nRC = 0;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, (int)llOffset);
	CAutoLock lock(&m_mtLockFunc);
	int nStartTime = qcGetSysTime();
	if (m_pMemData == NULL)
		m_pMemData = new CMemFile (m_pBaseInst);
	m_pMemData->Reset();
	m_pMemData->SetOpenCache(m_bOpenCache);
	if (m_pHttpData != NULL)
		Close ();
	
	m_llFileSize = QCIO_MAX_CONTENT_LEN; // 0
	m_llReadPos = 0;
	m_llDownPos = 0;
	m_nDLPercent = 0;
	if (llOffset >= 0)
	{
		m_llReadPos = llOffset;
		m_llDownPos = llOffset;
	}

	QC_DEL_A(m_pURL);
	m_pURL = new char[strlen(pURL) + 128];
	strcpy(m_pURL, pURL);
	m_nFlag = nFlag;

	if (m_pIOCache == NULL)
	{
		nRC = OpenURL();
	}
	else
	{
		m_bIsStreaming = m_pIOCache->m_bIsStreaming;
		m_llFileSize = m_pIOCache->m_llFileSize;
		m_llMoovPos = m_pIOCache->m_llMoovPos;
		m_nMoovSize = m_pIOCache->m_nMoovSize;
		CopyOtherMem(m_pIOCache);
	}
	return nRC;
}

int CHTTPIO2::Reconnect (const char * pNewURL, long long llOffset)
{
	m_bReconnect = true;
	CAutoLock lockRead(&m_mtLock);
	CAutoLock lockHttp(&m_mtLockHttp);
	m_pHttpData->Disconnect();
	m_bConnected = false;
	if (pNewURL != NULL)
	{
		QC_DEL_A(m_pURL);
		m_pURL = new char[strlen(pNewURL) + 1];
		strcpy(m_pURL, pNewURL);
	}
	if (llOffset >= 0)
	{
		m_llDownPos = llOffset;
	}
	else // for live source
	{
		m_llDownPos = 0;
		m_llReadPos = 0;
        m_llFileSize = QCIO_MAX_CONTENT_LEN;// 0
	}
    if (m_pMemData != NULL)
        m_pMemData->Reset ();

	int nRC = m_pHttpData->Connect(m_pURL, m_llDownPos, -1);
	int nTryTimes = 0;
	while (nRC != QC_ERR_NONE && !m_pBaseInst->m_bCheckReopn)
	{
        qcSleepEx(100000, &m_pBaseInst->m_bForceClose);
		nRC = m_pHttpData->Connect(m_pURL, m_llDownPos, -1);
		nTryTimes++;
		if (nTryTimes > 5 || m_pBaseInst->m_bForceClose)
			break;
		if (m_pBaseInst->m_bForceClose)
			break;
		QCLOGI("Try to connect server again at %d  times.", nTryTimes);
	}
	m_bReconnect = false;
	if (nRC != QC_ERR_NONE && m_bNotifyMsg)
	{
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL) {
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_CONNECT_FAILED, nRC, 0);
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_RECONNECT_FAILED, nRC, 0);
		}
		return QC_ERR_FAILED;
	}
	if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_RECONNECT_SUCESS, 0, 0);
	m_llFileSize = m_pHttpData->ContentLength ();
	m_bConnected = true;

	return QC_ERR_NONE;
}

int CHTTPIO2::Close (void)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	CAutoLock lock(&m_mtLockFunc);
	Stop();

	CAutoLock lockRead(&m_mtLock);
	CAutoLock lockHttp(&m_mtLockHttp);
	if (m_pMemData != NULL)
		m_pMemData->Reset ();
	QC_DEL_P (m_pHttpData);
	CAutoLock lockSpeed (&m_mtSpeed);
	CSpeedItem * pItem = m_lstSpeed.RemoveHead ();
	while (pItem != NULL)
	{
		QC_DEL_P (pItem);
		pItem = m_lstSpeed.RemoveHead ();
	}
	m_bConnected = false;
	m_llFileSize = QCIO_MAX_CONTENT_LEN;
	return QC_ERR_NONE;
}

int CHTTPIO2::Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	CAutoLock lock(&m_mtLockFunc);
	if (m_llReadPos >= m_llFileSize)
		return QC_ERR_FINISH;
	if (m_llReadPos + nSize > m_llFileSize)
		nSize = (int)(m_llFileSize - m_llReadPos);

	int nRC = QC_ERR_NONE;
	int nRead = nSize;
	if (!bFull)
	{
		CAutoLock lock (&m_mtLock);
		nSize = m_pMemData->ReadBuff (m_llReadPos, (char *)pBuff, nRead, false, nFlag);
		m_llReadPos += nSize;
		if (m_sStatus != QCIO_Run && m_sStatus != QCIO_Pause)
			return QC_ERR_STATUS;
		if (nSize == 0)
		{
			qcSleep (5000);
			return QC_ERR_RETRY;
		}
	}
	else
	{
		int nMemBuffSize = m_pMemData->GetBuffSize(m_llReadPos);
		while (nMemBuffSize < nSize)
		{
			qcSleep (1000);
			if (m_llReadPos + nSize > m_llFileSize)
			{
				nSize = (int)(m_llFileSize - m_llReadPos);
				nRead = nSize;
			}
			if (m_pBaseInst->m_bForceClose == true || m_llReadPos + nMemBuffSize > m_llFileSize)
				return QC_ERR_FINISH;
			if ((m_sStatus != QCIO_Run && m_sStatus != QCIO_Pause) || m_nExitRead > 0)
				return QC_ERR_STATUS;

            if (!m_bConnected)
            {
                // just keeep loop here for mp3, don't return immediately becasue mp3 ffmpeg
                // parser will not process QC_ERR_Disconnected. It will cause EOS.
                if(qcGetSourceFormat(m_pURL) == QC_PARSER_MP3)
                	qcSleep(2000);
				else
					return QC_ERR_Disconnected;
            }
			if (m_bReconnect)
				return QC_ERR_STATUS;
     
			nMemBuffSize = m_pMemData->GetBuffSize(m_llReadPos);
		}

		CAutoLock lock(&m_mtLock);
		nSize = m_pMemData->ReadBuff (m_llReadPos, (char *)pBuff, nRead, true, nFlag);
		m_llReadPos += nSize;
		if (nSize != nRead)
		{
			if (nFlag == QCIO_READ_AUDIO || nFlag == QCIO_READ_VIDEO)
				return QC_ERR_RETRY;
		}
	}
	return QC_ERR_NONE;
}

int	CHTTPIO2::ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	if (llPos >= m_llFileSize)
		return QC_ERR_FINISH;
	if (m_pHeadBuff != NULL && llPos + nSize <= m_llHeadSize)
	{
		memcpy(pBuff, m_pHeadBuff + (int)llPos, nSize);
		return QC_ERR_NONE;
	}
	if (m_pMoovBuff != NULL && llPos >= m_llMoovPos && llPos + nSize < m_llMoovPos + m_nMoovSize)
	{
		memcpy(pBuff, m_pMoovBuff + (int)(llPos - m_llMoovPos), nSize);
		return QC_ERR_NONE;
	}
	if (m_pHttpData == NULL)
	{
		m_llDownPos = m_pMemData->GetBuffSize(0);
		OpenURL();
	}

	m_mtLock.Lock();
	if (m_pMemData->GetStartPos() > llPos || llPos > m_llDownPos + 1024 * 1024 * 2)
	{
		QCLOGI("The read pos % 8lld is larger than start pos % 8lld in mem. ", llPos, m_pMemData->GetStartPos());
		SetPos(llPos, QCIO_SEEK_BEGIN);
	}
	m_llReadPos = llPos;
	m_mtLock.Unlock();

	return Read (pBuff, nSize, bFull, nFlag);
}

int	CHTTPIO2::Write (unsigned char * pBuff, int nSize, long long llPos)
{
	CAutoLock lock(&m_mtLockFunc);

	int nWriteSize = 0;
	if (m_pHttpData != NULL)
		nWriteSize = m_pHttpData->Send ((const char *)pBuff, nSize);
	return nWriteSize;
}

int CHTTPIO2::GetSpeed (int nLastSecs)
{
	int				nSpeed = 1024;
	long long		llSize = 0;
	int				nTime = 0;
	CSpeedItem *	pItem = NULL;
	NODEPOS 		pPos = m_lstSpeed.GetTailPosition ();
//	for (int i = 0; i <= nLastSecs; i++)
	nLastSecs = nLastSecs * 1000;
	while (nTime < nLastSecs)
	{
		pItem = m_lstSpeed.GetPrev (pPos);
		if (pItem == NULL)
			break;
		if (pItem->m_nSize > 0)
		{
			llSize += pItem->m_nSize;
			nTime += pItem->m_nUsedTime;
		}
	}
	if (nTime > 0)
		nSpeed = (int)(llSize * 1000 / nTime);
	else if (llSize > 0)
		nSpeed = 1024 * 256;
	if (m_llDownPos > m_llReadPos + 1024 * 1024 * 2)
		nSpeed = nSpeed * 6 / 4;
	if (nTime < 2000 && nSpeed > 1024 * 1024)
		nSpeed = 1024 * 1024;
	if (nSpeed == 0)
		nSpeed = 1;
	return nSpeed;
}

int CHTTPIO2::Run (void)
{
	CAutoLock lock(&m_mtLockFunc);
	if (m_sStatus == QCIO_Run)
		return QC_ERR_NONE;
	m_sStatus = QCIO_Run;
	if (m_pThreadWork == NULL)
	{
		m_pThreadWork = new CThreadWork(m_pBaseInst);
		m_pThreadWork->SetOwner (m_szObjName);
		m_pThreadWork->SetWorkProc (this, &CThreadFunc::OnWork);
	}
	m_pThreadWork->Start ();
	return QC_ERR_NONE;
}

int CHTTPIO2::Pause (void)
{
	m_sStatus = QCIO_Pause;
	if (m_pThreadWork != NULL)
		m_pThreadWork->Pause ();
	return QC_ERR_NONE;
}

int CHTTPIO2::Stop (void)
{
	if (m_sStatus == QCIO_Stop)
		return QC_ERR_NONE;
	if (m_pHttpData != NULL)
	{
		CAutoLock lockHttp(&m_mtLockHttp);
		m_pHttpData->Interrupt();
	}
	m_bConnected = false;
	m_sStatus = QCIO_Stop;
	if (m_pThreadWork != NULL)
		m_pThreadWork->Stop ();
	return QC_ERR_NONE;
}

long long CHTTPIO2::SetPos (long long llPos, int nFlag)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);

	m_bReconnect = true;
	CAutoLock lockHttp(&m_mtLockHttp);
	CAutoLock lock(&m_mtLock);
	m_llSeekPos = llPos;
	if (m_llReadPos == llPos)
	{
		m_bReconnect = false;
		return llPos;
	}

	m_pMemData->SetPos(llPos);
	long long llNowPos = llPos + m_pMemData->GetBuffSize(llPos);
	if (m_pHttpData != NULL && llNowPos < m_llFileSize && llNowPos != m_llDownPos)
	{
		int nRC = QC_ERR_NONE;
		m_llDownPos = llNowPos;
		nRC = m_pHttpData->Disconnect();

		int nStartTime = qcGetSysTime();
		int	nTimeOut = 50;
		m_bConnected = false;
		nRC = m_pHttpData->Connect(m_pURL, m_llDownPos, nTimeOut);
		while (nRC != QC_ERR_NONE)
		{
			qcSleep(1000);
			if (qcGetSysTime() - nStartTime > m_pBaseInst->m_pSetting->g_qcs_nTimeOutConnect)
				break;
			if (m_pBaseInst->m_bForceClose)
				break;
			nTimeOut += 50;
			nRC = m_pHttpData->Connect(m_pURL, m_llDownPos, nTimeOut);
		}
		if (nRC == QC_ERR_NONE)
			m_bConnected = true;
	}
	m_llReadPos = llPos;
	m_bReconnect = false;

	return m_llReadPos;
}

QCIOType CHTTPIO2::GetType (void)
{
	if (m_pHttpData != NULL)
	{
		if (m_bIsStreaming)
			return QC_IOTYPE_HTTP_LIVE;
		else
			return QC_IOTYPE_HTTP_VOD;
	}
	return QC_IOTYPE_HTTP_VOD;
}

int CHTTPIO2::GetParam (int nID, void * pParam)
{
	if (nID == QCIO_PID_HTTP_CONTENT_TYPE)
	{
		if (pParam == NULL)
			return QC_ERR_ARG;
		char ** ppType = (char **)pParam;
		*ppType = m_pHttpData->GetContentType();
		return QC_ERR_NONE;
	}
	else if (nID == QCIO_PID_HTTP_BUFF_SIZE)
	{
		if (m_pMemData == NULL)
			return 0;
		int nBuffSize = m_pMemData->GetBuffSize(m_llReadPos);
		if (nBuffSize < 0)
			nBuffSize = 0;
		return nBuffSize;
	}
	else if(nID == QCIO_PID_HTTP_HAD_DOWNLOAD)
	{
		if (m_pMemData == NULL)
			return QC_ERR_STATUS;
		QCIO_READ_INFO * pReadInfo = (QCIO_READ_INFO *)pParam;
		int nBuffSize = m_pMemData->GetBuffSize(pReadInfo->llPos);
		if (nBuffSize >= pReadInfo->nSize)
			return QC_ERR_NONE;
		else
			return QC_ERR_FAILED;
	}
	return CBaseIO::GetParam(nID, pParam);
}

int CHTTPIO2::SetParam (int nID, void * pParam)
{
	int nRC = 0;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, nID);	
	switch (nID)
	{
	case QCIO_PID_HTTP_DISCONNECT:
	{
		CAutoLock lockHttp(&m_mtLockHttp);
		if (m_pHttpData == NULL)
			return QC_ERR_STATUS;
		nRC = m_pHttpData->Disconnect();
		m_bConnected = false;
		if (m_pMemData != NULL)
			m_pMemData->ShowStatus();
		return nRC;
	}
	case QCIO_PID_HTTP_RECONNECT:
	{
		CAutoLock lockHttp(&m_mtLockHttp);
		if (m_pHttpData == NULL || m_pMemData == NULL)
			return QC_ERR_STATUS;
		m_llDownPos = m_pMemData->GetBuffSize(0);
		nRC = m_pHttpData->Connect(m_pURL, m_llDownPos, -1);
		if (nRC == QC_ERR_NONE)
			m_bConnected = true;
		return nRC;
	}

	case QCIO_PID_HTTP_MOOVPOS:
		m_llMoovPos = *(long long *)pParam;
		if (m_pMemData == NULL)
			return QC_ERR_STATUS;
		m_pMemData->SetMoovPos(m_llMoovPos);
		return QC_ERR_NONE;

	case QCIO_PID_HTTP_MOOVSIZE:
		m_nMoovSize = *(int *)pParam;
		return QC_ERR_NONE;

	case QCIO_PID_HTTP_DATAPOS:
		if (m_pMemData == NULL)
			return QC_ERR_STATUS;
		m_pMemData->SetDataPos(*(long long *)pParam);
		return QC_ERR_NONE;

	case QCIO_PID_HTTP_COPYMEM:
		m_pIOCache = (CHTTPIO2*)pParam;
		return QC_ERR_NONE;
	
	case QCIO_PID_HTTP_NOTIFY:
		if (*(int*)pParam > 0)
			m_bNotifyMsg = true;
		else
			m_bNotifyMsg = false;
		break;

	case QCIO_PID_HTTP_OPENCACHE:
		if (*(int*)pParam > 0)
			m_bOpenCache = true;
		else
			m_bOpenCache = false;
		break;

	case QCIO_PID_HTTP_CACHE_SIZE:
		m_nCacheSize = *(int*)pParam;
		break;

	default:
		break;
	}
	return CBaseIO::SetParam (nID, pParam);
}

int CHTTPIO2::OnWorkItem (void)
{
	if (m_pHttpData == NULL)
	{
		if (OpenURL() != QC_ERR_NONE)
		{
			qcSleep(10000);
			return QC_ERR_RETRY;
		}
	}

	int		nRead = 0;
	char *	pBuff = NULL;
	int		nSize = 0;

	if (m_llStopPos > 0 && m_llDownPos >= (m_llStopPos + MEM_BUFF_SIZE))
	{
		qcSleep(1000);
		return QC_ERR_NONE;
	}
	if (m_llDownPos >= m_llFileSize || m_pHttpData == NULL || m_pBaseInst->m_nDownloadPause == 1)
	{
		qcSleep (1000);
		return QC_ERR_NONE;
	}
	
	if (!m_bConnected)
	{
		if (m_bIsStreaming || m_bReconnect)
		{
			qcSleep (1000);
			return QC_ERR_RETRY;
		}
        int nInterval = m_pBaseInst?m_pBaseInst->m_pSetting->g_qcs_nReconnectInterval:5000;
		if (qcGetSysTime () - m_nRecntTime < nInterval)
		{
            qcSleepEx (100000, &m_pBaseInst->m_bForceClose);
			return QC_ERR_RETRY;
		}

		CAutoLock lockConnect (&m_mtLock);
		if (m_bConnected)
			return QC_ERR_RETRY;
		if (m_sStatus == QCIO_Stop)
			return QC_ERR_RETRY;	
		if (m_pBaseInst->m_bForceClose)
			return QC_ERR_STATUS;

		if (Reconnect (NULL, m_llReadPos) != QC_ERR_NONE)
		{
			m_nRecntTime = qcGetSysTime ();
		}
	}
	else
	{
		if (m_llDownPos == 0)
		{
			memset (m_nSpeedNotify, 0, sizeof (int) * 32);
			if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_SPEED, 0, 0, m_pURL);
		}

		/// for check speed
		m_pSpeedItem = GetLastSpeedItem ();
		if (m_pSpeedItem->m_nStartTime == 0)
			m_pSpeedItem->m_nStartTime = qcGetSysTime ();

		if (m_llDownPos > (m_llReadPos + 1024 * 1024) && !m_bReconnect)
			qcSleep(1000);
		if (m_nNeedSleep > 0 && !m_bReconnect)
			qcSleep(m_nNeedSleep);

		if (m_sStatus == QCIO_Stop)
			return QC_ERR_RETRY;	
		if (m_pMemData->GetBuffSize(m_llReadPos) > m_nMaxBuffSize)
			return QC_ERR_RETRY;
		if (m_bReconnect)
			return QC_ERR_RETRY;

		m_mtLockHttp.Lock();
		nRead = m_pHttpData->Read (m_pBuffData, m_nBuffSize);
        m_mtLock.Lock();
		if (nRead > 0)
		{
			m_pMemData->FillBuff(m_llDownPos, m_pBuffData, nRead);
			m_llDownPos += nRead;
		}
		m_mtLockHttp.Unlock();
		m_mtLock.Unlock();
		if (nRead == 0)
		{
			if (!m_bReconnect)
				qcSleep (1000);
			return QC_ERR_RETRY;
		}
		else if (nRead < 0)
		{
			qcSleep (1000);
			if (nRead == QC_ERR_ServerTerminated || nRead == QC_ERR_NTAbnormallDisconneted)
			{
				if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DISCONNECTED, 0, 0);
				m_bConnected = false;
				m_pMemData->Reset();
				m_nRecntTime = qcGetSysTime ();
			}
			else if (nRead == QC_ERR_HTTP_EOS)
			{
				m_llFileSize = m_pHttpData->ContentLength ();
				return QC_ERR_FINISH;
			}
			return QC_ERR_STATUS;
		}

		if (m_llFileSize > 0 && !m_bIsStreaming && m_nNotifyDLPercent > 0)
		{
			int nPercent = (int)(m_llDownPos * 100 / m_llFileSize);
			if (nPercent != m_nDLPercent)
			{
				m_nDLPercent = nPercent;
				if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL){
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_PERCENT, m_nDLPercent, m_llDownPos);
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_BUFFER_SIZE, 0, m_llDownPos - m_llReadPos);
				}
			}
		}
		m_pSpeedItem->m_nSize += nRead;

		if (m_nCacheSize > 0 && m_nCacheSize < m_llDownPos)
		{
			m_pHttpData->Disconnect();
			m_bConnected = false;
			m_pThreadWork->Exit();
		}
	}

	if (!m_bConnected)
		m_pMemData->CheckBuffSize ();
	if (qcGetSysTime () - m_pSpeedItem->m_nStartTime > 100)
		m_pSpeedItem->m_nUsedTime = qcGetSysTime () - m_pSpeedItem->m_nStartTime;

	if (m_llDownPos >= m_llFileSize && m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL) 
	{
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_SPEED, GetSpeed(5), m_llDownPos);
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_FINISH, 0, 0);
	}
	for (int i = 1; i < 5; i++)
	{
		if (m_llDownPos >= m_llFileSize * i / 5 && m_nSpeedNotify[i] == 0)
		{
			m_nSpeedNotify[i] = 1;
			if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_SPEED, GetSpeed(5), m_llDownPos);
		}
	}

// for debug dump file
#ifdef __QC_OS_WIN32__00
	QCLOGI("File read % 8d   pos: % 8d", nRead, (int)m_llDownPos);
	if (m_pFile == NULL)
	{
		char	szDumpFile[1024];
		strcpy (szDumpFile, "d:/temp");
		char * pFileName = strrchr (m_pURL, '/');
		if (pFileName != NULL)
			strcat(szDumpFile, pFileName);
		else
			strcat(szDumpFile, "0001.ts");
		m_pFile = new CFileIO(m_pBaseInst);
		m_pFile->Open(szDumpFile, 0, QCIO_FLAG_WRITE);
	}
	m_pFile->Write ((unsigned char *)pBuff, nRead);
	if (m_llDownPos >= m_llFileSize)
	{
		m_pFile->Close();
		delete m_pFile;
		m_pFile = NULL;
	}
#endif // __QC_OS_WIN32__
    
    //qcDumpFile(m_pBuffData, nRead, "flv");
    
	return QC_ERR_NONE;
}

int	CHTTPIO2::OpenURL(void)
{
	int nRC = QC_ERR_NONE;
	m_pSpeedItem = GetLastSpeedItem();
	if (m_pSpeedItem->m_nStartTime == 0)
		m_pSpeedItem->m_nStartTime = qcGetSysTime();

	m_pHttpData = new CHTTPClient(m_pBaseInst, m_pDNSCache);
	if (!m_bNotifyMsg)
		m_pHttpData->SetNotifyMsg(false);
	nRC = m_pHttpData->Connect(m_pURL, m_llDownPos, -1);
	int nTryTimes = 0;
	while (nRC != QC_ERR_NONE && !m_pBaseInst->m_bCheckReopn)
	{
		qcSleepEx(100000, &m_pBaseInst->m_bForceClose);
		nRC = m_pHttpData->Connect(m_pURL, m_llDownPos, -1);
		nTryTimes++;
		if (nTryTimes > 5 || m_pBaseInst->m_bForceClose)
			break;
		QCLOGI("Try to connect server again at %d  times.", nTryTimes);
	}
	if (nRC != QC_ERR_NONE)
	{
		if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_CONNECT_FAILED, nRC, 0);
		if (m_nFlag & QCIO_OPEN_CONTENT)
		{
			if (m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			{
				if (m_bConnectFailed)
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_RECONNECT_FAILED, nRC, 0);
				else
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DISCONNECTED, nRC, 0);
			}
			m_bConnectFailed = true;
		}
		return nRC;
	}

	m_bIsStreaming = m_pHttpData->IsStreaming();
	m_llFileSize = m_pHttpData->ContentLength();
	m_bConnected = true;
	if (m_bConnectFailed)
	{
		m_bConnectFailed = false;
		if ((m_nFlag & QCIO_OPEN_CONTENT) && m_bNotifyMsg && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_RECONNECT_SUCESS, nRC, 0);
	}
	if (m_pBaseInst != NULL)
		m_pBaseInst->m_pSetting->g_qcs_bIOReadError = false;

	Run();

	// those code should be remove when parser finished.
	if (0)//qcGetSourceFormat(m_pURL) == QC_PARSER_M3U8)
	{
		while (m_llFileSize == QCIO_MAX_CONTENT_LEN)
		{
			qcSleep(2000);
			if (m_sStatus != QCIO_Run && m_sStatus != QCIO_Pause)
				return QC_ERR_STATUS;
			if (!m_bConnected || m_pBaseInst->m_bForceClose)
				return QC_ERR_STATUS;
		}
	}
	return nRC;
}

int CHTTPIO2::CopyOtherMem(void * pHTTPIO)
{
	CHTTPIO2 * pSrcIO = (CHTTPIO2 *)pHTTPIO;
	if (pHTTPIO == NULL || pSrcIO->m_pMemData == NULL)
		return QC_ERR_ARG;
	m_llHeadSize = 0;
	pSrcIO->m_pMemData->SortFullList();
	CObjectList<CMemItem> * pMemList = pSrcIO->m_pMemData->GetFullList();
	long long	llStart = -1;
	long long	llEnd = -1;
	NODEPOS		pPos = NULL;
	CMemItem *	pItem = NULL;

	if (pMemList->GetCount() <= 0)
		return QC_ERR_NONE;

	pPos = pMemList->GetHeadPosition();
	while (pPos != NULL)
	{
		pItem = pMemList->GetNext(pPos);
		m_pMemData->FillBuff(pItem->m_llPos, pItem->m_pBuff, pItem->m_nDataSize);
	}

	pPos = pMemList->GetHeadPosition();
	while (pPos != NULL)
	{
		pItem = pMemList->GetNext(pPos);
		if (llStart < 0)
			llStart = pItem->m_llPos;
		if (llEnd < 0)
		{
			llEnd = pItem->m_llPos + pItem->m_nDataSize;
			continue;
		}
		if (pItem->m_llPos != llEnd)
		{
			m_llHeadSize = llEnd;
			break;
		}
		llEnd += pItem->m_nDataSize;
	}
	if (m_llHeadSize == 0)
	{
		pItem = pMemList->GetTail();
		m_llHeadSize = pItem->m_llPos + pItem->m_nDataSize;
	}
	QC_DEL_A(m_pHeadBuff);
	m_pHeadBuff = new unsigned char[(int)m_llHeadSize];
	int nPos = 0;
	int nCopySize = 0;
	pPos = pMemList->GetHeadPosition();
	while (pPos != NULL)
	{
		pItem = pMemList->GetNext(pPos);
		memcpy(m_pHeadBuff + nPos, pItem->m_pBuff, pItem->m_nDataSize);
		nPos += pItem->m_nDataSize;
		if (nPos >= m_llHeadSize)
			break;
	}
	m_llDownPos = m_llHeadSize;
	if (m_llHeadSize >= m_llMoovPos + m_nMoovSize)
		return QC_ERR_NONE;

	QC_DEL_A(m_pMoovBuff);
	m_pMoovBuff = new unsigned char[m_nMoovSize];
	nPos = 0;
	pPos = pMemList->GetHeadPosition();
	while (pPos != NULL)
	{
		pItem = pMemList->GetNext(pPos);
		if (pItem->m_llPos + pItem->m_nDataSize <= m_llMoovPos)
			continue;

		if (nPos == 0)
		{
			llStart = m_llMoovPos - pItem->m_llPos;
			nCopySize = pItem->m_nDataSize - (int)llStart;
			if (nCopySize > m_nMoovSize)
				nCopySize = m_nMoovSize;
			memcpy(m_pMoovBuff, pItem->m_pBuff + (int)llStart, nCopySize);
			nPos = nCopySize;
		}
		else
		{
			if (m_nMoovSize >= nPos + pItem->m_nDataSize)
				nCopySize = pItem->m_nDataSize;
			else
				nCopySize = m_nMoovSize - nPos;
			memcpy(m_pMoovBuff + nPos, pItem->m_pBuff, nCopySize);
			nPos += nCopySize;
		}
		if (nPos >= m_nMoovSize)
			break;
	}
	m_llDownPos = m_llMoovPos + nPos;
	m_pMemData->ShowStatus();
	return QC_ERR_NONE;
}

CSpeedItem * CHTTPIO2::GetLastSpeedItem (void)
{
	CAutoLock lock (&m_mtSpeed);
	CSpeedItem * pSpeedItem = m_lstSpeed.GetTail ();
	if (pSpeedItem != NULL)
	{
		if (pSpeedItem->m_nUsedTime > 100)
		{
			if (m_lstSpeed.GetCount () > 600)
			{
				pSpeedItem = m_lstSpeed.RemoveHead ();
				pSpeedItem->m_nStartTime = 0;
				pSpeedItem->m_nSize = 0;
				pSpeedItem->m_nUsedTime = 0;
				m_lstSpeed.AddTail (pSpeedItem);
			}
			else
			{
				pSpeedItem = NULL;
			}
		}
	}
	if (pSpeedItem == NULL)
	{
		pSpeedItem = new CSpeedItem ();
		m_lstSpeed.AddTail (pSpeedItem);
	}

	return pSpeedItem;
}

CSpeedItem::CSpeedItem (void)
{
	m_nStartTime = 0;
	m_nSize = 0;
	m_nUsedTime = 0;
}

CSpeedItem::~CSpeedItem (void)
{
}
