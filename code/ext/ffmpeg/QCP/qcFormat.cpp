/*******************************************************************************
	File:		qcCodec.cpp

	Contains:	Create the Codec with codec id

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-24		Bangfei			Create file

*******************************************************************************/
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

#include "CFFMpegParser.h"

int ffParser_Open(void * hParser, QC_IO_Func * pIO, const char * pURL, int nFlag)
{
	return ((CBaseFFParser *)hParser)->Open(pIO, pURL, nFlag);
}

int ffParser_Close(void * hParser)
{
	return ((CBaseFFParser *)hParser)->Close();
}

int	ffParser_GetStreamCount(void * hParser, QCMediaType nType)
{
	return ((CBaseFFParser *)hParser)->GetStreamCount(nType);
}

int	ffParser_GetStreamPlay(void * hParser, QCMediaType nType)
{
	return ((CBaseFFParser *)hParser)->GetStreamPlay(nType);
}

int	ffParser_SetStreamPlay(void * hParser, QCMediaType nType, int nStream)
{
	return ((CBaseFFParser *)hParser)->SetStreamPlay(nType, nStream);
}

long long ffParser_GetDuration(void * hParser)
{
	return ((CBaseFFParser *)hParser)->GetDuration();
}

int	ffParser_GetStreamFormat(void * hParser, int nID, QC_STREAM_FORMAT ** ppStreamFmt)
{
	return ((CBaseFFParser *)hParser)->GetStreamFormat(nID, ppStreamFmt);
}

int	ffParser_GetAudioFormat(void * hParser, int nID, QC_AUDIO_FORMAT ** ppAudioFmt)
{
	return ((CBaseFFParser *)hParser)->GetAudioFormat(nID, ppAudioFmt);
}

int	ffParser_GetVideoFormat(void * hParser, int nID, QC_VIDEO_FORMAT ** ppVideoFmt)
{
	return ((CBaseFFParser *)hParser)->GetVideoFormat(nID, ppVideoFmt);
}

int	ffParser_GetSubttFormat(void * hParser, int nID, QC_SUBTT_FORMAT ** ppSubttFmt)
{
	return ((CBaseFFParser *)hParser)->GetSubttFormat(nID, ppSubttFmt);
}

bool ffParser_IsEOS(void * hParser)
{
	return ((CBaseFFParser *)hParser)->IsEOS();
}

bool ffParser_IsLive(void * hParser)
{
	return ((CBaseFFParser *)hParser)->IsLive();
}

int	ffParser_EnableSubtt(void * hParser, bool bEnable)
{
	return ((CBaseFFParser *)hParser)->EnableSubtt(bEnable);
}

int ffParser_Run(void * hParser)
{
	return ((CBaseFFParser *)hParser)->Run();
}

int ffParser_Pause(void * hParser)
{
	return ((CBaseFFParser *)hParser)->Pause();
}

int ffParser_Stop(void * hParser)
{
	return ((CBaseFFParser *)hParser)->Stop();
}

int ffParser_Read(void * hParser, QC_DATA_BUFF * pBuff)
{
	return ((CBaseFFParser *)hParser)->Read(pBuff);
}

int ffParser_Process(void * hParser, unsigned char * pBuff, int nSize)
{
	return ((CBaseFFParser *)hParser)->Process(pBuff, nSize);
}

int ffParser_CanSeek(void * hParser)
{
	return ((CBaseFFParser *)hParser)->CanSeek();
}

long long ffParser_GetPos(void * hParser)
{
	return ((CBaseFFParser *)hParser)->GetPos();
}

long long ffParser_SetPos(void * hParser, long long llPos)
{
	return ((CBaseFFParser *)hParser)->SetPos(llPos);
}

int ffParser_GetParam(void * hParser, int nID, void * pParam)
{
	return ((CBaseFFParser *)hParser)->GetParam(nID, pParam);
}

int ffParser_SetParam(void * hParser, int nID, void * pParam)
{
	return ((CBaseFFParser *)hParser)->SetParam(nID, pParam);
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

	CBaseFFParser * pNewParser = NULL;
	pNewParser = new CFFMpegParser (nFormat);
	if (pNewParser == NULL)
		return QC_ERR_FAILED;

	pParser->hParser = (void *)pNewParser;
	
	qclog_init ();

	return QC_ERR_NONE;
}

int	ffDestroyParser(QC_Parser_Func * pParser)
{
	qclog_uninit();

	if (pParser == NULL || pParser->hParser == NULL)
		return QC_ERR_ARG;

	CBaseFFParser * parser = (CBaseFFParser *)pParser->hParser;
	delete parser;
	pParser->hParser = NULL;

	return QC_ERR_NONE;
}
