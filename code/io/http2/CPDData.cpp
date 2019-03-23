/*******************************************************************************
	File:		CPDData.cpp

	Contains:	PD Data implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-19		Bangfei			Create file

	*******************************************************************************/
#include "qcErr.h"
#include <string.h>
#include <stdlib.h>
#include "CPDData.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

//#define HTTPPD_CHECK_DATA

CPDData::CPDData(CBaseInst * pBaseInst)
	: CBaseObject(pBaseInst)
	, m_pIOFile(NULL)
	, m_pMainURL(NULL)
	, m_llFileSize(0)
	, m_bDownLoad(false)
	, m_bModified(false)
	, m_pPDLFileName(NULL)
	, m_pItem(NULL)
	, m_pPos(NULL)
	, m_ppPosList(NULL)
	, m_nPosListNum(64)
	, m_pThreadWork(NULL)
	, m_llEmptySize(0)
	, m_pEmptyBuff(NULL)
	, m_nEmptySize(32768)
	, m_nEmptyTime(0)
	, m_nDeleteFile(0)
{
	SetObjectName("CPDData");
	m_nHeadSize = 1024 * 1024 * 1;
	m_pHeadBuff = NULL;// new unsigned char[m_nHeadSize];
	memset(&m_moovData, 0, sizeof(m_moovData));
	m_bMoovSave = false;

	m_pHeadBuff = new unsigned char[m_nHeadSize];
	memset(m_pHeadBuff, 0, m_nHeadSize);

#ifdef HTTPPD_CHECK_DATA
	CFileIO ioSource (m_pBaseInst);
	ioSource.Open("c:\\temp\\0000\\Mux_000.mp4", 0, QCIO_FLAG_READ);
	int nRead = (int)ioSource.GetSize();
	m_pSourceData = new unsigned char[nRead];
	ioSource.Read(m_pSourceData, nRead, true, 0);
#endif // HTTPPD_CHECK_DATA
}

CPDData::~CPDData(void)
{
	Close();
	QC_DEL_A(m_ppPosList);
	QC_DEL_A(m_pEmptyBuff);
	QC_DEL_A(m_pHeadBuff);
}

int CPDData::Open(const char * pURL, long long llOffset, int nFlag)
{
	Close();

	int nRC = 0;
	QC_DEL_A(m_pMainURL);
	m_pMainURL = new char[strlen(pURL) + 1];
	strcpy(m_pMainURL, pURL);

	nRC = ParserInfo(pURL);
	if (m_llFileSize > 0)
	{
		nRC = OpenCacheFile();
		if (nRC != QC_ERR_NONE || m_pIOFile->GetSize() <= 0)
		{
			m_pItem = m_lstPos.RemoveHead();
			while (m_pItem != NULL)
			{
				delete m_pItem;
				m_pItem = m_lstPos.RemoveHead();
			}
			m_bDownLoad = false;
			return QC_ERR_FAILED;
		}

		CAutoLock lock(&m_mtLockData);
		int nRead = m_nHeadSize;
		if (nRead > m_pIOFile->GetSize())
			nRead = m_pIOFile->GetSize();
		m_pIOFile->ReadAt(0, m_pHeadBuff, nRead, true, 0);
	}

	return m_bDownLoad ? QC_ERR_NONE : QC_ERR_FAILED;
}

int CPDData::Close(void)
{
	if (m_pThreadWork != NULL)
		m_pThreadWork->Stop();
	QC_DEL_P(m_pThreadWork);
	
	SaveMoovData();
	QC_DEL_A(m_moovData.m_pMoovData);
	memset(&m_moovData, 0, sizeof(m_moovData));

	m_bMoovSave = false;
	m_llEmptySize = 0;

	SavePDLInfoFile();

	m_pItem = m_lstPos.RemoveHead();
	while (m_pItem != NULL)
	{
		delete m_pItem;
		m_pItem = m_lstPos.RemoveHead();
	}

	if (m_nDeleteFile == 1)
	{
		m_bModified = true;
		SavePDLInfoFile();
	}

	QC_DEL_P(m_pIOFile);

	QC_DEL_A(m_pMainURL);
	QC_DEL_A(m_pPDLFileName);

	m_llFileSize = 0;
	m_nEmptyTime = 0;

	return QC_ERR_NONE;
}

