/*******************************************************************************
File:		CNormalHLSDrm.h

Contains:	CNormalHLSDrm header file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-22		Qichao			Create file

*******************************************************************************/
#ifndef __QP_CNORMAL_HLS_DRM_H__
#define __QP_CNORMAL_HLS_DRM_H__

#include "AES_CBC.h"
#include "HLSDRM.h"

class CNormalHLSDrm
{
public:
	CNormalHLSDrm();
	~CNormalHLSDrm();
	void  Init(void*   pParams, int iExtraInfoSize, void* pExtraInfo);
	void  UnInit();
	int   DecryptData(const unsigned char *pSrc, unsigned int ulSrc, unsigned char *pDes, unsigned int* pulDes, bool bEnd);
	bool  NeedDecrypt();

private:
	int   GetKeyAndIV(void* pParmas, unsigned char*  pKeyData, int iKeySize, unsigned char* pIV, int iIVSize, E_HLS_DRM_TYPE&   eDrmType);
	int   GetKeyData(void*  pHandle, char*  pKeyURL, unsigned char* pKey, int iKeySize);
	void  GetAbsoluteURL(char*  pURL, char* pSrc, char* pRefer);
	void  PurgeURL(char*  pURL);
	int   GetIV_HLS(char* pKeyString, unsigned int uSequenceNum, unsigned char* pIV);
	void  Str2IV(unsigned char* pIV, char* pStr);
private:
	CAES_CBC*   m_pAESCBCIns;
	bool        m_bInited;
	E_HLS_DRM_TYPE m_eDrmType;

	char            m_strLastKeyURL[1024];
	unsigned char   m_aLastKeyValue[16];
	unsigned char   m_aLastIVValue[16];
	char            m_strLastKeyString[1024];
	bool            m_bDynamicKey;
};

#endif