/*******************************************************************************
File:		CHls_manager.h

Contains:	HLS Manager Header file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-03		Qichao			Create file

*******************************************************************************/

#ifndef __HLS_MANAGER_H__

#define __HLS_MANAGER_H__


#include "CHls_parser.h"
#include "AdaptiveStreamParser.h"


#define INVALID_PLALIST_ID                     0xffffffff
#define MAX_X_MEDIA_GROUP_ID_LEN               64
#define MAX_X_STREAM_CODEC_DESC_LEN            64
#define MAX_X_MEDIA_NAME_DESC_LEN              64
#define MAX_X_MEDIA_LANGUAGE_DESC_LEN          64
#define MAX_STREAM_MAX_MEDIA_COUNT             32
#define MAX_REQUEST_CHUNCK_ITEM_COUNT          4
#define MAX_CHARACTERISTICS_LENGTH             1024

#define HLS_INDEX_OFFSET_MAIN_STREAM          0x0
#define HLS_INDEX_OFFSET_X_VIDEO              0x100
#define HLS_INDEX_OFFSET_X_AUDIO              0x200
#define HLS_INDEX_OFFSET_X_SUBTITLE           0x300
#define HLS_INDEX_OFFSET_X_CC                 0x400
#define HLS_INDEX_OFFSET_I_FRAME_STREAM       0x500

#define NORMAL_DIRECTION 0
#define MAX_KEY_ENTRY_COUNT                   32

enum E_ITEM_FLAG_FROM_HLS
{
    E_NORMAL,
    E_DISCONTINUE,
    E_UNKNOWN,
};

enum E_CHUNCK_STATE
{
	E_CHUNCK_FORCE_NEW_STREAM,
	E_CHUNCK_NEW_STREAM,
	E_CHUNCK_SMOOTH_ADAPTION,
	E_CHUNCK_SMOOTH_ADAPTION_EX,
	E_CHUNCK_NORMAL,
	E_UNKNOWN_STATE,
};

enum E_PLAYLIST_TYPE
{
    E_VARIANT_STREAM,
    E_MAIN_STREAM,
    E_X_MEDIA_VIDEO_STREAM,
    E_X_MEDIA_AUDIO_STREAM,
    E_X_MEDIA_SUBTITLE_STREAM,
    E_X_MEDIA_CAPTION_STREAM,
    E_I_FRAME_STREAM,
    E_UNKNOWN_STREAM,
};

enum E_ITEM_TYPE
{
    E_NORMAL_WHOLE_CHUNCK_NODE,
    E_NORMAL_PART_CHUNCK_NODE,
    E_DISCONTINUITY_CHUNCK_NODE,
    E_PRIVATE_NODE,
    E_UNKNOWN_NODE,
};

typedef   struct S_MAIN_STREAM_ATTR
{
	unsigned int                      ulBitrate;
    char                     strCodecDesc[MAX_X_STREAM_CODEC_DESC_LEN];
    char                     strVideoAlterGroup[MAX_X_MEDIA_GROUP_ID_LEN];
    char                     strAudioAlterGroup[MAX_X_MEDIA_GROUP_ID_LEN];
    char                     strSubTitleAlterGroup[MAX_X_MEDIA_GROUP_ID_LEN];	
	char                     strClosedCaptionGroup[MAX_X_MEDIA_GROUP_ID_LEN];
    S_RESOLUTION                sResolution;
}S_MAIN_STREAM_ATTR;


typedef   struct S_CHUNCK_ITEM
{
    char               strChunkParentURL[4096];
    char               strChunckItemURL[4096];
    unsigned long long                ullChunckOffset;
    unsigned long long                ullChunckLen;
    char               strChunckItemTitle[64];
    unsigned int                ulDurationInMsec;
    unsigned int                ulChunckSequenceId;
    E_ITEM_TYPE           eChunckContentType;
    unsigned int                ulPrivateID;     //Dis Number
    unsigned int                ulPlayListId;
    unsigned int                ulDisSequenceId;
    unsigned long long                ullBeginTime;
    unsigned long long                ullEndTime;
    unsigned long long                ullTimeStampOffset;
	unsigned long long                ullProgramDateTime;
    unsigned int                nFlagForTimeStamp;
    unsigned int                ulSequenceIDForKey;
    char               strEXTKEYLine[1024];
    bool               bDisOccur;
    bool               bIndependent;
    char               strXMapURL[1024];
    unsigned long long                ullXMapOffset;
    unsigned long long                ullXMapLen;
    void*              pReserve;
    E_CHUNCK_STATE        eChunkState;
    S_CHUNCK_ITEM*        pNext;
}S_CHUNCK_ITEM;

