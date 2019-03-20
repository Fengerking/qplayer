/*******************************************************************************
	File:		qcFFParser.cpp

	Contains:	Create the ff parser.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2019-02-04		Bangfei			Create file

*******************************************************************************/
#ifdef __QC_OS_WIN32__
#ifdef __QC_LIB_FFMPEG__
#include "stdafx.h"
#endif // __QC_LIB_FFMPEG__
#endif // __QC_OS_WIN32__

#include "qcErr.h"
#include "qcParser.h"

#ifdef __QC_OS_NDK__
#include <unistd.h>
#include <android/log.h>
#endif // __QC_OS_NDK__

#if defined(__QC_OS_IOS__) || defined(__QC_OS_MACOS__)
#include <sys/sysctl.h>
#endif

#include "qcFFLog.h"
#include "./libWin32/qcFFWrap.h"
#include "CBaseFFParser.h"

typedef struct
{
	QCParserFormat		m_nFormat;
	QC_STREAM_FORMAT *	m_pFmtStream;
	QC_AUDIO_FORMAT *	m_pFmtAudio;
	QC_VIDEO_FORMAT *	m_pFmtVideo;
	QC_SUBTT_FORMAT *	m_pFmtSubtt;
	bool				m_bEOS;
	bool				m_bLive;
	QCIOType			m_nIOType;

	int					m_nStrmSourceCount;
	int					m_nStrmVideoCount;
	int					m_nStrmAudioCount;
	int					m_nStrmSubttCount;

	int					m_nStrmSourcePlay;
	int					m_nStrmVideoPlay;
	int					m_nStrmAudioPlay;
	int					m_nStrmSubttPlay;

	long long			m_llDuration;
	long long			m_llSeekPos;

	bool				m_bEnableSubtt;

	int					m_nNALLengthSize;
	unsigned char *		m_pAVCBuffer;
	int					m_nAVCSize;
} QCParserInfo;

int qcParser_Close(QCParserInfo * qcParserInfo)
{
	if (qcParserInfo->m_pFmtStream != NULL)
	{
		QC_DEL_P(qcParserInfo->m_pFmtStream);
	}
	if (qcParserInfo->m_pFmtAudio != NULL)
	{
		QC_DEL_A(qcParserInfo->m_pFmtAudio->pHeadData);
		QC_DEL_P(qcParserInfo->m_pFmtAudio);
	}
	if (qcParserInfo->m_pFmtVideo != NULL)
	{
		QC_DEL_A(qcParserInfo->m_pFmtVideo->pHeadData);
		QC_DEL_P(qcParserInfo->m_pFmtVideo);
	}
	if (qcParserInfo->m_pFmtSubtt != NULL)
	{
		QC_DEL_A(qcParserInfo->m_pFmtSubtt->pHeadData);
		QC_DEL_P(qcParserInfo->m_pFmtSubtt);
	}

	QC_DEL_A(qcParserInfo->m_pAVCBuffer);
	qcParserInfo->m_nAVCSize = 0;

	return QC_ERR_NONE;
}

int qcParser_DeleteFormat(QCParserInfo * qcParserInfo, QCMediaType nType)
{
	switch (nType)
	{
	case QC_MEDIA_Source:
		if (qcParserInfo->m_pFmtStream == NULL)
			break;
		delete qcParserInfo->m_pFmtStream;
		qcParserInfo->m_pFmtStream = NULL;
		break;
	case QC_MEDIA_Video:
		if (qcParserInfo->m_pFmtVideo == NULL)
			break;
		QC_DEL_A(qcParserInfo->m_pFmtVideo->pHeadData);
		QC_DEL_P(qcParserInfo->m_pFmtVideo);
		break;
	case QC_MEDIA_Audio:
		if (qcParserInfo->m_pFmtAudio == NULL)
			break;
		QC_DEL_A(qcParserInfo->m_pFmtAudio->pHeadData);
		QC_DEL_P(qcParserInfo->m_pFmtAudio);
		break;
	case QC_MEDIA_Subtt:
		if (qcParserInfo->m_pFmtSubtt == NULL)
			break;
		QC_DEL_A(qcParserInfo->m_pFmtAudio->pHeadData);
		QC_DEL_P(qcParserInfo->m_pFmtSubtt);
		break;

	default:
		break;
	}
	return QC_ERR_NONE;
}

