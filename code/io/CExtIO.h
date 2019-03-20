/*******************************************************************************
	File:		CExtIO.h

	Contains:	ffmpeg io header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-08		Bangfei			Create file

*******************************************************************************/
#ifndef __CExtIO_H__
#define __CExtIO_H__

#include "CBaseIO.h"
#include "CMemFile.h"
#include "ULibFunc.h"

class CExtIO : public CBaseIO
{
public:
	CExtIO(CBaseInst * pBaseInst);
    virtual ~CExtIO(void);

	virtual int 		Open (const char * pURL, long long llOffset, int nFlag);
	virtual int 		Close (void);

	virtual int			Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual int			ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual long long 	SetPos (long long llPos, int nFlag);
	virtual int			Write(unsigned char * pBuff, int & nSize, long long llPos = -1);

	virtual QCIOType	GetType (void);

	virtual int 		GetParam (int nID, void * pParam);
	virtual int 		SetParam (int nID, void * pParam);

protected:
	CMutexLock			m_mtLock;
	CMemFile *			m_pMemData;

};

#endif // __CExtIO_H__