typedef   struct S_I_FRAME_STREAM_ATTR
{
    unsigned int                      ulBitrate;
    char                     strCodecDesc[MAX_X_STREAM_CODEC_DESC_LEN];
}S_I_FRAME_STREAM_ATTR;

typedef   struct S_X_MEDIA_STREAM_ATTR
{
    E_PLAYLIST_TYPE             eStreamType;
    char                     strGroupId[MAX_X_MEDIA_GROUP_ID_LEN];	
    char                     strName[MAX_X_MEDIA_NAME_DESC_LEN];
    char                     strLanguage[MAX_X_MEDIA_LANGUAGE_DESC_LEN];	
	char                     strAssocLanguage[MAX_X_MEDIA_LANGUAGE_DESC_LEN];	
	char                     strCharacteristics[MAX_CHARACTERISTICS_LENGTH];	
	unsigned int                      ulAutoSelect;
    unsigned int                      ulDefault;
    unsigned int                      ulForced;
    unsigned int                      ulInStreamId;
}S_X_MEDIA_STREAM_ATTR;

typedef   struct S_PLAYLIST_NODE
{
    S_CHUNCK_ITEM*    pChunkItemHeader;
    S_CHUNCK_ITEM*    pChunkItemTail;
	M3U_MANIFEST_TYPE    eManifestType;
	M3U_CHUNCK_PLAYLIST_TYPE    eChuckPlayListType;
    M3U_CHUNCK_PLAYLIST_TYPE_EX eChunkPlayListTypeEx;
	char                     strShortURL[1024];
    char                     strRootURL[1024];
    char                     strInputURL[4096];
    E_PLAYLIST_TYPE             ePlayListType;
	S_PLAYLIST_NODE*            pNext;
	union
	{
	    S_MAIN_STREAM_ATTR    sVarMainStreamAttr;
        S_I_FRAME_STREAM_ATTR sVarIFrameSteamAttr;
        S_X_MEDIA_STREAM_ATTR sVarXMediaStreamAttr;
	};
    unsigned int                      ulItemCount;
    unsigned int                      ulPlayListId;
    unsigned int                      ulExtraIFramePlayListId;
    unsigned int                      ulCurrentMinSequenceIdInDvrWindow;
    unsigned int                      ulCurrentMaxSequenceIdInDvrWindow;
    unsigned int                      ulCurrentDvrDuration;
    unsigned int                      ulTargetDuration;
    unsigned int                      ulLastChunkDuration;
    unsigned int                      ulXStartExist;
    int                      ilXStartValue;	
    bool                     bIndependent;
}S_PLAYLIST_NODE;


typedef    struct S_PLAY_SESSION
{
    S_PLAYLIST_NODE*    pStreamPlayListNode;
    unsigned int              ulMainStreamSequenceId;
    E_CHUNCK_STATE      eMainStreamInAdaptionStreamState;
    unsigned int              ulCurrentDirectionForMainStream;
	
    S_PLAYLIST_NODE*    pAlterVideoPlayListNode;
    unsigned int              ulAlterVideoSequenceId;
    E_CHUNCK_STATE      eAlterVideoInAdaptionStreamState;
    unsigned int              ulCurrentDirectionForAlterVideo;
	
    S_PLAYLIST_NODE*    pAlterAudioPlayListNode;
    unsigned int              ulAlterAudioSequenceId;
    E_CHUNCK_STATE      eAlterAudioInAdaptionStreamState;
    unsigned int              ulCurrentDirectionForAlterAudio;

    S_PLAYLIST_NODE*    pAlterSubTitlePlayListNode;
    unsigned int              ulAlterSubTitleSequenceId;
    E_CHUNCK_STATE      eAlterSubTitleInAdaptionStreamState;
    unsigned int              ulCurrentDirectionForAlterSubTitle;

    S_PLAYLIST_NODE*    pIFramePlayListNode;	
    unsigned int              ulIFrameSequenceId;
    unsigned int              ulCurrentDirectionForIFrameOnly;
}S_PLAY_SESSION;


