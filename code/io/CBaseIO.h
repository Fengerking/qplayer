/*******************************************************************************
	File:		CBaseIO.h

	Contains:	the base io header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#ifndef __CBaseIO_H__
#define __CBaseIO_H__
#include "qcIO.h"
#include "CBaseObject.h"

typedef enum {
    QCIO_Init		= 0,
	QCIO_Open		= 1,
    QCIO_Run		= 2,
    QCIO_Pause	    = 3,
    QCIO_Stop		= 4,
    QCIO_MAX		= 0X7FFFFFFF
} QCIO_STATUS;

typedef struct {
	long long					m_llMoovPos;
	long long					m_llRecvPos;
	char *						m_pMoovData;
	int							m_nMoovSize;
} QCMP4_MOOV_BUFFER;

// Set. save the ext moov data
// The parameter should be QCMP4_MOOV_BUFFER *. 
#define	QCIO_PID_SAVE_MOOV_BUFFER			QC_MOD_IO_BASE + 0X302

class CBaseIO : public CBaseObject
{
public:
	CBaseIO(CBaseInst * pBaseInst);
    virtual ~CBaseIO(void);

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
	virtual int			Write (unsigned char * pBuff, int & nSize, long long llPos = -1);
	virtual long long 	SetPos (long long llPos, int nFlag);
	virtual void		SetHostMetaData(char* pHostHead) { strcpy(m_szHostAddr, pHostHead); }

	virtual long long 	GetReadPos (void);
	virtual long long 	GetDownPos (void);
	virtual int			GetSpeed (int nLastSecs);
	virtual QCIOType	GetType (void);
	virtual bool		IsStreaming (void);

	virtual int 		GetParam (int nID, void * pParam);
	virtual int 		SetParam (int nID, void * pParam);

	virtual int			CheckBitrate(int nReadSize);

protected:
	QCIO_STATUS			m_sStatus;
	char *				m_pURL;
	int					m_nFlag;
	long long			m_llFileSize;
	long long			m_llReadPos;
	long long			m_llDownPos;
	long long			m_llSeekPos;
	long long			m_llStopPos;
	int					m_nNeedSleep;
	bool				m_bIsStreaming;
	int					m_nExitRead;

	char				m_szHostAddr[1024];

	// for check bitrate
	int					m_nCheckBitrate;
	int					m_nOpenTime;
	long long			m_llReadSize;

	int					m_nNotifyDLPercent;

	long long			m_llHadReadSize;

};

#endif // __CBaseIO_H__
