/*******************************************************************************
	File:		CQCFFSource.cpp

	Contains:	qc ffmepg source implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-18		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"
#include "qcPlayer.h"

#include "CQCFFSource.h"
#include "CMsgMng.h"

#include "UAVParser.h"
#include "USystemFunc.h"
#include "USourceFormat.h"
#include "UUrlParser.h"
#include "ULogFunc.h"

CQCFFSource::CQCFFSource(CBaseInst * pBaseInst, void * hInst)
	: CQCSource(pBaseInst, hInst)
	, m_hLib (NULL)
	, m_llLastVideoTime(-1)
	, m_pVideoBuff(NULL)
	, m_bReadVideoHead(false)
	, m_bReadAudioHead(false)
	, m_bFindKeyFrame(false)
{
	SetObjectName ("CQCFFSource");
	memset(&m_datBuff, 0, sizeof(QC_DATA_BUFF));
    memset(&m_sResInfo, 0, sizeof(m_sResInfo));
}

CQCFFSource::~CQCFFSource(void)
{
	Close ();
}

int CQCFFSource::Open (const char * pSource, int nType)
{
    int nRC = CQCSource::Open(pSource, nType);
    if (nRC == QC_ERR_NONE)
        OnOpenDone (pSource);
    return nRC;
}

int CQCFFSource::OpenIO(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource)
{
    int nRC = CQCSource::OpenIO(pIO, nType, nFormat, pSource);
    if (nRC == QC_ERR_NONE)
        OnOpenDone (pSource);
    return nRC;
}


int CQCFFSource::Close(void)
{
	if (m_pVideoBuff != NULL)
	{
		m_pBuffMng->Return(m_pVideoBuff);
		m_pVideoBuff = NULL;
	}
	m_bFindKeyFrame = false;
    ReleaseResInfo();
	return CQCSource::Close();
}

int CQCFFSource::ReadBuff (QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait)
{
	int nRC = CQCSource::ReadBuff (pBuffInfo, ppBuffData, bWait);
	return nRC;
}

int CQCFFSource::SetParam (int nID, void * pParam)
{
	int nRC = QC_ERR_NONE;
	if (m_fParser.hParser != NULL)
		nRC = m_fParser.SetParam(m_fParser.hParser, nID, pParam);
	if (nRC == QC_ERR_NONE)
		return nRC;

	nRC = CQCSource::SetParam (nID, pParam);
	if (nRC == QC_ERR_NONE)
		return QC_ERR_NONE;

	return QC_ERR_PARAMID;
}

int CQCFFSource::GetParam (int nID, void * pParam)
{
	int nRC = QC_ERR_NONE;
	if (m_fParser.hParser != NULL)
		nRC = m_fParser.SetParam(m_fParser.hParser, nID, pParam);
	if (nRC == QC_ERR_NONE)
		return nRC;

	nRC = CQCSource::GetParam (nID, pParam);
	if (nRC == QC_ERR_NONE)
		return QC_ERR_NONE;

	return QC_ERR_PARAMID;
}

int CQCFFSource::CreateParser (QCIOProtocol nProtocol, QCParserFormat	nFormat)
{
	if (m_hLib != NULL)
		return QC_ERR_STATUS;

	if (nFormat == QC_PARSER_RTSP)
	{
		m_pBaseInst->m_pSetting->g_qcs_nMaxPlayBuffTime = 200;
		m_pBaseInst->m_pSetting->g_qcs_nMinPlayBuffTime = 50;
	}
#ifdef __QC_LIB_ONE__
	ffCreateParser (&m_fParser, nFormat);
#else
	if (m_pBaseInst->m_hLibCodec == NULL)
		m_hLib = (qcLibHandle)qcLibLoad("qcCodec", 0);
	else
		m_hLib = m_pBaseInst->m_hLibCodec;
	if (m_hLib == NULL)
		return QC_ERR_FAILED;
	FFCREATEPARSER pCreate = (FFCREATEPARSER)qcLibGetAddr (m_hLib, "ffCreateParser", 0);
	if (pCreate == NULL)
		return QC_ERR_FAILED;
	pCreate (&m_fParser, nFormat);
#endif //__QC_LIB_ONE__
	if (m_fParser.hParser == NULL)
	{
		QCLOGW ("Create ff source failed!");
		return QC_ERR_FORMAT;
	}
	m_fParser.SetParam(m_fParser.hParser, QCPLAY_PID_Log_Level, &g_nLogOutLevel);
	m_fParser.SetParam(m_fParser.hParser, QCIO_PID_RTSP_UDP_TCP_MODE, &m_pBaseInst->m_pSetting->g_qcs_nRTSP_UDP_TCP_Mode);

	m_llLastVideoTime = -1;
	m_bReadVideoHead = false;
	m_bReadAudioHead = false;

	if (m_nFormat != QC_PARSER_RTSP)
	{
		int nRC = QC_ERR_NONE;
		int nDLNotify = 1;
		if (m_pIO->hIO == NULL)
			nRC = qcCreateIO(m_pIO, nProtocol);
		if (nRC < 0)
			return nRC;
		m_pIO->SetParam(m_pIO->hIO, QCIO_PID_HTTP_NOTIFYDL_PERCENT, &nDLNotify);
	}
	return QC_ERR_NONE;
}

int CQCFFSource::DestroyParser (void)
{
#ifdef __QC_LIB_ONE__
	if (m_fParser.hParser != NULL)
		ffDestroyParser(&m_fParser);
#else
	if (m_hLib == NULL)
		return QC_ERR_NONE;
	if (m_fParser.hParser != NULL)
	{
		FFDESTROYPARSER fDestroy = (FFDESTROYPARSER)qcLibGetAddr (m_hLib, "ffDestroyParser", 0);
		if (fDestroy == NULL)
			return QC_ERR_FAILED;
		fDestroy (&m_fParser);
	}
	if (m_pBaseInst->m_hLibCodec == NULL)
		qcLibFree(m_hLib, 0);
	m_hLib = NULL;
#endif // __QC_LIB_ONE__
	return QC_ERR_NONE;
}

int CQCFFSource::ReadParserBuff (QC_DATA_BUFF * pBuffInfo)
{
	if (m_fParser.hParser == NULL)
		return QC_ERR_STATUS;

	if (pBuffInfo->nMediaType == QC_MEDIA_Video && !m_bReadVideoHead)
	{
		m_bReadVideoHead = true;
		if (CreateHeadBuff(pBuffInfo) == QC_ERR_NONE)
			return QC_ERR_NONE;
	}
	if (pBuffInfo->nMediaType == QC_MEDIA_Audio && !m_bReadAudioHead)
	{
		m_bReadAudioHead = true;
		if (CreateHeadBuff(pBuffInfo) == QC_ERR_NONE)
			return QC_ERR_NONE;
	}

	m_datBuff.nMediaType = pBuffInfo->nMediaType;
	int nRC = m_fParser.Read (m_fParser.hParser, &m_datBuff);
	if (nRC != QC_ERR_NONE)
		return nRC;

	QC_DATA_BUFF *	pDataBuff = NULL;
	int				nDataSize = m_datBuff.uSize;
	if (m_datBuff.nMediaType == QC_MEDIA_Video)
	{
		pDataBuff = m_pVideoBuff;
		if (m_pVideoBuff != NULL)
		{
			if (m_pVideoBuff->llTime != m_datBuff.llTime)
			{
				if (!m_bFindKeyFrame)
				{
					if (!qcAV_IsAVCReferenceFrame(m_pVideoBuff->pBuff, m_pVideoBuff->uSize))
					{
						m_pBuffMng->Return(m_pVideoBuff);
						m_pVideoBuff = NULL;
						pDataBuff = NULL;
						return QC_ERR_NONE;
					}
					m_bFindKeyFrame = true;
				}
				m_pBuffMng->Send(m_pVideoBuff);
				m_pVideoBuff = NULL;
				pDataBuff = NULL;
			}
		}
		if (pDataBuff == NULL)
		{
			nDataSize = m_datBuff.uSize * 32;
			//if (nDataSize < m_pFmtVideo->nWidth * m_pFmtVideo->nHeight / 4)
			//	nDataSize = m_pFmtVideo->nWidth * m_pFmtVideo->nHeight / 4;
			pDataBuff = m_pBuffMng->GetEmpty(m_datBuff.nMediaType, nDataSize);
			pDataBuff->uSize = 0;
			if (m_nFormat == QC_PARSER_RTSP)
				m_pVideoBuff = pDataBuff;
		}
	}
	else
	{
		nDataSize += 1024;
		pDataBuff = m_pBuffMng->GetEmpty(m_datBuff.nMediaType, nDataSize);
	}
	if (pDataBuff == NULL)
		return QC_ERR_MEMORY;

	if ((int)pDataBuff->uBuffSize < nDataSize)
	{
		QC_DEL_A(pDataBuff->pBuff);
		pDataBuff->uBuffSize = nDataSize;
	}
	if (pDataBuff->pBuff == NULL)
		pDataBuff->pBuff = new unsigned char[pDataBuff->uBuffSize];
	pDataBuff->uBuffType = QC_BUFF_TYPE_Data;
	pDataBuff->nMediaType = m_datBuff.nMediaType;

	if (m_nFormat == QC_PARSER_RTSP)
	{
		if (m_datBuff.nMediaType == QC_MEDIA_Audio)
		{
			if (qcAV_ConstructAACHeader(pDataBuff->pBuff, pDataBuff->uBuffSize,
				m_pFmtAudio->nSampleRate, m_pFmtAudio->nChannels, m_datBuff.uSize) != 7)
			{
				m_pBuffMng->Return(pDataBuff);
				return QC_ERR_STATUS;
			}
			memcpy(pDataBuff->pBuff + 7, m_datBuff.pBuff, m_datBuff.uSize);
			pDataBuff->uSize = m_datBuff.uSize + 7;
			pDataBuff->llTime = m_datBuff.llTime;
		}
		else
		{
			if (pDataBuff->uBuffSize < pDataBuff->uSize + m_datBuff.uSize)
			{
				nDataSize = pDataBuff->uSize + m_datBuff.uSize * 8;
				pDataBuff = m_pBuffMng->GetEmpty(m_datBuff.nMediaType, nDataSize);
				if ((int)pDataBuff->uBuffSize < nDataSize)
				{
					QC_DEL_A(pDataBuff->pBuff);
					pDataBuff->uBuffSize = nDataSize;
				}
				if (pDataBuff->pBuff == NULL)
					pDataBuff->pBuff = new unsigned char[pDataBuff->uBuffSize];
				pDataBuff->uBuffType = QC_BUFF_TYPE_Data;
				pDataBuff->nMediaType = m_datBuff.nMediaType;
				memcpy(pDataBuff->pBuff, m_pVideoBuff->pBuff, m_pVideoBuff->uSize);
				pDataBuff->uSize = m_pVideoBuff->uSize;
				pDataBuff->llTime = m_pVideoBuff->llTime;
				pDataBuff->uFlag = m_pVideoBuff->uFlag;
				m_pBuffMng->Return(m_pVideoBuff);
				m_pVideoBuff = pDataBuff;
			}
			memcpy(pDataBuff->pBuff + pDataBuff->uSize, m_datBuff.pBuff, m_datBuff.uSize);
			pDataBuff->uSize += m_datBuff.uSize;
			pDataBuff->llTime = m_datBuff.llTime;
			return QC_ERR_NONE;
		}
	}
	else
	{
		memcpy(pDataBuff->pBuff, m_datBuff.pBuff, m_datBuff.uSize);
		pDataBuff->uSize = m_datBuff.uSize;
		pDataBuff->llTime = m_datBuff.llTime;
	}

	m_pBuffMng->Send(pDataBuff);

	return nRC;
}

int	CQCFFSource::CreateHeadBuff(QC_DATA_BUFF * pBuffInfo)
{
	if (pBuffInfo == NULL)
		return QC_ERR_ARG;

	if (pBuffInfo->nMediaType == QC_MEDIA_Audio && m_nFormat == QC_PARSER_RTSP)
		return QC_ERR_FAILED;

	int				nHeadSize = 0;
	unsigned char * pHeadData = NULL;
	if (pBuffInfo->nMediaType == QC_MEDIA_Video)
	{
		if (m_pFmtVideo == NULL || m_pFmtVideo->nHeadSize <= 0)
			return QC_ERR_FAILED;
		nHeadSize = m_pFmtVideo->nHeadSize;
		pHeadData = m_pFmtVideo->pHeadData;
	}
	else
	{
		if (m_pFmtAudio == NULL || m_pFmtAudio->nHeadSize <= 0)
			return QC_ERR_FAILED;
		nHeadSize = m_pFmtAudio->nHeadSize;
		pHeadData = m_pFmtAudio->pHeadData;
	}

	QC_DATA_BUFF * pBuffData = m_pBuffMng->GetEmpty(pBuffInfo->nMediaType, nHeadSize);
	if (pBuffData == NULL)
		return QC_ERR_MEMORY;

	if ((int)pBuffData->uBuffSize < nHeadSize)
	{
		QC_DEL_A(pBuffData->pBuff);
		pBuffData->uBuffSize = nHeadSize + 128;
	}
	if (pBuffData->pBuff == NULL)
		pBuffData->pBuff = new unsigned char[pBuffData->uBuffSize];

	memcpy(pBuffData->pBuff, pHeadData, nHeadSize);
	pBuffData->uSize = nHeadSize;

	pBuffData->uFlag = QCBUFF_HEADDATA;
	pBuffData->llTime = 0;

	m_pBuffMng->Send(pBuffData);

	return QC_ERR_NONE;
}

void CQCFFSource::OnOpenDone(const char * pURL)
{
    if(!pURL)
        return;
    
    ReleaseResInfo();
    
    memset(&m_sResInfo, 0, sizeof(m_sResInfo));
    int nPort = 0;
    char szHost[1024];
    char szPath[4096];
    qcUrlParseUrl(pURL, szHost, szPath, nPort, NULL);
    m_sResInfo.pszURL = new char[strlen(pURL)+1];
    sprintf(m_sResInfo.pszURL, "%s", pURL);
    m_sResInfo.pszDomain = new char[strlen(szHost)+1];
    sprintf(m_sResInfo.pszDomain, "%s", szHost);
    m_sResInfo.pszFormat = new char[8+1];
    memset(m_sResInfo.pszFormat, 0, 8+1);
    qcUrlParseExtension(pURL, m_sResInfo.pszFormat, 8);
    
    if (m_pFmtAudio != NULL)
        m_sResInfo.nAudioCodec = m_pFmtAudio->nCodecID;
    if (m_pFmtVideo != NULL)
    {
        m_sResInfo.nVideoCodec = m_pFmtVideo->nCodecID;
        m_sResInfo.nWidth = m_pFmtVideo->nWidth;
        m_sResInfo.nHeight = m_pFmtVideo->nHeight;
    }
    m_sResInfo.llDuration = m_llDuration;
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PARSER_NEW_STREAM, QC_BA_MODE_AUTO, (long long)0, NULL, (void*)(&m_sResInfo));
}


void CQCFFSource::ReleaseResInfo()
{
    QC_DEL_A(m_sResInfo.pszURL);
    QC_DEL_A(m_sResInfo.pszFormat);
    QC_DEL_A(m_sResInfo.pszDomain);
}
