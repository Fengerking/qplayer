/*******************************************************************************
	File:		CFFMpegIO.h

	Contains:	local file io header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-08		Bangfei			Create file

*******************************************************************************/
#ifndef __CFFMpegIO_H__
#define __CFFMpegIO_H__

#include "CFFBaseIO.h"
#include <libavformat/avio.h>

class CFFMpegIO : public CFFBaseIO
{
public:
    CFFMpegIO(void);
    virtual ~CFFMpegIO(void);

	virtual int 		Open (const char * pURL, long long llOffset, int nFlag);
	virtual int 		Reconnect (const char * pNewURL, long long llOffset);
	virtual int 		Close (void);

	virtual int			Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual int			ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual int			Write (unsigned char * pBuff, int nSize);
	virtual long long 	SetPos (long long llPos, int nFlag);

	virtual QCIOType	GetType (void);

	virtual int 		GetParam (int nID, void * pParam);
	virtual int 		SetParam (int nID, void * pParam);

protected:
	AVIOContext *		m_pFFIO;

};

#endif // __CFFMpegIO_H__