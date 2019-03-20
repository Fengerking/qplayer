/*******************************************************************************
	File:		CBaseParser.h

	Contains:	the base parser header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#ifndef __CBaseParser_H__
#define __CBaseParser_H__
#include "qcIO.h"
#include "qcData.h"
#include "qcParser.h"
#include "CBaseObject.h"
#include "CBuffMng.h"
#include "CThreadWork.h"

typedef struct
{
	int				iSampleRate;
	int				iChannels;
	int				iSampleBit;
}QCM4AWaveFormat;

typedef struct
{
	unsigned char*	iData;
	unsigned int	iSize;
}QCMP4DecoderSpecificInfo;

typedef struct 
{
	unsigned char *	iData;
	unsigned int	iSize;
	unsigned char *	iConfigData;
	unsigned int	iConfigSize;
	unsigned char *	iSpsData;
	unsigned int	iSpsSize;
	unsigned char *	iPpsData;
	unsigned int	iPpsSize;
}QCAVCDecoderSpecificInfo;

class CBaseParser : public CBaseObject
{
public:
	CBaseParser(CBaseInst * pBaseInst);
    virtual ~CBaseParser(void);

	virtual void	SetBuffMng (CBuffMng * pBuffMng) {m_pBuffMng = pBuffMng;}
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

	virtual int		ConvertAVCHead(QCAVCDecoderSpecificInfo* AVCDecoderSpecificInfo, unsigned char * pInBuffer, int nInSzie);
	virtual int		ConvertHEVCHead(unsigned char * pOutBuffer, unsigned int& nOutSize, unsigned char * pInBuffer, int nInSzie);
	virtual int		ConvertAVCFrame(unsigned char * pBuffer, int nSize, unsigned int& nFrameLen, int& IsKeyFrame);	

protected:
	virtual int		DeleteFormat (QCMediaType nType);
    
    void			OnOpenDone(const char * pURL);
    void			ReleaseResInfo();



protected:
	QC_IO_Func *		m_fIO;
	CBuffMng *			m_pBuffMng;
	QC_STREAM_FORMAT *	m_pFmtStream;
	QC_AUDIO_FORMAT *	m_pFmtAudio;
	QC_VIDEO_FORMAT *	m_pFmtVideo;
	QC_SUBTT_FORMAT *	m_pFmtSubtt;
	long long			m_llFileSize;
	bool				m_bEOS;
	bool				m_bLive;

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

	long long	*		m_pIndexList;
	int					m_nIndexSize;
	int					m_nIndexNum;

	bool				m_bEnableSubtt;


	int					m_nNALLengthSize;
	unsigned char *		m_pAVCBuffer;
	int					m_nAVCSize;

	QCWORK_STATUS		m_sStatus;
	int					m_nExitRead;
	bool				m_bOpenCache;

	// for live input. Reconnect when stream stopped
	int					m_nLastReadTime;

    QC_RESOURCE_INFO 	m_sResInfo;

	long long			m_llSeekIOPos;
};

#endif // __CBaseParser_H__
