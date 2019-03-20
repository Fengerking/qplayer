/*******************************************************************************
File:		CDataBox.cpp

Contains:	DataBox for Callback implement file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-04		Qichao			Create file

*******************************************************************************/
#include "stdlib.h"
#include "CDataBox.h"
#include "qcErr.h"

CDataBox::CDataBox()
:m_pData(NULL)
,m_uDataSize(0)
,m_uUsedDataSize(0)
{
}

CDataBox::~CDataBox()
{
	if(m_pData)
	{
		delete []m_pData;
		m_pData = NULL;
		m_uDataSize = 0;
	}
}

unsigned int CDataBox::MallocData(void* pUserData, unsigned char** ppData, unsigned int ulSize)
{
	unsigned int ulRet = QC_ERR_FAILED;

	if(NULL == pUserData)
	{
		return ulRet;
	}

	CDataBox* pUser = (CDataBox*)pUserData;


	if(pUser->m_pData)
	{
		delete []pUser->m_pData;
		pUser->m_pData = NULL;
	}

	
	pUser->m_pData = new unsigned char[ulSize];
	
	if(pUser->m_pData)
	{
		pUser->m_uDataSize = ulSize;
		pUser->m_uUsedDataSize = 0;

		memset(pUser->m_pData, 0x0, pUser->m_uDataSize);
		*ppData = pUser->m_pData;
		ulRet = QC_ERR_NONE;
	}

	return ulRet;
}

unsigned int CDataBox::GetDataAndSize(unsigned char** ppData, unsigned int** ppUsedDataSize, unsigned int* pDataSize)
{
	if(NULL == ppData || NULL == ppUsedDataSize)
	{
		return QC_ERR_FAILED;
	}
	
	*ppData = m_pData;
	*ppUsedDataSize = &m_uUsedDataSize;
	
	if(pDataSize)
	{
		*pDataSize = m_uDataSize;
	}
	
	return QC_ERR_NONE;
}