int	CPDData::ReadData(long long llPos, unsigned char * pBuff, int & nSize, int nFlag)
{
	if (!HadDownload(llPos, nSize))
		return QC_ERR_RETRY;

	int nRC = -1;
	if (nFlag == QCIO_READ_HEAD)
	{
		CAutoLock lockHead(&m_mtLockHead);
		if (llPos + nSize <= m_nHeadSize)
		{
			memcpy(pBuff, m_pHeadBuff + (int)llPos, nSize);
			//if (!CheckData(llPos, pBuff, nSize))
			//	return QC_ERR_FAILED;
			return QC_ERR_NONE;
		}
		if (m_moovData.m_pMoovData != NULL && llPos >= m_moovData.m_llMoovPos && IsMoovAtEnd(llPos))
		{
			memcpy(pBuff, m_moovData.m_pMoovData + (int)(llPos - m_moovData.m_llMoovPos), nSize);
			//if (!CheckData(llPos, pBuff, nSize))
			//	return QC_ERR_FAILED;
			return QC_ERR_NONE;
		}
	}

	CAutoLock lockData(&m_mtLockData);
    if (!m_pIOFile)
        return QC_ERR_STATUS;
	nRC = m_pIOFile->ReadAt(llPos, pBuff, nSize, true, 0);
	if (nRC == QC_ERR_NONE)
		return QC_ERR_NONE;
	return QC_ERR_RETRY;
}

int	CPDData::RecvData(long long llPos, unsigned char * pBuff, int nSize, int nFlag)
{
	if (llPos + nSize <= m_nHeadSize)
	{
		CAutoLock lockHead(&m_mtLockHead);
		memcpy(m_pHeadBuff + (int)llPos, pBuff, nSize);
	}
	else if (IsMoovAtEnd(llPos))
	{
		CAutoLock lockHead(&m_mtLockHead);
		//if (!CheckData(llPos, pBuff, nSize))
		//	return QC_ERR_FAILED;
		if (m_moovData.m_pMoovData == NULL)
		{
			m_moovData.m_nMoovSize = (int)(m_llFileSize - llPos);
			m_moovData.m_llMoovPos = llPos;
			m_moovData.m_llRecvPos = llPos;
			m_moovData.m_pMoovData = new char[m_moovData.m_nMoovSize];
			memset(m_moovData.m_pMoovData, 0, m_moovData.m_nMoovSize);
		}
		if (llPos >= m_moovData.m_llMoovPos)
		{
			int nCopySize = nSize;
			if (llPos + nSize > m_moovData.m_llMoovPos + m_moovData.m_nMoovSize)
				nCopySize = (int)(m_moovData.m_llMoovPos + m_moovData.m_nMoovSize - llPos);
			memcpy(m_moovData.m_pMoovData + (int)(llPos - m_moovData.m_llMoovPos), pBuff, nCopySize);
			if (m_moovData.m_llRecvPos < llPos + nCopySize)
				m_moovData.m_llRecvPos = llPos + nCopySize;

			RecordItem(llPos, pBuff, nCopySize, nFlag);
			return QC_ERR_NONE;
		}
	}

	if (SaveData(llPos, pBuff, nSize, nFlag) > 0)
		RecordItem(llPos, pBuff, nSize, nFlag);

	return QC_ERR_NONE;
}

int	CPDData::SaveData(long long llPos, unsigned char * pBuff, int nSize, int nFlag)
{
	if (m_pThreadWork != NULL)
	{
		while (m_llEmptySize < llPos + nSize)
		{
			qcSleep(1000);
			if (m_llEmptySize >= m_llFileSize)
				break;
			if (m_pBaseInst->m_bForceClose == true)
				return 0;
		}
	}

	CAutoLock lock(&m_mtLockData);
	if (m_pIOFile == NULL)
		OpenCacheFile();
	if (m_pIOFile == NULL)
		return QC_ERR_STATUS;
	m_pIOFile->SetPos(llPos, QCIO_SEEK_BEGIN | QCIO_FLAG_WRITE);
	m_pIOFile->Write(pBuff, nSize);
	return nSize;
}

