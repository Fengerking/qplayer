/*******************************************************************************
	File:		qcCodec.cpp

	Contains:	Create the Codec with codec id

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-24		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "qcFFWrap.h"

void ffFormat_Init(QCParser_Context * pParser)
{
	av_register_all();
	avformat_network_init();
	pParser->m_nPacketSize = sizeof(AVPacket);

	pParser->m_pFmtOpts = NULL;
	pParser->m_pPackData = (AVPacket *) malloc (sizeof (AVPacket));
	av_init_packet(pParser->m_pPackData);
}

void ffFormat_Uninit(QCParser_Context * pParser)
{
	//Close();
	av_dict_free(&pParser->m_pFmtOpts);
	av_free_packet(pParser->m_pPackData);
	free (pParser->m_pPackData);
	avformat_network_deinit();
}

int ffFormat_Open(QCParser_Context * pParser, const char * pURL)
{
	if (pParser->m_pQCIOInfo != NULL && pParser->m_pQCIOInfo->m_pQCIO != NULL)
	{
		ffIO_Open(pParser->m_pQCIOInfo);
		pParser->m_pFmtCtx = avformat_alloc_context();
		pParser->m_pFmtCtx->pb = pParser->m_pQCIOInfo->m_pAVIO;
	}
	int nRC = avformat_open_input(&pParser->m_pFmtCtx, pURL, NULL, &pParser->m_pFmtOpts);
	if (nRC < 0)
	{
		av_log(NULL, AV_LOG_WARNING, "Open source %s failed! err = 0X%08X", pURL, nRC);
		return QC_ERR_FAILED;
	}
	// retrieve stream information
	nRC = avformat_find_stream_info(pParser->m_pFmtCtx, NULL);
	if (nRC < 0)
		return QC_ERR_FAILED;
	AVCodecContext * pDecCtx = NULL;
	pParser->m_nIdxVideo = av_find_best_stream(pParser->m_pFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (pParser->m_nIdxVideo >= 0)
	{
		// Check the video stream to get best video stream
		int nVideoNum = 0;
		int	nVideoIdx = -1;
		int nMaxW = 0;
		for (int i = 0; i < pParser->m_pFmtCtx->nb_streams; i++)
		{
			pDecCtx = pParser->m_pFmtCtx->streams[i]->codec;
			if (pParser->m_pFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				if (pDecCtx->width > nMaxW)
				{
					nMaxW = pDecCtx->width;
					nVideoIdx = i;
				}
				nVideoNum++;
			}
		}
		if (nVideoNum > 1 && nVideoIdx != pParser->m_nIdxVideo)
		{
			if (pParser->m_pFmtCtx->streams[nVideoIdx]->nb_frames >= pParser->m_pFmtCtx->streams[pParser->m_nIdxVideo]->nb_frames)
			{
				pParser->m_nIdxVideo = nVideoIdx;
			}
		}
		pParser->m_pStmVideo = pParser->m_pFmtCtx->streams[pParser->m_nIdxVideo];
		pDecCtx = pParser->m_pStmVideo->codec;
	}

	pParser->m_nIdxAudio = av_find_best_stream(pParser->m_pFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (pParser->m_nIdxAudio >= 0)
	{
		pDecCtx = pParser->m_pFmtCtx->streams[pParser->m_nIdxAudio]->codec;
		pParser->m_nAudioNum = 0;
		for (int i = 0; i < pParser->m_pFmtCtx->nb_streams; i++)
		{
			if (pParser->m_pFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
				pParser->m_nAudioNum++;
		}
		pParser->m_pStmAudio = pParser->m_pFmtCtx->streams[pParser->m_nIdxAudio];
	}
	long long llDuration = 0;
	if (pParser->m_pStmAudio != NULL)
		pParser->m_llDuration = ffBaseToTime(pParser->m_pStmAudio->duration, pParser->m_pStmAudio);
	if (pParser->m_pStmVideo != NULL)
		llDuration = ffBaseToTime(pParser->m_pStmVideo->duration, pParser->m_pStmVideo);
	if (llDuration > pParser->m_llDuration)
		pParser->m_llDuration = llDuration;
	if (pParser->m_llDuration == 0)
		pParser->m_llDuration = pParser->m_pFmtCtx->duration / 1000;
	return QC_ERR_NONE;
}

int ffFormat_Close(QCParser_Context * pParser)
{
	if (pParser->m_pFmtCtx != NULL)
		avformat_close_input(&pParser->m_pFmtCtx);
	pParser->m_pFmtCtx = NULL;

	return QC_ERR_NONE;
}

int ffFormat_Read(QCParser_Context * pParser, QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL)
		return QC_ERR_NONE;

	av_free_packet(pParser->m_pPackData);
	AVPacket *	pPacket = pParser->m_pPackData;// GetEmptyPacket();
	int nRC = av_read_frame(pParser->m_pFmtCtx, pPacket);
	if (nRC < 0)
	{
		pParser->m_bEOS = TRUE;
		return QC_ERR_FINISH;
	}

	if (pPacket->stream_index == pParser->m_nIdxAudio)
	{
		pBuff->nMediaType = QC_MEDIA_Audio;
		pBuff->llTime = ffBaseToTime(pPacket->pts, pParser->m_pStmAudio);
		pPacket->pts = ffBaseToTime(pPacket->pts, pParser->m_pStmAudio);
		pPacket->dts = ffBaseToTime(pPacket->dts, pParser->m_pStmAudio);
	}
	else if (pPacket->stream_index == pParser->m_nIdxVideo)
	{
		pBuff->nMediaType = QC_MEDIA_Video;
		pBuff->llTime = ffBaseToTime(pPacket->pts, pParser->m_pStmVideo);
		pBuff->uFlag = 0;
		if ((pPacket->flags & AV_PKT_FLAG_KEY) == AV_PKT_FLAG_KEY)
			pBuff->uFlag = QCBUFF_KEY_FRAME;
		pPacket->pts = ffBaseToTime(pPacket->pts, pParser->m_pStmVideo);
		pPacket->dts = ffBaseToTime(pPacket->dts, pParser->m_pStmVideo);
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
	return QC_ERR_NONE;
}

int ffFormat_SetPos(QCParser_Context * pParser, long long llPos)
{
	int			nRC = 0;
	AVPacket *	pPacket = NULL;
	long long	llNewPos = llPos;
	long long	llVideoPos = 0;
	BOOL		bSeekAudio = FALSE;
	long long	llPosAudio = 0;

	long long	llAudioDur = 0;
	long long	llVideoDur = 0;
	if (pParser->m_pStmAudio != NULL)
		llAudioDur = ffBaseToTime(pParser->m_pStmAudio->duration, pParser->m_pStmAudio);
	if (pParser->m_pStmVideo != NULL)
		llVideoDur = ffBaseToTime(pParser->m_pStmVideo->duration, pParser->m_pStmVideo);

	if (pParser->m_nIdxAudio >= 0)
	{
		llPosAudio = ffTimeToBase(llNewPos, pParser->m_pStmAudio);
		if (pParser->m_nIdxVideo < 0)
		{
			bSeekAudio = TRUE;
		}
		else
		{
			if (!strcmp(pParser->m_pFmtCtx->iformat->name, "rm") ||
				!strcmp(pParser->m_pFmtCtx->iformat->name, "asf") ||
				!strcmp(pParser->m_pFmtCtx->iformat->name, "aac") ||
				!strcmp(pParser->m_pFmtCtx->iformat->name, "mp3"))
			{
				bSeekAudio = TRUE;
			}
			else if (strstr("mpegts", pParser->m_pFmtCtx->iformat->name) != NULL)
			{
				bSeekAudio = TRUE;
			}

			if (llVideoDur > 0 && llPos > llVideoDur)
				bSeekAudio = TRUE;
		}
	}

	if (bSeekAudio)
	{
		if (llAudioDur > 0 && llPos > llAudioDur)
		{
			llPos = llAudioDur - 2000;
			if (llPos < 0)
				llPos = 0;
			llPosAudio = ffTimeToBase(llPos, pParser->m_pStmAudio);
		}
		nRC = av_seek_frame(pParser->m_pFmtCtx, pParser->m_nIdxAudio, llPosAudio, AVSEEK_FLAG_ANY);
		if (nRC != 0)
			return QC_ERR_FAILED;
		pParser->m_bEOS = FALSE;
	}
	else
	{
		if (llVideoDur > 0 && llPos > llVideoDur)
		{
			llPos = llVideoDur - 2000;
			if (llPos < 0)
				llPos = 0;
		}

		long long	llPosVideo = ffTimeToBase(llPos, pParser->m_pStmVideo);
		nRC = av_seek_frame(pParser->m_pFmtCtx, pParser->m_nIdxVideo, llPosVideo, AVSEEK_FLAG_BACKWARD);
		if (nRC != 0)
			return QC_ERR_FAILED;
		pParser->m_bEOS = FALSE;
	}
	return QC_ERR_NONE;
}

int	ffIO_Open(QCIOFuncInfo * pIOInfo)
{
	if (pIOInfo == NULL)
		return QC_ERR_ARG;
	if (pIOInfo->m_pBuffer == NULL)
		pIOInfo->m_pBuffer = (unsigned char *)av_malloc(pIOInfo->m_nBuffSize);
	pIOInfo->m_pAVIO = avio_alloc_context(pIOInfo->m_pBuffer, pIOInfo->m_nBuffSize, 0, pIOInfo, ffIO_Read, ffIO_Write, ffIO_Seek);
	return QC_ERR_NONE;
}

int	ffIO_Close(QCIOFuncInfo * pIOInfo)
{
	if (pIOInfo == NULL)
		return QC_ERR_ARG;

	if (pIOInfo->m_pAVIO == NULL)
		return QC_ERR_NONE;

	av_freep(&pIOInfo->m_pAVIO->buffer);
	pIOInfo->m_pBuffer = NULL;
	av_freep(&pIOInfo->m_pAVIO);
	pIOInfo->m_pAVIO = NULL;

	return 0;
}

int	ffMux_Open(QCMuxInfo * pMuxInfo)
{
	av_register_all();

	if (pMuxInfo == NULL || pMuxInfo->m_pFileName == NULL)
		return QC_ERR_EMPTYPOINTOR;

	pMuxInfo->m_nVideoCount = 0;
	// mov and flv support ADPCM-ALAW, ADPCM-MULAW
	if (pMuxInfo->m_nFileFormat == QC_PARSER_MP4)
		avformat_alloc_output_context2(&pMuxInfo->m_hCtx, NULL, "mov", pMuxInfo->m_pFileName);
	else
		avformat_alloc_output_context2(&pMuxInfo->m_hCtx, NULL, NULL, pMuxInfo->m_pFileName);
	if (!pMuxInfo->m_hCtx)
		return QC_ERR_FAILED;

	if (pMuxInfo->m_pAVIO != NULL)
		av_free(pMuxInfo->m_pAVIO);
	if (pMuxInfo->m_pBuffer == NULL)
		pMuxInfo->m_pBuffer = (unsigned char *)av_malloc(pMuxInfo->m_nBuffSize);
	pMuxInfo->m_pAVIO = avio_alloc_context(pMuxInfo->m_pBuffer, pMuxInfo->m_nBuffSize, AVIO_FLAG_WRITE, pMuxInfo, QCMUX_Read, QCMUX_Write, QCMUX_Seek);
	if (pMuxInfo->m_pAVIO == NULL)
		return QC_ERR_FAILED;
	pMuxInfo->m_hCtx->pb = pMuxInfo->m_pAVIO;

	if (pMuxInfo->m_hMuxFile != NULL)
		fclose(pMuxInfo->m_hMuxFile);
	pMuxInfo->m_hMuxFile = fopen(pMuxInfo->m_pFileName, "wb");

	pMuxInfo->m_bInitHeader = FALSE;
	pMuxInfo->m_llStartTime = -1;
	return QC_ERR_NONE;
}

int	ffMux_Close(QCMuxInfo * pMuxInfo)
{
	if (pMuxInfo->m_hCtx == NULL)
		return QC_ERR_NONE;

	if (pMuxInfo->m_hCtx->pb && pMuxInfo->m_bInitHeader)
		av_write_trailer(pMuxInfo->m_hCtx);
	if (pMuxInfo->m_pStmAudio)
	{
		//av_free(pMuxInfo->m_pStmAudio->codecpar->extradata);
		avcodec_parameters_free(&pMuxInfo->m_pStmAudio->codecpar);
		pMuxInfo->m_pStmAudio = NULL;
	}
	if (pMuxInfo->m_pStmVideo)
	{
		//av_free(pMuxInfo->m_pStmVideo->codecpar->extradata);
		avcodec_parameters_free(&pMuxInfo->m_pStmVideo->codecpar);
		pMuxInfo->m_pStmVideo = NULL;
	}
	avformat_free_context(pMuxInfo->m_hCtx);
	pMuxInfo->m_hCtx = NULL;

	pMuxInfo->m_llStartTime = -1;
	if (pMuxInfo->m_pAVIO != NULL)
	{
		av_freep(&pMuxInfo->m_pAVIO);
		//av_free(pMuxInfo->m_pBuffer);
		fclose(pMuxInfo->m_hMuxFile);
		pMuxInfo->m_hMuxFile = NULL;
	}
	pMuxInfo->m_pAVIO = NULL;
	return QC_ERR_NONE;
}

int	ffMux_AddTrack(QCMuxInfo * pMuxInfo, void * pFormat, QCMediaType nType)
{
	QC_AUDIO_FORMAT * pFmtAudio = NULL;
	QC_VIDEO_FORMAT * pFmtVideo = NULL;

	AVStream* st = avformat_new_stream(pMuxInfo->m_hCtx, NULL);
	if (!st)
		return QC_ERR_FAILED;

	if (nType == QC_MEDIA_Audio)
	{
		pFmtAudio = (QC_AUDIO_FORMAT*)pFormat;
		pMuxInfo->m_pStmAudio = st;
		st->id = pMuxInfo->m_hCtx->nb_streams - 1;

		AVCodecParameters* params = avcodec_parameters_alloc();
		params->codec_id = CodecIdQplayer2FF(pFmtAudio->nCodecID);
		if (params->codec_id == AV_CODEC_ID_NONE)
			params->codec_id = AV_CODEC_ID_AAC;
		params->codec_type = AVMEDIA_TYPE_AUDIO;
		params->codec_tag = 0;
		params->extradata = pFmtAudio->pHeadData;
		params->extradata_size = pFmtAudio->nHeadSize;
		params->channels = pFmtAudio->nChannels;
		params->sample_rate = pFmtAudio->nSampleRate;
		params->bits_per_coded_sample = pFmtAudio->nBits == 0 ? 16 : pFmtAudio->nBits;
		params->frame_size = 1024; // HE-AAC 2048, MP3 1152 ???
		avcodec_parameters_copy(st->codecpar, params);
		avcodec_parameters_free(&params);
	}
	else if (nType == QC_MEDIA_Video)
	{
		pFmtVideo = (QC_VIDEO_FORMAT*)pFormat;
		pMuxInfo->m_pStmVideo = st;
		st->id = pMuxInfo->m_hCtx->nb_streams - 1;

		AVCodecParameters* params = avcodec_parameters_alloc();
		params->codec_id = CodecIdQplayer2FF(pFmtVideo->nCodecID );
		if (params->codec_id == AV_CODEC_ID_NONE)
			params->codec_id = AV_CODEC_ID_H264;
		params->codec_type = AVMEDIA_TYPE_VIDEO;
		params->codec_tag = 0;
		if (pFmtVideo->pHeadData)
		{
			params->extradata = (uint8_t*)av_malloc(pFmtVideo->nHeadSize);
			memcpy(params->extradata, pFmtVideo->pHeadData, pFmtVideo->nHeadSize);
			params->extradata_size = pFmtVideo->nHeadSize;
		}
		params->width = pFmtVideo->nWidth;
		params->height = pFmtVideo->nHeight;
		params->bits_per_raw_sample = 8;
		params->profile = 77;//?params->level=30;
		avcodec_parameters_copy(st->codecpar, params);
		avcodec_parameters_free(&params);
	}
	return QC_ERR_NONE;
}

int	ffMux_Write(QCMuxInfo * pMuxInfo, QC_DATA_BUFF* pBuff)
{
	if (!pMuxInfo->m_hCtx)
		return QC_ERR_EMPTYPOINTOR;

	if (!pMuxInfo->m_bInitHeader)
	{
		int ret = avformat_write_header(pMuxInfo->m_hCtx, NULL);
		if (ret < 0)
			return QC_ERR_FAILED;
		pMuxInfo->m_bInitHeader = TRUE;
	}

	if (pBuff->nMediaType == QC_MEDIA_Video)
	{
		if (!pMuxInfo->m_pStmVideo)
			return QC_ERR_NONE;
		if (pMuxInfo->m_llStartTime == -1)
			pMuxInfo->m_llStartTime = pBuff->llTime;

		AVPacket pkt;
		av_new_packet(&pkt, pBuff->uSize);
		memcpy(pkt.data, pBuff->pBuff, pBuff->uSize);
		pkt.size = pBuff->uSize;
		pkt.flags = 0;
		if (pBuff->uFlag & QCBUFF_KEY_FRAME)
			pkt.flags |= AV_PKT_FLAG_KEY;
		long long llTime = pBuff->llTime - pMuxInfo->m_llStartTime + 40;
		pkt.pts = ffTimeToBase(llTime, pMuxInfo->m_pStmVideo);
		llTime = pBuff->llDelay - pMuxInfo->m_llStartTime;
		pkt.dts = ffTimeToBase(llTime, pMuxInfo->m_pStmVideo);
		pkt.duration = 0;
		pkt.pos = -1;
		pkt.stream_index = pMuxInfo->m_pStmVideo->id;
		int ret = av_interleaved_write_frame(pMuxInfo->m_hCtx, &pkt);
		av_free_packet(&pkt);
		++pMuxInfo->m_nVideoCount;
	}
	else if (pBuff->nMediaType == QC_MEDIA_Audio)
	{
		if (!pMuxInfo->m_pStmAudio)
			return QC_ERR_NONE;
		if (pMuxInfo->m_llStartTime == -1 || pBuff->llTime < pMuxInfo->m_llStartTime)
			return QC_ERR_STATUS;

		AVPacket pkt;
		av_new_packet(&pkt, pBuff->uSize);
		memcpy(pkt.data, pBuff->pBuff, pBuff->uSize);
		pkt.size = pBuff->uSize;
		pkt.pos = -1;
		pkt.stream_index = pMuxInfo->m_pStmAudio->id;
		pkt.duration = 0;// ffTimeToBase(21, pMuxInfo->m_pStmAudio);
		pkt.pts = ffTimeToBase(pBuff->llTime - pMuxInfo->m_llStartTime, pMuxInfo->m_pStmAudio);
		pkt.dts = pkt.pts;
		int nRC = av_interleaved_write_frame(pMuxInfo->m_hCtx, &pkt);
		av_free_packet(&pkt);
	}
	return QC_ERR_NONE;
}

int	QCMUX_Read(void *opaque, uint8_t *buf, int buf_size)
{
	QCMuxInfo * pMuxIO = (QCMuxInfo *)opaque;
	if (pMuxIO->m_hMuxFile == NULL)
		return -1;
	int nRC = fread(buf, 1, buf_size, pMuxIO->m_hMuxFile);
	return nRC;
}

int	QCMUX_Write(void *opaque, uint8_t *buf, int buf_size)
{
	QCMuxInfo * pMuxIO = (QCMuxInfo *)opaque;
	if (pMuxIO->m_hMuxFile == NULL)
		return -1;
	int nRC = fwrite(buf, 1, buf_size, pMuxIO->m_hMuxFile);
	return nRC;
}

int64_t	QCMUX_Seek(void *opaque, int64_t offset, int whence)
{
	QCMuxInfo * pMuxIO = (QCMuxInfo *)opaque;
	if (pMuxIO->m_hMuxFile == NULL)
		return -1;
	int64_t nRC = fseek(pMuxIO->m_hMuxFile, (long)offset, whence);
	return nRC;
}

long long ffBaseToTime(long long llBase, AVStream * pStream)
{
	long long llTime = llBase * 1000 * pStream->time_base.num / pStream->time_base.den;

	return llTime;
}

long long ffTimeToBase(long long llTime, AVStream * pStream)
{
	if (pStream->time_base.num == 0)
		return llTime;

	long long llBase = (llTime * pStream->time_base.den) / (pStream->time_base.num * 1000);

	return llBase;
}

int CodecIdQplayer2FF(int nCodecId)
{
	if (QC_CODEC_ID_H264 == nCodecId)
		return AV_CODEC_ID_H264;
	else if (QC_CODEC_ID_H265 == nCodecId)
		return AV_CODEC_ID_HEVC;
	else if (QC_CODEC_ID_MPEG4 == nCodecId)
		return AV_CODEC_ID_MPEG4;
	else if (QC_CODEC_ID_AAC == nCodecId)
		return AV_CODEC_ID_AAC;
	//    else if(QC_CODEC_ID_G711U == nCodecId)
	//        return AV_CODEC_ID_ADPCM_SWF;
	else if (QC_CODEC_ID_G711U == nCodecId)
		return AV_CODEC_ID_PCM_MULAW;
	else if (QC_CODEC_ID_G711A == nCodecId)
		return AV_CODEC_ID_PCM_ALAW;

	return AV_CODEC_ID_NONE;
}