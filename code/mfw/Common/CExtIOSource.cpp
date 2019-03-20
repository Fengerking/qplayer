/*******************************************************************************
	File:		CExtIOSource.cpp

	Contains:	qc CExtIOSource source implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-18		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"
#include "qcPlayer.h"

#include "CExtIOSource.h"
#include "CQCSource.h"
#include "CQCFFSource.h"
#include "CMsgMng.h"

#include "UAVParser.h"
#include "USystemFunc.h"
#include "USourceFormat.h"
#include "UUrlParser.h"
#include "ULogFunc.h"

CExtIOSource::CExtIOSource(CBaseInst * pBaseInst, void * hInst)
	: CBaseSource(pBaseInst, hInst)
{
	SetObjectName ("CExtIOSource");
	m_bSourceLive = true;
	m_nStmVideoNum = 1; 
	m_nStmAudioNum = 1;
	m_nStmVideoPlay = 0; 
	m_nStmAudioPlay = 0;

	memset(&m_fmtAudio, 0, sizeof(m_fmtAudio));
	m_fmtAudio.nCodecID = m_pBaseInst->m_nAudioCodecID;
	if (m_fmtAudio.nCodecID == QC_CODEC_ID_NONE)
		m_fmtAudio.nCodecID = QC_CODEC_ID_AAC;
	memset(&m_fmtVideo, 0, sizeof(m_fmtVideo));
	m_fmtVideo.nCodecID = m_pBaseInst->m_nVideoCodecID;

	m_pFmtVideo = &m_fmtVideo;
	m_pFmtAudio = &m_fmtAudio;
    
    m_llMinBuffTime = m_pBaseInst->m_pSetting->g_qcs_nMinPlayBuffTime;
    m_llMaxBuffTime = m_pBaseInst->m_pSetting->g_qcs_nMaxSaveLiveBuffTime;
    QCLOGI("Min buf time %lld, max buf time %lld", m_llMinBuffTime, m_llMaxBuffTime);

	memset(&m_fParser, 0, sizeof(QC_Parser_Func));
	m_fParser.pBaseInst = m_pBaseInst;
}

CExtIOSource::~CExtIOSource(void)
{
	Close ();
}

int CExtIOSource::OpenIO(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource)
{
	int nRC = QC_ERR_NONE;
	if (m_pBuffMng == NULL)
		m_pBuffMng = new CBuffMng(m_pBaseInst);

	m_fParser.pBuffMng = m_pBuffMng;
	nRC = qcCreateParser(&m_fParser, nFormat);
	if (m_fParser.hParser == NULL)
		return QC_ERR_FORMAT;

	return nRC;
}

int CExtIOSource::Close(void)
{
	if (m_fParser.hParser == NULL)
		return QC_ERR_NONE;

	if (m_fParser.hParser != NULL)
	{
		m_fParser.Close(m_fParser.hParser);
		qcDestroyParser(&m_fParser);
		m_fParser.hParser = NULL;
	}

	return CBaseSource::Close();
}

int	CExtIOSource::Start(void)
{
	return QC_ERR_NONE;
}

int CExtIOSource::SetParam (int nID, void * pParam)
{
	int nRC = QC_ERR_NONE;

	if (nID == QCPLAY_PID_EXT_SOURCE_DATA)
	{
        if(!m_fParser.hParser)
            return QC_ERR_STATUS;
		QC_DATA_BUFF * pBuff = (QC_DATA_BUFF *)pParam;
		m_fParser.Process(m_fParser.hParser, pBuff->pBuff, pBuff->uSize);
		return nRC;
	}

	nRC = CBaseSource::SetParam (nID, pParam);
	if (nRC == QC_ERR_NONE)
		return QC_ERR_NONE;

	return QC_ERR_PARAMID;
}

int CExtIOSource::GetParam (int nID, void * pParam)
{
	int nRC = QC_ERR_NONE;

	nRC = CBaseSource::GetParam (nID, pParam);
	if (nRC == QC_ERR_NONE)
		return QC_ERR_NONE;

	return QC_ERR_PARAMID;
}