typedef   struct
{
    char    strStreamVideoGroupDesc[MAX_X_MEDIA_GROUP_ID_LEN];
    unsigned int     ulAlterVideoCount;
	unsigned int*    pAlterVideoPlayListIdArray[MAX_STREAM_MAX_MEDIA_COUNT];

	char    strStreamAudioGroupDesc[MAX_X_MEDIA_GROUP_ID_LEN];
    unsigned int     ulAlterAudioCount;
	unsigned int*    pstrAlterAudioDescArray[MAX_STREAM_MAX_MEDIA_COUNT];

    char    strStreamSubTitleGroupDesc[MAX_X_MEDIA_GROUP_ID_LEN];
    unsigned int     ulAlterSubTitleoCount;	
	char*   pstrAlterSubTitleDescArray[MAX_STREAM_MAX_MEDIA_COUNT];	
	
	char    strStreamClosedCaptionGroupDesc[MAX_X_MEDIA_GROUP_ID_LEN];
	unsigned int     ulAlterClosedCaptionCount;	
	char*   pstrAlterClosedCaptionDescArray[MAX_STREAM_MAX_MEDIA_COUNT];	

	char    strCodecDesc[MAX_X_STREAM_CODEC_DESC_LEN];
    unsigned int     ulBitrate;
	unsigned int     ulWidth;
	unsigned int     ulHeight;
}S_STREAM_INFO;

typedef   struct  
{
    S_TAG_NODE*    pKeyTagArray[MAX_KEY_ENTRY_COUNT];
    unsigned int         ulCurrentKeyTagCount;
}S_KEY_TAG_ARRAY;

typedef   struct
{
    S_CHUNCK_ITEM   aChunckItems[MAX_REQUEST_CHUNCK_ITEM_COUNT];
    unsigned int          ulChunckCount;
}S_REQUEST_ITEM;

typedef   struct
{
    unsigned int              ulAvailable;
	unsigned int              ulCurrentMainStreamPlayListId;
    unsigned int              ulMainStreamSequenceId;	
    E_CHUNCK_STATE      eMainStreamInAdaptionStreamState;

	unsigned int              ulCurrentAlterVideoStreamPlayListId;
    unsigned int              ulAlterVideoStreamSequenceId;	
    E_CHUNCK_STATE      eAlterVideoStreamInAdaptionStreamState;

	unsigned int              ulCurrentAlterAudioStreamPlayListId;
    unsigned int              ulAlterAudioStreamSequenceId;	
    E_CHUNCK_STATE      eAlterAudioStreamInAdaptionStreamState;

    unsigned int              ulCurrentAlterSubTitleStreamPlayListId;
    unsigned int              ulAlterSubTitleStreamSequenceId;
    E_CHUNCK_STATE      eAlterSubTitleStreamInAdaptionStreamState;
}S_SESSION_CONTEXT;


