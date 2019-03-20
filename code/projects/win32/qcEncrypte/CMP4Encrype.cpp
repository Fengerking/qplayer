#include "stdafx.h"
#include "CMP4Encrype.h"

#define KBoxHeaderSize 16
#define QKEY_COMP_TEXT	"cpky"
#define QKEY_FILE_TEXT	"flky"

CMP4Encrype::CMP4Encrype()
{
}


CMP4Encrype::~CMP4Encrype()
{
}

bool CMP4Encrype::EncrypeFile(const char * pSource, const char * pDest, const char * pKeyComp, const char * pKeyFile)
{
	if (pSource == NULL || pDest == NULL || pKeyComp == NULL || pKeyFile == NULL)
		return false;
	if (strlen(pKeyComp) > 8 || strlen(pKeyFile) > 8)
		return false;

	HANDLE hFileSrc = CreateFile(pSource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, (DWORD)0, NULL);
	if (hFileSrc == INVALID_HANDLE_VALUE)
		return false;
	long long	llFileSize = 0;
	DWORD		dwRead = 0;
	DWORD		dwHigh = 0;
	DWORD		dwSize = GetFileSize(hFileSrc, &dwHigh);
	llFileSize = dwHigh;
	llFileSize = (llFileSize << 32) + dwSize;
	if (!ParserMP4(hFileSrc, llFileSize))
	{
		CloseHandle(hFileSrc);
		return false;
	}

	HANDLE hFileDst = CreateFile(pDest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, (DWORD)0, NULL);
	if (hFileDst == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFileSrc);
		return false;
	}

	DWORD			dwWrite = 0;
	unsigned int	nReadSize = 0;
	int				nBuffSize = 32 * 1024;
	unsigned char * pBuffData = new unsigned char[nBuffSize];
	SetFilePointer(hFileSrc, 0, NULL, FILE_BEGIN);

	nReadSize = m_nTypeSize;
	ReadFile(hFileSrc, pBuffData, nReadSize, &dwRead, NULL);
	WriteFile(hFileDst, pBuffData, dwRead, &dwWrite, NULL);
	if (dwRead != dwWrite)
	{
		CloseHandle(hFileSrc);
		CloseHandle(hFileDst);
		return false;
	}
	if (m_nQKeySize > 0)
		SetFilePointer(hFileSrc, m_nQKeySize, NULL, FILE_CURRENT);

	int		nBoxSize = m_nHeadSize + strlen(pKeyComp);
	char *	pBoxSize = (char *)&nBoxSize;
	pBuffData[0] = pBoxSize[3];	pBuffData[1] = pBoxSize[2];
	pBuffData[2] = pBoxSize[1];	pBuffData[3] = pBoxSize[0];
	memcpy(pBuffData + 4, QKEY_COMP_TEXT, 4);
	int nKeySize = strlen(pKeyComp);
	for (int i = 0; i < nKeySize; i++)
		*(pBuffData + 8 + i) = *(pKeyComp+i) + (nKeySize - i);
	WriteFile(hFileDst, pBuffData, nBoxSize, &dwWrite, NULL);
	if (nBoxSize != dwWrite)
	{
		CloseHandle(hFileSrc);
		CloseHandle(hFileDst);
		return false;
	}

	nBoxSize = m_nHeadSize + strlen(pKeyFile);
	pBoxSize = (char *)&nBoxSize;
	pBuffData[0] = pBoxSize[3];	pBuffData[1] = pBoxSize[2];
	pBuffData[2] = pBoxSize[1];	pBuffData[3] = pBoxSize[0];
	memcpy(pBuffData + 4, QKEY_FILE_TEXT, 4);
	memcpy(pBuffData + 8, pKeyFile, strlen(pKeyFile));
	WriteFile(hFileDst, pBuffData, nBoxSize, &dwWrite, NULL);
	if (nBoxSize != dwWrite)
	{
		CloseHandle(hFileSrc);
		CloseHandle(hFileDst);
		return false;
	}

	long long llReadPos = m_nTypeSize;
	while (llReadPos < m_llDataBeg)
	{
		if (m_llDataBeg - llReadPos >= nBuffSize)
			nReadSize = nBuffSize;
		else
			nReadSize = (int)(m_llDataBeg - llReadPos);
		ReadFile(hFileSrc, pBuffData, nReadSize, &dwRead, NULL);
		WriteFile(hFileDst, pBuffData, dwRead, &dwWrite, NULL);
		if (dwRead != dwWrite)
		{
			CloseHandle(hFileSrc);
			CloseHandle(hFileDst);
			return false;
		}
		llReadPos += nReadSize;
	}

	while (llReadPos < m_llDataEnd)
	{
		if (m_llDataEnd - llReadPos >= nBuffSize)
			nReadSize = nBuffSize;
		else
			nReadSize = (int)(m_llDataEnd - llReadPos);
		ReadFile(hFileSrc, pBuffData, nReadSize, &dwRead, NULL);
	
		int j = 0;
		nKeySize = strlen(pKeyFile);
		for (int i = 0; i < nReadSize; i++)
		{
			if (m_nQKeySize > 0)
			{
				int nOldKey = strlen(m_szFileKey);
				for (j = 0; j < nOldKey; j++)
					pBuffData[i] = pBuffData[i] ^ (m_szFileKey[j] + (nOldKey - j));
			}
			for (j = 0; j < nKeySize; j++)
				pBuffData[i] = pBuffData[i] ^ (pKeyFile[j] + (nKeySize - j));
		}

		WriteFile(hFileDst, pBuffData, dwRead, &dwWrite, NULL);
		if (dwRead != dwWrite)
		{
			CloseHandle(hFileSrc);
			CloseHandle(hFileDst);
			return false;
		}
		llReadPos += nReadSize;
	}

	while (llReadPos < llFileSize)
	{
		if (llFileSize - llReadPos >= nBuffSize)
			nReadSize = nBuffSize;
		else
			nReadSize = (int)(llFileSize - llReadPos);
		ReadFile(hFileSrc, pBuffData, nReadSize, &dwRead, NULL);
		WriteFile(hFileDst, pBuffData, dwRead, &dwWrite, NULL);
		if (dwRead != dwWrite)
		{
			CloseHandle(hFileSrc);
			CloseHandle(hFileDst);
			return false;
		}
		llReadPos += nReadSize;
	}

	CloseHandle(hFileSrc);
	CloseHandle(hFileDst);

	return true;
}

