/*******************************************************************************
	File:		qcParser.cpp

	Contains:	Create the parser with format

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcparser.h"

#include "CBaseParser.h"
#include "./flv/CFLVParser.h"
#include "./mp4/CMP4Parser.h"
#include "./ts/CTSParser.h"
#include "./m3u8/CAdaptiveStreamHLS.h"
#include "./rtsp/CRtspParser.h"


int qcParser_Open (void * hParser, QC_IO_Func * pIO, const char * pURL, int nFlag)
{
	return ((CBaseParser *)hParser)->Open (pIO, pURL, nFlag);
}

int qcParser_Close (void * hParser)
{
	return ((CBaseParser *)hParser)->Close ();
}

int	qcParser_GetStreamCount (void * hParser, QCMediaType nType)
{
	return ((CBaseParser *)hParser)->GetStreamCount (nType);
}

int	qcParser_GetStreamPlay (void * hParser, QCMediaType nType)
{
	return ((CBaseParser *)hParser)->GetStreamPlay (nType);
}

int	qcParser_SetStreamPlay (void * hParser, QCMediaType nType, int nStream)
{
	return ((CBaseParser *)hParser)->SetStreamPlay (nType, nStream);
}

long long qcParser_GetDuration (void * hParser)
{
	return ((CBaseParser *)hParser)->GetDuration ();
}

int	qcParser_GetStreamFormat (void * hParser, int nID, QC_STREAM_FORMAT ** ppStreamFmt)
{
	return ((CBaseParser *)hParser)->GetStreamFormat (nID, ppStreamFmt);
}

int	qcParser_GetAudioFormat (void * hParser, int nID, QC_AUDIO_FORMAT ** ppAudioFmt)
{
	return ((CBaseParser *)hParser)->GetAudioFormat (nID, ppAudioFmt);
}

int	qcParser_GetVideoFormat (void * hParser, int nID, QC_VIDEO_FORMAT ** ppVideoFmt)
{
	return ((CBaseParser *)hParser)->GetVideoFormat (nID, ppVideoFmt);
}

int	qcParser_GetSubttFormat (void * hParser, int nID, QC_SUBTT_FORMAT ** ppSubttFmt)
{
	return ((CBaseParser *)hParser)->GetSubttFormat (nID, ppSubttFmt);
}

bool qcParser_IsEOS (void * hParser)
{
	return ((CBaseParser *)hParser)->IsEOS ();
}

bool qcParser_IsLive (void * hParser)
{
	return ((CBaseParser *)hParser)->IsLive ();
}

int	qcParser_EnableSubtt (void * hParser,bool bEnable)
{
	return ((CBaseParser *)hParser)->EnableSubtt (bEnable);
}

int qcParser_Run (void * hParser)
{
	return ((CBaseParser *)hParser)->Run ();
}

int qcParser_Pause (void * hParser)
{
	return ((CBaseParser *)hParser)->Pause ();
}

int qcParser_Stop (void * hParser)
{
	return ((CBaseParser *)hParser)->Stop ();
}

int qcParser_Read (void * hParser, QC_DATA_BUFF * pBuff)
{
	return ((CBaseParser *)hParser)->Read (pBuff);
}

int qcParser_Process (void * hParser, unsigned char * pBuff, int nSize)
{
	return ((CBaseParser *)hParser)->Process (pBuff, nSize);
}

int qcParser_CanSeek (void * hParser)
{
	return ((CBaseParser *)hParser)->CanSeek ();
}

long long qcParser_GetPos (void * hParser)
{
	return ((CBaseParser *)hParser)->GetPos ();
}

long long qcParser_SetPos (void * hParser, long long llPos)
{
	return ((CBaseParser *)hParser)->SetPos (llPos);
}

int qcParser_GetParam (void * hParser, int nID, void * pParam)
{
	return ((CBaseParser *)hParser)->GetParam (nID, pParam);
}

int qcParser_SetParam (void * hParser, int nID, void * pParam)
{
	return ((CBaseParser *)hParser)->SetParam (nID, pParam);
}

int qcCreateParser (QC_Parser_Func * pParser, QCParserFormat nFormat)
{
	if (pParser == NULL)
		return QC_ERR_ARG;
	pParser->nVer = 1;
	pParser->Open = qcParser_Open;
	pParser->Close = qcParser_Close;
	pParser->GetStreamCount = qcParser_GetStreamCount;
	pParser->GetStreamPlay = qcParser_GetStreamPlay;
	pParser->SetStreamPlay = qcParser_SetStreamPlay;
	pParser->GetDuration = qcParser_GetDuration;
	pParser->GetStreamFormat = qcParser_GetStreamFormat;
	pParser->GetAudioFormat = qcParser_GetAudioFormat;
	pParser->GetVideoFormat = qcParser_GetVideoFormat;
	pParser->GetSubttFormat = qcParser_GetSubttFormat;
	pParser->IsEOS = qcParser_IsEOS;
	pParser->IsLive = qcParser_IsLive;
	pParser->EnableSubtt = qcParser_EnableSubtt;
	pParser->Read = qcParser_Read;
	pParser->Process = qcParser_Process;
	pParser->Run = qcParser_Run;
	pParser->Pause = qcParser_Pause;
	pParser->Stop = qcParser_Stop;
	pParser->CanSeek = qcParser_CanSeek;
	pParser->GetPos = qcParser_GetPos;
	pParser->SetPos = qcParser_SetPos;
	pParser->GetParam = qcParser_GetParam;
	pParser->SetParam = qcParser_SetParam;

	CBaseParser * pNewParser = NULL;
	if (nFormat == QC_PARSER_M3U8)
		pNewParser = new CAdaptiveStreamHLS((CBaseInst *)pParser->pBaseInst);
	if (nFormat == QC_PARSER_MP4)
		pNewParser = new CMP4Parser((CBaseInst *)pParser->pBaseInst);
	if (nFormat == QC_PARSER_TS)
		pNewParser = new CTSParser((CBaseInst *)pParser->pBaseInst);
	if (nFormat == QC_PARSER_FLV)
		pNewParser = new CFLVParser((CBaseInst *)pParser->pBaseInst);
#ifdef __QC_OS_WIN32__
	if (nFormat == QC_PARSER_RTSP)
		pNewParser = new CRtspParser((CBaseInst *)pParser->pBaseInst);
#endif // __QC_OS_WIN32__
	if (pNewParser == NULL)
		return QC_ERR_FAILED;

	pNewParser->SetBuffMng ((CBuffMng *)pParser->pBuffMng);
	pParser->hParser = (void *)pNewParser;

	return QC_ERR_NONE;
}

int	qcDestroyParser (QC_Parser_Func * pParser)
{
	if (pParser == NULL || pParser->hParser == NULL)
		return QC_ERR_ARG;

	CBaseParser * parser = (CBaseParser *)pParser->hParser;
	delete parser;
	pParser->hParser = NULL;

	return QC_ERR_NONE;
}