class C_M3U_Manager:public CBaseObject
{
public:
	C_M3U_Manager(CBaseInst * pBaseInst);
    ~C_M3U_Manager();
    unsigned int    ParseManifest(unsigned char* pPlayListContent, unsigned int ulPlayListContentLength, char*  pPlayListURL, unsigned int ulPlayListId);
    unsigned int    BuildMasterPlayList(char*  pPlayListURL);
	unsigned int    BuildMediaPlayList(char*  pPlayListURL, unsigned int  ulPlayListId);
    S_PLAYLIST_NODE*    FindPlayListById(unsigned int  ulPlayListId);
    bool   IsPlaySessionReady();
    unsigned int    GetCurReadyPlaySession(S_PLAY_SESSION**  ppPlaySession);
    unsigned int    SetStartPosForLiveStream();
    unsigned int    GetRootManifestType(M3U_MANIFEST_TYPE*  pRootManfestType);
    S_PLAYLIST_NODE*    GetPlayListNeedParseForSessionReady();
    unsigned int    SetPlayListToSession(unsigned int ulPlayListId);
    unsigned int    AdjustXMedia();
    unsigned int    AdjustIFrameOnly();
	unsigned int    AdjustChunkPosInListForBA(E_ADAPTIVESTREAMING_CHUNKPOS sPrepareChunkPos);
	unsigned int    SetThePos(unsigned int   ulTime, bool*   pbNeedResetParser, unsigned int* pulTimeChunkOffset, E_ADAPTIVESTREAMPARSER_SEEK_MODE_FLAG eSeekMode);
	unsigned int    GetTheDuration(unsigned int* pTimeDuration);
    void   ReleaseAllPlayList();
    unsigned int    GetTheEndTimeForLiveStream();
    unsigned int    GetTheDvrDurationForLiveStream();
    unsigned int    GetTheLiveTimeForLiveStream();
    unsigned int    GetChunkOffsetValueBySequenceId(unsigned int  ulSequenceId, unsigned int* pTimeOffset);
    unsigned int    GetTheDvrEndLengthForLiveStream(unsigned long long*   pEndLength);	
    unsigned int    GetCurrentProgreamStreamType(E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE*   peProgramType);
    void   SetUTCTime(unsigned long long*   pUTCTime);
    void   SetLiveLatencyValue(unsigned int*  pLiveLatencyValue);
	unsigned int    GetChunckItemIntervalTime();
	unsigned int    GetMainStreamArray(S_PLAYLIST_NODE**  pPlayListNodeArray, unsigned int ulArrayMaxSize, unsigned int*   pulArraySize);
	unsigned int    GetMainStreamCount(unsigned int*   pulArraySize);
	unsigned int    GetXMediaStreamArrayWithGroupAndType(S_PLAYLIST_NODE**  pPlayListNodeArray, char* pGroupId, E_PLAYLIST_TYPE  ePlayListType, unsigned int ulArrayMaxSize, unsigned int*   pulArraySize);
	unsigned int    GetXMediaStreamCountWithGroupAndType(char* pGroupId, E_PLAYLIST_TYPE  ePlayListType, unsigned int*   pulArraySize);
    unsigned int    GetCurrentChunk(E_PLAYLIST_TYPE  ePlayListType, S_CHUNCK_ITEM*   pChunkItems);
    void   ResetSessionContext();
	S_PLAYLIST_NODE*    FindTargetPlayListWithTrackTypeAndId(E_ADAPTIVESTREAMPARSER_TRACK_TYPE eType, unsigned int ulTrackId);
    void   AdjustLiveTimeAndDeadTimeForLive(S_PLAYLIST_NODE* pPlayList);
    void   AdjustIndependentFlag(S_PLAYLIST_NODE* pPlayList);
    S_PLAYLIST_NODE*    FindTheFirstMainStream();
    void   ResetPlayListContentForLiveUpdate(S_PLAYLIST_NODE* pPlayList);
    unsigned int    GetPlayListStartOffset(S_PLAYLIST_NODE* pPlayList);
    unsigned int    GetCurrentSessionDurationByChapterId(unsigned int uChapterId, unsigned int*   pTimeOutput);	
    unsigned int    AdjustSequenceIdInSession();	
	S_SESSION_CONTEXT*    GetSessionContext();
    void   PrepareSessionByMainStreamDefaultSetting(S_PLAYLIST_NODE* pPlayList);
    unsigned int    SeekForOneTrackOnly(unsigned int  ulPlaylistId, unsigned long long ullTimeOffset, unsigned int* pNewOffset);
    unsigned int    GetSegmentCountByASId(unsigned int  ulASId, unsigned int* pSegmentCount);
    unsigned int    GetCurrentIFrameOnlyPlaylistId(unsigned int* pIFrameASId);
    void   AddKeyTagNodeToKeyList(S_KEY_TAG_ARRAY*   pKeyArray, S_TAG_NODE*  pNewKeyTag);
    void   GenerateCombinedKeyLineContent(S_KEY_TAG_ARRAY*   pKeyArray, char*  pKeyContent, unsigned int  ulKeyContentMax);

private:
	void   ReleasePlayList(S_PLAYLIST_NODE*   pPlayListNode);
    unsigned int    CreatePlayList(S_PLAYLIST_NODE**   ppPlayListNode);
	unsigned int    MakeChunkPlayList(S_PLAYLIST_NODE*   ppPlayListNode);
    unsigned int    AssembleChunkItem(S_TAG_NODE* pTagInf, S_TAG_NODE* pURI, S_KEY_TAG_ARRAY* pKeyArray, unsigned long long  illProgramValue, 
		                        long long illOffset, long long illLength, unsigned int ulSequenceId, unsigned int  ulDisSequence,
								bool  bDisOccur, unsigned int  ulPlayListId, char*  pPlayListURI, S_TAG_NODE* pXMapTag);
    unsigned int    GetMediaTypeFromTagNode(E_PLAYLIST_TYPE* pePlayListType, S_TAG_NODE*pTagNode);
	void   AddPlayListNode(S_PLAYLIST_NODE*  pPlayList);
    void   FillPlayListInfo(S_PLAYLIST_NODE*  pPlayList, S_TAG_NODE* pTagNode);
	void   FillIFramePlayListInfo(S_PLAYLIST_NODE*  pPlayList, S_TAG_NODE* pTagNode);
	void   FillMainStreamPlayListInfo(S_PLAYLIST_NODE*  pPlayList, S_TAG_NODE* pTagNode);
	void   FillXMediaPlayListInfo(S_PLAYLIST_NODE*  pPlayList, S_TAG_NODE* pTagNode);
	S_PLAYLIST_NODE*    FindPreferXMediaPlayListInGroup(char*  pGroupId, E_PLAYLIST_TYPE ePlayListType);
    unsigned int    GetPreferValueForPlayList(S_PLAYLIST_NODE*   pPlayList);
	unsigned int    GetChunkItem(S_PLAYLIST_NODE*  pPlayListNode, S_CHUNCK_ITEM*   pChunkItem, unsigned int ulSeqenceId);
	unsigned int    FindPosInPlayList(unsigned int  ulTimeOffset, S_PLAYLIST_NODE*   pPlayList, unsigned int*  pulNewSequenceId, unsigned int* pNewOffset, E_ADAPTIVESTREAMPARSER_SEEK_MODE_FLAG eSeekMode);
    unsigned int    GetPlayListChunkOffsetValueBySequenceId(S_PLAYLIST_NODE*   pPlayListNode, unsigned int  ulSequenceId, unsigned int* pTimeOffset);	
    unsigned int    GetPlayListDurationByChapterId(S_PLAYLIST_NODE* pPlayList, unsigned int uChapterId, unsigned int*   pTimeOutput);
    unsigned int    AdjustSequenceIdByPlayListContext(E_PLAYLIST_TYPE  ePlayListType, S_PLAYLIST_NODE*  pPlayList);
    unsigned int    GetUTCTimeFromString(char*   pProgramDateTimeString, unsigned long long*  pUTCTimeValue);
    unsigned long long    GetUTCTime(unsigned int iYear, unsigned int iMonth, unsigned int iDay, unsigned int iHour, unsigned int iMin, unsigned int iSecond);	


    S_PLAY_SESSION    m_sPlaySession;
    S_PLAYLIST_NODE*     m_pPlayListNodeHeader;
	S_PLAYLIST_NODE*     m_pPlayListNodeTail;
    S_SESSION_CONTEXT    m_sSessionContext;

    M3U_MANIFEST_TYPE     m_eRootPlayListType;
    C_M3U_Parser          m_sParser;
    unsigned long long                m_ullUTCTime;
    unsigned int                m_ulSystemtimeForUTC;
};


#endif
