/*******************************************************************************
 File:        CBaseFFMuxer.h
 
 Contains:    CBaseFFMuxer implementation file.
 
 Written by:    Jun Lin
 
 Change History (most recent first):
 2018-01-06        Jun            Create file
 
 *******************************************************************************/
#ifdef __QC_OS_WIN32__
#ifdef __QC_LIB_FFMPEG__
#include "./libWin32/stdafx.h"
#endif // __QC_LIB_FFMPEG__
#endif // __QC_OS_WIN32__

#include "CBaseFFMuxer.h"
#include "USystemFunc.h"
#include "UAVParser.h"
#include "qcFFLog.h"

CBaseFFMuxer::CBaseFFMuxer(QCParserFormat nFormat)
	: m_hCtx(NULL)
	, m_nFileFormat(nFormat)
	, m_pStmAudio(NULL)
	, m_pStmVideo(NULL)
	, m_pFileName(NULL)
	, m_nVideoCount(0)
	, m_llStartTime(-1)
	, m_bInitHeader(false)
	, m_pAVIO(NULL)
	, m_pBuffer(NULL)
	, m_nBuffSize(32768)
	, m_hMuxFile(NULL)
{
    av_register_all();
	m_nBuffSize = 32768 * 2;
}

CBaseFFMuxer::~CBaseFFMuxer(void)
{
    Close();
}

int CBaseFFMuxer::Open(const char * pURL)
{
    if(!pURL)
        return QC_ERR_EMPTYPOINTOR;
    
    m_nVideoCount = 0;
    QC_DEL_A(m_pFileName);
    m_pFileName = new char[strlen(pURL)+1];
    memset(m_pFileName, 0, strlen(pURL)+1);
    strcpy(m_pFileName, pURL);
    
    // mov and flv support ADPCM-ALAW, ADPCM-MULAW
    if(m_nFileFormat == QC_PARSER_MP4)
    	avformat_alloc_output_context2(&m_hCtx, NULL, "mov", pURL);
    else
        avformat_alloc_output_context2(&m_hCtx, NULL, NULL, pURL);
    if(!m_hCtx)
        return QC_ERR_FAILED;

	if (m_pAVIO != NULL)
		av_free(m_pAVIO);
	if (m_pBuffer == NULL)
		m_pBuffer = (unsigned char *)av_malloc(m_nBuffSize);
	m_pAVIO = avio_alloc_context(m_pBuffer, m_nBuffSize, AVIO_FLAG_WRITE, this, QCMUX_Read, QCMUX_Write, QCMUX_Seek);
	if (m_pAVIO == NULL)
		return QC_ERR_FAILED;
	m_hCtx->pb = m_pAVIO;

	if (m_hMuxFile != NULL)
		fclose(m_hMuxFile);
	m_hMuxFile = fopen(pURL, "wb");

	m_bInitHeader = false;

    return QC_ERR_NONE;
}

int CBaseFFMuxer::Close()
{
    if(m_hCtx)
    {
        if(m_hCtx->pb && m_bInitHeader)
            av_write_trailer(m_hCtx);
        if(m_pStmAudio)
        {
            //av_free(m_pStmAudio->codecpar->extradata);
            avcodec_parameters_free(&m_pStmAudio->codecpar);
            m_pStmAudio = NULL;
        }
        if(m_pStmVideo)
        {
            //av_free(m_pStmVideo->codecpar->extradata);
            avcodec_parameters_free(&m_pStmVideo->codecpar);
            m_pStmVideo = NULL;
        }
        avformat_free_context(m_hCtx);
        m_hCtx = NULL;
    }

    QC_DEL_A(m_pFileName);
    m_llStartTime = -1;
   
	if (m_pAVIO != NULL)
	{
		av_freep(&m_pAVIO);
		//av_free(m_pBuffer);
		fclose(m_hMuxFile);
		m_hMuxFile = NULL;
	}
	m_pAVIO = NULL;

    return QC_ERR_NONE;
}

int CBaseFFMuxer::Init(void * pVideoFmt, void * pAudioFmt)
{
	if (pVideoFmt != NULL)
		AddVideoTrack((QC_VIDEO_FORMAT *)pVideoFmt);
	if (pAudioFmt != NULL)
		AddAudioTrack((QC_AUDIO_FORMAT *)pAudioFmt);
	return 0;
}

