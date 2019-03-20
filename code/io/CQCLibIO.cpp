/*******************************************************************************
	File:		CQCLibIO.cpp

	Contains:	ffmpeg io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-08		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CQCLibIO.h"

#include "ULogFunc.h"

CQCLibIO::CQCLibIO(CBaseInst * pBaseInst)
	: CBaseIO (pBaseInst)
	, m_hLib (NULL)
{
	SetObjectName ("CQCLibIO");
	memset (&m_fLibIO, 0, sizeof(m_fLibIO));
}

CQCLibIO::~CQCLibIO(void)
{
	Close ();
	if (m_fLibIO.hIO != NULL)
		m_fDestroy (&m_fLibIO);
	if (m_hLib != NULL)
		qcLibFree(m_hLib, 0);
}

int CQCLibIO::Open (const char * pURL, long long llOffset, int nFlag)
{
	if (m_fLibIO.hIO == NULL)
		return QC_ERR_STATUS;
	return m_fLibIO.Open(m_fLibIO.hIO, pURL, llOffset, nFlag);
}

int CQCLibIO::Reconnect (const char * pNewURL, long long llOffset)
{
	if (m_fLibIO.hIO == NULL)
		return QC_ERR_STATUS;
	return m_fLibIO.Reconnect(m_fLibIO.hIO, pNewURL, llOffset);
}

int CQCLibIO::Close (void)
{
	if (m_fLibIO.hIO == NULL)
		return QC_ERR_STATUS;
	return m_fLibIO.Close(m_fLibIO.hIO);
}

int CQCLibIO::Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	if (m_fLibIO.hIO == NULL)
		return QC_ERR_STATUS;
	return m_fLibIO.Read(m_fLibIO.hIO, pBuff, nSize, bFull, nFlag);
}

int	CQCLibIO::ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
	if (m_fLibIO.hIO == NULL)
		return QC_ERR_STATUS;
	return m_fLibIO.ReadAt(m_fLibIO.hIO, llPos, pBuff, nSize, bFull, nFlag);
}

int	CQCLibIO::Write (unsigned char * pBuff, int nSize, long long llPos)
{
	if (m_fLibIO.hIO == NULL)
		return QC_ERR_STATUS;
	return m_fLibIO.Write(m_fLibIO.hIO, pBuff, nSize, llPos);
}

long long CQCLibIO::SetPos (long long llPos, int nFlag)
{
	if (m_fLibIO.hIO == NULL)
		return QC_ERR_STATUS;
	return m_fLibIO.SetPos(m_fLibIO.hIO, llPos, nFlag);
}

QCIOType CQCLibIO::GetType (void)
{
	if (m_fLibIO.hIO == NULL)
		return QC_IOTYPE_NONE;
	return m_fLibIO.GetType(m_fLibIO.hIO);
}

int CQCLibIO::GetParam (int nID, void * pParam)
{
	if (m_fLibIO.hIO == NULL)
		return QC_IOTYPE_NONE;
	return m_fLibIO.GetParam(m_fLibIO.hIO, nID, pParam);
}

int CQCLibIO::SetParam (int nID, void * pParam)
{
	if (nID == QCIO_PID_EXT_LibName)
	{
		char * pLibInfo = (char *)pParam;
		char * pLibName = pLibInfo;
		char * pFunCreate = NULL;
		char * pFunDestroy = NULL;

		char * pPos = strstr(pLibInfo, ",");
		if (pPos == NULL)
			return QC_ERR_FAILED;
		*pPos = 0;
		pFunCreate = pPos + 1;
		pPos = strstr(pFunCreate, ",");
		if (pPos == NULL)
			return QC_ERR_FAILED;
		*pPos = 0;
		pFunDestroy = pPos + 1;
		if (pFunDestroy == NULL)
			return QC_ERR_FAILED;

		m_hLib = (qcLibHandle)qcLibLoad(pLibName, 0);
		if (m_hLib == NULL)
			return QC_ERR_FAILED;

		m_fCreate = (QCCREATEIO)qcLibGetAddr(m_hLib, pFunCreate, 0);
		if (m_fCreate == NULL)
			return QC_ERR_FAILED;

		m_fDestroy = (QCDESTROYIO)qcLibGetAddr(m_hLib, pFunDestroy, 0);
		if (m_fCreate == NULL)
			return QC_ERR_FAILED;

		int nRC = m_fCreate(&m_fLibIO, QC_IOPROTOCOL_NONE);
		if (nRC != QC_ERR_NONE)
			return nRC;

		return QC_ERR_NONE;
	}

	if (m_fLibIO.hIO == NULL)
		return QC_IOTYPE_NONE;
	return m_fLibIO.SetParam(m_fLibIO.hIO, nID, pParam);
}