bool CMP4Encrype::UnencrypeFile(const char * pSource, const char * pDest)
{
	if (pSource == NULL || pDest == NULL)
		return false;

	HANDLE hFileSrc = CreateFile(pSource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, (DWORD)0, NULL);
	if (hFileSrc == INVALID_HANDLE_VALUE)
		return false;
	long long	llFileSize = 0;
	DWORD		dwRead = 0;
	DWORD		dwHigh = 0;
	DWORD		dwSize = GetFileSize(hFileSrc, &dwHigh);
	llFileSize = dwHigh;
	llFileSize = (llFileSize << 32) + dwSize;
	if (!ParserMP4(hFileSrc, llFileSize))
	{
		CloseHandle(hFileSrc);
		return false;
	}

	HANDLE hFileDst = CreateFile(pDest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, (DWORD)0, NULL);
	if (hFileDst == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFileSrc);
		return false;
	}

	DWORD			dwWrite = 0;
	unsigned int	nReadSize = 0;
	int				nBuffSize = 32 * 1024;
	unsigned char * pBuffData = new unsigned char[nBuffSize];
	SetFilePointer(hFileSrc, 0, NULL, FILE_BEGIN);

	nReadSize = m_nTypeSize;
	ReadFile(hFileSrc, pBuffData, nReadSize, &dwRead, NULL);
	WriteFile(hFileDst, pBuffData, dwRead, &dwWrite, NULL);
	if (dwRead != dwWrite)
	{
		CloseHandle(hFileSrc);
		CloseHandle(hFileDst);
		return false;
	}
	if (m_nQKeySize > 0)
		SetFilePointer(hFileSrc, m_nQKeySize, NULL, FILE_CURRENT);
	long long llReadPos = m_nTypeSize + m_nQKeySize;
	while (llReadPos < m_llDataBeg)
	{
		if (m_llDataBeg - llReadPos >= nBuffSize)
			nReadSize = nBuffSize;
		else
			nReadSize = (int)(m_llDataBeg - llReadPos);
		ReadFile(hFileSrc, pBuffData, nReadSize, &dwRead, NULL);
		WriteFile(hFileDst, pBuffData, dwRead, &dwWrite, NULL);
		if (dwRead != dwWrite)
		{
			CloseHandle(hFileSrc);
			CloseHandle(hFileDst);
			return false;
		}
		llReadPos += nReadSize;
	}

	int nKeySize = strlen(m_szFileKey);
	while (llReadPos < m_llDataEnd)
	{
		if (m_llDataEnd - llReadPos >= nBuffSize)
			nReadSize = nBuffSize;
		else
			nReadSize = (int)(m_llDataEnd - llReadPos);
		ReadFile(hFileSrc, pBuffData, nReadSize, &dwRead, NULL);

		int j = 0;
		for (int i = 0; i < nReadSize; i++)
		{
			for (j = 0; j < nKeySize; j++)
				pBuffData[i] = pBuffData[i] ^ (m_szFileKey[j] + (nKeySize - j));
		}

		WriteFile(hFileDst, pBuffData, dwRead, &dwWrite, NULL);
		if (dwRead != dwWrite)
		{
			CloseHandle(hFileSrc);
			CloseHandle(hFileDst);
			return false;
		}
		llReadPos += nReadSize;
	}

	while (llReadPos < llFileSize)
	{
		if (llFileSize - llReadPos >= nBuffSize)
			nReadSize = nBuffSize;
		else
			nReadSize = (int)(llFileSize - llReadPos);
		ReadFile(hFileSrc, pBuffData, nReadSize, &dwRead, NULL);
		WriteFile(hFileDst, pBuffData, dwRead, &dwWrite, NULL);
		if (dwRead != dwWrite)
		{
			CloseHandle(hFileSrc);
			CloseHandle(hFileDst);
			return false;
		}
		llReadPos += nReadSize;
	}

	CloseHandle(hFileSrc);
	CloseHandle(hFileDst);
	return true;
}

