/*******************************************************************************
	File:		CFFBaseIO.h

	Contains:	the base io header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-08		Bangfei			Create file

*******************************************************************************/
#ifndef __CFFBaseIO_H__
#define __CFFBaseIO_H__
#include "qcIO.h"

typedef enum {
    QCIO_Init		= 0,
	QCIO_Open		= 1,
    QCIO_Run		= 2,
    QCIO_Pause	    = 3,
    QCIO_Stop		= 4,
    QCIO_MAX		= 0X7FFFFFFF
} QCIO_STATUS;

class CFFBaseIO
{
public:
    CFFBaseIO(void);
    virtual ~CFFBaseIO(void);

	virtual int 		Open (const char * pURL, long long llOffset, int nFlag);
	virtual int 		Reconnect (const char * pNewURL, long long llOffset);
	virtual int 		Close (void);

	virtual int 		Run (void);
	virtual int 		Pause (void);
	virtual int 		Stop (void);

	virtual long long 	GetSize (void);
	virtual int			Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual int			ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag);
	virtual int			ReadSync (long long llPos, unsigned char * pBuff, int nSize, int nFlag);
	virtual int			Write (unsigned char * pBuff, int & nSize, long long llPos);
	virtual long long 	SetPos (long long llPos, int nFlag);
	virtual void		SetHostMetaData(char* pHostHead) { strcpy(m_szHostAddr, pHostHead); }

	virtual long long 	GetReadPos (void);
	virtual long long 	GetDownPos (void);
	virtual int			GetSpeed (int nLastSecs);
	virtual QCIOType	GetType (void);
	virtual bool		IsStreaming (void);

	virtual int 		GetParam (int nID, void * pParam);
	virtual int 		SetParam (int nID, void * pParam);

protected:
	QCIO_STATUS			m_sStatus;
	char				m_szURL[1024];
	long long			m_llFileSize;
	long long			m_llReadPos;
	long long			m_llDownPos;
	bool				m_bIsStreaming;

	char				m_szHostAddr[256];
};

#endif // __CFFBaseIO_H__
