/*******************************************************************************
	File:		CQCLibIO.h

	Contains:	ffmpeg io header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-08		Bangfei			Create file

*******************************************************************************/
#ifndef __CQCLibIO_H__
#define __CQCLibIO_H__

#include "CBaseIO.h"
#include "ULibFunc.h"

class CQCLibIO : public CBaseIO
{
public:
	CQCLibIO(CBaseInst * pBaseInst);
    virtual ~CQCLibIO(void);

	virtual int 		Open (const char * pURL, long long llOffset, int nFlag);
	virtual int 		Reconnect (const char * pNewURL, long long llOffset);
	virtual int 		Close (void);

	virtual int			Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual int			ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual int			Write (unsigned char * pBuff, int nSize, long long llPos = -1);
	virtual long long 	SetPos (long long llPos, int nFlag);

	virtual QCIOType	GetType (void);

	virtual int 		GetParam (int nID, void * pParam);
	virtual int 		SetParam (int nID, void * pParam);

protected:
	qcLibHandle			m_hLib;
	QCCREATEIO			m_fCreate;
	QCDESTROYIO			m_fDestroy;

	QC_IO_Func			m_fLibIO;
};

#endif // __CQCLibIO_H__