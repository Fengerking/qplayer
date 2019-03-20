/*******************************************************************************
	File:		CExtAVSource.cpp

	Contains:	qc CExtAVSource source implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-18		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"
#include "qcPlayer.h"

#include "CExtAVSource.h"
#include "CQCSource.h"
#include "CQCFFSource.h"
#include "CMsgMng.h"

#include "UAVParser.h"
#include "USystemFunc.h"
#include "USourceFormat.h"
#include "UUrlParser.h"
#include "ULogFunc.h"

CExtAVSource::CExtAVSource(CBaseInst * pBaseInst, void * hInst)
	: CBaseSource(pBaseInst, hInst)
{
	SetObjectName ("CExtAVSource");
	m_bSourceLive = true;
	m_nStmVideoNum = 1; 
	m_nStmAudioNum = 1;
	m_nStmVideoPlay = 0; 
	m_nStmAudioPlay = 0;

	memset(&m_fmtAudio, 0, sizeof(m_fmtAudio));
	m_fmtAudio.nCodecID = m_pBaseInst->m_nAudioCodecID;
	m_fmtAudio.nChannels = 1;
	m_fmtAudio.nSampleRate = 0;
	m_fmtAudio.nBits = 16;
	memset(&m_fmtVideo, 0, sizeof(m_fmtVideo));
	m_fmtVideo.nCodecID = m_pBaseInst->m_nVideoCodecID;
	m_fmtVideo.nWidth = 640;
	m_fmtVideo.nHeight = 480;

	m_pFmtVideo = &m_fmtVideo;
	m_pFmtAudio = &m_fmtAudio;
    
    m_llMinBuffTime = m_pBaseInst->m_pSetting->g_qcs_nMinPlayBuffTime;
    m_llMaxBuffTime = m_pBaseInst->m_pSetting->g_qcs_nMaxSaveLiveBuffTime;
    QCLOGI("Min buf time %lld, max buf time %lld", m_llMinBuffTime, m_llMaxBuffTime);
}

CExtAVSource::~CExtAVSource(void)
{
	QC_DEL_A(m_fmtAudio.pHeadData);
	Close ();
}

int CExtAVSource::OpenIO(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource)
{
	int nRC = QC_ERR_NONE;
	if (m_pBuffMng == NULL)
		m_pBuffMng = new CBuffMng(m_pBaseInst);
	return nRC;
}

int CExtAVSource::Close(void)
{
	return CBaseSource::Close();
}

int	CExtAVSource::Start(void)
{
	return QC_ERR_NONE;
}

int CExtAVSource::SetParam (int nID, void * pParam)
{
	int nRC = QC_ERR_NONE;

	if (nID == QCPLAY_PID_EXT_SOURCE_DATA)
	{
		if (m_pBuffMng == NULL)
			m_pBuffMng = new CBuffMng(m_pBaseInst);
		QC_DATA_BUFF * pBuff = (QC_DATA_BUFF *)pParam;
		if (pBuff->nMediaType == QC_MEDIA_Video)
		{
//			QCLOGI("The MUX video IO --- Flag = %08X  Size = % 8d, Time = % 8lld", pBuff->uFlag, pBuff->uSize, pBuff->llTime);
		}
		else
		{
//			QCLOGI("The MUX audio IO +++ Flag = %08X  Size = % 8d, Time = % 8lld", pBuff->uFlag, pBuff->uSize, pBuff->llTime);
		}	
		if (pBuff->nMediaType == QC_MEDIA_Video && m_fmtVideo.pHeadData == NULL)
		{
			if ((pBuff->uFlag & QCBUFF_HEADDATA) == QCBUFF_HEADDATA)
			{
				m_fmtVideo.nHeadSize = pBuff->uSize;
				m_fmtVideo.pHeadData = new unsigned char[m_fmtVideo.nHeadSize];
				memcpy(m_fmtVideo.pHeadData, pBuff->pBuff, m_fmtVideo.nHeadSize);
			}
			else if ((pBuff->uFlag & QCBUFF_KEY_FRAME) == QCBUFF_KEY_FRAME)
			{
				int				nNalHead = 0X010000;
				unsigned char * pHead = pBuff->pBuff;
				unsigned char * pHeadPos = NULL;
				while (pHead - pBuff->pBuff < pBuff->uSize - 5)
				{
					if (memcmp(pHead, &nNalHead, 3) == 0)
					{
						if (pHeadPos == NULL)
						{
							pHeadPos = pHead;
							if (pHead > pBuff->pBuff && *(pHead - 1) == 0)
								pHeadPos = pHeadPos - 1;
						}

						if ((pHead[3] & 0X1F) == 5)
						{ 
							if (pHead > pBuff->pBuff && *(pHead - 1) == 0)
								pHead--;
							m_fmtVideo.nHeadSize = pHead - pHeadPos;
							m_fmtVideo.pHeadData = new unsigned char[m_fmtVideo.nHeadSize];
							memcpy(m_fmtVideo.pHeadData, pHeadPos, m_fmtVideo.nHeadSize);
							break;
						}
						pHead += 4;
					}
					pHead++;
				}
			}

			if (m_fmtVideo.pHeadData != NULL)
			{
				QCLOGI("The MUX video IO format --- Flag = %08X  Size = % 8d, Time = % 8lld", pBuff->uFlag, pBuff->uSize, pBuff->llTime);	
				int numRef = 0;
				qcAV_FindAVCDimensions(m_fmtVideo.pHeadData, m_fmtVideo.nHeadSize, &m_fmtVideo.nWidth, &m_fmtVideo.nHeight, &numRef);
				m_pBuffMng->SetFormat(QC_MEDIA_Video, &m_fmtVideo);
			}
		}
        else if (pBuff->nMediaType == QC_MEDIA_Audio && m_fmtAudio.nSampleRate == 0)
        {
            if (m_fmtAudio.nCodecID == QC_CODEC_ID_AAC)
            {
                if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT && pBuff->pFormat)
                {
                    QC_AUDIO_FORMAT* pFmt = (QC_AUDIO_FORMAT*)pBuff->pFormat;
                    m_fmtAudio.nSampleRate = pFmt->nSampleRate;
                    m_fmtAudio.nChannels = pFmt->nChannels;
                    m_fmtAudio.nBits = pFmt->nBits;
                }
                else
                {
                    qcAV_ParseADTSAACHeaderInfo(pBuff->pBuff, pBuff->uSize, &m_fmtAudio.nSampleRate, &m_fmtAudio.nChannels, &m_fmtAudio.nBits);
                }
            }
            else if(m_fmtAudio.nCodecID == QC_CODEC_ID_G711U || m_fmtAudio.nCodecID == QC_CODEC_ID_G711A)
                m_fmtAudio.nSampleRate = 8000;
			if (m_fmtAudio.nSampleRate != 0)
			{
				QCLOGI("The MUX audio IO format +++ Flag = %08X  Size = % 8d, Time = % 8lld", pBuff->uFlag, pBuff->uSize, pBuff->llTime);
				m_pBuffMng->SetFormat(QC_MEDIA_Audio, &m_fmtAudio);
			}
        }

		QC_DATA_BUFF * pNewBuff = m_pBuffMng->GetEmpty(pBuff->nMediaType, pBuff->uSize);
		if (pNewBuff == NULL)
			return QC_ERR_MEMORY;
		pNewBuff->uBuffType = QC_BUFF_TYPE_Data;
		pNewBuff->nMediaType = pBuff->nMediaType;
		pNewBuff->llTime = pBuff->llTime;
		pNewBuff->uFlag = pBuff->uFlag;
		pNewBuff->uSize = pBuff->uSize;
		if (pNewBuff->uBuffSize < pBuff->uSize)
		{
			QC_DEL_A(pNewBuff->pBuff);
			pNewBuff->uBuffSize = pBuff->uSize;
		}
		if (pNewBuff->pBuff == NULL)
			pNewBuff->pBuff = new unsigned char[pNewBuff->uBuffSize];
		memcpy(pNewBuff->pBuff, pBuff->pBuff, pBuff->uSize);
		m_pBuffMng->Send(pNewBuff);
		return nRC;
	}

	nRC = CBaseSource::SetParam (nID, pParam);
	if (nRC == QC_ERR_NONE)
		return QC_ERR_NONE;

	return QC_ERR_PARAMID;
}

int CExtAVSource::GetParam (int nID, void * pParam)
{
	int nRC = QC_ERR_NONE;

	nRC = CBaseSource::GetParam (nID, pParam);
	if (nRC == QC_ERR_NONE)
		return QC_ERR_NONE;

	return QC_ERR_PARAMID;
}