int ffParser_Open(void * hParser, QC_IO_Func * pIO, const char * pURL, int nFlag)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	QCParserInfo *		pQCParser = (QCParserInfo *)pContext->m_pQCParser;

	if (pIO != NULL && pIO->hIO != NULL)
	{
		if (pIO->GetSize(pIO->hIO) <= 0)
		{
			if (pIO->Open(pIO->hIO, pURL, 0, QCIO_FLAG_READ) != QC_ERR_NONE)
				return QC_ERR_FAILED;
		}
		pContext->m_pQCIOInfo->m_pQCIO = pIO;
	}
	int nRC = ffFormat_Open(pContext, pURL);
	if (nRC != QC_ERR_NONE)
		return nRC;

	AVCodecContext * pDecCtx = NULL;
	if (pContext->m_pStmVideo != NULL)
	{
		pDecCtx = pContext->m_pFmtCtx->streams[0]->codec;

		pQCParser->m_nStrmVideoCount = 1;
		pQCParser->m_nStrmVideoPlay = 0;

		qcParser_DeleteFormat(pQCParser, QC_MEDIA_Video);
		pQCParser->m_pFmtVideo = new QC_VIDEO_FORMAT();
		memset(pQCParser->m_pFmtVideo, 0, sizeof(QC_VIDEO_FORMAT));
		if (pDecCtx->codec_id == AV_CODEC_ID_H264)
			pQCParser->m_pFmtVideo->nCodecID = QC_CODEC_ID_H264;
		else if (pDecCtx->codec_id == AV_CODEC_ID_HEVC)
			pQCParser->m_pFmtVideo->nCodecID = QC_CODEC_ID_H265;
		pQCParser->m_pFmtVideo->nWidth = pDecCtx->width;
		pQCParser->m_pFmtVideo->nHeight = pDecCtx->height;
		pQCParser->m_pFmtVideo->nSourceType = QC_SOURCE_QC;
		if (pDecCtx->extradata_size > 0)
		{
			pQCParser->m_pFmtVideo->pHeadData = new unsigned char[pDecCtx->extradata_size];
			memcpy(pQCParser->m_pFmtVideo->pHeadData, pDecCtx->extradata, pDecCtx->extradata_size);
			pQCParser->m_pFmtVideo->nHeadSize = pDecCtx->extradata_size;
		}
		pQCParser->m_pFmtVideo->nSourceType = QC_SOURCE_FF;
		pQCParser->m_pFmtVideo->pPrivateData = pDecCtx;
		pQCParser->m_pFmtVideo->nPrivateFlag = QC_SOURCE_FF;
	}
	if (pContext->m_pStmAudio != NULL)
	{
		pDecCtx = pContext->m_pFmtCtx->streams[0]->codec;
		pQCParser->m_nStrmAudioCount = 1;
		pQCParser->m_nStrmAudioPlay = 0;

		qcParser_DeleteFormat(pQCParser, QC_MEDIA_Audio);
		pQCParser->m_pFmtAudio = new QC_AUDIO_FORMAT();
		memset(pQCParser->m_pFmtAudio, 0, sizeof(QC_AUDIO_FORMAT));
		if (pDecCtx->codec_id == AV_CODEC_ID_AAC)
			pQCParser->m_pFmtAudio->nCodecID = QC_CODEC_ID_AAC;
		else if (pDecCtx->codec_id == AV_CODEC_ID_MP3)
			pQCParser->m_pFmtAudio->nCodecID = QC_CODEC_ID_MP3;
		else if (pDecCtx->codec_id == AV_CODEC_ID_MP2)
			pQCParser->m_pFmtAudio->nCodecID = QC_CODEC_ID_MP2;
		pQCParser->m_pFmtAudio->nChannels = pDecCtx->channels;
		pQCParser->m_pFmtAudio->nSampleRate = pDecCtx->sample_rate;
		pQCParser->m_pFmtAudio->nBits = 16;
		pQCParser->m_pFmtAudio->nSourceType = QC_SOURCE_QC;
		if (pDecCtx->extradata_size > 0)
		{
			pQCParser->m_pFmtAudio->pHeadData = new unsigned char[pDecCtx->extradata_size];
			memcpy(pQCParser->m_pFmtAudio->pHeadData, pDecCtx->extradata, pDecCtx->extradata_size);
			pQCParser->m_pFmtAudio->nHeadSize = pDecCtx->extradata_size;
		}
		pQCParser->m_pFmtAudio->nSourceType = QC_SOURCE_FF;
		pQCParser->m_pFmtAudio->pPrivateData = pDecCtx;
		pQCParser->m_pFmtAudio->nPrivateFlag = QC_SOURCE_FF;
	}

	return QC_ERR_NONE;
}

