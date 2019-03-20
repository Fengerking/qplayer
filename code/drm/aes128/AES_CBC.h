/*******************************************************************************
File:		AES_CBC.h

Contains:	AES_CBC header file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-22		Qichao			Create file

*******************************************************************************/

#ifndef __CAES_CBC_H__
#define __CAES_CBC_H__
#include "aes.h"

#define AES128_ERR_PARAMS  1

class CAES_CBC
{
public:
	CAES_CBC(void);
	~CAES_CBC(void);

	u32 setKey(const u8 *pKey, u32 uLen);
	u32 setIV(const u8 *pIV, u32 uLen);
	u32 decryptData(const u8 *pSrc, u32 uSrc, u8 *pDes, u32* puDes, bool bEnd);

private:
	void* m_ctx;
	u8 m_CBC[AES_BLOCK_SIZE];
};

#endif //__CAES_CBC_H__
