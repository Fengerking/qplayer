/*******************************************************************************
File:		AdaptiveStreamingCommon.h

Contains:	AdaptiveStreaming Common header file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-03		Qichao			Create file

*******************************************************************************/
#ifndef __QP_ADAPTIVE_STREAMING_COMMON_H__
#define __QP_ADAPTIVE_STREAMING_COMMON_H__
#include "qcType.h"

#define MAX_URL_SIZE  4096
#define INAVALIBLEU64 0xffffffffffffffffll
#define PERFER_READ_FILE_FOR_TS   48128                         /* 188*256 */

#define QC_EVENTID_ADAPTIVESTREAMING_NEEDPARSEITEM	 (0x0001)	/*!< Notify there is some item need to be parsed , Param1 will be S_ADAPTIVESTREAM_PLAYLISTDATA* */
#define QC_EVENTID_RESET_TS_PARSER_FOR_NEW_STREAM	 (0x0002)	/*!< Reset the TS Parser For New Stream */
#define QC_EVENTID_RESET_TS_PARSER_FOR_SEEK	         (0x0003)	/*!< Reset the TS Parser For Seek*/
#define QC_EVENTID_SET_TS_PARSER_TIME_OFFSET	     (0x0004)	/*!< Set the TS Time Offset*/
#define QC_EVENTID_SET_TS_PARSER_BA_MODE	         (0x0005)	/*!< Set the BA Mode*/
#define QC_EVENTID_SET_TS_PARSER_RES_INFO	         (0x0006)	/*!< Set the Res Info*/
#define QC_EVENTID_SET_TS_PARSER_FLUSH               (0x0007)	/*!< Flush the Ts Parser*/


#define QC_PARSER_HEADER_FLAG_NEW_STREAM	         (0x0)	    /*!< Header Flag For New Stream */
#define QC_PARSER_HEADER_FLAG_SEEK	                 (0x1)	    /*!< Header Flag For Seek*/
#define QC_PARSER_SET_QINIU_DRM_INFO	             (0x2)	    /*!< QINIU DRM Info*/

enum E_ADAPTIVESTREAMPARSER_CHUNKFLAG
{
	E_ADAPTIVESTREAMPARSER_CHUNKFLAG_FORMATCHANGE = 0x00000001,
	E_ADAPTIVESTREAMPARSER_CHUNKFLAG_INDEPENDENT,
	E_ADAPTIVESTREAMPARSER_CHUNKFLAG_MAX = 0X7FFFFFFF,
};

enum E_ADAPTIVESTREAMPARSER_STREAMTYPE
{
	E_ADAPTIVESTREAMPARSER_STREAMTYPE_UNKOWN,
	E_ADAPTIVESTREAMPARSER_STREAMTYPE_HLS,
	E_ADAPTIVESTREAMPARSER_STREAMTYPE_DASH,
	E_ADAPTIVESTREAMPARSER_STREAMTYPE_MAX = 0X7FFFFFFF,
};

enum E_ADAPTIVESTREAMPARSER_DRM_TYPE
{
	E_ADAPTIVESTREAMPARSER_DRM_UNKNOWN,
	E_ADAPTIVESTREAMPARSER_DRM_AES128,
	E_ADAPTIVESTREAMPARSER_DRM_QN,
	E_ADAPTIVESTREAMPARSER_DRM_TYPE_MAX = 0X7FFFFFFF,
};

enum E_ADAPTIVESTREAMPARSER_CHUNKTYPE
{
	E_ADAPTIVESTREAMPARSER_CHUNKTYPE_AUDIO,
	E_ADAPTIVESTREAMPARSER_CHUNKTYPE_VIDEO,
	E_ADAPTIVESTREAMPARSER_CHUNKTYPE_AUDIOVIDEO,
	E_ADAPTIVESTREAMPARSER_CHUNKTYPE_HEADDATA,
	E_ADAPTIVESTREAMPARSER_CHUNKTYPE_SUBTITLE,	
	E_ADAPTIVESTREAMPARSER_CHUNKTYPE_SEGMENTINDEX,   	/*!< identify the chunk type is segment index, for DASH 'sidx' */
	E_ADAPTIVESTREAMPARSER_CHUNKTYPE_INITDATA,        /*!< identify the mpd does not contain "InitRange" & "IndexRange" , for DASH 'moov' & 'sidx' */
    E_ADAPTIVESTREAMPARSER_CHUNKTYPE_IFRAME_ONLY,     /*! <identify the IFrame Only segment type in HLS>*/	
	E_ADAPTIVESTREAMPARSER_CHUNKTYPE_UNKNOWN = 255,
	E_ADAPTIVESTREAMPARSER_CHUNKTYPE_MAX = 0X7FFFFFFF,
};

enum E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE
{
	E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_UNKNOWN,
	E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_VOD,
	E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_LIVE,
	E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_MAX = 0X7FFFFFFF,
};