int ffParser_Close(void * hParser)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	int nRC = ffFormat_Close(pContext);
	QCParserInfo *	pQCParser = (QCParserInfo *)pContext->m_pQCParser;
	if (pQCParser != NULL)
	{
		qcParser_Close(pQCParser);
		delete pQCParser;
		pContext->m_pQCParser = NULL;
	}

	if (pContext->m_pQCIOInfo != NULL)
	{
		ffIO_Close(pContext->m_pQCIOInfo);
		delete pContext->m_pQCIOInfo;
		pContext->m_pQCIOInfo = NULL;
	}

	return QC_ERR_NONE;
}

int	ffParser_GetStreamCount(void * hParser, QCMediaType nType)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	QCParserInfo *	pQCParser = (QCParserInfo *)pContext->m_pQCParser;
	if (nType == QC_MEDIA_Source)
		return pQCParser->m_nStrmSourceCount;
	else if (nType == QC_MEDIA_Video)
		return pQCParser->m_nStrmVideoCount;
	else if (nType == QC_MEDIA_Audio)
		return pQCParser->m_nStrmAudioCount;
	else if (nType == QC_MEDIA_Subtt)
		return pQCParser->m_nStrmSubttCount;
	else
		return 0;
}

int	ffParser_GetStreamPlay(void * hParser, QCMediaType nType)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	QCParserInfo *	pQCParser = (QCParserInfo *)pContext->m_pQCParser;
	if (nType == QC_MEDIA_Source)
		return pQCParser->m_nStrmSourcePlay;
	else if (nType == QC_MEDIA_Video)
		return pQCParser->m_nStrmVideoPlay;
	else if (nType == QC_MEDIA_Audio)
		return pQCParser->m_nStrmAudioPlay;
	else if (nType == QC_MEDIA_Subtt)
		return pQCParser->m_nStrmSubttPlay;
	else
		return QC_ERR_IMPLEMENT;
}

int	ffParser_SetStreamPlay(void * hParser, QCMediaType nType, int nStream)
{
	return QC_ERR_IMPLEMENT;
}

long long ffParser_GetDuration(void * hParser)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	return pContext->m_llDuration;
}

int	ffParser_GetStreamFormat(void * hParser, int nID, QC_STREAM_FORMAT ** ppStreamFmt)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	QCParserInfo *	pQCParser = (QCParserInfo *)pContext->m_pQCParser;
	if (ppStreamFmt == NULL)
		return QC_ERR_ARG;
	*ppStreamFmt = pQCParser->m_pFmtStream;
	return QC_ERR_NONE;
}

int	ffParser_GetAudioFormat(void * hParser, int nID, QC_AUDIO_FORMAT ** ppAudioFmt)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	QCParserInfo *	pQCParser = (QCParserInfo *)pContext->m_pQCParser;
	if (ppAudioFmt == NULL)
		return QC_ERR_ARG;
	*ppAudioFmt = pQCParser->m_pFmtAudio;
	return QC_ERR_NONE;
}