int	CPDData::SetFileSize (long long llSize)
{
	if (m_llFileSize == llSize && m_llEmptySize == llSize)
		return QC_ERR_NONE;

	CAutoLock lock(&m_mtLockData);
	m_llFileSize = llSize;
	if (m_pIOFile == NULL)
		OpenCacheFile();

	if (m_pEmptyBuff == NULL)
	{
		m_pEmptyBuff = new unsigned char[m_nEmptySize];
		memset(m_pEmptyBuff, 0, m_nEmptySize);
	}
	if (m_pThreadWork == NULL)
	{
		m_pThreadWork = new CThreadWork(m_pBaseInst);
		m_pThreadWork->SetOwner(m_szObjName);
		m_pThreadWork->SetWorkProc(this, &CThreadFunc::OnWork);
	}
	m_pThreadWork->Start();

	return QC_ERR_NONE;
}

long long CPDData::GetDownPos(long long llPos)
{
	CAutoLock lockData(&m_mtLockData);
	long long llNowPos = llPos;
	if (m_lstPos.GetCount() <= 0)
		return llNowPos;

	m_pItem = NULL;
	m_pPos = m_lstPos.GetHeadPosition();
	while (m_pPos != NULL)
	{
		m_pItem = m_lstPos.GetNext(m_pPos);
		if (llPos >= m_pItem->llBeg && llPos < m_pItem->llEnd)
		{
			llNowPos = m_pItem->llEnd;
			break;
		}
	}
	return llNowPos;
}

bool CPDData::IsMoovAtEnd(long long llPos)
{
	if (m_bMoovSave)
		return false;
	if (!m_pBaseInst->m_bHadOpened && llPos > m_llFileSize / 2 && (m_llFileSize - llPos) < 1024 * 1024 * 64)
		return true;
	else
		return false;
}

void CPDData::SaveMoovData(void)
{
	CAutoLock lock(&m_mtLockData);
	if (m_moovData.m_pMoovData == NULL)
		return;

	CObjectList<QCPD_POS_INFO>	lstFailed;

	int			nSave = 0;
	long long	llBeg = 0;
	long long	llEnd = 0;
	int			nRC = 0;
	m_pPos = m_lstPos.GetHeadPosition();
	while (m_pPos != NULL)
	{
		m_pItem = m_lstPos.GetNext(m_pPos);
		if (m_pItem->llEnd <= m_moovData.m_llMoovPos || m_pItem->llBeg >= m_moovData.m_llRecvPos)
			continue;

		llBeg = m_pItem->llBeg;
		if (llBeg < m_moovData.m_llMoovPos)
			llBeg = m_moovData.m_llMoovPos;
		llEnd = m_pItem->llEnd;
		if (llEnd > m_moovData.m_llRecvPos)
			llEnd = m_moovData.m_llRecvPos;
		nSave = (int)(llEnd - llBeg);
		nRC = SaveData(llBeg, (unsigned char *)(m_moovData.m_pMoovData + (int)(llBeg - m_moovData.m_llMoovPos)), nSave, 0);
		if (nSave != nRC)
			lstFailed.AddTail(m_pItem);
	}

	m_pItem = lstFailed.RemoveHead();
	while (m_pItem != NULL)
	{
		m_bModified = true;
		m_lstPos.Remove(m_pItem);
		delete m_pItem;
		m_pItem = lstFailed.RemoveHead();
	}
}

bool CPDData::RecordItem(long long llPos, unsigned char * pBuff, int nSize, int nFlag)
{
	CAutoLock lockData(&m_mtLockData);

	bool			bFound = false;
	QCPD_POS_INFO *	pNextItem = NULL;
	m_pItem = NULL;
	m_pPos = m_lstPos.GetHeadPosition();
	while (m_pPos != NULL)
	{
		m_pItem = m_lstPos.GetNext(m_pPos);
		if (m_pItem->llBeg <= llPos && m_pItem->llEnd >= llPos)
		{
			bFound = true;
			if (m_pPos != NULL)
				pNextItem = m_lstPos.GetNext(m_pPos);
			break;
		}
	}

	m_bModified = true;
	if (!bFound)
	{
		QCPD_POS_INFO * pItem = new QCPD_POS_INFO();
		pItem->llBeg = llPos;
		pItem->llEnd = llPos + nSize;
		m_lstPos.AddTail(pItem);
		AdjustSortList();
	}
	else
	{
		if (m_pItem->llEnd < llPos + nSize)
			m_pItem->llEnd = llPos + nSize;

		if (pNextItem != NULL)
		{
			if (pNextItem->llBeg <= m_pItem->llEnd)
			{
				AdjustSortList();
				m_pItem = NULL;
			}
		}
		if (llPos + nSize >= m_llFileSize)
			AdjustSortList();
	}

	return true;
}

