/*******************************************************************************
	File:		CBaseVideoDec.h

	Contains:	The base video dec header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __CBaseVideoDec_H__
#define __CBaseVideoDec_H__

#include "CBaseObject.h"
#include "CMutexLock.h"

#include "CFileIO.h"

#include "qcData.h"

class CBaseVideoDec : public CBaseObject
{
public:
	CBaseVideoDec(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBaseVideoDec(void);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		SetBuff (QC_DATA_BUFF * pBuff);
	virtual int		GetBuff (QC_DATA_BUFF ** ppBuff);

	virtual int		Start (void);
	virtual int		Pause (void);
	virtual int		Stop (void);
	virtual int		Flush (void);
	virtual int		PushRestOut (void);

	virtual QC_VIDEO_FORMAT *	GetVideoFormat (void) {return &m_fmtVideo;}

protected:
	void *				m_hInst;
	QC_VIDEO_FORMAT		m_fmtVideo;

	CMutexLock			m_mtBuffer;
	unsigned int		m_uBuffFlag;
	QC_DATA_BUFF *		m_pBuffData;
	QC_VIDEO_BUFF		m_buffVideo;
	int					m_nDecCount;

	// For test dump
	CFileIO *				m_pDumpFile;

};

#endif // __CBaseVideoDec_H__
