/*******************************************************************************
	File:		CBaseAudioDec.cpp

	Contains:	The base audio dec implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CBaseAudioDec.h"

CBaseAudioDec::CBaseAudioDec(CBaseInst * pBaseInst, void * hInst)
	: CBaseObject (pBaseInst)
	, m_hInst (hInst)
	, m_nVolume(100)
	, m_uBuffFlag (0)
	, m_pBuffData (NULL)
	, m_nDecCount (0)
{
	SetObjectName ("CBaseAudioDec");

	memset (&m_fmtAudio, 0, sizeof (m_fmtAudio));
}

CBaseAudioDec::~CBaseAudioDec(void)
{
	Uninit();
	QC_DEL_A (m_fmtAudio.pHeadData);
}

int CBaseAudioDec::Init (QC_AUDIO_FORMAT * pFmt)
{
	return QC_ERR_IMPLEMENT;
}

int CBaseAudioDec::Uninit (void)
{
	return QC_ERR_NONE;
}

int CBaseAudioDec::SetBuff (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL)
		return QC_ERR_ARG;
	if ((pBuff->uFlag & QCBUFF_EOS) == QCBUFF_EOS)
		m_uBuffFlag |= QCBUFF_EOS;
	if ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
		m_uBuffFlag |= QCBUFF_NEW_POS;
	if ((pBuff->uFlag & QCBUFF_DISCONNECT) == QCBUFF_DISCONNECT)
		m_uBuffFlag |= QCBUFF_DISCONNECT;
	return QC_ERR_NONE;
}

int CBaseAudioDec::GetBuff (QC_DATA_BUFF ** ppBuff)
{
	if (ppBuff == NULL || *ppBuff == NULL)
		return QC_ERR_ARG;
	if ((m_uBuffFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
		(*ppBuff)->uFlag |= QCBUFF_NEW_POS;
	if ((m_uBuffFlag & QCBUFF_EOS) == QCBUFF_EOS)
		(*ppBuff)->uFlag |= QCBUFF_EOS;
	if ((m_uBuffFlag & QCBUFF_DISCONNECT) == QCBUFF_DISCONNECT)
		(*ppBuff)->uFlag |= QCBUFF_DISCONNECT;
	m_uBuffFlag = 0;
	return QC_ERR_NONE;
}

int	CBaseAudioDec::Start (void)
{
	return QC_ERR_NONE;
}

int CBaseAudioDec::Pause (void)
{
	return QC_ERR_NONE;
}

int	CBaseAudioDec::Stop (void)
{
	return QC_ERR_NONE;
}

int CBaseAudioDec::Flush (void)
{
	return QC_ERR_NONE;
}

int CBaseAudioDec::UpdateAudioVolume(unsigned char* pBuffer, int nSize)
{
    if (m_nVolume == 100)
        return QC_ERR_NONE;
    if (!pBuffer || nSize <= 0)
        return QC_ERR_NONE;

    int nAudioValume = m_nVolume;
    
    if (nAudioValume == 0)
    {
        memset (pBuffer, 0, nSize);
    }
    else
    {
        if (m_fmtAudio.nBits == 8)
        {
            int nTmp = 0;
            char * pCData = pCData = (char *)pBuffer;
            for (int i = 0; i < nSize; i++)
            {
                nTmp = (*pCData) * nAudioValume / 100;
                
                if(nTmp >= -256 && nTmp <= 255)
                {
                    *pCData = (char)nTmp;
                }
                else if(nTmp < -256)
                {
                    *pCData = (char)-256;
                }
                else if(nTmp > 255)
                {
                    *pCData = (char)255;
                }
                
                pCData++;
            }
        }
        else
        {
            int nTmp = 0;
            short * pSData = (short *)pBuffer;
            for (int i = 0; i < nSize; i+=2)
            {
                nTmp = ((*pSData) * nAudioValume / 100);
                if (nTmp > 0X7FFF)
                    nTmp = 0X7FFF;
                else if (nTmp < -0X7FFF)
                    nTmp = -0X7FFF;
                *pSData = (short)nTmp;
                pSData++;
            }
        }
    }
    
    return QC_ERR_NONE;
}