int	ffParser_GetVideoFormat(void * hParser, int nID, QC_VIDEO_FORMAT ** ppVideoFmt)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	QCParserInfo *	pQCParser = (QCParserInfo *)pContext->m_pQCParser;
	if (ppVideoFmt == NULL)
		return QC_ERR_ARG;
	*ppVideoFmt = pQCParser->m_pFmtVideo;
	return QC_ERR_NONE;
}

int	ffParser_GetSubttFormat(void * hParser, int nID, QC_SUBTT_FORMAT ** ppSubttFmt)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	QCParserInfo *	pQCParser = (QCParserInfo *)pContext->m_pQCParser;
	if (ppSubttFmt == NULL)
		return QC_ERR_ARG;
	*ppSubttFmt = pQCParser->m_pFmtSubtt;
	return QC_ERR_NONE;
}

bool ffParser_IsEOS(void * hParser)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	if (pContext->m_bEOS)
		return true;
	else
		return false;
}

bool ffParser_IsLive(void * hParser)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	if (pContext->m_bLive)
		return true;
	else
		return false;
}

int	ffParser_EnableSubtt(void * hParser, bool bEnable)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	return QC_ERR_NONE;
}

int ffParser_Run(void * hParser)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	return QC_ERR_NONE;
}

int ffParser_Pause(void * hParser)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	return QC_ERR_NONE;
}

int ffParser_Stop(void * hParser)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	return QC_ERR_NONE;
}

int ffParser_Read(void * hParser, QC_DATA_BUFF * pBuff)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	int nRC = ffFormat_Read(pContext, pBuff);
	return QC_ERR_NONE;
}

int ffParser_Process(void * hParser, unsigned char * pBuff, int nSize)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	return QC_ERR_NONE;
}

int ffParser_CanSeek(void * hParser)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	return TRUE;
}

long long ffParser_GetPos(void * hParser)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	return QC_ERR_NONE;
}

long long ffParser_SetPos(void * hParser, long long llPos)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	int nRC = ffFormat_SetPos(pContext, llPos);
	return QC_ERR_NONE;
}

int ffParser_GetParam(void * hParser, int nID, void * pParam)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	return QC_ERR_NONE;
}

int ffParser_SetParam(void * hParser, int nID, void * pParam)
{
	QCParser_Context *	pContext = (QCParser_Context *)hParser;
	return QC_ERR_NONE;
}

int ffCreateParser(QC_Parser_Func * pParser, QCParserFormat nFormat)
{
	if (pParser == NULL)
		return QC_ERR_ARG;
	pParser->nVer = 1;
	pParser->Open = ffParser_Open;
	pParser->Close = ffParser_Close;
	pParser->GetStreamCount = ffParser_GetStreamCount;
	pParser->GetStreamPlay = ffParser_GetStreamPlay;
	pParser->SetStreamPlay = ffParser_SetStreamPlay;
	pParser->GetDuration = ffParser_GetDuration;
	pParser->GetStreamFormat = ffParser_GetStreamFormat;
	pParser->GetAudioFormat = ffParser_GetAudioFormat;
	pParser->GetVideoFormat = ffParser_GetVideoFormat;
	pParser->GetSubttFormat = ffParser_GetSubttFormat;
	pParser->IsEOS = ffParser_IsEOS;
	pParser->IsLive = ffParser_IsLive;
	pParser->EnableSubtt = ffParser_EnableSubtt;
	pParser->Read = ffParser_Read;
	pParser->Process = ffParser_Process;
	pParser->Run = ffParser_Run;
	pParser->Pause = ffParser_Pause;
	pParser->Stop = ffParser_Stop;
	pParser->CanSeek = ffParser_CanSeek;
	pParser->GetPos = ffParser_GetPos;
	pParser->SetPos = ffParser_SetPos;
	pParser->GetParam = ffParser_GetParam;
	pParser->SetParam = ffParser_SetParam;

	QCParser_Context * pNewParser = (QCParser_Context*)malloc(sizeof(QCParser_Context));
	if (pNewParser == NULL)
		return QC_ERR_FAILED;
	memset(pNewParser, 0, sizeof(QCParser_Context));

	ffFormat_Init(pNewParser);
	QCParserInfo * qcParserInfo = new QCParserInfo ();
	memset(qcParserInfo, 0, sizeof(QCParserInfo));
	qcParserInfo->m_nStrmSourceCount = 1;
	qcParserInfo->m_nStrmVideoPlay = -1;
	qcParserInfo->m_nStrmAudioPlay = -1;
	qcParserInfo->m_nStrmSubttPlay = -1;
	qcParserInfo->m_nNALLengthSize = 4;
	pNewParser->m_pQCParser = qcParserInfo;

	QCIOFuncInfo * qcIOInfo = new QCIOFuncInfo();
	memset(qcIOInfo, 0, sizeof(QCIOFuncInfo));
	qcIOInfo->m_nBuffSize = 32768;
	pNewParser->m_pQCIOInfo = qcIOInfo;

	pParser->hParser = (void *)pNewParser;
	qclog_init ();

	return QC_ERR_NONE;
}

