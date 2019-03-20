/*******************************************************************************
File:		CHls_parser.h

Contains:	HLS Parser Header file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-03		Qichao			Create file

*******************************************************************************/

#ifndef __HLS_PARSER_H__
#define __HLS_PARSER_H__

#include "CBaseObject.h"

#define HLS_ERR_NONE                               0
#define HLS_ERR_EMPTY_POINTER                      1
#define HLS_ERR_WRONG_MANIFEST_FORMAT              2
#define HLS_ERR_LACK_MEMORY                        3
#define HLS_UN_IMPLEMENT                           4
#define HLS_ERR_NOT_ENOUGH_BUFFER                  5
#define HLS_ERR_NOT_EXIST                          6
#define HLS_ERR_NOT_ENOUGH_PLAYLIST_PARSED         7
#define HLS_ERR_NEED_DOWNLOAD                      8
#define HLS_ERR_ALREADY_EXIST                      9
#define HLS_ERR_FAIL                               10
#define HLS_ERR_UNKNOWN                            11
#define HLS_ERR_VOD_END                            12
#define HLS_PLAYLIST_END                           13


#define COMMON_TAG_HEADER                           "#EXT"

#define BEGIN_NAME_INDEX                             0
#define TARGETDURATION_NAME_INDEX                    1
#define MEDIA_SEQUENCE_NAME_INDEX                    2
#define BYTERANGE_NAME_INDEX                         3

#define INF_NAME_INDEX                               4
#define KEY_NAME_INDEX                               5
#define STREAM_INF_NAME_INDEX                        6
#define PROGRAM_DATE_TIME_NAME_INDEX                 7

#define I_FRAME_STREAM_INF_NAME_INDEX                8
#define ALLOW_CACHE_NAME_INDEX                       9
#define MEDIA_NAME_INDEX                             10
#define PLAYLIST_TYPE_NAME_INDEX                     11

#define I_FRAMES_ONLY_NAME_INDEX                     12
#define DISCONTINUITY_NAME_INDEX                     13
#define ENDLIST_NAME_INDEX                           14
#define VERSION_NAME_INDEX                           15

#define MAP_NAME_INDEX                               16
#define START_NAME_INDEX                             17
#define DISCONTINUITY_SEQUENCE_NAME_INDEX            18
#define INDEPENDENT_NAME_INDEX                       19
#define NORMAL_URI_NAME_INDEX                        20



#define INVALID_TAG_INDEX                            21
#define V13_SPEC_TAG_TYPE_COUNT                      21
#define MAX_ATTR_COUNT                               16


//TargetDuration
#define TARGETDURATION_VALUE_ATTR_ID                 0
#define TARGETDURATION_MAX_ATTR_COUNT                1

//Media Sequence
#define MEDIA_SEQUENCE_VALUE_ATTR_ID                 0
#define MEDIA_SEQUENCE_MAX_ATTR_COUNT                1

//ByteRange
#define BYTERANGE_RANGE_ATTR_ID                      0
#define BYTERANGE_MAX_ATTR_COUNT                     1

//Inf
#define INF_DURATION_ATTR_ID                         0
#define INF_DESC_ATTR_ID                             1
#define INF_MAX_ATTR_COUNT                           2

//KEY   
#define KEY_LINE_CONTENT                             0
#define KEY_METHOD_ID                                1
#define KEY_METHOD_KEYFORMAT                         2

#define KEY_MAX_ATTR_COUNT                           3


//ProgramDataTime
#define PROGRAM_DATE_TIME_ATTR_ID                    0
#define PROGRAM_MAX_ATTR_COUNT                       1

//Allow Cache
#define ALLOW_CACHE_VALUE_ATTR_ID                    0
#define ALLOW_CACHE_MAX_ATTR_COUNT                   1

//PlayList Type
#define PALYLIST_TYPE_VALUE_ATTR_ID                  0
#define PALYLIST_TYPE_MAX_ATTR_COUNT                 1

//Media
#define MEDIA_TYPE_ATTR_ID                           0
#define MEDIA_GROUP_ID_ATTR_ID                       1
#define MEDIA_NAME_ATTR_ID                           2
#define MEDIA_DEFAULT_ATTR_ID                        3
#define MEDIA_URI_ATTR_ID                            4
#define MEDIA_AUTOSELECT_ATTR_ID                     5
#define MEDIA_LANGUAGE_ATTR_ID                       6
#define MEDIA_ASSOC_LANGUAGE_ATTR_ID                 7
#define MEDIA_FORCED_ATTR_ID                         8
#define MEDIA_INSTREAM_ATTR_ID                       9
#define MEDIA_CHARACTERISTICS_ATTR_ID                10
#define MEDIA_MAX_ATTR_COUNT                         11

