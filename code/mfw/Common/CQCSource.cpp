/*******************************************************************************
	File:		CQCSource.cpp

	Contains:	the qc own source implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-18		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"
#include "qcPlayer.h"

#include "CQCSource.h"

#include "AdaptiveStreamParser.h"

#include "USystemFunc.h"
#include "USourceFormat.h"
#include "UUrlParser.h"
#include "ULogFunc.h"

CQCSource::CQCSource(CBaseInst * pBaseInst, void * hInst)
	: CBaseSource(pBaseInst, hInst)
	, m_bNewStream (false)
	, m_nProtocol (QC_IOPROTOCOL_NONE)
	, m_nFormat (QC_PARSER_NONE)
	, m_nHadReadVideo(0)
{
	SetObjectName ("CQCSource");
	memset (&m_fParser, 0, sizeof (QC_Parser_Func));
	m_fParser.pBaseInst = m_pBaseInst;

	memset(m_szQiniuDrmKey, 0, sizeof(m_szQiniuDrmKey));
}

CQCSource::~CQCSource(void)
{
	Close ();
}

int CQCSource::Open (const char * pSource, int nType)
{
	int		nRC = 0;
	char	szURL[2048];
	memset (szURL, 0, sizeof (szURL));
	if (qcGetSourceProtocol (pSource) == QC_IOPROTOCOL_HTTP || 
		qcGetSourceProtocol(pSource) == QC_IOPROTOCOL_RTMP || 
		qcGetSourceProtocol(pSource) == QC_IOPROTOCOL_RTSP)
		qcUrlConvert (pSource, szURL, sizeof (szURL));
	else
		strcpy (szURL, pSource);
	pSource = szURL;
	m_pIO = &m_fIO;
	m_nFormat = QC_PARSER_NONE;
	if ((nType & QCPLAY_OPEN_SAME_SOURCE) == QCPLAY_OPEN_SAME_SOURCE)
		return OpenSameSource(pSource, nType);
	else
		return OpenSource(pSource, nType);
}

int CQCSource::OpenIO(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource)
{
	if (pIO == NULL)
		return QC_ERR_ARG;
	m_pIO = pIO;
	m_nFormat = nFormat;
	m_nProtocol = qcGetSourceProtocol(pSource);

	int nRC = OpenSource(pSource, nType);
	if (nRC != QC_ERR_NONE)
		return nRC;

	if ((nType & QCPLAY_OPEN_SAME_SOURCE) == QCPLAY_OPEN_SAME_SOURCE)
	{
		m_llSeekPos = 0;
		m_bAudioNewPos = true;
		m_bVideoNewPos = true;
		m_bSubttNewPos = true;
	}
	return nRC;
}

int	CQCSource::OpenSource(const char * pSource, int nType)
{
	int nRC = CBaseSource::Open(pSource, nType);
	if (nRC < 0)
		return nRC;
	if (m_fParser.pBuffMng == NULL)
		m_fParser.pBuffMng = m_pBuffMng;

	if (m_nFormat == QC_PARSER_NONE)
		UpdateProtocolFormat(pSource);
	// Create the parser with different format
	nRC = CreateParser(m_nProtocol, m_nFormat);
	if (nRC != QC_ERR_NONE)
		return nRC;

	CAutoLock lock(&m_lckParser);
	m_fParser.SetParam(m_fParser.hParser, QCPARSER_PID_OpenCache, &m_nOpenCache);
	if (m_szQiniuDrmKey[0] != 0)
		m_fParser.SetParam(m_fParser.hParser, QC_PARSER_SET_QINIU_DRM_INFO, m_szQiniuDrmKey);
	nRC = m_fParser.Open(m_fParser.hParser, m_pIO, pSource, 0);
	if (nRC < 0)
	{
		m_fParser.Close(m_fParser.hParser);
		DestroyParser();
		if (m_pIO->hIO != NULL)
		{
			m_pIO->Close(m_pIO->hIO);
			qcDestroyIO(&m_fIO);
		}
		return nRC;
	}

	UpdateInfo();
    m_bSourceLive = m_fParser.IsLive(m_fParser.hParser);
    if(m_pBuffMng)
        m_pBuffMng->SetSourceLive(m_bSourceLive);

	if (m_nFormat == QC_PARSER_M3U8)
	{
		if (m_pIO->hIO != NULL && m_pIO->GetType(m_pIO->hIO) == QC_IOTYPE_FILE)
			m_llMaxBuffTime = m_pBaseInst->m_pSetting->g_qcs_nMaxSaveVodeBuffTime;
		else
			m_llMaxBuffTime = m_pBaseInst->m_pSetting->g_qcs_nMaxSaveLiveBuffTime;
	}
	else
	{
		if (m_pIO->hIO != NULL && m_pIO->GetType(m_pIO->hIO) == QC_IOTYPE_FILE)
			m_llMaxBuffTime = m_pBaseInst->m_pSetting->g_qcs_nMaxSaveLcleBuffTime;
		else
			m_llMaxBuffTime = m_pBaseInst->m_pSetting->g_qcs_nMaxSaveVodeBuffTime;
	}
	if (m_bSourceLive)
		m_llMinBuffTime = m_pBaseInst->m_pSetting->g_qcs_nMinPlayBuffTime; // 500
	else
		m_llMinBuffTime = m_pBaseInst->m_pSetting->g_qcs_nMinPlayBuffTime; // 2000
	m_nHadReadVideo = 0;
	m_buffInfo.nMediaType = QC_MEDIA_MAX;
    
    QCLOGI("Min buf time %lld, max buf time %lld", m_llMinBuffTime, m_llMaxBuffTime);

	return nRC;
}

int	CQCSource::OpenSameSource(const char * pSource, int nType)
{
	if (m_fParser.hParser == NULL)
		return QC_ERR_STATUS;
	
	int nRC = QC_ERR_NONE;
	int	nBitrate = 0;
	int nStreamPlay = m_fParser.GetStreamPlay(m_fParser.hParser, QC_MEDIA_Source);
	QC_STREAM_FORMAT * pFmtStream = NULL;
	m_fParser.GetStreamFormat(m_fParser.hParser, nStreamPlay, &pFmtStream);
	if (pFmtStream != NULL)
	{
		nBitrate = pFmtStream->nBitrate;
		nBitrate = nBitrate / 2;
	}

	int nExitRead = 1;
	m_fParser.SetParam(m_fParser.hParser, QCPARSER_PID_ExitRead, &nExitRead);
	m_pBaseInst->m_bForceClose = true;
	int nStartTime = qcGetSysTime();
	if (m_pReadThread != NULL)
		m_pReadThread->Pause();
	CAutoLock lock(&m_lckParser);
	m_fParser.Close(m_fParser.hParser);
	DestroyParser();
	m_pIO->Close(m_pIO->hIO);
	qcDestroyIO(m_pIO);
	m_pBaseInst->m_bForceClose = false;
	QCLOGI("FastOpen Close Parser Used Time = %d", qcGetSysTime() - nStartTime);

	QC_DEL_A(m_pSourceName);
	m_pSourceName = new char[strlen(pSource) + 1];
	strcpy(m_pSourceName, pSource);

	if (m_nFormat == QC_PARSER_NONE)
		UpdateProtocolFormat(m_pSourceName);

	nRC = CreateParser(m_nProtocol, m_nFormat);
	if (nRC != QC_ERR_NONE)
		return nRC;

	CAutoLock lockReader(&m_lckReader);
	if (m_pBuffMng != NULL)
		m_pBuffMng->ReleaseBuff(false);

	m_llSeekPos = 0;
	m_bAudioNewPos = true;
	m_bVideoNewPos = true;
	m_bSubttNewPos = true;
	m_bEOA = m_nStmAudioNum > 0 ? false : true;
	m_bEOV = m_nStmVideoNum > 0 ? false : true;
	m_bNeedBuffing = false;
	m_nBuffRead = 0;
	m_fParser.SetParam(m_fParser.hParser, QCPARSER_PID_LastBitrate, &nBitrate);
	nRC = m_fParser.Open(m_fParser.hParser, &m_fIO, pSource, 0);
	if (m_szQiniuDrmKey[0] != 0)
		m_fParser.SetParam(m_fParser.hParser, QC_PARSER_SET_QINIU_DRM_INFO, m_szQiniuDrmKey);
	m_llDuration = m_fParser.GetDuration(m_fParser.hParser);
	m_nHadReadVideo = 0;
	m_buffInfo.nMediaType = QC_MEDIA_MAX;
    m_bSourceLive = m_fParser.IsLive(m_fParser.hParser);
    if(m_pBuffMng)
        m_pBuffMng->SetSourceLive(m_bSourceLive);
	//QCLOGI ("New Source *************************************************************************");
	if (m_pReadThread != NULL)
		m_pReadThread->Start();
	return nRC;
}

int CQCSource::Close (void)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);

	if (m_fParser.hParser == NULL)
		return QC_ERR_NONE;

	Stop ();

	CAutoLock lock(&m_lckParser);
	if (m_fParser.hParser != NULL)
	{
		m_fParser.Close (m_fParser.hParser);
		DestroyParser ();
		m_fParser.hParser = NULL;
	}

	CBaseSource::Close ();
	memset(m_szQiniuDrmKey, 0, sizeof(m_szQiniuDrmKey));

	return QC_ERR_NONE;
}

int	CQCSource::CanSeek (void)
{
	long long   llSize = 0;
	if (m_fParser.hParser == NULL || m_pIO == NULL || m_pIO->hIO == NULL)
		return QC_ERR_STATUS;
	QCIOType nType = QC_IOTYPE_NONE;
	nType = m_pIO->GetType(m_pIO->hIO);
	llSize = m_pIO->GetSize(m_pIO->hIO);
	if ((nType == QC_IOTYPE_HTTP_LIVE && (llSize == QCIO_MAX_CONTENT_LEN || llSize == 0)) || nType == QC_IOTYPE_RTMP)
		return 0;

	//CAutoLock lock(&m_lckParser);
	return m_fParser.CanSeek (m_fParser.hParser);
}

long long CQCSource::SetPos (long long llPos)
{
	if (m_fParser.hParser == NULL)
		return QC_ERR_STATUS;
	CAutoLock lockSeek(&m_lckSeek);
	long long llNewPos = CBaseSource::SetPos (llPos);
	if (llNewPos == 0)
	{
		int nExitRead = 1;
		m_fParser.SetParam(m_fParser.hParser, QCPARSER_PID_ExitRead, &nExitRead);
		CAutoLock lock(&m_lckParser);
		llNewPos = m_fParser.SetPos(m_fParser.hParser, llPos);
		nExitRead = 0;
		m_fParser.SetParam(m_fParser.hParser, QCPARSER_PID_ExitRead, &nExitRead);
	}
	m_buffInfo.nMediaType = QC_MEDIA_MAX;

	return llNewPos;
}
 
int CQCSource::SetStreamPlay (QCMediaType nType, int nStream)
{
	if (m_fParser.hParser == NULL || m_pIO  == NULL || m_nStmSourceSel == nStream)
		return QC_ERR_STATUS;

	if (m_pIO->hIO != NULL && nType == QC_MEDIA_Source)
		m_pIO->Stop (m_pIO->hIO);
//	m_fParser.SetParam (m_fParser.hParser, QCPARSER_PID_ExitRead, 1);
//	CAutoLock lock(&m_lckParser);
	if (m_pBuffMng != NULL)
		m_pBuffMng->SetStreamPlay (nType, nStream);
	int nRC = m_fParser.SetStreamPlay (m_fParser.hParser, nType, nStream);
//	m_fParser.SetParam (m_fParser.hParser, QCPARSER_PID_ExitRead, 0);
	if (nRC < 0)
		return nRC;

	if (nType == QC_MEDIA_Source)
		m_nStmSourceSel = nStream;
	else if (nType == QC_MEDIA_Video)
		m_nStmVideoPlay = nStream;
	else if (nType == QC_MEDIA_Audio)
		m_nStmAudioPlay = nStream;
	else if (nType == QC_MEDIA_Subtt)
		m_nStmSubttPlay = nStream;

	UpdateInfo ();
	if (nStream >= 0)
		m_bNewStream = true;

	return QC_ERR_NONE;
}

int	CQCSource::Start (void)
{
	if (m_fParser.hParser == NULL)
		return QC_ERR_STATUS;
	m_fParser.Run (m_fParser.hParser);
	return CBaseSource::Start ();
}

int CQCSource::Pause (void)
{
	return QC_ERR_NONE;
	// it doesn't need to pause because it used thread to read buffer.
	if (m_fParser.hParser == NULL)
		return QC_ERR_STATUS;
	int nRC = CBaseSource::Pause ();
	m_fParser.Pause (m_fParser.hParser);
	return nRC;
}

int	CQCSource::Stop (void)
{
	if (m_fParser.hParser == NULL || m_pIO == NULL || m_pIO->hIO == NULL)
		return QC_ERR_STATUS;
	m_pIO->Stop (m_pIO->hIO);
	m_fParser.Stop (m_fParser.hParser);
	int nRC = CBaseSource::Stop ();
	return nRC;
}

int CQCSource::ReadBuff (QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait)
{
//	QCLOGI ("Download Speed: % 8d", m_pIO->GetSpeed (m_pIO->hIO));
	int nRC = CBaseSource::ReadBuff (pBuffInfo, ppBuffData, bWait);
	return nRC;
}

int CQCSource::SetParam (int nID, void * pParam)
{
	if (nID == QCPLAY_PID_Reconnect)
	{
		if (m_pIO != NULL && m_pIO->hIO != NULL)
			return m_pIO->Reconnect (m_pIO->hIO, NULL, -1);
	}
	else if (nID == QC_PARSER_SET_QINIU_DRM_INFO)
	{
		memcpy(m_szQiniuDrmKey, (char *)pParam, 16);
		if (m_fParser.hParser != NULL)
			m_fParser.SetParam(m_fParser.hParser, QC_PARSER_SET_QINIU_DRM_INFO, m_szQiniuDrmKey);
		return QC_ERR_NONE;
	}
	else if (nID == QCPLAY_PID_Flush_Buffer)
	{
		if (m_pBuffMng != NULL)
			m_pBuffMng->FlushBuff();
		return QC_ERR_NONE;
	}
	return QC_ERR_PARAMID;
}

int CQCSource::GetParam (int nID, void * pParam)
{
    if(nID == QCPLAY_PID_RTMP_AUDIO_MSG_TIMESTAMP)
    {
		if (m_pIO != NULL && m_pIO->hIO != NULL)
            return m_pIO->GetParam(m_pIO->hIO, QCIO_PID_RTMP_AUDIO_MSG_TIMESTAMP, pParam);
    }
    else if(nID == QCPLAY_PID_RTMP_VIDEO_MSG_TIMESTAMP)
    {
		if (m_pIO != NULL && m_pIO->hIO != NULL)
        	return m_pIO->GetParam(m_pIO->hIO, QCIO_PID_RTMP_VIDEO_MSG_TIMESTAMP, pParam);
    }
	else if (nID == QCIO_PID_SourceType)
	{
		if (m_fParser.hParser == NULL)
			return QC_IOPROTOCOL_NONE;

		long long llPos = 0;
		m_fParser.GetParam(m_fParser.hParser, QCPARSER_PID_SEEK_IO_POS, &llPos);

		if (m_pIO != NULL && m_pIO->hIO != NULL)
			return m_pIO->GetParam(m_pIO->hIO, QCIO_PID_SourceType, &llPos);
		else
			return QC_IOPROTOCOL_NONE;
	}

	return QC_ERR_PARAMID;
}

QC_STREAM_FORMAT * CQCSource::GetStreamFormat (int nID)
{
	if (nID == -1)
		return m_pFmtStream;
	m_pFmtStreamGet = NULL;
	m_fParser.GetStreamFormat (m_fParser.hParser, nID, &m_pFmtStreamGet);
	return m_pFmtStreamGet;
}

QC_AUDIO_FORMAT * CQCSource::GetAudioFormat (int nID)
{
	if (nID == -1)
		return m_pFmtAudio;
	m_pFmtAudioGet = NULL;
	m_fParser.GetAudioFormat (m_fParser.hParser, nID, &m_pFmtAudioGet);
	return m_pFmtAudioGet;
}

QC_VIDEO_FORMAT * CQCSource::GetVideoFormat (int nID) 
{
	if (nID == -1)
		return m_pFmtVideo;
	m_pFmtVideoGet = NULL;
	m_fParser.GetVideoFormat (m_fParser.hParser, nID, &m_pFmtVideoGet);
	return m_pFmtVideoGet;
}

QC_SUBTT_FORMAT * CQCSource::GetSubttFormat (int nID) 
{
	if (nID == -1)
		return m_pFmtSubtt;
	m_pFmtSubttGet = NULL;
	m_fParser.GetSubttFormat (m_fParser.hParser, nID, &m_pFmtSubttGet);
	return m_pFmtSubttGet;
}

int CQCSource::CreateParser (QCIOProtocol nProtocol, QCParserFormat	nFormat)
{
	int nRC = QC_ERR_NONE;
	nRC = qcCreateParser (&m_fParser, nFormat);
	if (m_fParser.hParser == NULL)
		return QC_ERR_FORMAT;

	if (m_pIO != &m_fIO)
		return QC_ERR_NONE;

//	strcpy(m_pIO->m_szLibInfo, "qcCodec,qcFFCreateIO,qcFFDestroyIO");
//	m_pIO->m_nProtocol = QC_IOPROTOCOL_EXTLIB;
//	int nRC = qcCreateIO(&m_fIO, QC_IOPROTOCOL_EXTLIB);
	if (nFormat == QC_PARSER_MP4 && (nProtocol == QC_IOPROTOCOL_HTTP || nProtocol == QC_IOPROTOCOL_HTTPPD))
	{
		while (m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol == QC_IOPROTOCOL_HTTPPD)
		{
			long long llFreeSpace = qcGetFreeSpace(m_pBaseInst->m_pSetting->g_qcs_szPDFileCachePath);
			if (llFreeSpace < 1024 * 1024 * 64)
			{
				QCLOGW ("The free space is too small. FreeSpace = %lld", llFreeSpace);
				break;
			}

			if (m_pIO->hIO != NULL)
			{
				m_pIO->Stop(m_pIO->hIO);
				qcDestroyIO(&m_fIO);
			}

			nRC = qcCreateIO(&m_fIO, QC_IOPROTOCOL_HTTPPD);
			if (nRC != QC_ERR_NONE)
			{
				qcDestroyIO(&m_fIO);
				QCLOGW ("CreatePD IO failed!");				
				break;
			}
			nRC = m_pIO->Open(m_pIO->hIO, m_pSourceName, 0, QCIO_FLAG_READ);
			if (nRC != QC_ERR_NONE)
			{
				qcDestroyIO(&m_fIO);
				QCLOGW ("PD IO open failed!");				
				break;
			}
			if (m_pIO->GetDownPos(m_pIO->hIO) > 0)
				break;

			if (m_pIO->GetSize(m_pIO->hIO) > 1024 * 1024 * 1024 || m_pIO->GetSize(m_pIO->hIO) + 1024 * 1024 * 32 > llFreeSpace)
			{
				qcDestroyIO(&m_fIO);
				QCLOGW ("The free space is not enought!");
				break;
			}
			break;
		}
	}

	if (m_pIO->hIO == NULL && nProtocol != QC_IOPROTOCOL_RTSP)
	{
		nRC = qcCreateIO(&m_fIO, nProtocol);
		if (nRC < 0)
			return nRC;
	}

	return QC_ERR_NONE;
}

int CQCSource::DestroyParser (void)
{
	if (m_fParser.hParser != NULL)
		qcDestroyParser (&m_fParser);
	return QC_ERR_NONE;
}

int CQCSource::ReadParserBuff (QC_DATA_BUFF * pBuffInfo)
{
	if (m_fParser.hParser == NULL)
		return QC_ERR_STATUS;
	return m_fParser.Read (m_fParser.hParser, pBuffInfo);
}

int CQCSource::UpdateProtocolFormat(const char * pSource)
{
	m_nProtocol = qcGetSourceProtocol(pSource);
	if (m_pBaseInst->m_pSetting->g_qcs_nPerferFileForamt != QC_PARSER_NONE)
		m_nFormat = (QCParserFormat)m_pBaseInst->m_pSetting->g_qcs_nPerferFileForamt;
	else
		m_nFormat = qcGetSourceFormat(pSource);
	if (m_nProtocol == QC_IOPROTOCOL_RTMP)
		m_nFormat = QC_PARSER_FLV;
	else if (m_nProtocol == QC_IOPROTOCOL_RTSP)
		m_nFormat = QC_PARSER_RTSP;
	if (m_nFormat == QC_PARSER_NONE || m_nFormat == QC_PARSER_MAX)
	{
		if (m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol == QC_IOPROTOCOL_HTTPPD)
			m_nFormat = QC_PARSER_MP4;
		else
			m_nFormat = GetSourceFormat(pSource);
	}
	return QC_ERR_NONE;
}

int CQCSource::UpdateInfo (void)
{
	if (m_fParser.hParser == NULL)
		return QC_ERR_STATUS;

	CBaseSource::UpdateInfo ();
	m_nStmSourceNum = m_fParser.GetStreamCount (m_fParser.hParser, QC_MEDIA_Source);
	m_nStmVideoNum = m_fParser.GetStreamCount(m_fParser.hParser, QC_MEDIA_Video);
	m_nStmAudioNum = m_fParser.GetStreamCount (m_fParser.hParser, QC_MEDIA_Audio);
	m_nStmSubttNum = m_fParser.GetStreamCount (m_fParser.hParser, QC_MEDIA_Subtt);
	m_nStmSourcePlay = m_fParser.GetStreamPlay (m_fParser.hParser, QC_MEDIA_Source);
	m_nStmAudioPlay = m_fParser.GetStreamPlay (m_fParser.hParser, QC_MEDIA_Audio);
	m_nStmVideoPlay = m_fParser.GetStreamPlay (m_fParser.hParser, QC_MEDIA_Video);
	m_nStmSubttPlay = m_fParser.GetStreamPlay (m_fParser.hParser, QC_MEDIA_Subtt);

	m_llDuration = m_fParser.GetDuration (m_fParser.hParser);

	m_fParser.GetStreamFormat (m_fParser.hParser, m_nStmSourcePlay, &m_pFmtStream);
	m_fParser.GetAudioFormat (m_fParser.hParser, m_nStmAudioPlay, &m_pFmtAudio);
	m_fParser.GetVideoFormat (m_fParser.hParser, m_nStmVideoPlay, &m_pFmtVideo);
	m_fParser.GetSubttFormat (m_fParser.hParser, m_nStmSubttPlay, &m_pFmtSubtt);

	m_bEOA = m_nStmAudioNum > 0 ? false : true;
	m_bEOV = m_nStmVideoNum > 0 ? false : true;

	return QC_ERR_NONE;
}

int CQCSource::OnWorkItem (void)
{
	if (m_fParser.hParser == NULL || m_pBuffMng == NULL)
		return QC_ERR_STATUS;
	if ((m_bEOA && m_bEOV) || m_pBaseInst->m_bForceClose)
	{
		qcSleep (2000);
		return QC_ERR_STATUS;
	}

    // sometimes CQCSource::SetPos can't contend the m_lckSeek
    qcSleep (1000);
	CAutoLock lockSeek(&m_lckSeek);
	int nRC = QC_ERR_NONE;
	long long llBuffAudio = m_pBuffMng->GetBuffTime (QC_MEDIA_Audio);
	long long llBuffVideo = m_pBuffMng->GetBuffTime (QC_MEDIA_Video);
	if (!m_pBuffMng->InSwitching() && (llBuffVideo > m_llMaxBuffTime || llBuffAudio > m_llMaxBuffTime))
	{
		qcSleep (2000);
		return QC_ERR_RETRY;
	}

	if (m_buffInfo.nMediaType == QC_MEDIA_MAX)
	{
#if 0
		m_buffInfo.nMediaType = QC_MEDIA_Audio;
		if (m_bEOA)
			m_buffInfo.nMediaType = QC_MEDIA_Video;
		if (llBuffVideo < llBuffAudio && !m_bEOV)
			m_buffInfo.nMediaType = QC_MEDIA_Video;
#else
		long long llLastVideo = m_pBuffMng->GetLastTime(QC_MEDIA_Video);
		long long llLastAudio = m_pBuffMng->GetLastTime(QC_MEDIA_Audio);
//		if (m_llSeekPos > llLastVideo)
//			llLastVideo = m_llSeekPos;
//		if (m_llSeekPos > llLastAudio)
//			llLastAudio = m_llSeekPos;
		if (llLastAudio < m_pBaseInst->m_llFAudioTime)
			llLastAudio = m_pBaseInst->m_llFAudioTime;
		if (llLastVideo < m_pBaseInst->m_llFVideoTime)
			llLastVideo = m_pBaseInst->m_llFVideoTime;

		m_buffInfo.nMediaType = QC_MEDIA_Video;
		//if (m_pBaseInst->m_nVideoRndCount > 0)
		//if (m_pBuffMng->GetBuffCount (QC_MEDIA_Video) > 3 || !m_bNewSource)
		if (m_nHadReadVideo > 3)
		{
			if (llLastVideo > llLastAudio)
				m_buffInfo.nMediaType = QC_MEDIA_Audio;
		}
		if (m_bEOA)
			m_buffInfo.nMediaType = QC_MEDIA_Video;
		if (m_bEOV)
			m_buffInfo.nMediaType = QC_MEDIA_Audio;
#endif // 0
	}

	// read the buffer from parser
	{
		CAutoLock lockParser (&m_lckParser);
		nRC = ReadParserBuff (&m_buffInfo);
		if (nRC == QC_ERR_NONE && m_buffInfo.nMediaType == QC_MEDIA_Video)
			m_nHadReadVideo++;
	}
	if (nRC == QC_ERR_FINISH)
	{
		if (m_buffInfo.nMediaType == QC_MEDIA_Audio)
        {
            m_bEOA = true;
            // for HLS pure video
            m_bAudioNewPos = false;
        }
		else if (m_buffInfo.nMediaType == QC_MEDIA_Video)
        {
            m_bEOV = true;
            // for HLS pure audio
            m_bVideoNewPos = false;
        }
		if (m_pBuffMng != NULL)
			m_pBuffMng->SetAVEOS(m_bEOA, m_bEOV);
	}
	else if (nRC == QC_ERR_RETRY)
	{
		qcSleep (2000);
		return QC_ERR_RETRY;
	}

	if (nRC != QC_ERR_NONE)
	{
		CAutoLock lockParser(&m_lckParser);
		if (m_buffInfo.nMediaType == QC_MEDIA_Audio && !m_bEOV)
			m_buffInfo.nMediaType = QC_MEDIA_Video;
		else if (!m_bEOA)
			m_buffInfo.nMediaType = QC_MEDIA_Audio;
		else
			m_buffInfo.nMediaType = QC_MEDIA_MAX;
		if (m_buffInfo.nMediaType != QC_MEDIA_MAX)
			nRC = ReadParserBuff (&m_buffInfo);
		if (nRC == QC_ERR_FINISH)
		{
			if (m_buffInfo.nMediaType == QC_MEDIA_Audio)
            {
                m_bEOA = true;
                m_bAudioNewPos = false;
            }
			else if (m_buffInfo.nMediaType == QC_MEDIA_Video)
            {
                m_bEOV = true;
                m_bVideoNewPos = false;
            }
			if (m_pBuffMng != NULL)
				m_pBuffMng->SetAVEOS(m_bEOA, m_bEOV);
		}
		if (nRC == QC_ERR_NONE && m_buffInfo.nMediaType == QC_MEDIA_Video)
			m_nHadReadVideo++;
	}
	if (m_bNewStream && m_pBuffMng->HaveNewStreamBuff () > 0)
		m_bNewStream = false;
    
    {
        CAutoLock lock(&m_lckParser);
        if (m_fParser.hParser)
            m_bSourceLive = m_fParser.IsLive(m_fParser.hParser);
        if (m_pBuffMng)
            m_pBuffMng->SetSourceLive(m_bSourceLive);
    }

	if (m_bEOA && m_bEOV)
	{
		m_pBuffMng->SetEOS (true);
		qcSleep (5000);
	}
	m_buffInfo.nMediaType = QC_MEDIA_MAX;

	//qcSleep (1000);

	return QC_ERR_NONE;
}

QCIOType CQCSource::GetIOType()
{
	if (m_pIO != NULL && m_pIO->GetType && m_pIO->hIO)
        return m_pIO->GetType (m_pIO->hIO);
    return QC_IOTYPE_NONE;
}