int CBaseFFMuxer::Write(QC_DATA_BUFF* pBuff)
{
    if(!m_hCtx)
        return QC_ERR_EMPTYPOINTOR;
     
	if (!m_bInitHeader)
	{
		int ret = avformat_write_header(m_hCtx, NULL);
		if (ret < 0)
			return QC_ERR_FAILED;
		//av_dump_format(m_hCtx, 0, m_pFileName, 1);
		m_bInitHeader = true;
	}

	if (pBuff->nMediaType == QC_MEDIA_Video)
    {
        if(!m_pStmVideo)
            return QC_ERR_NONE;
        WriteVideoSample(pBuff);
    }
    else if (pBuff->nMediaType == QC_MEDIA_Audio)
    {
        if(!m_pStmAudio)
            return QC_ERR_NONE;
        WriteAudioSample(pBuff);
    }

    return QC_ERR_NONE;
}

int CBaseFFMuxer::AddAudioTrack(QC_AUDIO_FORMAT * pFmt)
{
    AVStream* st = avformat_new_stream(m_hCtx, NULL);
    if(!st)
        return QC_ERR_FAILED;
    
    m_pStmAudio = st;
    st->id = m_hCtx->nb_streams-1;
    
    AVCodecParameters* params = avcodec_parameters_alloc();
	params->codec_id = (AVCodecID)CodecIdQplayer2FF(pFmt ? pFmt->nCodecID : QC_CODEC_ID_AAC);
    params->codec_type = AVMEDIA_TYPE_AUDIO;
    params->codec_tag = 0;
	if (pFmt)
	{
		params->extradata = pFmt->pHeadData;
		params->extradata_size = pFmt->nHeadSize;
		params->channels = pFmt->nChannels;
		params->sample_rate = pFmt->nSampleRate;
		params->bits_per_coded_sample = pFmt->nBits == 0 ? 16 : pFmt->nBits;

	}
    params->frame_size = 1024; // HE-AAC 2048, MP3 1152 ???
	avcodec_parameters_copy(st->codecpar, params);
	avcodec_parameters_free(&params);
	return QC_ERR_NONE;
}

int CBaseFFMuxer::AddVideoTrack(QC_VIDEO_FORMAT * pFmt)
{
    AVStream* st = avformat_new_stream(m_hCtx, NULL);
    if(!st)
        return QC_ERR_FAILED;
    m_pStmVideo = st;
    st->id = m_hCtx->nb_streams-1;
    
    AVCodecParameters* params = avcodec_parameters_alloc();
	params->codec_id = (AVCodecID)CodecIdQplayer2FF(pFmt ? pFmt->nCodecID : QC_CODEC_ID_H264);
    params->codec_type = AVMEDIA_TYPE_VIDEO;
    params->codec_tag = 0;
	if (pFmt->pHeadData)
    {
		params->extradata = (uint8_t*)av_malloc(pFmt->nHeadSize);
		memcpy(params->extradata, pFmt->pHeadData, pFmt->nHeadSize);
		params->extradata_size = pFmt->nHeadSize;
    }
    params->width = pFmt->nWidth;
	params->height = pFmt->nHeight;
    params->bits_per_raw_sample = 8;
    params->profile = 77;//?params->level=30;
    avcodec_parameters_copy(st->codecpar, params);
	avcodec_parameters_free(&params);
    return QC_ERR_NONE;
}

int CBaseFFMuxer::WriteAudioSample(QC_DATA_BUFF* pBuff)
{
	if (m_pStmVideo != NULL)
	{
		if (m_llStartTime == -1 || pBuff->llTime < m_llStartTime)
			return QC_ERR_STATUS;
	}
	else
	{
		if (m_llStartTime == -1)
			m_llStartTime = pBuff->llTime;
	}

    AVPacket pkt;
    av_new_packet(&pkt, pBuff->uSize);
    memcpy(pkt.data, pBuff->pBuff, pBuff->uSize);
    pkt.size = pBuff->uSize;
    pkt.pos = -1;
    pkt.stream_index = m_pStmAudio->id;
	pkt.duration = 0;// ffTimeToBase(21, m_pStmAudio);
    pkt.pts = ffTimeToBase(pBuff->llTime - m_llStartTime, m_pStmAudio);
    pkt.dts = pkt.pts;  
	int nRC = av_interleaved_write_frame(m_hCtx, &pkt);
    av_free_packet(&pkt);
    return QC_ERR_NONE;
}