//StreamInf
#define STREAM_INF_BANDWIDTH_ATTR_ID                 0
#define STREAM_INF_CODECS_ATTR_ID                    1
#define STREAM_INF_VIDEO_ATTR_ID                     2
#define STREAM_INF_AUDIO_ATTR_ID                     3
#define STREAM_INF_SUBTITLE_ATTR_ID                  4
#define STREAM_INF_CLOSED_CAPTIONS_ATTR_ID           5
#define STREAM_INF_PROGRAM_ID_ATTR_ID                6
#define STREAM_INF_RESOLUTION_ATTR_ID                7
#define STREAM_MAX_ATTR_COUNT                        8

//Discontinuity sequence
#define DISCONTINUITY_SEQUENCE_ATTR_ID               0
#define DISCONTINUITY_SEQUENCE_MAX_ATTR_COUNT        1

//XMAP
#define XMAP_URI_ATTR_ID                             0
#define XMAP_BYTERANGE_ATTR_ID                       1
#define XMAP_MAX_ATTR_COUNT                          2

//I FRAME StreamInf
#define IFRAME_STREAM_URI_ATTR_ID                    0
#define IFRAME_STREAM_BANDWIDTH_ATTR_ID              1
#define IFRAME_STREAM_CODECS_ATTR_ID                 2
#define IFRAME_STREAM_MAX_ATTR_COUNT                 3

//START
#define X_START_TIMEOFFSET_ATTR_ID                   0
#define X_START_PRECISE_ATTR_ID                      1
#define X_START_MAX_ATTR_COUNT                       2

//VERSION
#define VERSION_NUMBER_ATTR_ID                       0
#define VERSION_MAX_ATTR_COUNT                       1

//URI Line
#define URI_LINE_ATTR_ID                             0
#define URI_LINE_MAX_ATTR_COUNT                      1

enum M3U_MEDIA_STREAM_TYPE
{
	M3U_MEDIA_STREAM_X_TYPE,
	M3U_MEDIA_STREAM_MAIN_TYPE,
	M3U_MEDIA_STREAM_UNKNOWN_TYPE
};

enum M3U_MANIFEST_TYPE
{
	M3U_CHUNK_PLAYLIST,
	M3U_STREAM_PLAYLIST,
	M3U_UNKNOWN_PLAYLIST
};

enum M3U_CHUNCK_PLAYLIST_TYPE
{
	M3U_LIVE,
	M3U_VOD,
	M3U_EVENT,
	M3U_INVALID_CHUNK_PLAYLIST_TYPE
};

enum M3U_CHUNCK_PLAYLIST_TYPE_EX
{
	M3U_NORMAL,
	M3U_VISUALON_PD,
	M3U_INVALID_EX    
};

enum M3U_DATA_TYPE
{
	M3U_INT,
	M3U_FLOAT,
	M3U_STRING,
	M3U_HEX_DATA,
    M3U_BYTE_RANGE,
	M3U_DECIMAL_RESOLUTION,
	M3U_UNKNOWN
};

typedef struct
{
	unsigned int    ulHeight;
	unsigned int    ulWidth;
}S_RESOLUTION;

typedef struct
{
	unsigned long long    ullLength;
	unsigned long long    ullOffset;
}S_BYTE_RANGE;


typedef struct S_ATTR_VALUE
{
	M3U_DATA_TYPE   ulDataValueType;
	union
	{
		float           fFloatValue;
		long long          illIntValue;
		char*        pString;
		unsigned char*        pHexData;
		S_RESOLUTION*   pResolution;
        S_BYTE_RANGE*   pRangeInfo;
	};
	unsigned int              ulDataLength;
}S_ATTR_VALUE;

typedef struct S_TAG_NODE
{
	unsigned int          ulTagIndex;
	unsigned int          ulAttrSet;
	unsigned int          ulAttrMaxCount;
    S_ATTR_VALUE**  ppAttrArray;
    S_TAG_NODE*     pNext;
}S_TAG_NODE;

