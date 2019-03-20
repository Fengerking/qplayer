/*******************************************************************************
File:		CAdaptiveStreamHLS.h

Contains:	CAdaptiveStreamHLS Header file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-04		Qichao			Create file

*******************************************************************************/

#ifndef __ADAPTIVE_STREAM_HLS_H__
#define __ADAPTIVE_STREAM_HLS_H__


#include "CHls_entity.h"
#include "AdaptiveStreamParser.h"
#include "HLSDRM.h"
#include "CMutexLock.h"
#include "CTSParser.h"
#include "CAdaptiveStreamBA.h"
#include "CNormalHLSDrm.h"
#include "qcData.h"
#include "qcIO.h"

#include "../CBaseParser.h"

//Max TsParser need: main, audio, video, subtitle
#define MAX_SEGEMNT_WORKING_COUNT 4

#define HLS_SEGEMNT_MAIN_STREAM_INDEX 0
#define HLS_SEGEMNT_ALTER_AUDIO_INDEX 1
#define HLS_SEGEMNT_ALTER_VIDEO_INDEX 2
#define HLS_SEGEMNT_ALTER_SUB_INDEX   3

#define IDLE_STATE_FOR_READ           0
#define WORKING_STATE_FOR_READ        1
#define DEFAULT_ROLL_BACK_LENGTH_BITRATE_DROP    5000
#define DEFAULT_M3U8_MAX_DATA_SIZE               (1024*1024*2)
#define FORATM_DESC_STRING            "m3u8"

typedef struct
{
	unsigned char*    pSegmentBuffer;
	int               iSegmentMaxSize;
} S_Segment_Buffer_Ctx;

typedef struct
{
	CTSParser*              pTsParser;
	QC_IO_Func*             pIOHandle;
	CNormalHLSDrm*          pDrmIns;
	S_Segment_Buffer_Ctx*   pBufferCtx;
	void*                   pDrmInfo;
	unsigned int            ulPlaylistId;
	unsigned int            ulChunkID;
	int                     iStartTransTime;
	unsigned long long      ullM3u8Offset;
	long long               illSegmentSize;
	long long               illHandleOffset;
	char                    strURL[MAX_URL_SIZE];
	unsigned int            ulReadStateFlag;
	unsigned int            ulForceStopFlag;
	bool                    bChunkMode;
	bool                    bGotFinished;
	bool                    bVodEnd;
	unsigned int            ulLastConnectFailTime;
} S_Segment_Handle_Ctx;

typedef struct
{
	int iMediaStreamCount;
	int iAudioTrackInfoDone;
	int iVideoTrackInfoDone;
	int iSubTrackInfoDone;
} S_Output_Ctrl_Ctx;

class CAdaptiveStreamHLS : public CBaseParser
{
public:
	CAdaptiveStreamHLS(CBaseInst * pBaseInst);
    virtual ~CAdaptiveStreamHLS(void);

	virtual int			Open (QC_IO_Func * pIO, const char * pURL, int nFlag);
	virtual int 		Close (void);

	virtual int			Read (QC_DATA_BUFF * pBuff);

	virtual int		 	CanSeek (void);
	virtual bool		IsLive(void);
	virtual long long 	GetPos(void);
	virtual long long 	SetPos (long long llPos);

	virtual int 		GetParam (int nID, void * pParam);
	virtual int 		SetParam (int nID, void * pParam);
	virtual int         SetStreamPlay(QCMediaType nType, int nStream);
	virtual int			GetStreamFormat (int nID, QC_STREAM_FORMAT ** ppAudioFmt);

	static int QC_API   OnEvent(void* pUserData, unsigned int ulID, void* ulParam1, void* ulParam2);
	int                 DownloadM3u8ForCallback(void*  nM3u8CallbackReq);

	virtual int			SendBuff(QC_DATA_BUFF * pBuff);

protected:
	void                InitAllDrmContext();
	void                InitAllParserContext();
	void                InitAllIOContext();
	void                InitAllM3u8SegmentBuffer();
	void                InitAllSegmentHandleCtx();

