/*******************************************************************************
	File:		CFileIO.cpp

	Contains:	local file io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-05		Bangfei			Create file

*******************************************************************************/

#if defined __QC_OS_NDK__ || defined __QC_OS_IOS__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "qcErr.h"

#include "CFileIO.h"

#include "ULogFunc.h"

CFileIO::CFileIO(CBaseInst * pBaseInst)
	: CBaseIO(pBaseInst)
	, m_hFile(NULL)
	, m_nFD(-1)
	, m_nOpenFlag(0)
	, m_bHadWrite(false)
	, m_pReadBuff(NULL)
	, m_nBuffSize(32768 * 2)
	, m_nReadSize(0)
	, m_nReadPos(0)
	, m_pKeyText(NULL)
{
	SetObjectName ("CFileIO");
	m_pReadBuff = new unsigned char[m_nBuffSize];
}

CFileIO::~CFileIO(void)
{
	Close ();
	QC_DEL_A(m_pReadBuff);
	QC_DEL_A(m_pKeyText);
}

int CFileIO::Open (const char * pURL, long long llOffset, int nFlag)
{
	m_nOpenFlag = nFlag;
	char * pNewURL = (char *)pURL;
	if (!strncmp (pURL, "file://", 7))
		pNewURL = (char *)pURL + 7;
#ifdef __QC_OS_WIN32__
	if (nFlag == QCIO_FLAG_READ)
		m_hFile = CreateFile(pNewURL, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, (DWORD) 0, NULL);
	else if (nFlag == QCIO_FLAG_WRITE) 
		m_hFile = CreateFile(pNewURL, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, (DWORD) 0, NULL);
	else if (nFlag == QCIO_FLAG_READ_WRITE)
		m_hFile = CreateFile(pNewURL, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, (DWORD)0, NULL);
	else
		m_hFile = CreateFile(pNewURL, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, (DWORD) 0, NULL);

	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		m_hFile = NULL;
		//QCLOGE ("Open file %s failed!", pNewURL);
		return -1;
	}
	if (nFlag & QCIO_FLAG_READ)
	{
		DWORD dwHigh = 0;
		DWORD dwSize = GetFileSize (m_hFile, &dwHigh);
		m_llFileSize = dwHigh;
		m_llFileSize = m_llFileSize << 32;
		m_llFileSize += dwSize;
	}
#elif defined __QC_OS_NDK__ || defined __QC_OS_IOS__
	int nOpenFlag = O_RDONLY;
	if (nFlag == QCIO_FLAG_READ)
		nOpenFlag = O_RDONLY;
	else if (nFlag == QCIO_FLAG_WRITE) 
		nOpenFlag = O_RDWR | O_CREAT;
	else if (nFlag == QCIO_FLAG_READ_WRITE) 
		nOpenFlag = O_RDWR | O_CREAT;
	else
		nOpenFlag = O_RDWR | O_CREAT;

	int nMode = 0640;
#ifdef _OS_LINUX_X86
	m_nFD = open64 (pNewURL, nOpenFlag, nMode);
#elif defined __QC_OS_NDK__ || defined __QC_OS_IOS__
	m_nFD = ::open (pNewURL, nOpenFlag, nMode);
#endif // _OS_LINUX_X86
	if (m_nFD > 0 && (nFlag & QCIO_FLAG_READ))
	{
		struct stat st;
		memset(&st, 0, sizeof(struct stat));
	    fstat(m_nFD, &st); 
		m_llFileSize = st.st_size;
	}
	else
	{
		if (nFlag & QCIO_FLAG_READ)
			m_hFile = fopen (pNewURL, "rb");
		else if (nFlag & QCIO_FLAG_WRITE) 
			m_hFile = fopen (pNewURL, "wb");
		else
			m_hFile = fopen (pNewURL, "a+b");
			
		if (m_hFile != NULL && (nFlag & QCIO_FLAG_READ))
		{
			fseeko (m_hFile, 0LL, SEEK_END);
			m_llFileSize = ftello (m_hFile);
			fseeko (m_hFile, 0, SEEK_SET);		
		}
	}

	if (m_hFile == NULL && m_nFD <= 0)
	{
        QCLOGE ("Open file %s failed!", pNewURL);
		return -1;
	}
