/*******************************************************************************
File:		tsparser.h

Contains:	ts parse header file.

Written by:	Qichao Shen

Change History (most recent first):
2016-12-22		Qichao			Create file

*******************************************************************************/
#ifndef __QP_TS_PARSER_H__
#define __QP_TS_PARSER_H__

#include "cmbasetype.h"
#include "tsbase.h"

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMAX3(a,b,c) FFMAX(FFMAX(a,b),c)
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMIN3(a,b,c) FFMIN(FFMIN(a,b),c)

#define AV_RB16(x)                           \
	((((const uint8*)(x))[0] << 8) |          \
	((const uint8*)(x))[1])



//Error Code
#define AVERROR_INVALIDDATA     -10


#define TransportPacketSyncByte   0x47
#define DEFAULT_FIND_PACKET_SIZE_LEN        (188*20)
#define DEFAULT_MAX_CACHE_SIZE_FOR_HEADER   (188*204)
#define DEFAULT_MAX_BUFFER_SIZE_FOR_PES    (256*1024)



#define TS_FEC_PACKET_SIZE 204
#define TS_DVHS_PACKET_SIZE 192
#define TS_PACKET_SIZE 188
#define TS_MAX_PACKET_SIZE 204

#define NB_PID_MAX 8192
#define MAX_SECTION_SIZE 4096
#define MAX_PIDS_PER_PROGRAM 64

/* pids */
#define PAT_PID                 0x0000

/* table ids */
#define PAT_TID   0x00
#define PMT_TID   0x02

#define STREAM_TYPE_AUDIO_MPEG1     0x03
#define STREAM_TYPE_PES_PRIVATE     0x06
#define STREAM_TYPE_AUDIO_AAC       0x0f
#define STREAM_TYPE_VIDEO_H264      0x1b
#define STREAM_TYPE_VIDEO_HEVC      0x24



typedef void FrameParsedCallback (void*  pMediaSample);

typedef struct
{
    FrameParsedCallback*	pProc;
    void*			        pUserData;
}CM_PARSER_INIT_INFO;

typedef enum  
{
    MPEGTS_NO = 0,
    MPEGTS_PAT,
    MPEGTS_PMT,
    MPEGTS_PAYLOAD,
}E_TSState;


typedef struct  
{
    uint16  usProNum;
    uint16  usPMTPID;
    uint16  usPIDCount;
    uint16  aPIDs[MAX_PIDS_PER_PROGRAM];
    uint8   aStreamTypes[MAX_PIDS_PER_PROGRAM];
	S_Track_Info_From_Desc  aSEsInfo[MAX_PIDS_PER_PROGRAM];
}S_Program_Info;

typedef struct  
{
    E_TSState       eState;
    S_Program_Info  sProInfo;
	S_Program_Info  sProInfoLast;
    uint8         aBufferForPartTs[1024];
    uint32        ulBufferSizeForPartTs;
    uint8*        pBufferForCacheHeader;
    uint32        ulMaxSizeForCacheHeader;
    uint32        ulCurSizeForCacheHeader;

    uint8*       aPesBufferData[MAX_PIDS_PER_PROGRAM];
    uint8        aPesElementType[MAX_PIDS_PER_PROGRAM];
    uint32       aPesBufferMaxSize[MAX_PIDS_PER_PROGRAM];
    uint32       aPesBufferSize[MAX_PIDS_PER_PROGRAM];
    uint32       aPesTimeStamp[MAX_PIDS_PER_PROGRAM];
    uint32       aPesLengthInPesInfo[MAX_PIDS_PER_PROGRAM];
    uint16       aPesPID[MAX_PIDS_PER_PROGRAM];
    uint16       usPesPIDCount;
    uint32       ulTsPacketSize;

    CM_PARSER_INIT_INFO		sCallbackProcInfo;
}S_Ts_Parser_Context;

int IsIFrameForH264(uint8* pData, uint32 ulSize);
int IsIFrameForHEVC(uint8* pData, uint32 ulSize);

int   InitTsParser(CM_PARSER_INIT_INFO*   pParserInit, S_Ts_Parser_Context*  pTsContext);
int   ProcessTs(uint8*  pTsData, uint32  ulTsSize, S_Ts_Parser_Context*  pTsContext);
int   FlushAllCacheData(S_Ts_Parser_Context*  pTsContext);
int   UnInitTsParser( S_Ts_Parser_Context*  pTsContext);
int   GetEsTrackInfoByPID(S_Ts_Parser_Context*  pTsContext, unsigned short ulPID, S_Track_Info_From_Desc*  pDesc);

#endif