enum E_ADAPTIVESTREAMPARSER_TRACK_TYPE
{
	E_ADAPTIVE_TT_AUDIO				= 0X00000001,	/*!< video track*/
	E_ADAPTIVE_TT_VIDEO				= 0X00000002,	/*!< audio track*/
	E_ADAPTIVE_TT_SUBTITLE			= 0X00000003,   /*!< sub title track & closed caption*/
	E_ADAPTIVE_TT_TRACKINFO			= 0X00000004,	/*!< track info */
	E_ADAPTIVE_TT_AUDIOGROUP		= 0X00000005,	/*!< if donot know how many audio in, set it*/
	E_ADAPTIVE_TT_VIDEOGROUP		= 0X00000006,	/*!< if donot know how many video in, set it*/
	E_ADAPTIVE_TT_SUBTITLEGROUP		= 0X00000007,	/*!< if donot know how many subtitle in ,set it*/
	E_ADAPTIVE_TT_MUXGROUP			= 0X00000008,	/*!< if you donot know anything ,set it*/
	E_ADAPTIVE_TT_MAX				= 0X7FFFFFFF
};

enum E_ADAPTIVESTREAMING_CHUNKPOS
{
	E_ADAPTIVESTREAMING_CHUNKPOS_PRESENT = 0, /*!< sepcify adaptiveStreaming parser shall give the present Chunk*/
	E_ADAPTIVESTREAMING_CHUNKPOS_NEXT = 1,		/*!< sepcify adaptiveStreaming parser shall give the next Chunk*/	
};

enum E_ADAPTIVESTREAMING_ADAPTION_MODE
{
	E_ADAPTIVESTREAMING_ADAPTION_AUTO  = 0, /*!< sepcify adaption mode auto*/
	E_ADAPTIVESTREAMING_ADAPTION_FORCE = 1,		/*!< sepcify adaption mode force*/
	E_ADAPTIVESTREAMING_ADAPTION_MAX = 0X7FFFFFFF,		/*!< sepcify adaption mode force*/
};


typedef struct
{
	void* pUserData;
	int (QC_API * SendEvent) (void* pUserData, unsigned int ulID, void * ulParam1, void * ulParam2);
} S_SOURCE_EVENTCALLBACK;

typedef struct
{
	char		strRootUrl[MAX_URL_SIZE];							/*!< The URL of parent playlist */
	char		strUrl[MAX_URL_SIZE];								/*!< The URL of the playlist , maybe relative URL */
	char		strNewUrl[MAX_URL_SIZE];							/*!< The URL after download( maybe redirect forever ), you should always use this url after get this struct */
	unsigned char*	pData;											/*!< The data in the playlist */
	unsigned int	ulDataSize;										/*!< Playlist size */
	void*           pReserve;                                       /*!< The Callback Instance */
} S_ADAPTIVESTREAM_PLAYLISTDATA;


#define  ADAPTIVE_AUTO_STREAM_ID  0x1fffffff
typedef struct
{
	unsigned int  ulStreamID;
	long long     illBitrateInManifest;
	long long     illBitrateActualInFile;
	long long     illBitrateActualInNetwork;
	char          strAbsoluteURL[512];
	char          strDomainURL[512];
	char          strPathURL[512];
	int           iPort;
} S_ADAPTIVESTREAM_BITRATE;

typedef struct
{
	E_ADAPTIVESTREAMPARSER_CHUNKTYPE	eType;						/*!< The type of this chunk */	

	char								strRootUrl[MAX_URL_SIZE];	/*!< The URL of manifest. It must be filled by parser. */
	char								strUrl[MAX_URL_SIZE];		/*!< URL of this chunk , maybe relative URL */

	unsigned long long					ullChunkOffset;				/*!< The download offset of the url, if this info is no avalible, please use INAVALIBLEU64 */
	unsigned long long					ullChunkSize;				/*!< The download size of the url, if this info is no avalible, please use INAVALIBLEU64 */

	unsigned long long					ullChunkLiveTime;			/*!< It is UTC time, it indicates if the chunk is live playback, when should this chunk play , if no such info the value should be INAVALIBLEU64 */
	unsigned long long					ullChunkDeadTime;			/*!< It is UTC time, it indicates when the chunk is not avalible, if no such info the value should be INAVALIBLEU64 */

	unsigned long long					ullStartTime;				/*!< The start offset time of this chunk , the unit of ( ullStartTime / ullTimeScale * 1000 ) should be ms */

	unsigned long long					ullDuration;				/*!< Duration of this chunk , the unit of ( ullDuration / ullTimeScale * 1000 ) should be ms */
	unsigned long long					ullTimeScale;				/*!< TimeScale of this chunk */

	unsigned int						ulFlag;						/*!< Flag to describe pFlagData */
	unsigned int						ulPlaylistId;	            /*!< the playlist id */
	unsigned int						ulPeriodSequenceNumber;	    /*!< the period sequence number */
	unsigned int						ulChunkID;				/*!< unique ID of chunk in playlist */
	void*								pChunkDRMInfo;				/*!< DRM info */
} S_ADAPTIVESTREAMPARSER_CHUNK;

enum E_ADAPTIVESTREAMPARSER_SEEK_MODE_FLAG
{
	E_ADAPTIVESTREAMPARSER_SEEK_MODE_NORMAL = 0x0,
	E_ADAPTIVESTREAMPARSER_SEEK_MODE_FOR_SWITCH_STREAM = 0x1,
	E_ADAPTIVESTREAMPARSER_SEEK_MODE_MAX = 0X7FFFFFFF,
};

#endif
