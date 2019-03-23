/*******************************************************************************
	File:		CPDFileIO.cpp

	Contains:	http io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-06		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "CPDFileIO.h"
#include "CMsgMng.h"

#include "USystemFunc.h"
#include "UUrlParser.h"
#include "ULogFunc.h"

CPDFileIO::CPDFileIO(CBaseInst * pBaseInst)
	: CBaseIO (pBaseInst)
	, m_pDNSCache (NULL)
	, m_pHttpData (NULL)
	, m_pPDData (NULL)
	, m_bSetNewPos(false)
	, m_bConnected (false)
	, m_nRecntTime (0)
	, m_pThreadWork (NULL)
	, m_pFile (NULL)
{
	SetObjectName ("CPDFileIO");

	m_nBuffSize = 32768;
	m_pBuffData = new char[m_nBuffSize];
	m_pDNSCache = m_pBaseInst->m_pDNSCache;
}

CPDFileIO::~CPDFileIO(void)
{
	Close ();
	QC_DEL_P(m_pThreadWork);
	delete[]m_pBuffData;
}

int CPDFileIO::Open (const char * pURL, long long llOffset, int nFlag)
{
	m_llFileSize = 0;
	m_llReadPos = 0;
	m_nReadSize = 0;
	m_llDownPos = 0;
	m_bSetNewPos = false;
	m_nDLPercent = 0;
	if (llOffset >= 0)
		m_llDownPos = llOffset;

	QC_DEL_A(m_pURL);
	m_pURL = new char[strlen(pURL) + 1];
	strcpy(m_pURL, pURL);
	if (m_pPDData == NULL)
		m_pPDData = new CPDData (m_pBaseInst);
    int nRC = m_pPDData->Open(pURL, llOffset, nFlag);
	if (nRC == QC_ERR_NONE)
	{
		m_llFileSize = m_pPDData->GetFileSize();
		m_llDownPos = m_llFileSize;
		m_nDLPercent = 100;
		if (m_nNotifyDLPercent > 0 && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_PERCENT, m_nDLPercent, m_llDownPos);
		return QC_ERR_NONE;
	}

	nRC = QC_ERR_NONE;
	m_llFileSize = m_pPDData->GetFileSize();
	m_llDownPos = m_pPDData->GetDownPos(m_llReadPos);
	int nNeedSize = 1024 * 1024 * 4;
	if (m_llFileSize > 0 && nNeedSize > m_llFileSize)
		nNeedSize = m_llFileSize;

	if (m_llDownPos < nNeedSize)
		nRC = DoOpen();

	if (nRC == QC_ERR_NONE)
	{
		if (m_pBaseInst != NULL)
			m_pBaseInst->m_pSetting->g_qcs_bIOReadError = false;
	}
	return nRC;
}

int	CPDFileIO::DoOpen(void)
{
	int nRC = QC_ERR_NONE;
	if (m_pHttpData == NULL)
		m_pHttpData = new CHTTPClient(m_pBaseInst, m_pDNSCache);

	m_pSpeedItem = GetLastSpeedItem();
	if (m_pSpeedItem->m_nStartTime == 0)
		m_pSpeedItem->m_nStartTime = qcGetSysTime();

	nRC = m_pHttpData->Connect(m_pURL, m_llDownPos, -1);
	if (nRC != QC_ERR_NONE)
	{
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_CONNECT_FAILED, nRC, 0);
		return nRC;
	}

	m_bConnected = true;
	m_bIsStreaming = m_pHttpData->IsStreaming();
	m_llFileSize = m_pHttpData->ContentLength();
	if (m_llFileSize < 0X7FFFFFFF)
	{
		if (m_pPDData->GetDownPos(0) <= 0)
		{
			long long llFreeSpace = qcGetFreeSpace(m_pBaseInst->m_pSetting->g_qcs_szPDFileCachePath);
			if (m_llFileSize > llFreeSpace + 32 * 1024 * 1024)
				return QC_ERR_Overflow;
		}
		nRC = m_pPDData->SetFileSize(m_llFileSize);
	}
	else
	{
		QCLOGW("The file size is larger than 2G. It will return QC_ERR_Overflow.");
		return QC_ERR_Overflow;
	}
	if (m_bIsStreaming)
		return QC_ERR_UNSUPPORT;

	Run();

	return nRC;
}

int CPDFileIO::Close (void)
{
	Stop ();
	QC_DEL_P(m_pThreadWork);

	QC_DEL_P (m_pHttpData);
	QC_DEL_P(m_pPDData);

	CAutoLock lock (&m_mtSpeed);
	CSpeedItem * pItem = m_lstSpeed.RemoveHead ();
	while (pItem != NULL)
	{
		QC_DEL_P (pItem);
		pItem = m_lstSpeed.RemoveHead ();
	}
	m_bConnected = false;
	m_llFileSize = 0;
	return QC_ERR_NONE;
}

int CPDFileIO::Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	return QC_ERR_FAILED;
}

int	CPDFileIO::ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	if (llPos >= m_llFileSize)
		return QC_ERR_FINISH;

	if (!m_pPDData->HadDownload(llPos, nSize * 200))
	{
		if (m_pThreadWork == NULL && m_pHttpData == NULL)
			DoOpen ();
	}

	m_mtLock.Lock();
	m_llReadPos = llPos;
	m_nReadSize = nSize;
	if (m_llReadPos + nSize > m_llFileSize)
		nSize = (int)(m_llFileSize - m_llReadPos);
//	QCLOGI("ReadAt pos % 8lld   size % 8d   down % 8lld", llPos, nSize, m_llDownPos);
	int nRC = m_pPDData->ReadData(m_llReadPos, pBuff, nSize, nFlag);
	m_mtLock.Unlock();
	while (nRC == QC_ERR_RETRY)
	{
		if (m_pBaseInst->m_bForceClose == true)
			nRC = QC_ERR_FINISH;
		if (m_sStatus == QCIO_Stop || m_nExitRead > 0)
			nRC = QC_ERR_STATUS;
		if (!m_bConnected)
			nRC = QC_ERR_Disconnected;
		if (nRC != QC_ERR_RETRY)
			break;

		qcSleep(1000);
		if (m_bSetNewPos)
			return QC_ERR_STATUS;
		m_mtLock.Lock();
		nRC = m_pPDData->ReadData(m_llReadPos, pBuff, nSize, nFlag);
		m_mtLock.Unlock();
	}
	if (nRC == QC_ERR_NONE)
		m_llReadPos += nSize;

	if (nRC != QC_ERR_NONE)
	{
		if (m_pBaseInst != NULL)
			m_pBaseInst->m_pSetting->g_qcs_bIOReadError = true;
		return nRC;
	}

	return QC_ERR_NONE;
}

int	CPDFileIO::Write (unsigned char * pBuff, int nSize, long long llPos)
{
	int nWriteSize = 0;
	if (m_pHttpData != NULL)
		nWriteSize = m_pHttpData->Send ((const char *)pBuff, nSize);
	return nWriteSize;
}

int CPDFileIO::GetSpeed (int nLastSecs)
{
	int				nSpeed = 1024;
	long long		llSize = 0;
	int				nTime = 0;
	CSpeedItem *	pItem = NULL;
	NODEPOS 		pPos = m_lstSpeed.GetTailPosition ();
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
	return nSpeed;
}

int CPDFileIO::Run (void)
{
	m_sStatus = QCIO_Run;
	if (m_pThreadWork == NULL)
	{
		m_pThreadWork = new CThreadWork(m_pBaseInst);
		m_pThreadWork->SetOwner (m_szObjName);
		m_pThreadWork->SetWorkProc (this, &CThreadFunc::OnWork);
		m_pThreadWork->SetStartStopFunc (&CThreadFunc::OnStart, &CThreadFunc::OnStop);
	}
	m_pThreadWork->Start ();
	return QC_ERR_NONE;
}

int CPDFileIO::Pause (void)
{
	m_sStatus = QCIO_Pause;
	if (m_pThreadWork != NULL)
		m_pThreadWork->Pause ();
	return QC_ERR_NONE;
}

int CPDFileIO::Stop (void)
{
	if (m_pHttpData != NULL)
		m_pHttpData->Interrupt ();
	m_bConnected = false;
	m_sStatus = QCIO_Stop;
	if (m_pThreadWork != NULL)
		m_pThreadWork->Stop ();
	return QC_ERR_NONE;
}

long long CPDFileIO::SetPos (long long llPos, int nFlag)
{
	if (llPos >= m_llFileSize)
		return m_llFileSize;

	QCLOGI ("Set Pos: % 12lld, Read: % 12lld  Down: % 12lld", llPos, m_llReadPos, m_llDownPos);
	m_bSetNewPos = true;
	CAutoLock lockHttp(&m_mtLockHttp);
	CAutoLock lock(&m_mtLock);
	m_llSeekPos = llPos;
	if (m_llReadPos == llPos)
	{
		m_bSetNewPos = false;
		return llPos;
	}

	long long llNowPos = m_pPDData->GetDownPos(llPos);
	if (m_pHttpData != NULL && llNowPos != m_llDownPos && llNowPos < m_llFileSize)
	{
		int nRC = QC_ERR_NONE;
		m_llDownPos = llNowPos;
		nRC = m_pHttpData->Disconnect();

		int nStartTime = qcGetSysTime();
		int nTimeOut = 50;
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
	m_bSetNewPos = false;
	return m_llReadPos;
}

long long CPDFileIO::GetDownPos(void)
{
	long long llDownPos = m_llDownPos;
	if (m_pPDData != NULL)
		llDownPos = m_pPDData->GetDownPos(m_llReadPos);
	return llDownPos;
}

QCIOType CPDFileIO::GetType (void)
{
	if (m_pHttpData != NULL)
	{
		if (m_pHttpData->IsStreaming ())
			return QC_IOTYPE_HTTP_LIVE;
		else
			return QC_IOTYPE_HTTP_VOD;
	}
	return QC_IOTYPE_HTTP_VOD;
}

int CPDFileIO::GetParam (int nID, void * pParam)
{
	switch (nID)
	{
	case QCIO_PID_SourceType:
	{
		if (m_pPDData == NULL)
			return QC_IOPROTOCOL_HTTP;
		long long llPos = 0;
		if (pParam != NULL)
			llPos = *(long long *)pParam;
		if (m_pPDData->IsHadDownLoad ())
			return QC_IOPROTOCOL_FILE;
		else
			return QC_IOPROTOCOL_HTTP;
	}

	case QCIO_PID_HTTP_CONTENT_TYPE:
	{
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_pHttpData == NULL)
			return QC_ERR_STATUS;
		char ** ppType = (char **)pParam;
		*ppType = m_pHttpData->GetContentType();
		return QC_ERR_NONE;
	}

	case QCIO_PID_HTTP_HAD_DOWNLOAD:
	{
		if (m_pPDData == NULL)
			return QC_ERR_STATUS;
		QCIO_READ_INFO * pReadInfo = (QCIO_READ_INFO *)pParam;
		return m_pPDData->HadDownload(pReadInfo->llPos, pReadInfo->nSize) ? QC_ERR_NONE : QC_ERR_FAILED;
	}

	default:
		break;
	}

	return CBaseIO::GetParam(nID, pParam);
}

int CPDFileIO::SetParam (int nID, void * pParam)
{
	switch (nID)
	{
	case QCIO_PID_SAVE_MOOV_BUFFER:
	{
		if (m_pPDData == NULL || pParam == NULL)
			return QC_ERR_STATUS;
		QCMP4_MOOV_BUFFER * pMoovBuff = (QCMP4_MOOV_BUFFER *)pParam;
		m_pPDData->RecvData(pMoovBuff->m_llMoovPos, (unsigned char *)pMoovBuff->m_pMoovData, pMoovBuff->m_nMoovSize, QCIO_READ_HEAD);
		return QC_ERR_NONE;
	}

	case QCIO_PID_HTTP_DEL_FILE:
		if (m_pPDData != NULL)
			m_pPDData->DeleteCacheFile(*(int*)pParam);
		break;

	default:
		break;
	}
	return CBaseIO::SetParam (nID, pParam);
}

int CPDFileIO::OnStartFunc(void)
{
	int nRC = QC_ERR_NONE;

	if (m_llDownPos < m_llFileSize && m_pHttpData == NULL)
		nRC = DoOpen();

	return nRC;
}

int CPDFileIO::OnWorkItem (void)
{
	int		nRC = 0;
	int		nRead = 0;
	char *	pBuff = NULL;
	int		nSize = 0;

	if (m_llDownPos >= m_llFileSize || m_pHttpData == NULL || m_pBaseInst->m_nDownloadPause == 1)
	{
		qcSleep (5000);
		return QC_ERR_NONE;
	}
	else if (!m_bConnected)
	{
		if (qcGetSysTime () - m_nRecntTime < 500)
		{
            qcSleepEx (1000, &m_pBaseInst->m_bForceClose);
			return QC_ERR_RETRY;
		}

		CAutoLock lockConnect (&m_mtLock);
		if (m_bConnected)
			return QC_ERR_RETRY;
		if (m_sStatus == QCIO_Stop)
			return QC_ERR_RETRY;			

		nRC = m_pHttpData->Disconnect();
		nRC = m_pHttpData->Connect(m_pURL, m_llDownPos, -1);
		if (nRC == QC_ERR_NONE)
		{
			m_bConnected = true;
			if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_RECONNECT_SUCESS, 0, 0);
		}
		else
		{
			m_nRecntTime = qcGetSysTime ();
			if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_RECONNECT_FAILED, 0, 0);
		}
		return QC_ERR_RETRY;
	}
	else
	{
		if (m_llDownPos == 0)
		{
			memset(m_nSpeedNotify, 0, sizeof(int) * 32);
			if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_SPEED, 0, 0, m_pURL);
		}

		/// for check speed
		m_pSpeedItem = GetLastSpeedItem();
		if (m_pSpeedItem->m_nStartTime == 0)
			m_pSpeedItem->m_nStartTime = qcGetSysTime();

		if (m_sStatus == QCIO_Stop)
			return QC_ERR_RETRY;

		m_mtLockHttp.Lock();
		nRead = m_pHttpData->Read(m_pBuffData, m_nBuffSize);
		m_mtLock.Lock();
		if (nRead > 0)
		{
			m_pPDData->RecvData(m_llDownPos, (unsigned char *)m_pBuffData, nRead, QCIO_READ_DATA);
			m_llDownPos += nRead;
		}
		m_mtLockHttp.Unlock();
		m_mtLock.Unlock();
        if (m_llDownPos > (m_llReadPos + 1024 * 1024))
            qcSleep(1000);
        if (m_nNeedSleep > 0 && !m_bSetNewPos)
            qcSleep(m_nNeedSleep);

		if (nRead == 0)
		{
			qcSleep(1000);
			return QC_ERR_RETRY;
		}
		else if (nRead < 0)
		{
			qcSleep(2000);
			if (nRead == QC_ERR_ServerTerminated || nRead == QC_ERR_NTAbnormallDisconneted)
			{
				if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DISCONNECTED, 0, 0);
				m_bConnected = false;
				m_nRecntTime = qcGetSysTime();
			}
			else if (nRead == QC_ERR_HTTP_EOS)
			{
				m_llFileSize = m_pHttpData->ContentLength();
				return QC_ERR_FINISH;
			}
			return QC_ERR_STATUS;
		}

		if (m_llFileSize > 0 && m_nNotifyDLPercent > 0)
		{
			int nPercent = (int)(m_llDownPos * 100 / m_llFileSize);
			if (nPercent != m_nDLPercent)
			{
				m_nDLPercent = nPercent;
				if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL){
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_PERCENT, m_nDLPercent, m_llDownPos);
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_BUFFER_SIZE, 0, m_llDownPos - m_llReadPos);
				}
			}
		}
		m_pSpeedItem->m_nSize += nRead;

		CAutoLock lockCheck(&m_mtLock);
		long long llDownPos = m_pPDData->GetDownPos(m_llReadPos);
		if (llDownPos == m_llFileSize)
			llDownPos = m_pPDData->GetDownPos(0);
		if (llDownPos < m_llDownPos || llDownPos > m_llDownPos + 1024 * 1024 * 1)
		{
			if (!m_pPDData->HadDownload(m_llReadPos, m_nReadSize) && llDownPos < m_llFileSize)
			{
				m_llDownPos = llDownPos;
				nRC = m_pHttpData->Disconnect();
				nRC = m_pHttpData->Connect(m_pURL, m_llDownPos, -1);
			}
		}
	}
	
	if (qcGetSysTime () - m_pSpeedItem->m_nStartTime > 100)
		m_pSpeedItem->m_nUsedTime = qcGetSysTime () - m_pSpeedItem->m_nStartTime;

	if (m_llDownPos >= m_llFileSize)
	{
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL){
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_SPEED, GetSpeed(5), m_llDownPos);
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_FINISH, 0, 0);
		}
	}
	for (int i = 1; i < 5; i++)
	{
		if (m_llDownPos >= m_llFileSize * i / 5 && m_nSpeedNotify[i] == 0)
		{
			m_nSpeedNotify[i] = 1;
			if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_DOWNLOAD_SPEED, GetSpeed(5), m_llDownPos);
		}
	}

	return QC_ERR_NONE;
}

CSpeedItem * CPDFileIO::GetLastSpeedItem (void)
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
