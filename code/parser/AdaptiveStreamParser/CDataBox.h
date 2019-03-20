/*******************************************************************************
File:		CDataBox.h

Contains:	DataBox for Callback Header file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-04		Qichao			Create file

*******************************************************************************/
#ifndef __CDATA_BOX_H__
#define __CDATA_BOX_H__
#include "stdio.h"
#include "qcType.h"

typedef struct
{
	//callback instance
	void* pUserData;

/**
 * Callback function. The source will send the data out..
 * \param pUserData [in] The user data which was set by Open().
 * \param pData [in] The pData type is unsigned char
 * \param nSize [in] The nSize is unsigned int
 *                   the param type is depended on the nOutputType
 */
	unsigned int (QC_API * MallocData) (void* pUserData, unsigned char** ppData, unsigned int ulSize);

}S_DATABOX_CALLBACK;

class CDataBox
{
public:
	CDataBox();
	~CDataBox();
	
	static unsigned int MallocData(void* pUserData, unsigned char** ppData, unsigned int ulSize);
	unsigned int GetDataAndSize(unsigned char** ppData, unsigned int** ppUsedDataSize, unsigned int* pDataSize = NULL);
public:
	unsigned char*	    m_pData;
	unsigned int		m_uUsedDataSize;
	unsigned int		m_uDataSize;	
};

#endif


