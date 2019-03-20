/*******************************************************************************
File:		CNormalHLSDrm.cpp

Contains:	CNormalHLSDrm implement file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-22		Qichao			Create file

*******************************************************************************/
#include "CNormalHLSDrm.h"
#include "HLSDRM.h"
#include "qcDef.h"
#include "qcIO.h"
#include "stdio.h"


CNormalHLSDrm::CNormalHLSDrm()
{
	m_pAESCBCIns = NULL;
	m_bInited = false;
	m_eDrmType = E_HLS_DRM_NONE;

	memset(m_strLastKeyURL, 0, 1024);
	memset(m_aLastKeyValue, 0, 16);
	memset(m_aLastIVValue, 0, 16);
	memset(m_strLastKeyString, 0, 1024);
	memset(m_strLastKeyURL, 0, 1024);
	m_bDynamicKey = false;
}

CNormalHLSDrm::~CNormalHLSDrm()
{
	QC_DEL_P(m_pAESCBCIns);
}

void  CNormalHLSDrm::Init(void*   pParams, int iExtraInfoSize, void* pExtraInfo)
{
	char*   pFindIV = NULL;
	bool    bSameKeyIV = false;
	E_HLS_DRM_TYPE   eDrmType = E_HLS_DRM_MAX;
	S_DRM_HSL_PROCESS_INFO*   pDRMHls = (S_DRM_HSL_PROCESS_INFO*)pParams;
	if (pDRMHls == NULL)
	{
		return;
	}

	if (strlen(pDRMHls->strKeyString) == 0)
	{
		return;
	}

	if (m_pAESCBCIns == NULL)
	{
		m_pAESCBCIns = new CAES_CBC;
	}

	pFindIV = strstr(pDRMHls->strKeyString, "IV=");
	if (strlen(m_strLastKeyString) != 0)
	{
		if (strcmp(m_strLastKeyString, pDRMHls->strKeyString) == 0 && pFindIV != NULL && m_bDynamicKey == false)
		{
			printf("same key and IV!");
			bSameKeyIV = true;
		}
	}
	
	if (bSameKeyIV == false)
	{
		GetKeyAndIV(pParams, m_aLastKeyValue, 16, m_aLastIVValue, 16, m_eDrmType);
		strcpy(m_strLastKeyString, pDRMHls->strKeyString);
	}

	if (m_eDrmType == E_HLS_DRM_QINIU_DRM && pExtraInfo != NULL && iExtraInfoSize <= 16)
	{
		memcpy(m_aLastKeyValue, pExtraInfo, iExtraInfoSize);
	}

	if (m_pAESCBCIns != NULL)
	{
		m_pAESCBCIns->setKey(m_aLastKeyValue, 16);
		m_pAESCBCIns->setIV(m_aLastIVValue, 16);
	}
}

void  CNormalHLSDrm::UnInit()
{
	QC_DEL_P(m_pAESCBCIns);
}

int   CNormalHLSDrm::DecryptData(const unsigned char *pSrc, unsigned int ulSrc, unsigned char *pDes, unsigned int* pulDes, bool bEnd)
{
	m_pAESCBCIns->decryptData(pSrc, ulSrc, pDes, pulDes, bEnd);
	return 0;
}