#endif // __QC_OS_WIN32__

	if (llOffset > 0)
		SetPos(llOffset, QCIO_SEEK_BEGIN);
	else
		m_llReadPos = 0;
	m_llDownPos = m_llFileSize;

	m_nReadPos = 0;
	m_nReadSize = 0;

	if (m_pBaseInst != NULL)
		m_pBaseInst->m_pSetting->g_qcs_bIOReadError = false;

	return QC_ERR_NONE;
}

int CFileIO::Reconnect (const char * pNewURL, long long llOffset)
{
	m_llReadPos = llOffset;
	return QC_ERR_NONE;
}

int CFileIO::Close (void)
{
	CAutoLock lock(&m_mtFile);
	if (m_nFD > 0)
	{
#if defined __QC_OS_NDK__ || defined __QC_OS_IOS__
		::close (m_nFD);
#endif // _OS_LINUX
		m_nFD = -1;
	}
	
	if (m_hFile != NULL)
#ifdef __QC_OS_WIN32__
		CloseHandle (m_hFile);
#elif defined __QC_OS_NDK__ || defined __QC_OS_IOS__
		fclose(m_hFile);
#endif // __QC_OS_WIN32__
	m_hFile = NULL;
	m_llFileSize = 0;

	return QC_ERR_NONE;
}

int CFileIO::Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	CAutoLock lock(&m_mtFile);
	if (m_nFD <= 0 && m_hFile == NULL)
		return QC_ERR_FAILED;

	if (m_llReadPos + m_nReadPos >= m_llFileSize)
	{
		nSize = 0;
		return QC_ERR_FINISH;
	}

	if (m_bHadWrite)
		return ReadInFile(pBuff, nSize);

	int				nRC = 0;
	int				nRead = 0;
	unsigned char * pReadBuff = pBuff;
	int				nRestSize = nSize;
	// Read the buff from read buffer
	if (m_nReadSize > m_nReadPos)
	{
		if (m_nReadSize - m_nReadPos >= nSize)
		{
			memcpy(pBuff, m_pReadBuff + m_nReadPos, nSize);
			m_nReadPos += nSize;
			if (m_nReadSize == m_nReadPos)
			{
				m_llReadPos += m_nReadSize;
				m_nReadSize = 0;
				m_nReadPos = 0;
			}
			return QC_ERR_NONE;
		}

		nRead = m_nReadSize - m_nReadPos;
		memcpy(pReadBuff, m_pReadBuff + m_nReadPos, nRead);
		pReadBuff += nRead;
		nRestSize -= nRead;

		m_llReadPos += m_nReadSize;
		m_nReadSize = 0;
		m_nReadPos = 0;
	}

	if (nRestSize >= m_nBuffSize)
	{
		nRC = ReadInFile(pReadBuff, nRestSize);
		nSize = nRead + nRestSize;
		return nRC;
	}

	m_nReadSize = m_nBuffSize;
	m_nReadPos = 0;
	nRC = ReadInFile(m_pReadBuff, m_nReadSize);
	if (nRC != QC_ERR_NONE || m_nReadSize == 0)
	{
		if (nRead > 0)
		{
			nSize = nRead;
			return QC_ERR_NONE;
		}
		return nRC;
	}
	m_llReadPos -= m_nReadSize;

	if (m_llReadPos + nRestSize > m_llFileSize)
		nRestSize = (int)(m_llFileSize - m_llReadPos);
	memcpy(pReadBuff, m_pReadBuff, nRestSize);
	m_nReadPos += nRestSize;
	nSize = nRead + nRestSize;

	return QC_ERR_NONE;
}

