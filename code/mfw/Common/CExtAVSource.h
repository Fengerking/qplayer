/*******************************************************************************
	File:		CExtAVSource.h

	Contains:	The qc CExtAVSource source header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-16		Bangfei			Create file

*******************************************************************************/
#ifndef __CExtAVSource_H__
#define __CExtAVSource_H__

#include "CBaseSource.h"

class CExtAVSource : public CBaseSource
{
public:
	CExtAVSource(CBaseInst * pBaseInst, void * hInst);
	virtual ~CExtAVSource(void);

	virtual int			OpenIO(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource);
	virtual int			Close(void);
	virtual int			Start(void);

	virtual int 		SetParam (int nID, void * pParam);
	virtual int			GetParam (int nID, void * pParam);

protected:
	QC_AUDIO_FORMAT 	m_fmtAudio;
	QC_VIDEO_FORMAT 	m_fmtVideo;

};

#endif // __CExtAVSource_H__