int   CNormalHLSDrm::GetKeyAndIV(void* pParmas, unsigned char*  pKeyData, int iKeySize, unsigned char* pIV, int iIVSize, E_HLS_DRM_TYPE&   eDrmType)
{
	S_DRM_HSL_PROCESS_INFO*   pDRMHls = (S_DRM_HSL_PROCESS_INFO*)pParmas;
	char*  pFindStart = NULL;
	char*  pFindEnd = NULL;
	char   strKeyURLInner[1024] = { 0 };
	char   strAbsKeyURL[1024] = { 0 };
	int    iRet = 0;

	eDrmType = E_HLS_DRM_NONE;
	pFindStart = strstr(pDRMHls->strKeyString, "METHOD=");
	if (pFindStart != NULL)
	{
		if (memcmp(pFindStart, "METHOD=NONE", strlen("METHOD=NONE")) == 0)
		{
			m_eDrmType = E_HLS_DRM_NONE;
		}
		else if (memcmp(pFindStart, "METHOD=AES-128", strlen("METHOD=AES-128")) == 0)
		{
			m_eDrmType = E_HLS_DRM_AES128;
		}
		else if (memcmp(pFindStart, "METHOD=QINIU-PROTECTION-", strlen("METHOD=QINIU-PROTECTION-")) == 0)
		{
			m_eDrmType = E_HLS_DRM_QINIU_DRM;
		}
		else
		{
			m_eDrmType = E_HLS_DRM_MAX;
		}
	}

	//Get Key
	pFindStart = strstr(pDRMHls->strKeyString, "URI=\"");
	if (pFindStart != NULL)
	{
		pFindEnd = strstr(pFindStart+5, "\"");
		if (pFindEnd != NULL)
		{
			memcpy(strKeyURLInner, pFindStart + 5, pFindEnd - pFindStart - 5);
			GetAbsoluteURL(strAbsKeyURL, strKeyURLInner, pDRMHls->strCurURL);
			if (m_bDynamicKey == true || strcmp(strAbsKeyURL, m_strLastKeyURL) != 0)
			{
				GetKeyData(pDRMHls->pReserved, strAbsKeyURL, pKeyData, iKeySize);
				memset(m_strLastKeyURL, 0, 1024);
				strcpy(m_strLastKeyURL, strAbsKeyURL);
			}
		}
	}

	//Get IV
	iRet = GetIV_HLS(pDRMHls->strKeyString, pDRMHls->ulSequenceNum, pIV);
	return iRet;
}

int   CNormalHLSDrm::GetKeyData(void*  pHandle, char*  pKeyURL, unsigned char* pKey, int iKeySize)
{
	long long  illDataSize = 0;
	int iRet = QC_ERR_FAILED;
	QC_IO_Func*   pIOHandle = (QC_IO_Func*)pHandle;

	if (pHandle == NULL)
	{
		return iRet;
	}

    do 
    {
		if (pIOHandle->Open(pIOHandle->hIO, pKeyURL, 0, QCIO_FLAG_READ) != QC_ERR_NONE)
		{
			printf("Can't open the url:%s", pKeyURL);
		}

		illDataSize = pIOHandle->GetSize(pIOHandle->hIO);
		if (illDataSize == -1)
		{
			printf("Can't get the size");
			break;
		}
		iRet = pIOHandle->Read(pIOHandle->hIO, pKey, iKeySize, true, QCIO_READ_DATA);

    } while (false);

	pIOHandle->Close(pIOHandle->hIO);
	return iRet;
}


void CNormalHLSDrm::PurgeURL(char*  pURL)
{
	int  iLen = 0;
	int  iIndex = 0;
	char  strForSplit[4096] = { 0 };
	char*  pCur = NULL;
	char*  pFind = NULL;
	char*  pPathList[1024] = { 0 };
	int    iPathListCount = 0;
	if (strstr(pURL, "http") != NULL && strstr(pURL, "/../"))
	{
		pFind = strstr(pURL, "://");
		strcpy(strForSplit, pFind + 3);
		pCur = strtok(strForSplit, "/");
		pPathList[iPathListCount] = pCur;
		iPathListCount++;

		while ((pCur = strtok(NULL, "/")))
		{
			if (strcmp(pCur, "..") == 0)
			{
				iPathListCount--;
			}
			else
			{
				pPathList[iPathListCount] = pCur;
				iPathListCount++;
			}
		}

		memset(pFind + 2, 0, strlen(pFind) - 2);
		for (iIndex = 0; iIndex < iPathListCount; iIndex++)
		{
			strcat(pURL, "/");
			strcat(pURL, pPathList[iIndex]);
		}
	}
}