int	CFileIO::ReadInFile(unsigned char * pBuff, int & nSize)
{
	int nRead = -1;
#ifdef __QC_OS_WIN32__
	DWORD	dwRead = 0;
	ReadFile(m_hFile, pBuff, nSize, &dwRead, NULL);
	nSize = (int)dwRead;
	if (dwRead == 0)
	{
		if (m_pBaseInst != NULL)
			m_pBaseInst->m_pSetting->g_qcs_bIOReadError = true;
		if (m_llReadPos < m_llFileSize)
			return QC_ERR_STATUS;
		return QC_ERR_FAILED;
	}
	m_llReadPos += dwRead;
#elif defined __QC_OS_NDK__ || defined __QC_OS_IOS__
	if (m_nFD > 0)
		nRead = ::read(m_nFD, pBuff, nSize);
	else
		nRead = fread(pBuff, 1, nSize, m_hFile);
	if (nRead == -1)
	{
		if (m_pBaseInst != NULL)
			m_pBaseInst->m_pSetting->g_qcs_bIOReadError = true;
		QCLOGE("It was error when Read file!");
		if (m_llReadPos < m_llFileSize)
			//			g_bFileError = true;
			return QC_ERR_FAILED;
	}
	m_llReadPos += nRead;
	if (nRead < nSize)
	{
		if (m_hFile != NULL)
		{
			if (feof(m_hFile) == 0)
			{
				if (m_pBaseInst != NULL)
					m_pBaseInst->m_pSetting->g_qcs_bIOReadError = true;
				QCLOGE("It can't Read data from file enough! Read: % 8d, Size: % 8d, pos: % 8d", nRead, nSize, (int)m_llReadPos);
				return QC_ERR_FAILED;
			}
		}
	}
	nSize = nRead;
#endif // __QC_OS_WIN32__
	if (nSize > 0 && m_pKeyText != NULL)
	{
		for (int i = 0; i < m_nKeySize; i++)
		{
			for (int j = 0; j < nSize; j++)
			{
				pBuff[j] = pBuff[j] ^ m_pKeyText[i];
			}
		}
	}

	return QC_ERR_NONE;
}

int	CFileIO::ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	CAutoLock lock(&m_mtFile);

//	QCLOGI("ReadAT: % 8lld   Size  % 8d", llPos, nSize);
	if ((m_nOpenFlag & QCIO_FLAG_WRITE) == 0)
	{
		if (llPos != m_llReadPos + m_nReadPos)
			SetPos(llPos, QCIO_SEEK_BEGIN);
	}
	else
	{
		SetPos(llPos, QCIO_SEEK_BEGIN);
	}
	return Read (pBuff, nSize, bFull, nFlag);
}

int	CFileIO::Write(unsigned char * pBuff, int nSize, long long llPos)
{
	CAutoLock lock(&m_mtFile);
	if (m_nFD <= 0 && m_hFile == NULL)
		return QC_ERR_FAILED;

	if ((nSize & 0X80000000) == 0)
	{
		if (nSize > 0 && m_pKeyText != NULL)
		{
			for (int i = 0; i < m_nKeySize; i++)
			{
				for (int j = 0; j < nSize; j++)
				{
					pBuff[j] = pBuff[j] ^ m_pKeyText[i];
				}
			}
		}
	}
	nSize = nSize & 0X7FFFFFFF;

	m_bHadWrite = true;
	unsigned int uWrite = 0;
#ifdef __QC_OS_WIN32__
	WriteFile(m_hFile, pBuff, nSize, (DWORD *)&uWrite, NULL);
	FlushFileBuffers(m_hFile);
#elif defined __QC_OS_NDK__ || defined __QC_OS_IOS__
	if (m_nFD > 0)
	{
		uWrite = ::write (m_nFD, pBuff, nSize);
		fsync(m_nFD);
	}
	else
	{
		uWrite = fwrite(pBuff,1, nSize, m_hFile);
		fflush(m_hFile);
	}
#endif //__QC_OS_WIN32__
	m_llReadPos += uWrite;
	if (m_llFileSize < m_llReadPos)
		m_llFileSize = m_llReadPos;

	return uWrite;
}