	void                InitContext();
	int                 CheckM3u8DataFromIO(char*  pURL, unsigned char**  ppBufferOutput, int* piBufferMaxSize, int* piDataSize, long long&  illDownloadBitrate, int nFlag);
	int                 HandleSegmentFromIO(S_Segment_Handle_Ctx*  pSegmentHandleCtx, long long&  illDownloadBitrate);
	void                GetAbsoluteURL(char*  pURL, char* pSrc, char* pRefer);
	void                PurgeURL(char*  pURL);
	void                StopAllRead();
	void                CheckM3u8Validation();
	int                 ProcessSegment();
	void                ReconnectAllIO();
	int                 ReallocBufferAsNeed(unsigned char*&  pCurBuffer, int& iCurMaxSize, int iNewSize, int iCurDataSize, int  iExtraAppendSize);
	void                DoM3u8UpdateIfNeed();
	void                AdjustFrameFlags(QC_DATA_BUFF * pBuff);

	int							GetIndexByChunkType(E_ADAPTIVESTREAMPARSER_CHUNKTYPE  eType, int&  iIndex);
	CTSParser*					GetTsParserByChunkType(E_ADAPTIVESTREAMPARSER_CHUNKTYPE  eType, bool bCreate);
	QC_IO_Func*					GetIOHandleByChunkType(E_ADAPTIVESTREAMPARSER_CHUNKTYPE  eType, bool bCreate);
	CNormalHLSDrm*				GetDrmHandleByChunkType(E_ADAPTIVESTREAMPARSER_CHUNKTYPE  eType, bool bCreate);
	S_Segment_Buffer_Ctx*		GetSegmentBufferByChunkType(E_ADAPTIVESTREAMPARSER_CHUNKTYPE  eType, bool bCreate);

	S_Segment_Handle_Ctx*		GetWorkingSegmentHandleCtx();
	S_Segment_Handle_Ctx*		GetIdleSegmentHandleCtx(E_ADAPTIVESTREAMPARSER_CHUNKTYPE  eType);
	void                        SelectPreferBitrateInOpen();

	void						CreateFmtInfoFromTs();
	void						CreateDefaultFmtInfo();
	void						ResetAllParser(bool bSeek);
	void						FillBAInfo(const char* pURL);
	void                        FillResInfo(QC_RESOURCE_INFO*  pResInfo, unsigned int ulStreamId);
	void						DumpSegment(int iPlaylistID, int iSequenceId, unsigned char*  pData, int iDataSize);

private:
	long long		          	m_llOffset;
	QCIOProtocol				m_ioProtocal;
	char						m_szHostAddr[256];

	C_HLS_Entity*	          	m_pHLSParser;
	CTSParser*		          	m_pTsParserWorking[MAX_SEGEMNT_WORKING_COUNT];
	QC_IO_Func*               	m_pIOHandleWorking[MAX_SEGEMNT_WORKING_COUNT];
	CNormalHLSDrm*            	m_pHLSNormalDrmWorking[MAX_SEGEMNT_WORKING_COUNT];
	S_Segment_Buffer_Ctx      	m_aSegmentBufferCtx[MAX_SEGEMNT_WORKING_COUNT];
	S_Segment_Handle_Ctx      	m_aSegmentHandleCtx[MAX_SEGEMNT_WORKING_COUNT];
	bool						m_bHLSOpened;
	int							m_iBufferMaxSizeForM3u8;
	unsigned char*				m_pBufferForM3u8;
	S_SOURCE_EVENTCALLBACK		m_sSourceCallback;
	CAdaptiveStreamBA*			m_pAdaptiveBA;
	S_ADAPTIVESTREAM_BITRATE*   m_pBitrateInfo;
	int                         m_iBitrateCount;
	CMutexLock					m_mtFunc;
	int                         m_iLastM3u8UpdateTime;
	int                         m_iLastSegmentDuration;
	long long		          	m_llPreferBitrate;
	bool                        m_bStopReadFlag;
	unsigned char               m_aQiniuDrmInfo[128];
	int                         m_iQiniuDrmInfoSize;

	S_Segment_Handle_Ctx *		m_pCurSegment;
	S_ADAPTIVESTREAMPARSER_CHUNK* m_pCurChunk;

	long long					m_llChunkTimeStart;
	long long					m_llChunkTimeVideo;
	long long					m_llChunkTimeAudio;
	long long					m_llVideoTimeNew;
	long long					m_llVideoTimeEnd;
	long long					m_llAudioTimeNew;
	long long					m_llAudioTimeEnd;

	// for notify download percent
	int							m_nDownloadPercent;
	bool						m_bLiveLive;
	S_Output_Ctrl_Ctx           m_sOutputCtx;
};

#endif