bool CMP4Encrype::ParserMP4(HANDLE hFileSrc, long long llFileSize)
{
	m_nHeadSize = 0;
	m_nTypeSize = 0;
	m_nQKeySize = 0;
	m_llMoovBeg = 0;
	m_llMoovEnd = 0;
	m_llDataBeg = 0;
	m_llDataEnd = 0;

	memset(m_szCompKey, 0, sizeof(m_szCompKey));
	memset(m_szFileKey, 0, sizeof(m_szFileKey));

	long long		llReadPos = 0;
	DWORD			dwRead = 0;
	LONG			dwPos = 0;
	LONG			dwPosH = 0;
	int				nBoxSize = 0;
	unsigned char	szBuff[256];
	while (llReadPos < llFileSize)
	{
		dwPos = (long)llReadPos;
		dwPosH = (long)(llReadPos >> 32);
		SetFilePointer(hFileSrc, dwPos, &dwPosH, FILE_BEGIN);
		ReadFile(hFileSrc, szBuff, KBoxHeaderSize, &dwRead, NULL);
		if (dwRead != KBoxHeaderSize)
			return false;
		
		nBoxSize = qcIntReadUint32BE(szBuff);
		if (nBoxSize >= llFileSize)
			return false;
		if (nBoxSize == 1)
		{
			nBoxSize = (int)qcIntReadUint64BE(szBuff + 8);
			if (nBoxSize < 16)
				return false;
			m_nHeadSize = 16;
		}
		else
		{
			if (nBoxSize < 8)
				return false;
			m_nHeadSize = 8;
		}

		if (memcmp(szBuff + 4, QKEY_COMP_TEXT, 4) == 0)
		{
			m_nQKeySize += nBoxSize;
			memcpy(m_szCompKey, szBuff + 8, nBoxSize - 6);
			int nKeySize = strlen(m_szCompKey);
			for (int i = 0; i < nKeySize; i++)
				m_szCompKey[i] = m_szCompKey[i] - (nKeySize - i);
		}
		else if (memcmp(szBuff + 4, QKEY_FILE_TEXT, 4) == 0)
		{
			m_nQKeySize += nBoxSize;
			memcpy(m_szFileKey, szBuff + 8, nBoxSize - 6);
		}
		else if (memcmp(szBuff + 4, "moov", 4) == 0)
		{
			m_llMoovBeg = llReadPos + m_nHeadSize;
			m_llMoovEnd = llReadPos + nBoxSize;
		}
		else if (memcmp(szBuff + 4, "mdat", 4) == 0)
		{
			m_llDataBeg = llReadPos + m_nHeadSize;
			m_llDataEnd = llReadPos + nBoxSize;
			break;
		}
		if (llReadPos == 0)
			m_nTypeSize = nBoxSize;
		llReadPos += nBoxSize;
	}

	if (m_llMoovEnd == 0 && m_llDataBeg == 0)
		return false;

	if (m_llDataBeg == 0)
	{
		m_llDataBeg = m_llMoovEnd;
		m_llDataEnd = llFileSize;
	}

	return true;
}

unsigned int CMP4Encrype::qcIntReadUint32BE(const unsigned char* aReadPtr)
{
	return (unsigned int)(aReadPtr[0] << 24) | (unsigned int)(aReadPtr[1] << 16) | (unsigned int)(aReadPtr[2] << 8) | aReadPtr[3];
}

unsigned long long CMP4Encrype::qcIntReadUint64BE(const unsigned char* aReadPtr)
{
	unsigned int nSizeHigh = (unsigned int)(aReadPtr[0] << 24) | (unsigned int)(aReadPtr[1] << 16) | (unsigned int)(aReadPtr[2] << 8) | aReadPtr[3];
	unsigned int nSizeLow = (unsigned int)(aReadPtr[4] << 24) | (unsigned int)(aReadPtr[5] << 16) | (unsigned int)(aReadPtr[6] << 8) | aReadPtr[7];

	unsigned long long nRetVal = nSizeHigh;

	return ((nRetVal << 32) | nSizeLow);
}
