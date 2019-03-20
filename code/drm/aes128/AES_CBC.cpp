/*******************************************************************************
File:		AES_CBC.cpp

Contains:	AES_CBC implement file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-22		Qichao			Create file

*******************************************************************************/
#include "AES_CBC.h"
#include "stdio.h"

CAES_CBC::CAES_CBC(void)
: m_ctx(NULL)
{
	memset(m_CBC, 0, 16);
}

CAES_CBC::~CAES_CBC(void)
{
	if (m_ctx)
	{
		aes_decrypt_deinit(m_ctx);
		m_ctx = NULL;
	}
}

u32 CAES_CBC::setKey(const u8 *pKey, u32 uLen)
{
	if (m_ctx)
	{
		aes_decrypt_deinit(m_ctx);
		m_ctx = NULL;
	}

	m_ctx = aes_decrypt_init(pKey, uLen);
	return (NULL != m_ctx) ? 0 : AES128_ERR_PARAMS;
}

u32 CAES_CBC::setIV(const u8 *pIV, u32 uLen)
{
	memcpy(m_CBC, pIV, uLen);
	return 0;
}

u32 CAES_CBC::decryptData(const u8 *pSrc, u32 uSrc, u8 *pDes, u32* puDes, bool bEnd)
{
	if (NULL == pSrc || NULL == pDes || NULL == puDes)
	{
		return AES128_ERR_PARAMS;
	}

	u8 tmp[AES_BLOCK_SIZE] = {0};

	if (uSrc % 16)
	{
		//printf("%d is not an integer multiple of 16", uSrc);
	}

	for (u32 i = 0; i < (uSrc / AES_BLOCK_SIZE); i++)
	{
		const u8 *pos = &pSrc[AES_BLOCK_SIZE * i];
		memcpy(tmp, pos, AES_BLOCK_SIZE);

		aes_decrypt(m_ctx, pos, &pDes[AES_BLOCK_SIZE * i]);
		for (int j = 0; j < AES_BLOCK_SIZE; j++)
			pDes[AES_BLOCK_SIZE * i + j] ^= m_CBC[j];

		memcpy(m_CBC, tmp, AES_BLOCK_SIZE);
	}

	if (bEnd == true)
	{
		*puDes = uSrc - pDes[uSrc - 1];

		if (0 == pDes[uSrc - 1])
		{
			//printf("last: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", pDes[uSrc - 16], pDes[uSrc - 15], pDes[uSrc - 14], pDes[uSrc - 13], pDes[uSrc - 12], pDes[uSrc - 11], pDes[uSrc - 10], pDes[uSrc - 9], pDes[uSrc - 8], pDes[uSrc - 7], pDes[uSrc - 6], pDes[uSrc - 5], pDes[uSrc - 4], pDes[uSrc - 3], pDes[uSrc - 2], pDes[uSrc - 1]);
		}
	}
	else
	{
		*puDes = uSrc;
	}

	return 0;
}