bool CPDData::HadDownload(long long llPos, long long llSize)
{
	CAutoLock lock(&m_mtLockData);
	if (m_lstPos.GetCount() <= 0)
		return false;

	long long llEnd = llPos;
	if (llSize < 0)
		llEnd = m_llFileSize;
	else
		llEnd = llPos + llSize;
	if (llEnd > m_llFileSize)
		llEnd = m_llFileSize;

	m_pItem = NULL;
	// try to find in range or not
	m_pPos = m_lstPos.GetHeadPosition();
	while (m_pPos != NULL)
	{	
		m_pItem = m_lstPos.GetNext(m_pPos);
		if (llPos >= m_pItem->llBeg && llEnd <= m_pItem->llEnd)
		{
			return true;
		}
	}
	return false;
}

int	CPDData::ParserInfo (const char * pURL)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);

	if (CreatePDLFileName(pURL) != QC_ERR_NONE)
		return QC_ERR_FAILED;

	CFileIO fileIO(m_pBaseInst);
	if (fileIO.Open(m_pPDLFileName, 0, QCIO_FLAG_READ) != QC_ERR_NONE)
		return QC_ERR_FAILED;

	char *	pTextLine = new char[4096];
	int		nLineSize = 4096;

	int nFileSize = (int)fileIO.GetSize();
	char * pFileData = new char[nFileSize];
	fileIO.Read((unsigned char *)pFileData, nFileSize, true, 0);

	char *	pPoa = pFileData;
	int		nRestSize = nFileSize;
	nLineSize = 4096;
	nLineSize = qcReadTextLine(pPoa, nRestSize, pTextLine, nLineSize);

	pPoa += nLineSize;
	nRestSize = nRestSize - nLineSize;
	if (nRestSize <= 0)
		return QC_ERR_FAILED;
	nLineSize = 4096;
	nLineSize = qcReadTextLine(pPoa, nRestSize, pTextLine, nLineSize);
	char * pSize = strstr(pTextLine, "=");
	if (pSize != NULL)
		m_llFileSize = atoi(pSize+1);

	QCPD_POS_INFO * pItem = NULL;
	pPoa += nLineSize;
	nRestSize = nRestSize - nLineSize;
	while (nRestSize > 2)
	{
		nLineSize = 4096;
		nLineSize = qcReadTextLine(pPoa, nRestSize, pTextLine, nLineSize);
		if (nLineSize > 2)
		{
			pItem = new QCPD_POS_INFO();
			pItem->llBeg = atoi(pTextLine);
			pSize = strstr(pTextLine, "-");
			if (pSize != NULL)
				pItem->llEnd = atoi(pSize + 1);
			m_lstPos.AddTail(pItem);
		}
		pPoa += nLineSize;
		nRestSize = nRestSize - nLineSize;
	}
	AdjustSortList();

	delete[]pFileData;
	delete[]pTextLine;

	return QC_ERR_NONE;
}

int	CPDData::CreatePDLFileName (const char * pURL)
{
	if (m_pPDLFileName != NULL)
		return QC_ERR_NONE;

	char * pName = strstr((char *)pURL, "//");
	if (pName == NULL)
		return QC_ERR_FAILED;
	char * pDomain = strstr((char *)pURL, "?domain");

	m_pPDLFileName = new char[strlen(pURL) + strlen(m_pBaseInst->m_pSetting->g_qcs_szPDFileCachePath)];
	strcpy(m_pPDLFileName, m_pBaseInst->m_pSetting->g_qcs_szPDFileCachePath);
	if (m_pPDLFileName[strlen(m_pPDLFileName) - 1] != '/' && m_pPDLFileName[strlen(m_pPDLFileName) - 1] != '\\')
		strcat (m_pPDLFileName, "/");
	if (pDomain == NULL)
	{
		strcat(m_pPDLFileName, pName + 2);
	}
	else
	{
		pName = strstr(pName + 2, "/");
		strcat(m_pPDLFileName, pName + 1);
	}
	strcat(m_pPDLFileName, ".pdl");

	pName = m_pPDLFileName + strlen(m_pBaseInst->m_pSetting->g_qcs_szPDFileCachePath) + 1;
	char * pPos = pName;
	while (*pPos != NULL)
	{
		if (*pPos == '/' || *pPos == '\\' || *pPos == '?' || *pPos == ':' || 
			*pPos == '|' || *pPos == '<' || *pPos == '>' || *pPos == '*')
			*pPos = '-';
		pPos++;
	}

	int nLen = strlen(m_pPDLFileName);
	if (nLen > 256)
	{
		int nLastNum = 0;
		int nAllNum = 0;
		int nKeepNum = 232;
		char * pNum = m_pPDLFileName + nKeepNum;
		while (pNum - m_pPDLFileName <= nLen - 4)
		{
			nLastNum += *pNum;
			pNum++;
		}
		pNum = m_pPDLFileName;
		while (pNum - m_pPDLFileName <= nLen - 4)
		{
			nAllNum += *pNum;
			pNum++;
		}
		char szPrevName[256];
		memset(szPrevName, 0, sizeof(szPrevName));
		strncpy(szPrevName, m_pPDLFileName, nKeepNum);
		memset(m_pPDLFileName, 0, nLen);
		sprintf(m_pPDLFileName, "%s_%d_%d.pdl", szPrevName, nLastNum, nAllNum);
	}

	return QC_ERR_NONE;
}

