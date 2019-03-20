/*******************************************************************************
	File:		qcIO.cpp

	Contains:	Create IO..

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcIO.h"

#include "CBaseIO.h"

#include "CFileIO.h"
#include "./http2/CHTTPIO2.h"
#include "./http2/CPDFileIO.h"
#include "CRTMPIO.h"
#include "CQCLibIO.h"
#include "CExtIO.h"

int qcIO_Open (void * hIO, const char * pURL, long long llOffset, int nFlag)
{
	return ((CBaseIO *)hIO)->Open (pURL, llOffset, nFlag);
}

int qcIO_Reconnect (void * hIO, const char * pNewURL, long long llOffset)
{
	return ((CBaseIO *)hIO)->Reconnect (pNewURL, llOffset);
}

int qcIO_Close (void * hIO)
{
	return ((CBaseIO *)hIO)->Close ();
}

int qcIO_Run (void * hIO)
{
	return ((CBaseIO *)hIO)->Run ();
}

int qcIO_Pause (void * hIO)
{
	return ((CBaseIO *)hIO)->Pause ();
}

int qcIO_Stop (void * hIO)
{
	return ((CBaseIO *)hIO)->Stop ();
}

long long qcIO_GetSize (void * hIO)
{
	return ((CBaseIO *)hIO)->GetSize ();
}

int qcIO_Read (void * hIO, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	return ((CBaseIO *)hIO)->Read (pBuff, nSize, bFull, nFlag);
}

int	qcIO_ReadAt (void * hIO, long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	return ((CBaseIO *)hIO)->ReadAt (llPos, pBuff, nSize, bFull, nFlag);
}

int	qcIO_ReadSync (void * hIO, long long llPos, unsigned char * pBuff, int nSize, int nFlag)
{
	return ((CBaseIO *)hIO)->ReadSync (llPos, pBuff, nSize, nFlag);
}

int	qcIO_Write (void * hIO, unsigned char * pBuff, int & nSize, long long llPos)
{
	return ((CBaseIO *)hIO)->Write (pBuff, nSize, llPos);
}

long long qcIO_SetPos (void * hIO, long long llPos, int nFlag)
{
	return ((CBaseIO *)hIO)->SetPos (llPos, nFlag);
}

long long qcIO_GetReadPos (void * hIO)
{
	return ((CBaseIO *)hIO)->GetReadPos ();
}

long long qcIO_GetDownPos (void * hIO)
{
	return ((CBaseIO *)hIO)->GetDownPos ();
}

int qcIO_GetSpeed (void * hIO, int nLastSecs)
{
	return ((CBaseIO *)hIO)->GetSpeed (nLastSecs);
}

QCIOType qcIO_GetType (void * hIO)
{
	return ((CBaseIO *)hIO)->GetType ();
}

bool qcIO_IsStreaming (void * hIO)
{
	return ((CBaseIO *)hIO)->IsStreaming ();
}

int qcIO_GetParam (void * hIO, int nID, void * pParam)
{
	return ((CBaseIO *)hIO)->GetParam (nID, pParam);
}

int qcIO_SetParam (void * hIO, int nID, void * pParam)
{
	return ((CBaseIO *)hIO)->SetParam (nID, pParam);
}

int	qcCreateIO (QC_IO_Func * pIO, QCIOProtocol nProt)
{
	if (pIO == NULL)
		return QC_ERR_ARG;
	pIO->nVer = 1;
	pIO->hIO = NULL;
	pIO->Open = qcIO_Open;
	pIO->Reconnect = qcIO_Reconnect;
	pIO->Close = qcIO_Close;
	pIO->Run = qcIO_Run;
	pIO->Pause = qcIO_Pause;
	pIO->Stop = qcIO_Stop;
	pIO->GetSize = qcIO_GetSize;
	pIO->Read = qcIO_Read;
	pIO->ReadAt = qcIO_ReadAt;
	pIO->ReadSync = qcIO_ReadSync;
	pIO->Write = qcIO_Write;
	pIO->SetPos = qcIO_SetPos;
	pIO->GetReadPos = qcIO_GetReadPos;
	pIO->GetDownPos = qcIO_GetDownPos;
	pIO->GetSpeed = qcIO_GetSpeed;
	pIO->GetType = qcIO_GetType;
	pIO->IsStreaming = qcIO_IsStreaming;
	pIO->GetParam = qcIO_GetParam;
	pIO->SetParam = qcIO_SetParam;

	if (nProt == QC_IOPROTOCOL_FILE)
		pIO->hIO = new CFileIO((CBaseInst *)pIO->pBaseInst);
	else if (nProt == QC_IOPROTOCOL_HTTP)
		pIO->hIO = new CHTTPIO2((CBaseInst *)pIO->pBaseInst);
//		pIO->hIO = new CPDFileIO((CBaseInst *)pIO->pBaseInst);
	else if (nProt == QC_IOPROTOCOL_HTTPPD)
		pIO->hIO = new CPDFileIO((CBaseInst *)pIO->pBaseInst);
	else if (nProt == QC_IOPROTOCOL_RTMP)
		pIO->hIO = new CRTMPIO((CBaseInst *)pIO->pBaseInst);
	if (nProt == QC_IOPROTOCOL_EXTLIB)
	{
		pIO->hIO = new CQCLibIO((CBaseInst *)pIO->pBaseInst);
		((CQCLibIO *)pIO->hIO)->SetParam(QCIO_PID_EXT_LibName, pIO->m_szLibInfo);
	}
	else if (nProt == QC_IOPROTOCOL_EXTIO)
		pIO->hIO = new CExtIO((CBaseInst *)pIO->pBaseInst);
	if (pIO->hIO == NULL)
		return QC_ERR_FAILED;
	pIO->m_nProtocol = nProt;

	return QC_ERR_NONE;
}


int	qcDestroyIO (QC_IO_Func * pIO)
{
	if (pIO == NULL || pIO->hIO == NULL)
		return QC_ERR_ARG;

	CBaseIO * io = (CBaseIO *)pIO->hIO;
	delete io;
	pIO->hIO = NULL;

	return QC_ERR_NONE;
}
