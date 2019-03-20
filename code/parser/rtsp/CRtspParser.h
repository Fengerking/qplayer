/*******************************************************************************
File:		CRtspParser.h

Contains:	CRtspParser header file.

Written by:	Qichao Shen

Change History (most recent first):
2018-05-03		Qichao			Create file

*******************************************************************************/
#ifndef __QP_CRtspParser_H__
#define __QP_CRtspParser_H__

#include "../CBaseParser.h"
#include "stdio.h"
#include "qcData.h"
#include "CRtspParserWrapper.h"
#include "CCheckTimestampCache.h"

typedef struct
{
	QCMediaType   eMediaType;
	int   ulMediaCodecId;
	long long   ullTimeStamp;
	unsigned char*  pSampleBuffer;
	int            iSampleBufferSize;
	unsigned int   ulSampleFlag;
} S_Rtsp_Media_Sample;



class CRtspParser : public CBaseParser
{
public:
	CRtspParser(CBaseInst * pBaseInst);
	virtual ~CRtspParser(void);

	virtual int			Open(QC_IO_Func * pIO, const char * pURL, int nFlag);
	virtual int 		Close(void);

	virtual int			Read(QC_DATA_BUFF * pBuff);

	virtual int		 	CanSeek(void);
	virtual long long 	GetPos(void);
	virtual long long 	SetPos(long long llPos);

	virtual int 		GetParam(int nID, void * pParam);
	virtual int 		SetParam(int nID, void * pParam);

	int                 SendRtspBuff(S_Rtsp_Media_Sample * pRtspBuff);
	int                 SendRtspBuffInner(S_Ts_Media_Sample * pSample);
	int				    ExtractSeiPrivate(unsigned char*  pFrameData, int iFrameSize, char*  pOutput);
	void                SetRtspState(int iState);

protected:
	void                Init();
	void                UnInit();
	int                 FlushData();
	void                CreateDefaultFmtInfo();
	void                PreProcessTimestamp(CheckTimestampCache*  pCheckTimeStampCache, S_Rtsp_Media_Sample * pRtspBuff, bool& bCanCommit);
private:
	void                DoVideoHeaderNotify();

	void                DumpMedia(void* pMediaFrame);
	void                InitDump();
	void                UnInitDump();
private:

	CheckTimestampCache*   m_pVideoTimeStampCheckCache;
	CheckTimestampCache*   m_pAudioTimeStampCheckCache;

	CRtspParserWrapper*               m_pRtspWrapper;
	unsigned int		m_nMaxAudioSize;
	unsigned int		m_nMaxVideoSize;
	long long           m_illFirstTimeStamp;
	int                 m_iState;
};

#endif // __CBaseParser_H__