int	CPDData::OpenCacheFile(void)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);

	if (m_pIOFile != NULL)
		return QC_ERR_NONE;

	if (m_pPDLFileName == NULL)
		return QC_ERR_FAILED;

	char * pExt = strrchr(m_pPDLFileName, '.');
	if (pExt == NULL)
	{
		QCLOGW ("It can find the ext pd file name.");
		return QC_ERR_NONE;
	}
	strcpy(pExt + 1, m_pBaseInst->m_pSetting->g_qcs_szPDFileCacheExtName);
	//strcpy(pExt, ".mp4");

	if (m_pIOFile == NULL)
		m_pIOFile = new CFileIO(m_pBaseInst);
	int nRC = QC_ERR_NONE;
	if (m_bDownLoad)
		nRC = m_pIOFile->Open(m_pPDLFileName, 0, QCIO_FLAG_READ);
	else
		nRC = m_pIOFile->Open(m_pPDLFileName, 0, QCIO_FLAG_READ_WRITE);
	if (nRC != QC_ERR_NONE)
	{
		QCLOGW ("Open %s failed!", m_pPDLFileName);
		QC_DEL_P(m_pIOFile);
		return QC_ERR_FAILED;
	}

	if (strlen (m_pBaseInst->m_pSetting->g_qcs_pFileKeyText) > 0)
		m_pIOFile->SetParam(QCIO_PID_FILE_KEY, (void *)m_pBaseInst->m_pSetting->g_qcs_pFileKeyText);

	if (m_llFileSize >= m_pIOFile->GetSize())
		m_llEmptySize = m_pIOFile->GetSize();
	if (m_pIOFile->GetSize() >= m_llFileSize)
		m_bMoovSave = true;
	if (m_llFileSize == 0)
		m_llFileSize = m_pIOFile->GetSize();

	return QC_ERR_NONE;
}

int	CPDData::SavePDLInfoFile(void)
{
	if (m_bModified == false)
		return QC_ERR_NONE;
	if (m_pMainURL == NULL)
		return QC_ERR_FAILED;
	if (CreatePDLFileName(m_pMainURL) != QC_ERR_NONE)
		return QC_ERR_FAILED;
	char * pExt = strrchr(m_pPDLFileName, '.');
	if (pExt == NULL)
		return QC_ERR_NONE;
	strcpy(pExt, ".pdl");

	char	szLine[4096];
	CFileIO fileIO(m_pBaseInst);
	if (fileIO.Open(m_pPDLFileName, 0, QCIO_FLAG_WRITE) != QC_ERR_NONE)
		return QC_ERR_FAILED;
	fileIO.Write((unsigned char *)m_pMainURL, strlen(m_pMainURL));
	strcpy(szLine, "\r\n");
	fileIO.Write((unsigned char *)szLine, strlen(szLine));

	sprintf(szLine, "FileSize=%d\r\n", m_llFileSize);
	fileIO.Write((unsigned char *)szLine, strlen(szLine));

	long long			llLastPos = 0;
	QCPD_POS_INFO *		pItem = NULL;
	NODEPOS				pos = m_lstPos.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_lstPos.GetNext(pos);
		if (pItem->llEnd <= llLastPos)
			continue;
		if (pItem->llBeg < llLastPos)
			pItem->llBeg = llLastPos;

		sprintf(szLine, "%lld-%lld\r\n", pItem->llBeg, pItem->llEnd);
		fileIO.Write((unsigned char *)szLine, strlen(szLine));

		llLastPos = pItem->llEnd;
	}
	fileIO.Close();
	m_bModified = false;

	return QC_ERR_NONE;
}

