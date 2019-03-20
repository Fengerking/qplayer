/*******************************************************************************
File:		CTSParser.h

Contains:	CTSParser header file.

Written by:	Qichao Shen

Change History (most recent first):
2016-12-22		Qichao			Create file

*******************************************************************************/
#ifndef __QP_CTSParser_H__
#define __QP_CTSParser_H__

#include "../CBaseParser.h"
#include "tsparser.h"
#include "CFrameSpliter.h"
#include "CCheckTimestampCache.h"
#include "AdaptiveStreamParser.h"
#include "stdio.h"
#include "qcData.h"


#define MAX_TRACK_COUNT_IN_TS   8

typedef struct
{
	CFrameSpliter*         pFrameSpliter;
	CheckTimestampCache*   pTimeStampCheckCache;
	unsigned char*  pHeaderData;
	unsigned int    ulHeaderDataSize;
	uint32          ulPID;
	QCMediaType     eMediaType;
	void*           pFmtTrack;
	int             iCodecId;
	int             iGetHeaderInfo;
	int             iCommitEnabled;
}S_TS_Track_Info;

class CTSParser : public CBaseParser
{
public:
	CTSParser(CBaseInst * pBaseInst);
	virtual ~CTSParser(void);

	virtual int			Open(QC_IO_Func * pIO, const char * pURL, int nFlag);
	virtual int 		Close(void);

	virtual int			Read(QC_DATA_BUFF * pBuff);

	virtual long long 	GetPos(void);
	virtual long long 	SetPos(long long llPos);

	virtual int 		GetParam(int nID, void * pParam);
	virtual int 		SetParam(int nID, void * pParam);
	virtual int			Process(unsigned char * pBuff, int nSize);

	void				FrameParsedTrans(void*  pMediaSample);
	int                 GetForamtInfo(QC_AUDIO_FORMAT*  pAudioFmt, QC_VIDEO_FORMAT*  pVideoFmt, QC_SUBTT_FORMAT*  pSubFmt);

	virtual void		SetSendBuffFunc(void * pSendBuff) { m_pSendBuff = pSendBuff; }

protected:
	static void   QC_API   ParsePorc(void*  pDataCallback);
	void                Init();
	void                InitWithoutMem();
	void                UnInit();
	void                PreProcessTimestamp(S_TS_Track_Info*  pTrackInfo,void* pSampleFromTs, bool& bCanCommit);
	int                 FindTrackIndexByPID(uint32 ulPIDValue);
	int                 GetAvailableTrackIndex();
	S_TS_Track_Info*    CreateTSTrackInfo(uint32 ulPID, int iCodecId, int iMediaType);
	int                 ParseMediaHeader(S_TS_Track_Info*  pTsTrackInfo, uint8*  pData, int iDataSize, int iCodecId);
	int                 SplitMediaFrame(S_TS_Track_Info*  pTsTrackInfo, void* pInputMediaFrame, int iArrayMaxSize, S_Ts_Media_Sample*  pMediaFrameArray, int& iSplitCount);
	int                 FlushData();
	int                 GetFrameDuration(S_Ts_Media_Sample*  pTsMediaSample);

private:
	void                SetCommitFlag(S_TS_Track_Info*  pTsTrackInfo);
	void                AdjustAuidoCommitFlag();
	int                 CommitMediaFrameToBuffer(int iCommitFlag, void* pMediaFrame);
	int                 ParseH264Header(S_TS_Track_Info*  pTsTrackInfo, uint8*  pData, int iDataSize);
	int                 ParseHEVCHeader(S_TS_Track_Info*  pTsTrackInfo, uint8*  pData, int iDataSize);
	int                 ParseAACHeader(S_TS_Track_Info*  pTsTrackInfo, uint8*  pData, int iDataSize);
	int                 ParseMp3Header(S_TS_Track_Info*  pTsTrackInfo, uint8*  pData, int iDataSize);
	int                 ParseG711Header(S_TS_Track_Info*  pTsTrackInfo, uint8*  pData, int iDataSize);

	int                 CommitMediaHeader(unsigned char*  pData, int iDataSize, void*  pTrackInfo, uint16 usMediaType);
	void                DoVideoHeaderNotify();

	void                DumpMedia(void* pMediaFrame);
	void                InitDump();
	void                UnInitDump();
private:
	unsigned int		m_nMaxAudioSize;
	unsigned int		m_nMaxVideoSize;
	S_Ts_Parser_Context	m_sTsParserContext;
	S_TS_Track_Info*	m_pTsTrackInfo[MAX_TRACK_COUNT_IN_TS];
	int					m_iCurTrackCount;
	int					m_iCurAudioTrackCount;
	bool				m_bInitedFlag;
	unsigned int		m_ulHeaderDataFlag;
	unsigned int		m_ulOffsetInM3u8;
	int					m_iCurParsedCount;
	int					m_iBAMode;
	QC_RESOURCE_INFO	m_sResInfo;
	int                 m_iAudioCurPID;

	void *				m_pSendBuff;

	unsigned char *		m_pReadBuff;
	int					m_nReadSize;
	bool				m_bTSFile;

#ifdef _DUMP_FRAME_
	FILE*              m_aFMediaData[MAX_TRACK_COUNT_IN_TS];
	FILE*              m_aFMediaInfo[MAX_TRACK_COUNT_IN_TS];
	int                m_aPIDArray[MAX_TRACK_COUNT_IN_TS];
	int                m_iPIDCount;
#endif
};

#endif // __CBaseParser_H__