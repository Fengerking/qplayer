/*******************************************************************************
	File:		CFFMpegParser.cpp

	Contains:	ffmpeg format implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-15		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcDef.h"
#include "qcPlayer.h"

#include "CFFMpegParser.h"
#include "qcFFLog.h"

CFFMpegParser::CFFMpegParser(QCParserFormat nFormat)
	: CBaseFFParser(nFormat)
	, m_pFmtCtx (NULL)
	, m_nIdxAudio (-1)
	, m_pStmAudio (NULL)
	, m_nIdxVideo (-1)
	, m_pStmVideo (NULL)
	, m_nIdxSubtt (-1)
	, m_pStmSubtt (NULL)
	, m_pFFIO (NULL)
{
	av_register_all();
    avformat_network_init();
	m_nPacketSize = sizeof(AVPacket);

	m_pFmtOpts = NULL;
//	av_dict_get(m_pFmtOpts, "rtsp_transport", NULL, 0);
//	av_dict_set(&m_pFmtOpts, "rtsp_transport", "tcp", 0);
	m_pPackData = new AVPacket();
	av_init_packet(m_pPackData);
}

CFFMpegParser::~CFFMpegParser(void)
{
	Close ();
	av_dict_free(&m_pFmtOpts);
	av_free_packet(m_pPackData);
	delete m_pPackData;
	avformat_network_deinit();
}

int CFFMpegParser::Open (QC_IO_Func * pIO, const char * pURL, int nFlag)
{
	Close();

	m_bLive = false;
	if (!strncmp(pURL, "rtsp:", 5))
	{
		m_nIOType = QC_IOTYPE_RTSP;
		m_bLive = true;
	}
	else if (!strncmp(pURL, "rtmp:", 5))
	{
		m_nIOType = QC_IOTYPE_RTMP;
		m_bLive = true;
	}
	else if (!strncmp(pURL, "http:", 5))
		m_nIOType = QC_IOTYPE_HTTP_VOD;
	else if (!strncmp(pURL, "https:", 6))
		m_nIOType = QC_IOTYPE_HTTP_VOD;
	else
		m_nIOType = QC_IOTYPE_FILE;
	
	if (pIO != NULL && pIO->hIO != NULL)
	{
		if (m_pFFIO == NULL)
		{
			m_pFFIO = new CFFMpegInIO();
			if (m_pFFIO->Open(pIO, pURL) != QC_ERR_NONE)
			{
				delete m_pFFIO;
				m_pFFIO = NULL;
			}
			else
			{
				if (m_pFmtCtx == NULL)
					m_pFmtCtx = avformat_alloc_context();
				m_pFmtCtx->pb = m_pFFIO->GetAVIIO();
			}
		}
	}
	
	int nRC = avformat_open_input(&m_pFmtCtx, pURL, NULL, &m_pFmtOpts);
	if (nRC < 0)
	{
		av_log (NULL, AV_LOG_WARNING, "Open source %s failed! err = 0X%08X", pURL, nRC);
		return QC_ERR_FAILED;
	}
	// retrieve stream information
	nRC = avformat_find_stream_info(m_pFmtCtx, NULL);
	if (nRC < 0)
		return QC_ERR_FAILED;
	AVCodecContext * pDecCtx = NULL;
	m_nIdxVideo = av_find_best_stream(m_pFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (m_nIdxVideo >= 0)
	{
		// Check the video stream to get best video stream
		int nVideoNum = 0;
		int	nVideoIdx = -1;
		int nMaxW = 0;
		for (int i = 0; i < m_pFmtCtx->nb_streams; i++)
		{
			pDecCtx = m_pFmtCtx->streams[i]->codec;
			if (m_pFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				if (pDecCtx->width > nMaxW)
				{
					nMaxW = pDecCtx->width;
					nVideoIdx = i;
				}
				nVideoNum++;
			}
		}
		if (nVideoNum > 1 && nVideoIdx != m_nIdxVideo)
		{
			if (m_pFmtCtx->streams[nVideoIdx]->nb_frames >= m_pFmtCtx->streams[m_nIdxVideo]->nb_frames)
			{
				m_nIdxVideo = nVideoIdx;
			}
		}
		m_pStmVideo = m_pFmtCtx->streams[m_nIdxVideo];
		pDecCtx = m_pStmVideo->codec;
		m_nStrmVideoCount = 1;
		m_nStrmVideoPlay = 0;

		DeleteFormat(QC_MEDIA_Video);
		m_pFmtVideo = new QC_VIDEO_FORMAT();
		memset(m_pFmtVideo, 0, sizeof(QC_VIDEO_FORMAT));
		if (pDecCtx->codec_id == AV_CODEC_ID_H264)
			m_pFmtVideo->nCodecID = QC_CODEC_ID_H264;
		else if (pDecCtx->codec_id == AV_CODEC_ID_HEVC)
			m_pFmtVideo->nCodecID = QC_CODEC_ID_H265;
		m_pFmtVideo->nWidth = pDecCtx->width;
		m_pFmtVideo->nHeight = pDecCtx->height;
		m_pFmtVideo->nSourceType = QC_SOURCE_QC;
		if (pDecCtx->extradata_size > 0)
		{
			m_pFmtVideo->pHeadData = new unsigned char[pDecCtx->extradata_size];
			memcpy(m_pFmtVideo->pHeadData, pDecCtx->extradata, pDecCtx->extradata_size);
			m_pFmtVideo->nHeadSize = pDecCtx->extradata_size;
		}
		//m_pFmtVideo->nSourceType = QC_SOURCE_FF;
		m_pFmtVideo->pPrivateData = pDecCtx;
		m_pFmtVideo->nPrivateFlag = QC_SOURCE_FF;
	}
	
	m_nIdxAudio = av_find_best_stream(m_pFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (m_nIdxAudio >= 0)
	{
		pDecCtx = m_pFmtCtx->streams[m_nIdxAudio]->codec;
		m_nStrmAudioCount = 0;
		for (int i = 0; i < m_pFmtCtx->nb_streams; i++)
		{
			if (m_pFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
				m_nStrmAudioCount++;
		}
		m_pStmAudio = m_pFmtCtx->streams[m_nIdxAudio];
		m_nStrmAudioPlay = 0;

		DeleteFormat(QC_MEDIA_Audio);
		m_pFmtAudio = new QC_AUDIO_FORMAT();
		memset(m_pFmtAudio, 0, sizeof(QC_AUDIO_FORMAT));
		if (pDecCtx->codec_id == AV_CODEC_ID_AAC)
			m_pFmtAudio->nCodecID = QC_CODEC_ID_AAC;
		else if (pDecCtx->codec_id == AV_CODEC_ID_MP3)
			m_pFmtAudio->nCodecID = QC_CODEC_ID_MP3;
		else if (pDecCtx->codec_id == AV_CODEC_ID_MP2)
			m_pFmtAudio->nCodecID = QC_CODEC_ID_MP2;
		m_pFmtAudio->nChannels = pDecCtx->channels;
		m_pFmtAudio->nSampleRate = pDecCtx->sample_rate;
		m_pFmtAudio->nBits = 16;
		m_pFmtAudio->nSourceType = QC_SOURCE_QC;
		if (pDecCtx->extradata_size > 0)
		{
			m_pFmtAudio->pHeadData = new unsigned char[pDecCtx->extradata_size];
			memcpy(m_pFmtAudio->pHeadData, pDecCtx->extradata, pDecCtx->extradata_size);
			m_pFmtAudio->nHeadSize = pDecCtx->extradata_size;
		}
		//m_pFmtAudio->nSourceType = QC_SOURCE_FF;
		m_pFmtAudio->pPrivateData = pDecCtx;
		m_pFmtAudio->nPrivateFlag = QC_SOURCE_FF;
	}
	
	m_nIdxSubtt = av_find_best_stream(m_pFmtCtx, AVMEDIA_TYPE_SUBTITLE, -1, -1, NULL, 0);
	if (m_nIdxSubtt >= 0 && m_nIdxSubtt < m_pFmtCtx->nb_streams)
	{
		pDecCtx = m_pFmtCtx->streams[m_nIdxSubtt]->codec;
		m_pStmSubtt = m_pFmtCtx->streams[m_nIdxSubtt];
		m_nStrmSubttCount = 0;
		for (int i = 0; i < m_pFmtCtx->nb_streams; i++)
		{
			if (m_pFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE)
				m_nStrmSubttCount++;
		}
	}

	long long llDuration = 0;
	if (m_pStmAudio != NULL)
		m_llDuration = ffBaseToTime(m_pStmAudio->duration, m_pStmAudio);
	if (m_pStmVideo != NULL)
		llDuration = ffBaseToTime(m_pStmVideo->duration, m_pStmVideo);
	if (llDuration > m_llDuration)
		m_llDuration = llDuration;
	if (m_llDuration == 0)
		m_llDuration = m_pFmtCtx->duration / 1000;

	return QC_ERR_NONE;
}

int CFFMpegParser::Close (void)
{
	if (m_pFmtCtx != NULL)
		avformat_close_input(&m_pFmtCtx);
	m_pFmtCtx = NULL;

	CBaseFFParser::Close();

	QC_DEL_P (m_pFFIO);

	return QC_ERR_NONE;
}

int CFFMpegParser::Read (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL)
		return QC_ERR_NONE;

	av_free_packet(m_pPackData);
	AVPacket *	pPacket = m_pPackData;// GetEmptyPacket();
	int nRC = av_read_frame(m_pFmtCtx, pPacket);
	if (nRC < 0)
	{
		if (m_nIOType == QC_IOTYPE_RTSP)
		{
			return QC_ERR_RETRY;
		}
		else
		{
			m_bEOS = true;
			return QC_ERR_FINISH;
		}
	}
	if (pPacket->stream_index == m_nIdxAudio)
	{
		pBuff->nMediaType = QC_MEDIA_Audio;
		pBuff->llTime = ffBaseToTime(pPacket->pts, m_pStmAudio);
		pPacket->pts = ffBaseToTime(pPacket->pts, m_pStmAudio);
		pPacket->dts = ffBaseToTime(pPacket->dts, m_pStmAudio);
	}
	else if (pPacket->stream_index == m_nIdxVideo)
	{
		pBuff->nMediaType = QC_MEDIA_Video;
		pBuff->llTime = ffBaseToTime(pPacket->pts, m_pStmVideo);
		pBuff->uFlag = 0;
		if ((pPacket->flags & AV_PKT_FLAG_KEY) == AV_PKT_FLAG_KEY)
			pBuff->uFlag = QCBUFF_KEY_FRAME;
		pPacket->pts = ffBaseToTime(pPacket->pts, m_pStmVideo);
		pPacket->dts = ffBaseToTime(pPacket->dts, m_pStmVideo);
	}
	else if (pPacket->stream_index == m_nIdxSubtt)
	{
		pBuff->nMediaType = QC_MEDIA_Subtt;
		pBuff->llTime = ffBaseToTime(pPacket->pts, m_pStmSubtt);
		pPacket->pts = ffBaseToTime(pPacket->pts, m_pStmSubtt);
		pPacket->dts = ffBaseToTime(pPacket->dts, m_pStmSubtt);
	}
	else
	{
		return QC_ERR_RETRY;
	}

//	pBuff->uBuffType = QC_BUFF_TYPE_Packet;
//	pBuff->pBuffPtr = pPacket;
//	pBuff->uSize = m_nPacketSize;
	pBuff->uBuffType = QC_BUFF_TYPE_Data;
	pBuff->pBuff = pPacket->data;
	pBuff->uSize = pPacket->size;
	pBuff->pUserData = this;

	return QC_ERR_NONE;
}

int	CFFMpegParser::CanSeek (void)
{
	if (m_nIOType == QC_IOTYPE_RTSP)
		return 0;
	else
		return 1;
}

long long CFFMpegParser::GetPos (void)
{
	return 0;
}

long long CFFMpegParser::SetPos (long long llPos)
{
	int			nRC = 0;
	AVPacket *	pPacket = NULL;
	long long	llNewPos = llPos;
	long long	llVideoPos = 0;
	bool		bSeekAudio = false;
	long long	llPosAudio = 0;

	long long	llAudioDur = 0;
	long long	llVideoDur = 0;
	if (m_pStmAudio != NULL)
		llAudioDur = ffBaseToTime(m_pStmAudio->duration, m_pStmAudio);
	if (m_pStmVideo != NULL)
		llVideoDur = ffBaseToTime(m_pStmVideo->duration, m_pStmVideo);

	if (m_nIdxAudio >= 0)
	{
		llPosAudio = ffTimeToBase(llNewPos, m_pStmAudio);
		if (m_nIdxVideo < 0)
		{
			bSeekAudio = true;
		}
		else
		{
			if (!strcmp(m_pFmtCtx->iformat->name, "rm") ||
				!strcmp(m_pFmtCtx->iformat->name, "asf") ||
				!strcmp(m_pFmtCtx->iformat->name, "aac") ||
				!strcmp(m_pFmtCtx->iformat->name, "mp3"))
			{
				bSeekAudio = true;
			}
			else if (strstr("mpegts", m_pFmtCtx->iformat->name) != NULL)
			{
				bSeekAudio = true;
			}

			if (llVideoDur > 0 && llPos > llVideoDur)
				bSeekAudio = true;
		}
	}

	if (bSeekAudio)
	{
		if (llAudioDur > 0 && llPos > llAudioDur)
		{
			llPos = llAudioDur - 2000;
			if (llPos < 0)
				llPos = 0;
			llPosAudio = ffTimeToBase(llPos, m_pStmAudio);
		}
		nRC = av_seek_frame(m_pFmtCtx, m_nIdxAudio, llPosAudio, AVSEEK_FLAG_ANY);
		if (nRC != 0)
			return QC_ERR_FAILED;
		m_bEOS = false;
	}
	else
	{
		if (llVideoDur > 0 && llPos > llVideoDur)
		{
			llPos = llVideoDur - 2000;
			if (llPos < 0)
				llPos = 0;
		}

		long long	llPosVideo = ffTimeToBase(llPos, m_pStmVideo);
		nRC = av_seek_frame(m_pFmtCtx, m_nIdxVideo, llPosVideo, AVSEEK_FLAG_BACKWARD);
		if (nRC != 0)
			return QC_ERR_FAILED;
		m_bEOS = false;
	}
	
	return QC_ERR_NONE;
}

int CFFMpegParser::GetParam (int nID, void * pParam)
{
	return QC_ERR_IMPLEMENT;
}

int CFFMpegParser::SetParam (int nID, void * pParam)
{
	switch (nID)
	{
	case QCPLAY_PID_Log_Level:
		g_nQcCodecLogLevel = *(int *)pParam;
		return QC_ERR_NONE;

	case QCIO_PID_RTSP_UDP_TCP_MODE:
		if (pParam == NULL)
			return QC_ERR_ARG;
		if (m_nFormat != QC_PARSER_RTSP)
			return QC_ERR_STATUS;
		if (*(int *)pParam == 1)
			av_dict_set(&m_pFmtOpts, "rtsp_transport", "tcp", 0);
		else
			av_dict_set(&m_pFmtOpts, "rtsp_transport", "udp", 0);
		return QC_ERR_NONE;

	default:
		break;
	}

	return QC_ERR_IMPLEMENT;
}

long long CFFMpegParser::ffBaseToTime(long long llBase, AVStream * pStream)
{
	long long llTime = llBase * 1000 * pStream->time_base.num / pStream->time_base.den;

	return llTime;
}

long long CFFMpegParser::ffTimeToBase(long long llTime, AVStream * pStream)
{
	if (pStream->time_base.num == 0)
		return llTime;

	long long llBase = (llTime * pStream->time_base.den) / (pStream->time_base.num * 1000);

	return llBase;
}