int CPDData::AdjustSortList(void)
{
	if (m_lstPos.GetCount() <= 1)
	{
		if (m_lstPos.GetCount() == 1)
		{
			m_pItem = m_lstPos.GetHead();
			if (m_pItem->llBeg == 0 && m_pItem->llEnd >= m_llFileSize)
			{
				m_bDownLoad = true;
				SavePDLInfoFile();
			}
		}
		return QC_ERR_NONE;
	}

	CAutoLock lock(&m_mtLockData);
	if (m_lstPos.GetCount() > m_nPosListNum)
	{
		QC_DEL_A(m_ppPosList);
		m_nPosListNum = m_lstPos.GetCount() + 8;
	}
	if (m_ppPosList == NULL)
		m_ppPosList = new QCPD_POS_INFO*[m_nPosListNum];

	m_pItem = NULL;
	m_pPos = m_lstPos.GetHeadPosition();
	int nIndex = 0;
	while (m_pPos != NULL)
		m_ppPosList[nIndex++] = m_lstPos.GetNext(m_pPos);

	qsort(m_ppPosList, m_lstPos.GetCount(), sizeof(QCPD_POS_INFO *), compareFilePos);

	m_lstPos.RemoveAll();
	QCPD_POS_INFO * pPrevItem = m_ppPosList[0];
	for (int i = 1; i < nIndex; i++)
	{
		if (m_ppPosList[i]->llBeg <= pPrevItem->llEnd)
		{
			pPrevItem->llEnd = m_ppPosList[i]->llEnd;
			delete m_ppPosList[i];
			continue;
		}
		else
		{
			m_lstPos.AddTail(pPrevItem);
			pPrevItem = m_ppPosList[i];
		}
	}
	m_lstPos.AddTail(pPrevItem);

	m_bDownLoad = false;
	if (m_lstPos.GetCount() == 1 && (pPrevItem->llBeg == 0 && pPrevItem->llEnd >= m_llFileSize))
		m_bDownLoad = true;
	if (m_bDownLoad == true)
		SavePDLInfoFile();

	return QC_ERR_NONE;
}

int CPDData::OnWorkItem(void)
{
	if (m_nEmptyTime == 0)
		m_nEmptyTime = qcGetSysTime();
	if (m_llEmptySize > 1024 * 1024 * 4)
	{
		if (qcGetSysTime() - m_nEmptyTime < 2000)
			return QC_ERR_RETRY;
	}

	if (m_pIOFile == NULL || m_llEmptySize >= m_llFileSize)
	{
		qcSleep(5000);
		return QC_ERR_RETRY;
	}

	int	nWrite = m_nEmptySize;
	if (nWrite > (int)(m_llFileSize - m_llEmptySize))
		nWrite = (int)(m_llFileSize - m_llEmptySize);
	m_mtLockData.Lock();
	m_pIOFile->SetPos(m_llEmptySize, QCIO_SEEK_BEGIN | QCIO_FLAG_WRITE);
	m_pIOFile->Write(m_pEmptyBuff, (nWrite | 0X80000000));
	m_llEmptySize += nWrite;
	m_mtLockData.Unlock();
	qcSleep(2000);

	return QC_ERR_NONE;
}

//	qsort(ppPriceItems, nItemNum, sizeof(QCPD_POS_INFO *), compareFilePos);
int CPDData::compareFilePos(const void *arg1, const void *arg2)
{
	QCPD_POS_INFO * pItem1 = *(QCPD_POS_INFO **)arg1;
	QCPD_POS_INFO * pItem2 = *(QCPD_POS_INFO **)arg2;
	return (int)(pItem1->llBeg - pItem2->llBeg);
}

bool CPDData::CheckData(long long llPos, unsigned char * pBuff, int nSize)
{
#ifdef HTTPPD_CHECK_DATA
	if (memcmp(m_pSourceData + (int)llPos, pBuff, nSize))
		return false;
	else
		return true;
#else
	return true;
#endif //HTTPPD_CHECK_DATA
}