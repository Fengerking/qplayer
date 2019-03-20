/*******************************************************************************
	File:		CExtIOSource.h

	Contains:	The qc CExtIOSource source header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-16		Bangfei			Create file

*******************************************************************************/
#ifndef __CExtIOSource_H__
#define __CExtIOSource_H__

#include "CBaseSource.h"

class CExtIOSource : public CBaseSource
{
public:
	CExtIOSource(CBaseInst * pBaseInst, void * hInst);
	virtual ~CExtIOSource(void);

	virtual int			OpenIO(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource);
	virtual int			Close(void);
	virtual int			Start(void);

	virtual int 		SetParam (int nID, void * pParam);
	virtual int			GetParam (int nID, void * pParam);

protected:
	QC_AUDIO_FORMAT 	m_fmtAudio;
	QC_VIDEO_FORMAT 	m_fmtVideo;

	QC_Parser_Func		m_fParser;

};

#endif // __CExtIOSource_H__
