/*******************************************************************************
	File:		qcFFIO.cpp

	Contains:	Create FFmpeg IO..

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-08		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcIO.h"

#include "CFFMpegIO.h"
#include "qcFFLog.h"

int qcFFIO_Open (void * hIO, const char * pURL, long long llOffset, int nFlag)
{
	return ((CFFBaseIO *)hIO)->Open (pURL, llOffset, nFlag);
}

int qcFFIO_Reconnect (void * hIO, const char * pNewURL, long long llOffset)
{
	return ((CFFBaseIO *)hIO)->Reconnect (pNewURL, llOffset);
}

int qcFFIO_Close (void * hIO)
{
	return ((CFFBaseIO *)hIO)->Close ();
}

int qcFFIO_Run (void * hIO)
{
	return ((CFFBaseIO *)hIO)->Run ();
}

int qcFFIO_Pause (void * hIO)
{
	return ((CFFBaseIO *)hIO)->Pause ();
}

int qcFFIO_Stop (void * hIO)
{
	return ((CFFBaseIO *)hIO)->Stop ();
}

long long qcFFIO_GetSize (void * hIO)
{
	return ((CFFBaseIO *)hIO)->GetSize ();
}

int qcFFIO_Read (void * hIO, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	return ((CFFBaseIO *)hIO)->Read (pBuff, nSize, bFull, nFlag);
}

int	qcFFIO_ReadAt (void * hIO, long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	return ((CFFBaseIO *)hIO)->ReadAt (llPos, pBuff, nSize, bFull, nFlag);
}

int	qcFFIO_ReadSync (void * hIO, long long llPos, unsigned char * pBuff, int nSize, int nFlag)
{
	return ((CFFBaseIO *)hIO)->ReadSync (llPos, pBuff, nSize, nFlag);
}

int	qcFFIO_Write (void * hIO, unsigned char * pBuff, int & nSize, long long llPos)
{
	return ((CFFBaseIO *)hIO)->Write (pBuff, nSize, llPos);
}

long long qcFFIO_SetPos (void * hIO, long long llPos, int nFlag)
{
	return ((CFFBaseIO *)hIO)->SetPos (llPos, nFlag);
}

long long qcFFIO_GetReadPos (void * hIO)
{
	return ((CFFBaseIO *)hIO)->GetReadPos ();
}

long long qcFFIO_GetDownPos (void * hIO)
{
	return ((CFFBaseIO *)hIO)->GetDownPos ();
}

int qcFFIO_GetSpeed (void * hIO, int nLastSecs)
{
	return ((CFFBaseIO *)hIO)->GetSpeed (nLastSecs);
}

QCIOType qcFFIO_GetType (void * hIO)
{
	return ((CFFBaseIO *)hIO)->GetType ();
}

bool qcFFIO_IsStreaming (void * hIO)
{
	return ((CFFBaseIO *)hIO)->IsStreaming ();
}

int qcFFIO_GetParam (void * hIO, int nID, void * pParam)
{
	return ((CFFBaseIO *)hIO)->GetParam (nID, pParam);
}

int qcFFIO_SetParam (void * hIO, int nID, void * pParam)
{
	return ((CFFBaseIO *)hIO)->SetParam (nID, pParam);
}

int	qcFFCreateIO (QC_IO_Func * pIO, QCIOProtocol nProt)
{
	if (pIO == NULL)
		return QC_ERR_ARG;
	pIO->nVer = 1;
	pIO->Open = qcFFIO_Open;
	pIO->Reconnect = qcFFIO_Reconnect;
	pIO->Close = qcFFIO_Close;
	pIO->Run = qcFFIO_Run;
	pIO->Pause = qcFFIO_Pause;
	pIO->Stop = qcFFIO_Stop;
	pIO->GetSize = qcFFIO_GetSize;
	pIO->Read = qcFFIO_Read;
	pIO->ReadAt = qcFFIO_ReadAt;
	pIO->ReadSync = qcFFIO_ReadSync;
	pIO->Write = qcFFIO_Write;
	pIO->SetPos = qcFFIO_SetPos;
	pIO->GetReadPos = qcFFIO_GetReadPos;
	pIO->GetDownPos = qcFFIO_GetDownPos;
	pIO->GetSpeed = qcFFIO_GetSpeed;
	pIO->GetType = qcFFIO_GetType;
	pIO->IsStreaming = qcFFIO_IsStreaming;
	pIO->GetParam = qcFFIO_GetParam;
	pIO->SetParam = qcFFIO_SetParam;

	pIO->hIO = new CFFMpegIO ();

	qclog_init();

	return QC_ERR_NONE;
}

int	qcFFDestroyIO (QC_IO_Func * pIO)
{
	qclog_uninit();

	if (pIO == NULL || pIO->hIO == NULL)
		return QC_ERR_ARG;

	CFFBaseIO * io = (CFFBaseIO *)pIO->hIO;
	delete io;
	pIO->hIO = NULL;

	return QC_ERR_NONE;
}