long long CFileIO::SetPos (long long llPos, int nFlag)
{
	CAutoLock lock(&m_mtFile);
	if (m_nFD <= 0 && m_hFile == NULL)
		return -1;

	if ((nFlag & QCIO_FLAG_WRITE) == QCIO_FLAG_WRITE)
		m_bHadWrite = true;
	nFlag = nFlag & 0XFF00;

	if (!m_bHadWrite)
	{
		if (nFlag == QCIO_SEEK_BEGIN)
		{
			if (llPos >= m_llReadPos && llPos < m_llReadPos + m_nReadSize)
			{
				m_nReadPos = (int)(llPos - m_llReadPos);
				return llPos;
			}
		}
		else if (nFlag == QCIO_SEEK_CUR)
		{
			if ((int)llPos < m_nReadSize)
			{
				m_nReadPos = (int)llPos;
				return llPos;
			}
		}
		else if (nFlag == QCIO_SEEK_END)
		{
			long long llReadPos = m_llFileSize - llPos;
			if (llReadPos >= m_llReadPos && llReadPos < m_llReadPos + m_nReadSize)
			{
				m_nReadPos = (int)(llReadPos - m_llReadPos);
				return llPos;
			}
		}
	}
	m_nReadSize = 0;
	m_nReadPos = 0;

#ifdef __QC_OS_WIN32__
	long		lPos = (long)llPos;
	long		lHigh = (long)(llPos >> 32);
	
	int sMove = FILE_BEGIN;
	if (nFlag == QCIO_SEEK_BEGIN)
	{
		sMove = FILE_BEGIN;
		m_llReadPos = llPos;
		lPos = (long)llPos;
		lHigh = (long)(llPos >> 32);
	}
	else if (nFlag == QCIO_SEEK_CUR)
	{
		sMove = FILE_CURRENT;
		m_llReadPos = m_llReadPos + llPos;
	}
	else if (nFlag == QCIO_SEEK_END)
	{
		sMove = FILE_END;
		m_llReadPos = m_llFileSize - llPos;
		lPos = (long)llPos;
		lHigh = (long)(llPos >> 32);
	}

	if ((m_nOpenFlag & QCIO_FLAG_WRITE) == 0)
	{
		if (m_llReadPos > m_llFileSize)
			return QC_ERR_STATUS;
	}


	DWORD dwRC = SetFilePointer (m_hFile, lPos, &lHigh, sMove);
	//modefied by Aiven,return the currect file pointer if finish the seek.
	if(dwRC == INVALID_SET_FILE_POINTER)
		return QC_ERR_FAILED;
	return llPos;

#elif defined __QC_OS_NDK__ || defined __QC_OS_IOS__
	if (nFlag == QCIO_SEEK_BEGIN)
		m_llReadPos = llPos;
	else if (nFlag == QCIO_SEEK_CUR)
		m_llReadPos = m_llReadPos + llPos;
	else if (nFlag == QCIO_SEEK_END)
		m_llReadPos = m_llFileSize - llPos;

	if ((m_nOpenFlag & QCIO_FLAG_WRITE) == 0)
	{
		if (m_llReadPos > m_llFileSize)
			return QC_ERR_STATUS;
	}
	
	nFlag = SEEK_SET;
	if (m_nFD > 0)
	{
#ifdef __QC_OS_IOS__
		if((llPos = lseek (m_nFD, m_llReadPos, nFlag)) < 0)
			return QC_ERR_FAILED;
#else
        if((llPos = lseek64 (m_nFD, m_llReadPos, nFlag)) < 0)
            return QC_ERR_FAILED;
#endif // __QC_OS_NDK__
	}
	else
	{	
		if (fseeko (m_hFile, m_llReadPos, nFlag) < 0)
		{
			QCLOGE("fseeko to  : %lld failed", (long long) llPos);
			return -1;
		}			
		llPos = ftello (m_hFile);
		if (llPos < 0)
		{
			QCLOGE("ftello the position failed");
			return -1;
		}	
	}
	return llPos;
#endif // __QC_OS_WIN32__
}

QCIOType CFileIO::GetType (void)
{
	return QC_IOTYPE_FILE;
}

int CFileIO::GetParam (int nID, void * pParam)
{
	return CBaseIO::GetParam (nID, pParam);
}

int CFileIO::SetParam (int nID, void * pParam)
{
	switch (nID)
	{
	case QCIO_PID_FILE_KEY:
	{
		if (pParam == NULL)
			return QC_ERR_ARG;
		char * pKey = (char *)pParam;
		QC_DEL_A(m_pKeyText);
		m_nKeySize = strlen(pKey);
		m_pKeyText = new char[m_nKeySize + 1];
		strcpy(m_pKeyText, pKey);
		return QC_ERR_NONE;
	}

	default:
		break;
	}
	return CBaseIO::SetParam (nID, pParam);
}