void  CNormalHLSDrm::GetAbsoluteURL(char*  pURL, char* pSrc, char* pRefer)
{
	//and more case later
	char*  pFind = NULL;
	char*  pLastSep = NULL;
	char*  pFirstSep = NULL;
	int    iIndex = 0;


	//pSrc is http:// or https://
	pFind = strstr(pSrc, "://");
	if (pFind != NULL)
	{
		strcpy(pURL, pSrc);
	}
	else
	{
		pFind = strstr(pRefer, "://");
		if (pFind != NULL)
		{
			pFirstSep = strchr(pFind + strlen("://"), '/');
			pLastSep = strrchr(pRefer, '/');
			if (*pSrc == '/')
			{
				memcpy(pURL, pRefer, pFirstSep - pRefer);
				strcat(pURL, pSrc);
			}
			else
			{
				if (pLastSep != NULL)
				{
					memcpy(pURL, pRefer, pLastSep - pRefer + 1);
					strcat(pURL, pSrc);
					PurgeURL(pURL);
				}
			}
		}
		else
		{
			//local path
			//Linux
			pLastSep = strrchr(pRefer, '/');
			if (pLastSep != NULL)
			{
				memcpy(pURL, pRefer, pLastSep - pRefer + 1);
				strcat(pURL, pSrc);
				return;
			}

			//Windows
			pLastSep = strrchr(pRefer, '\\');
			if (pLastSep != NULL)
			{
				memcpy(pURL, pRefer, pLastSep - pRefer + 1);
				strcat(pURL, pSrc);
				return;
			}
		}
	}

	return;
}

int  CNormalHLSDrm::GetIV_HLS(char* pKeyString, unsigned int uSequenceNum, unsigned char* pIV)
{
	char*  pFindStart = NULL;
	char*  pFindEnd = NULL;
	char   strIV[128] = { 0 };
	if (NULL == pKeyString || NULL == pIV)
	{
		printf("empty pointor");
		return AES128_ERR_PARAMS;
	}

	pFindStart = strstr(pKeyString, "IV=");
	if (pFindStart)
	{
		pFindStart += strlen("IV=");
		pFindEnd = strchr(pFindStart, ',');
		if (pFindEnd != NULL)
		{
			strncpy(strIV, pFindStart, pFindEnd - pFindStart);
		}
		else
		{
			strcpy(strIV, pFindStart);
		}
	}

	if (strlen(strIV) != 0)
	{
		if ('0' == strIV[0] &&
			('x' == strIV[1] || 'X' == strIV[1]))
		{
			Str2IV(pIV, &(strIV[2]));
		}
		else
		{
			strcpy((char *)pIV, strIV);
		}
	}
	else
	{
		pIV[15] = (unsigned char)((uSequenceNum)& 0x000000ff);
		pIV[14] = (unsigned char)(((uSequenceNum) >> 8) & 0x000000ff);
		pIV[13] = (unsigned char)(((uSequenceNum) >> 16) & 0x000000ff);
		pIV[12] = (unsigned char)(((uSequenceNum) >> 24) & 0x000000ff);
	}

	return 0;
}

void CNormalHLSDrm::Str2IV(unsigned char* pIV, char* pStr)
{
	char *pCur = NULL;
	char strIV[33] = {0};
	char strtemp[3] = {0};
	int  iValue = 0;

	if (NULL == pIV || NULL == pStr)
	{
		printf("empty pointor");
		return;
	}

	memset(strIV, '0', 33);

	pCur = strIV + 32 - strlen(pStr);
	strcpy(pCur, pStr);

	for (int i = 0; i < 16; i++)
	{
		memset(strtemp, 0, 3);
		iValue = 0;
		strtemp[0] = strIV[2 * i];
		strtemp[1] = strIV[2 * i + 1];
		strtemp[2] = '\0';
		sscanf(strtemp, "%x", &iValue);
		pIV[i] = iValue;
	}
}

bool CNormalHLSDrm::NeedDecrypt()
{
	if (m_eDrmType == E_HLS_DRM_AES128 || m_eDrmType == E_HLS_DRM_QINIU_DRM)
	{
		return true;
	}

	return false;
}