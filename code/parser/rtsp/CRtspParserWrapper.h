/*******************************************************************************
File:		CRtspParserWrapper.h

Contains:	CRtspParserWrapper header file.

Written by:	Qichao Shen

Change History (most recent first):
2018-05-03		Qichao			Create file

*******************************************************************************/
#ifndef __QP_CRtspParserWrapper_H__
#define __QP_CRtspParserWrapper_H__

#include "qcData.h"
#include "ULibFunc.h"
#include "CBaseObject.h"
#include "RTSPClientAPI.h"

#define RTSP_INS_UNINITED 0
#define RTSP_INS_INITED 1
#define RTSP_MAX_STREAM_COUNT 3
#define RTSP_VIDEO_STREAM_INDEX 0
#define RTSP_AUDIO_STREAM_INDEX 1
#define RTSP_MAX_VIDEO_FRAME_SIZE 1024*1024
#define RTSP_MAX_AUDIO_FRAME_SIZE 256*1024


typedef void FrameParsedCallback (void*  pMediaSample);



class CRtspParserWrapper : public CBaseObject
{
public:
	CRtspParserWrapper(CBaseInst * pBaseInst);
	virtual ~CRtspParserWrapper(void);

	virtual int			Open(const char * pURL);
	virtual int 		Close(void);

	virtual int 		GetParam(int nID, void * pParam);
	virtual int 		SetParam(int nID, void * pParam);

public:
	int                 GetCurState();
	int                 SetFrameHandle(void*   pSendFrameIns);
	int                 SendFrame(void*  pRtspFrame);
	int                 InitFrameBufferCtx();
	int                 UnInitFrameBufferCtx();
	int                 TransactionFrames(int iFrameType, void *pBuf, void* pFrameInfo);
	int                 GetMediaTrackinfo(int iMediaType, int*  piCodecId, char**  ppCodecHeader, int* piCodecHeaderSize);

protected:
	static int			ParseProc(int _chid, void *_chPtr, int _frameType, void *_pBuf, void* pFrameInfo);
	void                Init();
	void                UnInit();
	int                 FlushData();
	int                 MediaTransaction(char *pBuf, void* pFrameInfo);
	int                 GetVideoTrackInfoFromRtsp(S_Media_Track_Info*   pTrackInfo);
	int                 GetAudioTrackInfoFromRtsp(S_Media_Track_Info*   pTrackInfo);

private:
	void                DumpMedia(void* pFrame);
	void                InitDump();
	void                UnInitDump();
private:


	FrameParsedCallback*    m_pFuncCallback;
	void*    m_fRTSPHandle;
	void*               m_pFrameHandle;
	unsigned    char*   m_pVideoBuffer;
	int                 m_iVideoBufferMaxSize;
	bool                m_iFirstIFrameArrived;
	long long           m_illCurVideoFrameTime;
	int                 m_iCurVideoFrameSize;

	unsigned    char*   m_pAudioBuffer;
	int                 m_iAudioBufferMaxSize;
	int                 m_iCurAudioFrameSize;
	long long           m_illCurAudioFrameTime;



	int                 m_iVideoCodecId;
	int                 m_iIFrameFlag;
	int                 m_iFirstISent;
	unsigned char       m_aVideoConfigData[256];
	int                 m_iVideoConfigSize;

	unsigned char       m_aAudioConfigData[256];
	int                 m_iAudioConfigSize;
	int                 m_iAudioCodecId;

	CM_Rtsp_Ins         m_sRtspIns;
	qcLibHandle         m_hLib;
	int                 m_iVideoTrackCount;
	int                 m_iAudioTrackCount;
	int                 m_iMediaDataCome;

#ifdef _DUMP_FRAME_
	FILE*              m_aFMediaData[RTSP_MAX_STREAM_COUNT];
	FILE*              m_aFMediaInfo[RTSP_MAX_STREAM_COUNT];
#endif
};

#endif // __CBaseParser_H__