/*******************************************************************************
	File:		CQCAdpcmDec.cpp

	Contains:	The qc audio dec implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-02		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcPlayer.h"

#include "CQCAdpcmDec.h"
#include "ULogFunc.h"

static unsigned short MuLawDecompressTable[] =
{
	0x8284, 0x8684, 0x8A84, 0x8E84, 0x9284, 0x9684, 0x9A84, 0x9E84,
	0xA284, 0xA684, 0xAA84, 0xAE84, 0xB284, 0xB684, 0xBA84, 0xBE84,
	0xC184, 0xC384, 0xC584, 0xC784, 0xC984, 0xCB84, 0xCD84, 0xCF84,
	0xD184, 0xD384, 0xD584, 0xD784, 0xD984, 0xDB84, 0xDD84, 0xDF84,
	0xE104, 0xE204, 0xE304, 0xE404, 0xE504, 0xE604, 0xE704, 0xE804,
	0xE904, 0xEA04, 0xEB04, 0xEC04, 0xED04, 0xEE04, 0xEF04, 0xF004,
	0xF0C4, 0xF144, 0xF1C4, 0xF244, 0xF2C4, 0xF344, 0xF3C4, 0xF444,
	0xF4C4, 0xF544, 0xF5C4, 0xF644, 0xF6C4, 0xF744, 0xF7C4, 0xF844,
	0xF8A4, 0xF8E4, 0xF924, 0xF964, 0xF9A4, 0xF9E4, 0xFA24, 0xFA64,
	0xFAA4, 0xFAE4, 0xFB24, 0xFB64, 0xFBA4, 0xFBE4, 0xFC24, 0xFC64,
	0xFC94, 0xFCB4, 0xFCD4, 0xFCF4, 0xFD14, 0xFD34, 0xFD54, 0xFD74,
	0xFD94, 0xFDB4, 0xFDD4, 0xFDF4, 0xFE14, 0xFE34, 0xFE54, 0xFE74,
	0xFE8C, 0xFE9C, 0xFEAC, 0xFEBC, 0xFECC, 0xFEDC, 0xFEEC, 0xFEFC,
	0xFF0C, 0xFF1C, 0xFF2C, 0xFF3C, 0xFF4C, 0xFF5C, 0xFF6C, 0xFF7C,
	0xFF88, 0xFF90, 0xFF98, 0xFFA0, 0xFFA8, 0xFFB0, 0xFFB8, 0xFFC0,
	0xFFC8, 0xFFD0, 0xFFD8, 0xFFE0, 0xFFE8, 0xFFF0, 0xFFF8, 0x0000,
	0x7D7C, 0x797C, 0x757C, 0x717C, 0x6D7C, 0x697C, 0x657C, 0x617C,
	0x5D7C, 0x597C, 0x557C, 0x517C, 0x4D7C, 0x497C, 0x457C, 0x417C,
	0x3E7C, 0x3C7C, 0x3A7C, 0x387C, 0x367C, 0x347C, 0x327C, 0x307C,
	0x2E7C, 0x2C7C, 0x2A7C, 0x287C, 0x267C, 0x247C, 0x227C, 0x207C,
	0x1EFC, 0x1DFC, 0x1CFC, 0x1BFC, 0x1AFC, 0x19FC, 0x18FC, 0x17FC,
	0x16FC, 0x15FC, 0x14FC, 0x13FC, 0x12FC, 0x11FC, 0x10FC, 0x0FFC,
	0x0F3C, 0x0EBC, 0x0E3C, 0x0DBC, 0x0D3C, 0x0CBC, 0x0C3C, 0x0BBC,
	0x0B3C, 0x0ABC, 0x0A3C, 0x09BC, 0x093C, 0x08BC, 0x083C, 0x07BC,
	0x075C, 0x071C, 0x06DC, 0x069C, 0x065C, 0x061C, 0x05DC, 0x059C,
	0x055C, 0x051C, 0x04DC, 0x049C, 0x045C, 0x041C, 0x03DC, 0x039C,
	0x036C, 0x034C, 0x032C, 0x030C, 0x02EC, 0x02CC, 0x02AC, 0x028C,
	0x026C, 0x024C, 0x022C, 0x020C, 0x01EC, 0x01CC, 0x01AC, 0x018C,
	0x0174, 0x0164, 0x0154, 0x0144, 0x0134, 0x0124, 0x0114, 0x0104,
	0x00F4, 0x00E4, 0x00D4, 0x00C4, 0x00B4, 0x00A4, 0x0094, 0x0084,
	0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050, 0x0048, 0x0040,
	0x0038, 0x0030, 0x0028, 0x0020, 0x0018, 0x0010, 0x0008, 0x0000
};

CQCAdpcmDec::CQCAdpcmDec(CBaseInst * pBaseInst, void * hInst)
	: CBaseAudioDec(pBaseInst, hInst)
	, m_bNewFormat(false)
{
	SetObjectName ("CQCAdpcmDec");
	memset(&m_buffData, 0, sizeof(m_buffData));
	m_buffData.nMediaType = QC_MEDIA_Audio;
	m_buffData.uBuffType = QC_BUFF_TYPE_Data;
}

CQCAdpcmDec::~CQCAdpcmDec(void)
{
	Uninit ();
}

int CQCAdpcmDec::Init (QC_AUDIO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;

	Uninit ();

	int nRC = 0;
	if (pFmt->pHeadData != NULL && pFmt->nHeadSize > 0)
	{
		// do nothing now.
	}
	memcpy (&m_fmtAudio, pFmt, sizeof (m_fmtAudio));
	m_fmtAudio.pHeadData = NULL;
	m_fmtAudio.nHeadSize = 0;
	m_fmtAudio.pPrivateData = NULL;
	if (m_fmtAudio.nChannels > 2)
		m_fmtAudio.nChannels = 2;

	if (m_fmtAudio.nCodecID == QC_CODEC_ID_G711A || pFmt->nCodecID == QC_CODEC_ID_G711U)
	{
		m_fmtAudio.nChannels = 1;
		if (m_fmtAudio.nSampleRate != 8000 && m_fmtAudio.nSampleRate != 16000)
			m_fmtAudio.nSampleRate = 8000;
	}

	m_uBuffFlag = 0;
	m_nDecCount = 0;

	m_buffData.uBuffSize = 192000;
	m_buffData.pBuff = new unsigned char[m_buffData.uBuffSize];

	return QC_ERR_NONE;
}

int CQCAdpcmDec::Uninit(void)
{
	QC_DEL_A(m_buffData.pBuff);
	m_buffData.uBuffSize = 0;

	return QC_ERR_NONE;
}

int CQCAdpcmDec::Flush (void)
{
	CAutoLock lock (&m_mtBuffer);
	return QC_ERR_NONE;
}

int CQCAdpcmDec::SetBuff (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL)
		return QC_ERR_ARG;

	CAutoLock lock (&m_mtBuffer);
	CBaseAudioDec::SetBuff (pBuff);

	if ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
	{
		if (m_nDecCount > 0)
			Flush ();
	}
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_AUDIO_FORMAT * pFmt = (QC_AUDIO_FORMAT *)pBuff->pFormat;
		if (pFmt != NULL && ((m_fmtAudio.nSampleRate == 0 || m_fmtAudio.nChannels == 0) || (pFmt->nCodecID != m_fmtAudio.nCodecID)))
		{
			pFmt->nChannels = 0;
			pFmt->nSampleRate = 0;
			Init(pFmt);
		}
	}
	if (m_pBuffData != NULL)
		m_uBuffFlag = pBuff->uFlag;

	m_buffData.llTime = pBuff->llTime;
	if (m_fmtAudio.nCodecID == QC_CODEC_ID_G711A)
	{
		short * pOut = (short *)m_buffData.pBuff;
		for (int i = 0; i < pBuff->uSize; i++)
		{
			*pOut++ = g711Decode(pBuff->pBuff[i]);
		}
		m_buffData.uSize = pBuff->uSize * 2;
	}
	else if (m_fmtAudio.nCodecID == QC_CODEC_ID_G711U)
	{
		short * pOut = (short *)m_buffData.pBuff;
		for (int i = 0; i < pBuff->uSize; i++)
		{
			*pOut++ = (short)MuLawDecompressTable[pBuff->pBuff[i]];
		}
		m_buffData.uSize = pBuff->uSize * 2;
	}
	m_pBuffData = &m_buffData;

	return QC_ERR_NONE;
}

int CQCAdpcmDec::GetBuff (QC_DATA_BUFF ** ppBuff)
{
	if (ppBuff == NULL)
		return QC_ERR_ARG;

	if (m_pBuffData == NULL)
		return QC_ERR_NEEDMORE;

	CAutoLock lock (&m_mtBuffer);
	if (m_pBuffData != NULL)
        m_pBuffData->uFlag = 0;
    
	CBaseAudioDec::GetBuff (&m_pBuffData);

	*ppBuff = m_pBuffData;
	m_pBuffData = NULL;
	m_nDecCount++;
  
	return QC_ERR_NONE;
}

short CQCAdpcmDec::g711Decode(unsigned char cData)
{
	cData ^= 0xD5;
	int sign = cData & 0x80;
	int exponent = (cData & 0x70) >> 4;
	int data = cData & 0x0f;
	data <<= 4;
	data += 8;
	if (exponent != 0)
		data += 0x100;
	if (exponent > 1)
		data <<= (exponent - 1);

	return (short)(sign == 0 ? data : -data);
}