class C_M3U_Parser:public CBaseObject
{
public:
	C_M3U_Parser(CBaseInst * pBaseInst);
    ~C_M3U_Parser();
    unsigned int   ParseManifest(unsigned char*   pManifestData, unsigned int ulDataLength);
    unsigned int   GetTagList(S_TAG_NODE** ppTagNode);
	unsigned int   GetManifestType(M3U_MANIFEST_TYPE*  peManifestType, M3U_CHUNCK_PLAYLIST_TYPE* peChucklistType, M3U_CHUNCK_PLAYLIST_TYPE_EX*  peChunklistTypeEx);
	void           ReleaseAllTagNode();
	void           ResetContext();
    unsigned int   ParseLine(char* pManifestLine);

    
private:
    unsigned int   InitParseContext();
	void  ReleaseParseContext();
    unsigned int   AddURILine(char*   pLine);
	unsigned int   ParseTagLine(char*   pLine);
	unsigned int   CheckWorkMemory(unsigned int  ulNewDataLength);
    unsigned int   VerifyHeader(unsigned char*   pManifestData);
    unsigned int   ReadNextLineWithoutCopy(char* pSrc, char* pEnd, char** ppLine, char** pNext);
	unsigned int   FindAttrValueByName(char*   pOriginalXMediaLine, char*  pAttrValue, unsigned int ulAttrValueSize, char*   pAttrName);
    unsigned int   CreateTagNode(S_TAG_NODE**  ppTagNode, unsigned int  ulTagType);
	void  ReleaseTagNode(S_TAG_NODE*  pTagNode);
    void  AddTag(S_TAG_NODE*  pTagNode);
	unsigned int   GetTagType(char*   pLine);

	unsigned int   ParseTargeDuration(char*   pLine);
	unsigned int   ParseMediaSequence(char*   pLine);
	unsigned int   ParseByteRange(char*   pLine);
	unsigned int   ParseInf(char*   pLine);
	unsigned int   ParseKey(char*   pLine);
	unsigned int   ParseStreamInf(char*   pLine);
	unsigned int   ParseProgramDataTime(char*   pLine);
	unsigned int   ParseIFrameStreamInf(char*   pLine);
	unsigned int   ParseAllowCache(char*   pLine);
	unsigned int   ParseXMedia(char*   pLine);
	unsigned int   ParsePlayListType(char*   pLine);
	unsigned int   ParseIFrameOnly(char*   pLine);
    unsigned int   ParseDisContinuity(char*   pLine);
	unsigned int   ParseEndList(char*   pLine);
	unsigned int   ParseVersion(char*   pLine);
	unsigned int   ParseXMap(char*   pLine);
	unsigned int   ParseXStart(char*   pLine);
	unsigned int   ParseDisSequence(char*   pLine);
	unsigned int   ParseIndependent(char*   pLine);
    
    unsigned int   ParseInt(char*   pLine, char* pAttrName, S_TAG_NODE*  pTagNode, unsigned int ulAttrIndex);
	unsigned int   ParseFloat(char*   pLine, char* pAttrName, S_TAG_NODE*  pTagNode, unsigned int ulAttrIndex);
	unsigned int   ParseString(char*   pLine, char* pAttrName, S_TAG_NODE*  pTagNode, unsigned int ulAttrIndex);
	unsigned int   ParseByteRangeInfo(char*   pLine, char* pAttrName, S_TAG_NODE*  pTagNode, unsigned int ulAttrIndex);
	unsigned int   ParseTotalLine(char*   pLine, S_TAG_NODE*  pTagNode, unsigned int ulAttrIndex);
	unsigned int   ParseResolution(char*   pLine, char* pAttrName, S_TAG_NODE*  pTagNode, unsigned int ulAttrIndex);



    unsigned char*   m_pManifestData;
	unsigned int     m_ulManifestDataMaxLength;
    S_TAG_NODE*    m_pTagNodeHeader;
	S_TAG_NODE*    m_pTagNodeTail;
    M3U_MANIFEST_TYPE   m_eCurrentManifestType;
    M3U_CHUNCK_PLAYLIST_TYPE	 m_eCurrentChuckPlayListType;
    M3U_CHUNCK_PLAYLIST_TYPE_EX  m_eCurrentChuckPlayListTypeEx;

    char**                    m_ppTagName;
	unsigned int*                      m_pAttrMaxCountSet;
    unsigned int                       m_ulTagTypeCount;
};



#ifdef _VONAMESPACE
}
#endif

#endif