int	ffDestroyParser(QC_Parser_Func * pParser)
{
	qclog_uninit();

	if (pParser == NULL || pParser->hParser == NULL)
		return QC_ERR_ARG;

	QCParser_Context *	pContext = (QCParser_Context *)pParser->hParser;
	ffParser_Close(pContext);
	ffFormat_Uninit(pContext);
	free(pContext);

	pParser->hParser = NULL;

	return QC_ERR_NONE;
}

int	ffIO_Read(void *opaque, uint8_t *buf, int buf_size)
{
	QCIOFuncInfo *	pIOInfo = (QCIOFuncInfo *)opaque;
	QC_IO_Func *	pIOFunc = (QC_IO_Func *)pIOInfo->m_pQCIO;
	if (pIOFunc == NULL || pIOFunc->hIO == NULL)
		return -1;
	int nRead = buf_size;
	int nRC = pIOFunc->Read(pIOFunc->hIO, buf, nRead, true, 0);
	if (nRC != QC_ERR_NONE)
		return 0;
	return nRead;
}

int	ffIO_Write(void *opaque, uint8_t *buf, int buf_size)
{
	QCIOFuncInfo *	pIOInfo = (QCIOFuncInfo *)opaque;
	QC_IO_Func *	pIOFunc = (QC_IO_Func *)pIOInfo->m_pQCIO;
	if (pIOFunc == NULL || pIOFunc->hIO == NULL)
		return -1;
	int nRC = pIOFunc->Write(pIOFunc->hIO, buf, buf_size, 0);
	return nRC;
}

int64_t	ffIO_Seek(void *opaque, int64_t offset, int whence)
{
	QCIOFuncInfo *	pIOInfo = (QCIOFuncInfo *)opaque;
	QC_IO_Func *	pIOFunc = (QC_IO_Func *)pIOInfo->m_pQCIO;
	if (pIOFunc == NULL || pIOFunc->hIO == NULL)
		return -1;

	if (whence == AVSEEK_SIZE)
		return pIOFunc->GetSize(pIOFunc->hIO);

	int nSeekFlag = QCIO_SEEK_BEGIN;
	if (whence == SEEK_SET)
		nSeekFlag = QCIO_SEEK_BEGIN;
	else if (whence == SEEK_CUR)
		nSeekFlag = QCIO_SEEK_CUR;
	else if (whence == SEEK_END)
		nSeekFlag = QCIO_SEEK_END;
	long long llPos = pIOFunc->SetPos(pIOFunc->hIO, offset, nSeekFlag);
	if (llPos >= 0)
		return 0;
	else
		return -1;
}
