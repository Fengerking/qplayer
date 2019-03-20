/*******************************************************************************
File:		CHls_Entity.h

Contains:	HLS Entity Header file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-03		Qichao			Create file

*******************************************************************************/

#ifndef __HLS_ENTITY_H__
#define __HLS_ENTITY_H__


#include "CHls_manager.h"
#include "AdaptiveStreamParser.h"
#include "HLSDRM.h"
#include "CMutexLock.h"
#include "qcData.h"


typedef   struct
{
    S_ADAPTIVESTREAMPARSER_CHUNK    sCurrentAdaptiveStreamItem;
    S_DRM_HSL_PROCESS_INFO         sCurrentDrm;
}S_CHUNK_NODE;


typedef   struct
{
    S_CHUNK_NODE     sMainStreamChunk;
    S_CHUNK_NODE     sAltrerAudioChunk;
    S_CHUNK_NODE     sAltrerVideoChunk;
    S_CHUNK_NODE     sAltrerSubTitleChunk;
    S_CHUNK_NODE     sIFrameOnlyChunk;    
}S_CHUNK_CONTAINER;

#define  HLS_MAX_MANIFEST_RETRY_COUNT   5
#define  HLS_ROOT_MANIFEST_ID           0xffffffff
#define  HLS_MAX_STREAM_COUNT_IN_MASTER   256
#define  HLS_MAX_X_MEDIA_COUNT_IN_GROUP   256

class C_HLS_Entity:public CBaseObject
{
public:
	C_HLS_Entity(CBaseInst * pBaseInst);
	~C_HLS_Entity();
	
    //the new interface 
    unsigned int     Init_HLS(S_ADAPTIVESTREAM_PLAYLISTDATA * pData, S_SOURCE_EVENTCALLBACK*  pEventCallback);
    unsigned int     Uninit_HLS();
    unsigned int     Close_HLS();
    unsigned int     Start_HLS();
    unsigned int     Stop_HLS();
    unsigned int     Open_HLS();
    unsigned int     GetChunk_HLS(E_ADAPTIVESTREAMPARSER_CHUNKTYPE uID ,  S_ADAPTIVESTREAMPARSER_CHUNK **ppChunk);
	unsigned int     Seek_HLS(unsigned long long*  pTimeStamp, E_ADAPTIVESTREAMPARSER_SEEK_MODE_FLAG eSeekMode);
    unsigned int     GetDuration_HLS(unsigned long long * pDuration);
    unsigned int     GetProgramCounts_HLS(unsigned int*  pProgramCounts);
    unsigned int     SelectStream_HLS(unsigned int uStreamId, E_ADAPTIVESTREAMING_CHUNKPOS ePrepareChunkPos);
    unsigned int     SelectTrack_HLS(E_ADAPTIVESTREAMPARSER_TRACK_TYPE  eType, unsigned int ulTrackID);
    unsigned int     GetParam_HLS(unsigned int nParamID, void* pParam );
    unsigned int     SetParam_HLS(unsigned int nParamID, void* pParam );
	unsigned int     GetProgramInfo_HLS(QC_STREAM_FORMAT**   &ppStreamInfo, int* piStreamCount);

    //the new interface 

	//add for new interface
	unsigned int  ParseHLSPlaylist(void*  pPlaylistData, unsigned int ulPlayListId);
    unsigned int  CommitPlayListData(void*  pPlaylistData);
	void ResetAllContext();
    unsigned int  PreparePlaySession();
    void SetEventCallbackFunc(void*   pCallbackFunc);
    unsigned int  SelectDefaultStream(unsigned int*  pDefaultBitrate);
    unsigned int  GetChunckItem(E_ADAPTIVESTREAMPARSER_CHUNKTYPE uID ,  S_ADAPTIVESTREAMPARSER_CHUNK **ppChunk);
    unsigned int  GetTrackCountByMainPlayList(S_PLAYLIST_NODE* pPlayListInfo, unsigned int*  pulCount);
    unsigned int  GenerateTheProgramInfo();
	unsigned int  NotifyToParse(char*  pURLRoot, char*   pURL, unsigned int ulPlayListId);
    unsigned int  PlayListUpdateForLive();
	void StopPlaylistUpdate();
    unsigned int  UpdateThePlayListForLive();
    unsigned int  RequestManfestAndParsing(S_ADAPTIVESTREAM_PLAYLISTDATA*  pPlayListData, char*  pRootPath, char* pManifestURL, unsigned int ulPlayListId);
    unsigned int  AdjustTheSequenceIDForMainStream();
	E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE   GetProgramType();
    //add for new interface

    //add the sample
    unsigned int   ConvertErrorCodeToSource2(unsigned int   ulErrorCodeInHLS);
    void  InitChunkNode(S_CHUNK_NODE*  pChunkNode);
protected:


    //add for the common 	
	void    DeleteAllProgramInfo();	
    //add for the common


    //add for parser
    unsigned int  ParserPlayList(unsigned char pBuffer, unsigned int ulBufferSize, char*   pPlayListPath);
    //add for parser

private:
    C_M3U_Manager                  m_sM3UManager;
    S_SOURCE_EVENTCALLBACK*        m_pEventCallbackFunc;
    S_ADAPTIVESTREAM_PLAYLISTDATA    m_sCurrentAdaptiveStreamItemForPlayList;
    S_CHUNK_CONTAINER              m_sChunkContainer;
    bool                           m_bUpdateRunning;
	CMutexLock					   m_sListLock;
	QC_STREAM_FORMAT**             m_ppStreamArray;
	unsigned int                   m_ulStreamCount;
	E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE    m_eProgramType;
};



#endif
