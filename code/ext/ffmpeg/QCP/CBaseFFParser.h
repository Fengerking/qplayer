/*******************************************************************************
	File:		CBaseFFParser.h

	Contains:	the base parser header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#ifndef __CBaseFFParser_H__
#define __CBaseFFParser_H__
#include "qcIO.h"
#include "qcData.h"
#include "qcParser.h"

class CBaseFFParser
{
public:
	CBaseFFParser(QCParserFormat nFormat);
    virtual ~CBaseFFParser(void);

	virtual int		Open (QC_IO_Func * pIO, const char * pURL, int nFlag);
	virtual int 	Close (void);

	virtual int		GetStreamCount (QCMediaType nType);
	virtual int		GetStreamPlay (QCMediaType nType);
	virtual int		SetStreamPlay (QCMediaType nType, int nStream);

	virtual long long	GetDuration (void);

	virtual int		GetStreamFormat (int nID, QC_STREAM_FORMAT ** ppAudioFmt);
	virtual int		GetAudioFormat (int nID, QC_AUDIO_FORMAT ** ppAudioFmt);
	virtual int		GetVideoFormat (int nID, QC_VIDEO_FORMAT ** ppVidEOSmt);
	virtual int		GetSubttFormat (int nID, QC_SUBTT_FORMAT ** ppSubttFmt);

	virtual bool	IsEOS (void); 
	virtual bool	IsLive (void); 

	virtual int		EnableSubtt (bool bEnable);

	virtual int 	Run (void);
	virtual int 	Pause (void);
	virtual int 	Stop (void);

	virtual int		Read (QC_DATA_BUFF * pBuff);
	virtual int		Process (unsigned char * pBuff, int nSize);

	virtual int		 	CanSeek (void);
	virtual long long 	GetPos (void);
	virtual long long 	SetPos (long long llPos);

	virtual int 	GetParam (int nID, void * pParam);
	virtual int 	SetParam (int nID, void * pParam);

//	virtual int		ConvertAVCHead(QCAVCDecoderSpecificInfo* AVCDecoderSpecificInfo, unsigned char * pInBuffer, int nInSzie);
	virtual int		ConvertHEVCHead(unsigned char * pOutBuffer, unsigned int& nOutSize, unsigned char * pInBuffer, int nInSzie);
	virtual int		ConvertAVCFrame(unsigned char * pBuffer, int nSize, unsigned int& nFrameLen, int& IsKeyFrame);	

	virtual int		DeleteFormat (QCMediaType nType);


public:
	QCParserFormat		m_nFormat;
	QC_STREAM_FORMAT *	m_pFmtStream;
	QC_AUDIO_FORMAT *	m_pFmtAudio;
	QC_VIDEO_FORMAT *	m_pFmtVideo;
	QC_SUBTT_FORMAT *	m_pFmtSubtt;
	bool				m_bEOS;
	bool				m_bLive;
	QCIOType			m_nIOType;

	int					m_nStrmSourceCount;
	int					m_nStrmVideoCount;
	int					m_nStrmAudioCount;
	int					m_nStrmSubttCount;

	int					m_nStrmSourcePlay;
	int					m_nStrmVideoPlay;
	int					m_nStrmAudioPlay;
	int					m_nStrmSubttPlay;

	long long			m_llDuration;
	long long			m_llSeekPos;

	bool				m_bEnableSubtt;

	int					m_nNALLengthSize;
	unsigned char *		m_pAVCBuffer;
	int					m_nAVCSize;
};

#endif // __CBaseFFParser_H__