int CBaseFFMuxer::WriteVideoSample(QC_DATA_BUFF* pBuff)
{
    if(m_llStartTime == -1)
        m_llStartTime = pBuff->llTime;

    AVPacket pkt;
    av_new_packet(&pkt, pBuff->uSize);
    memcpy(pkt.data, pBuff->pBuff, pBuff->uSize);
    pkt.size = pBuff->uSize;
    
	pkt.flags = 0;
    if(pBuff->uFlag & QCBUFF_KEY_FRAME)
        pkt.flags |= AV_PKT_FLAG_KEY;
	long long llTime = pBuff->llTime - m_llStartTime + 40;
	pkt.pts = ffTimeToBase(llTime, m_pStmVideo);
	llTime = pBuff->llDelay - m_llStartTime;
	pkt.dts = ffTimeToBase(llTime, m_pStmVideo);
    pkt.duration = 0;
    pkt.pos = -1;
    pkt.stream_index = m_pStmVideo->id;   
	int ret = av_interleaved_write_frame(m_hCtx, &pkt);
    av_free_packet(&pkt);
    ++m_nVideoCount;
    return QC_ERR_NONE;
}

int CBaseFFMuxer::GetParam(int nID, void * pParam)
{
    if(!m_hCtx)
        return QC_ERR_EMPTYPOINTOR;

    return QC_ERR_IMPLEMENT;
}

int CBaseFFMuxer::SetParam(int nID, void * pParam)
{
    if(!m_hCtx)
        return QC_ERR_EMPTYPOINTOR;

    return QC_ERR_IMPLEMENT;
}

long long CBaseFFMuxer::ffBaseToTime(long long llBase, AVStream* pStream)
{
    long long llTime = llBase * 1000 * pStream->time_base.num / pStream->time_base.den;
    
    return llTime;
}

long long CBaseFFMuxer::ffTimeToBase(long long llTime, AVStream* pStream)
{
    if (pStream->time_base.num == 0)
        return llTime;
    
    long long llBase = (llTime * pStream->time_base.den) / (pStream->time_base.num * 1000);
    
    return llBase;
}

int CBaseFFMuxer::CodecIdQplayer2FF(int nCodecId)
{
    if(QC_CODEC_ID_H264 == nCodecId)
        return AV_CODEC_ID_H264;
    else if(QC_CODEC_ID_H265 == nCodecId)
        return AV_CODEC_ID_HEVC;
    else if(QC_CODEC_ID_MPEG4 == nCodecId)
        return AV_CODEC_ID_MPEG4;
    else if(QC_CODEC_ID_AAC == nCodecId)
        return AV_CODEC_ID_AAC;
//    else if(QC_CODEC_ID_G711U == nCodecId)
//        return AV_CODEC_ID_ADPCM_SWF;
    else if(QC_CODEC_ID_G711U == nCodecId)
        return AV_CODEC_ID_PCM_MULAW;
    else if(QC_CODEC_ID_G711A == nCodecId)
        return AV_CODEC_ID_PCM_ALAW;

    return AV_CODEC_ID_NONE;
}

int	CBaseFFMuxer::MuxRead(uint8_t *buf, int buf_size)
{
	int nRead = buf_size;
	return nRead;
}

int	CBaseFFMuxer::MuxWrite(uint8_t *buf, int buf_size)
{
	int nRC = fwrite(buf, 1, buf_size, m_hMuxFile);
	return nRC;
}

int64_t	CBaseFFMuxer::MuxSeek(int64_t offset, int whence)
{
	int64_t nRC = fseek(m_hMuxFile, (long)offset, whence);
	return nRC;
}

int	CBaseFFMuxer::QCMUX_Read(void *opaque, uint8_t *buf, int buf_size)
{
	CBaseFFMuxer * pMuxIO = (CBaseFFMuxer *)opaque;
	return pMuxIO->MuxRead(buf, buf_size);
}

int	CBaseFFMuxer::QCMUX_Write(void *opaque, uint8_t *buf, int buf_size)
{
	CBaseFFMuxer * pMuxIO = (CBaseFFMuxer *)opaque;
	return pMuxIO->MuxWrite(buf, buf_size);
}

int64_t	CBaseFFMuxer::QCMUX_Seek(void *opaque, int64_t offset, int whence)
{
	CBaseFFMuxer * pMuxIO = (CBaseFFMuxer *)opaque;
	return pMuxIO->MuxSeek(offset, whence);
}

int	CBaseFFMuxer::QCMUX_Pasue(void *opaque, int pause)
{
	return 0;
}
