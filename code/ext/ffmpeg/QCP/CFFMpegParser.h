/*******************************************************************************
	File:		CFFMpegParser.h

	Contains:	the ffmpeg parser header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-15		Bangfei			Create file

*******************************************************************************/
#ifndef __CFFMpegParser_H__
#define __CFFMpegParser_H__

#include "CBaseFFParser.h"
#include "CFFMpegInIO.h"

#include <libavformat/avformat.h>
#include <libavutil/dict.h>

class CFFMpegParser : public CBaseFFParser
{
public:
	CFFMpegParser(QCParserFormat nFormat);
    virtual ~CFFMpegParser(void);

	virtual int			Open (QC_IO_Func * pIO, const char * pURL, int nFlag);
	virtual int 		Close (void);

	virtual int			Read (QC_DATA_BUFF * pBuff);

	virtual int		 	CanSeek (void);
	virtual long long 	GetPos (void);
	virtual long long 	SetPos (long long llPos);

	virtual int 		GetParam (int nID, void * pParam);
	virtual int 		SetParam (int nID, void * pParam);

protected:
	long long			ffBaseToTime(long long llBase, AVStream * pStream);
	long long			ffTimeToBase(long long llTime, AVStream * pStream);

protected:
	AVFormatContext *			m_pFmtCtx;
	// Define audio stream parameters
	int							m_nIdxAudio;
	AVStream *					m_pStmAudio;
	// Define video stream parameters
	int							m_nIdxVideo;
	AVStream *					m_pStmVideo;
	// Define Subtt stream parameters
	int							m_nIdxSubtt;
	AVStream *					m_pStmSubtt;

	int							m_nPacketSize;

	CFFMpegInIO *				m_pFFIO;

	AVDictionary *				m_pFmtOpts;
	AVPacket *					m_pPackData;
};

#endif // __CBaseParser_H__
