/*******************************************************************************
File:		CHls_manager.cpp

Contains:	HLS Manager implement file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-03		Qichao			Create file

*******************************************************************************/

#include "string.h"
#include "CHls_manager.h"
#include "AdaptiveStreamParser.h"
#include "USystemFunc.h"
#include "ULogFunc.h"

C_M3U_Manager::C_M3U_Manager(CBaseInst * pBaseInst)
	: CBaseObject(pBaseInst)
	, m_sParser(pBaseInst)
{
	SetObjectName("C_M3U_Manager");
    ResetSessionContext();
    m_pPlayListNodeHeader = NULL;
    m_pPlayListNodeTail = NULL;
    m_eRootPlayListType = M3U_UNKNOWN_PLAYLIST;
	m_ullUTCTime = 0;
    m_ulSystemtimeForUTC = 0;    
    memset(&m_sSessionContext, 0, sizeof(S_SESSION_CONTEXT));
    m_sSessionContext.ulCurrentMainStreamPlayListId = INVALID_PLALIST_ID;
    m_sSessionContext.ulCurrentAlterVideoStreamPlayListId = INVALID_PLALIST_ID;
    m_sSessionContext.ulCurrentAlterAudioStreamPlayListId = INVALID_PLALIST_ID;
    m_sSessionContext.ulCurrentAlterSubTitleStreamPlayListId = INVALID_PLALIST_ID;    
}

C_M3U_Manager::~C_M3U_Manager()
{
    ReleaseAllPlayList();
}

unsigned int    C_M3U_Manager::ParseManifest(unsigned char* pPlayListContent,unsigned int ulPlayListContentLength, char*   pPlayListURL, unsigned int ulPlayListId)
{
    unsigned int    ulRet = 0;
    M3U_MANIFEST_TYPE  eManifestType = M3U_UNKNOWN_PLAYLIST;
    M3U_CHUNCK_PLAYLIST_TYPE eChucklistType = M3U_INVALID_CHUNK_PLAYLIST_TYPE;
    M3U_CHUNCK_PLAYLIST_TYPE_EX  eChunklistTypeEx = M3U_INVALID_EX;
    S_PLAYLIST_NODE*             pPlayListNode = NULL;
    if(pPlayListContent == NULL)
    {
		return HLS_ERR_EMPTY_POINTER;
	}

    ulRet = m_sParser.ParseManifest(pPlayListContent, ulPlayListContentLength);
	if(ulRet != 0)
    {
        QCLOGI("Parse the playlist error");
        return HLS_ERR_UNKNOWN;
	}

    ulRet = m_sParser.GetManifestType(&eManifestType, &eChucklistType, &eChunklistTypeEx);
    if(ulRet != 0)
    {
		QCLOGI("Parse the playlist error");
		return HLS_ERR_UNKNOWN;
	}

	if (eManifestType == M3U_UNKNOWN_PLAYLIST)
	{
		return HLS_ERR_UNKNOWN;
	}

    if(m_eRootPlayListType == M3U_UNKNOWN_PLAYLIST)
    {
		m_eRootPlayListType = eManifestType;
    }

    switch(eManifestType)
    {
        case M3U_CHUNK_PLAYLIST:
        {
			if(m_eRootPlayListType == M3U_CHUNK_PLAYLIST && m_pPlayListNodeHeader == NULL)
            {
                ulRet = CreatePlayList(&pPlayListNode);
				if(ulRet != 0 )
				{
					return ulRet;
				}
				else
				{
                    pPlayListNode->ePlayListType = E_MAIN_STREAM;
					m_pPlayListNodeHeader = m_pPlayListNodeTail = pPlayListNode;
					pPlayListNode->ulPlayListId = 0;
                    m_sPlaySession.pStreamPlayListNode = pPlayListNode;
                    memcpy(pPlayListNode->strRootURL, pPlayListURL, strlen(pPlayListURL));
                    memcpy(pPlayListNode->strShortURL, pPlayListURL, strlen(pPlayListURL));
				}
            }

            pPlayListNode = FindPlayListById(ulPlayListId);
			if(pPlayListNode == NULL)
			{
				QCLOGI("Can't find the PlayList with id:%d", ulPlayListId);
                return HLS_ERR_UNKNOWN;
			}

            ReleasePlayList(pPlayListNode);
			pPlayListNode->eManifestType = eManifestType;
			pPlayListNode->eChuckPlayListType = eChucklistType;
            pPlayListNode->eChunkPlayListTypeEx = eChunklistTypeEx;
            BuildMediaPlayList(pPlayListURL, pPlayListNode->ulPlayListId);
            break;
        }
        case M3U_STREAM_PLAYLIST:
        {
            BuildMasterPlayList(pPlayListURL);
            break;
	    }
    }

    return 0;
}

unsigned int    C_M3U_Manager::BuildMasterPlayList(char*  pPlayListURL)
{
    S_PLAYLIST_NODE*   pPlayListNode = NULL;
    S_TAG_NODE*        pTagNode = NULL;
    unsigned int             ulStreamCount = 0;
    unsigned int             ulXVideoCount = 0;
	unsigned int             ulXAudioCount = 0;
	unsigned int             ulXSubTitleCount = 0;
	unsigned int             ulXCaptionCount = 0;
	unsigned int             ulIFrameCount = 0;
    S_PLAYLIST_NODE*   pLastStreamPlayListNode = NULL;
    unsigned int             ulRet = 0;
    E_PLAYLIST_TYPE    ePlayListType = E_UNKNOWN_STREAM;
    unsigned int             ulPlayListId = 0;
    unsigned int             ulLength = 0;

    ulRet = m_sParser.GetTagList(&pTagNode);
    if(ulRet != 0)
    {
		return HLS_ERR_UNKNOWN;
    }

    while(pTagNode != NULL)
    {
        switch(pTagNode->ulTagIndex)
        {
            case STREAM_INF_NAME_INDEX:
			case MEDIA_NAME_INDEX:
			case I_FRAME_STREAM_INF_NAME_INDEX:
			{
                pPlayListNode = NULL;
				ulRet = GetMediaTypeFromTagNode(&ePlayListType, pTagNode);
                if(ulRet == 0)
                {
					ulRet = CreatePlayList(&pPlayListNode);
					if(ulRet != 0)
					{
						return HLS_ERR_UNKNOWN;
					}

					pPlayListNode->ePlayListType = ePlayListType;
                    memcpy(pPlayListNode->strRootURL, pPlayListURL, strlen(pPlayListURL));
					FillPlayListInfo(pPlayListNode, pTagNode);
                    switch(ePlayListType)
					{
					    case E_MAIN_STREAM:
						{
							ulPlayListId =HLS_INDEX_OFFSET_MAIN_STREAM+ulStreamCount;
							pLastStreamPlayListNode = pPlayListNode;
							ulStreamCount++;
							break;
						}
						case E_X_MEDIA_AUDIO_STREAM:
						{
							ulPlayListId =HLS_INDEX_OFFSET_X_AUDIO+ulXAudioCount;
							ulXAudioCount++;
							break;
						}
						case E_X_MEDIA_VIDEO_STREAM:
						{
							ulPlayListId =HLS_INDEX_OFFSET_X_VIDEO+ulXVideoCount;
							ulXVideoCount++;
							break;
						}
						case E_X_MEDIA_SUBTITLE_STREAM:
						{
							ulPlayListId =HLS_INDEX_OFFSET_X_SUBTITLE+ulXSubTitleCount;
							ulXSubTitleCount++;
							break;
						}
						case E_X_MEDIA_CAPTION_STREAM:
						{
							ulPlayListId =HLS_INDEX_OFFSET_X_CC+ulXCaptionCount;
							ulXCaptionCount++;
							break;
						}
						case E_I_FRAME_STREAM:
						{
							ulPlayListId =HLS_INDEX_OFFSET_I_FRAME_STREAM+ulIFrameCount;
							if(pLastStreamPlayListNode != NULL)
							{
								pLastStreamPlayListNode->ulExtraIFramePlayListId = ulPlayListId;
							}
							ulIFrameCount++;
							break;
						}
						default:
						{
							QCLOGI("M3u8 content error!");
						}
					}

					pPlayListNode->ulPlayListId = ulPlayListId;
				}
				else
				{
					QCLOGI("M3u8 content error!");
				}

				AddPlayListNode(pPlayListNode);
				break;
			}
			case NORMAL_URI_NAME_INDEX:
			{
				if(pLastStreamPlayListNode != NULL)
				{
					if(pTagNode->ppAttrArray[URI_LINE_ATTR_ID] != NULL &&
					   pTagNode->ppAttrArray[URI_LINE_ATTR_ID]->pString != NULL)
					{
						ulLength = (strlen(pTagNode->ppAttrArray[URI_LINE_ATTR_ID]->pString)>1023)?1023:(strlen(pTagNode->ppAttrArray[URI_LINE_ATTR_ID]->pString));
						memcpy(pLastStreamPlayListNode->strShortURL, pTagNode->ppAttrArray[URI_LINE_ATTR_ID]->pString, ulLength);
					}
				}
			}
		}

		pTagNode = pTagNode->pNext;
    }

    return 0;
}

unsigned int    C_M3U_Manager::BuildMediaPlayList(char*  pPlayListURL,unsigned int  ulPlayListId)
{
    unsigned int             ulRet = 0;
    S_PLAYLIST_NODE*   pPlayListNode = NULL;
    S_TAG_NODE*        pTagNode = NULL;

    S_TAG_NODE*        pTagNodeSequenceId = NULL;
    S_TAG_NODE*        pTagNodeURI = NULL;
    S_TAG_NODE*        pTagNodeInf = NULL;
    S_TAG_NODE*        pTagNodeByteRange = NULL;
    S_TAG_NODE*        pTagNodeProgramDataTime = NULL;
    S_TAG_NODE*        pTagNodeXMap = NULL;
	S_TAG_NODE*        pTagNodeDis = NULL;
	S_TAG_NODE*        pTagNodeDisSequence = NULL;
    S_KEY_TAG_ARRAY    sKeyTagArray = {0};

    unsigned int             ulSequenceIdValue = 0;
    unsigned int             ulDisSequenceIdValue = 0;
    unsigned long long             illCurrentOffset = INAVALIBLEU64;
    unsigned long long             illCurrentLength = INAVALIBLEU64;
    unsigned int             ulChunkDurationInMs = 0;
	unsigned long long             illProgramTime = INAVALIBLEU64;
	unsigned int             ulPlayListDurationInMs = 0;
    bool            bFindDis = false;

	pPlayListNode = FindPlayListById(ulPlayListId);
    if(pPlayListNode == NULL)
    {
		return HLS_ERR_UNKNOWN;
    }

    ulRet = m_sParser.GetTagList(&pTagNode);
    if(ulRet != 0)
    {
		return HLS_ERR_UNKNOWN;
    }

    while(pTagNode != NULL)
    {
        switch(pTagNode->ulTagIndex)
        {
            case  MEDIA_SEQUENCE_NAME_INDEX:
            {
				if(pTagNode->ppAttrArray[MEDIA_SEQUENCE_VALUE_ATTR_ID] != NULL)
				{
					ulSequenceIdValue = (unsigned int)(pTagNode->ppAttrArray[MEDIA_SEQUENCE_VALUE_ATTR_ID]->illIntValue);
				}
				break;
			}

            case START_NAME_INDEX:
            {
                pPlayListNode->ulXStartExist = 1;
                pPlayListNode->ilXStartValue = (int)(pTagNode->ppAttrArray[X_START_TIMEOFFSET_ATTR_ID]->illIntValue)*1000;
                break;
            }

			case TARGETDURATION_NAME_INDEX:
			{
				if(pTagNode->ppAttrArray[TARGETDURATION_VALUE_ATTR_ID] != NULL)
                {
					pPlayListNode->ulTargetDuration = (unsigned int)(pTagNode->ppAttrArray[TARGETDURATION_VALUE_ATTR_ID]->illIntValue*1000);
				}
				break;
			}

			case  BYTERANGE_NAME_INDEX:
            {
				if(pTagNode->ppAttrArray[BYTERANGE_RANGE_ATTR_ID] != NULL)
				{
					if(pTagNode->ppAttrArray[BYTERANGE_RANGE_ATTR_ID]->pRangeInfo->ullOffset == INAVALIBLEU64)
					{
                        illCurrentOffset = illCurrentOffset;
					}
					else
					{
						illCurrentOffset = pTagNode->ppAttrArray[BYTERANGE_RANGE_ATTR_ID]->pRangeInfo->ullOffset;					
					}
					illCurrentLength = pTagNode->ppAttrArray[BYTERANGE_RANGE_ATTR_ID]->pRangeInfo->ullLength;
				}

				break;
            }

			case  INF_NAME_INDEX:
            {
				if(pTagNode->ppAttrArray[INF_DURATION_ATTR_ID] != NULL)
				{
					ulChunkDurationInMs = (unsigned int)(1000*(pTagNode->ppAttrArray[INF_DURATION_ATTR_ID]->fFloatValue));
				}
				pTagNodeInf = pTagNode;
				break;
			}

            case  KEY_NAME_INDEX:
		    {
                AddKeyTagNodeToKeyList(&sKeyTagArray, pTagNode);
				break;
		    }

			case NORMAL_URI_NAME_INDEX:
            {
				pTagNodeURI = pTagNode;
				break;
			}

			case DISCONTINUITY_SEQUENCE_NAME_INDEX:
			{
				if(pTagNode->ppAttrArray[DISCONTINUITY_SEQUENCE_NAME_INDEX] != NULL)
				{
					ulDisSequenceIdValue = (unsigned int)(pTagNode->ppAttrArray[DISCONTINUITY_SEQUENCE_NAME_INDEX]->illIntValue);
				}
				break;
			}

			case PROGRAM_DATE_TIME_NAME_INDEX:
			{
                if(pTagNode->ppAttrArray != NULL && pTagNode->ppAttrArray[PROGRAM_DATE_TIME_ATTR_ID] != NULL &&
                   pTagNode->ppAttrArray[PROGRAM_DATE_TIME_ATTR_ID]->pString != NULL)
                {
                    ulRet = GetUTCTimeFromString(pTagNode->ppAttrArray[PROGRAM_DATE_TIME_ATTR_ID]->pString, &illProgramTime);
                    if(ulRet != 0)
                    {
                        illProgramTime = INAVALIBLEU64;
                    }
                    else
                    {
                        QCLOGI("DateTime String:%s, UTC Time:%llu", pTagNode->ppAttrArray[PROGRAM_DATE_TIME_ATTR_ID]->pString, illProgramTime);
                    }
                }
                else
                {
                    illProgramTime = INAVALIBLEU64;
                }
				break;
			}

			case DISCONTINUITY_NAME_INDEX:
		    {
				bFindDis = true;
				ulDisSequenceIdValue++;
				break;
			}

			case MAP_NAME_INDEX:
            {
				pTagNodeXMap = pTagNode;
				break;
			}

            case INDEPENDENT_NAME_INDEX:
            {
                pPlayListNode->bIndependent = true;
                break;
            }
		}

        if(pTagNodeURI != NULL)
		{
			AssembleChunkItem(pTagNodeInf, pTagNodeURI, &sKeyTagArray, illProgramTime, 
				              illCurrentOffset, illCurrentLength, ulSequenceIdValue, ulDisSequenceIdValue, 
							  bFindDis, ulPlayListId, pPlayListURL, pTagNodeXMap);

			//Reset and Modify the value
			pTagNodeInf = NULL;
			pTagNodeURI = NULL;
			if(illProgramTime != INAVALIBLEU64)
			{
				illProgramTime += ulChunkDurationInMs;
			}

			ulPlayListDurationInMs += ulChunkDurationInMs;
			illCurrentOffset += illCurrentLength;
			ulSequenceIdValue++;
			bFindDis = false;
		}

		pTagNode = pTagNode->pNext;
    }

    if(pPlayListNode->pChunkItemHeader != NULL)
    {
		pPlayListNode->ulCurrentMinSequenceIdInDvrWindow = pPlayListNode->pChunkItemHeader->ulSequenceIDForKey;
    }

	if(pPlayListNode->pChunkItemTail != NULL)
	{
		pPlayListNode->ulCurrentMaxSequenceIdInDvrWindow = pPlayListNode->pChunkItemTail->ulSequenceIDForKey;
	}

	pPlayListNode->ulCurrentDvrDuration = ulPlayListDurationInMs;
    if(pPlayListNode->ulLastChunkDuration == 0)
    {
        pPlayListNode->ulLastChunkDuration = pPlayListNode->ulTargetDuration;    
    }

    QCLOGI("PlayList ID:%d, Min Seq:%d, Max Seq:%d, Dvr Duration:%d", pPlayListNode->ulPlayListId, pPlayListNode->ulCurrentMinSequenceIdInDvrWindow,
            pPlayListNode->ulCurrentMaxSequenceIdInDvrWindow, pPlayListNode->ulCurrentDvrDuration);

    AdjustLiveTimeAndDeadTimeForLive(pPlayListNode);
    AdjustIndependentFlag(pPlayListNode);    
    return 0;
}


void    C_M3U_Manager::AdjustLiveTimeAndDeadTimeForLive(S_PLAYLIST_NODE* pPlayList)
{
    unsigned long long   ullTimeLive = 0;
    unsigned long long   ullDeadLive = 0;
    unsigned long long   ullStartOffset = 0;
	unsigned int   ulSystemGetManifest = qcGetSysTime();
    S_CHUNCK_ITEM*   pChunkItem = NULL;

    if(pPlayList == NULL)
    {
        return;
    }

    pChunkItem = pPlayList->pChunkItemHeader;

    ullTimeLive = m_ullUTCTime +(unsigned long long)ulSystemGetManifest - (unsigned long long)m_ulSystemtimeForUTC - (unsigned long long)(pPlayList->ulCurrentDvrDuration);
    ullDeadLive = m_ullUTCTime +(unsigned long long)ulSystemGetManifest - (unsigned long long)m_ulSystemtimeForUTC + (unsigned long long)(pPlayList->ulCurrentDvrDuration) + (unsigned long long)(pPlayList->ulTargetDuration);
    
    while(pChunkItem != NULL)
    {
        switch(pPlayList->eChuckPlayListType)
        {
            case M3U_VOD:
            {
                pChunkItem->ullBeginTime = INAVALIBLEU64;
                pChunkItem->ullEndTime = INAVALIBLEU64;
                break;
            }

            case M3U_LIVE:
            case M3U_EVENT:
            {                
                pChunkItem->ullBeginTime = ullTimeLive;
                pChunkItem->ullEndTime = ullDeadLive;
                break;
            }       
        }

        pChunkItem->ullTimeStampOffset = ullStartOffset;
        ullTimeLive += pChunkItem->ulDurationInMsec;
        ullDeadLive += pChunkItem->ulDurationInMsec;
        ullStartOffset += pChunkItem->ulDurationInMsec;
        pChunkItem = pChunkItem->pNext;
    }

    return ;
}


void    C_M3U_Manager::AdjustIndependentFlag(S_PLAYLIST_NODE* pPlayList)
{
    S_CHUNCK_ITEM*   pChunkItem = NULL;

    if(pPlayList == NULL)
    {
        return;
    }

    pChunkItem = pPlayList->pChunkItemHeader;
    while(pChunkItem != NULL)
    {
        if(pPlayList->bIndependent == true)
        {
            pChunkItem->bIndependent = true;
        }
        else
        {
            pChunkItem->bIndependent = false;        
        }

        pChunkItem = pChunkItem->pNext;
    }

    return ;
}


S_PLAYLIST_NODE*    C_M3U_Manager::FindPlayListById(unsigned int  ulPlayListId)
{
    S_PLAYLIST_NODE*   pPlayListNode = NULL;
	pPlayListNode = m_pPlayListNodeHeader;

	if(m_eRootPlayListType == M3U_CHUNK_PLAYLIST)
	{
		return m_pPlayListNodeHeader;
	}
	
	while(pPlayListNode != NULL)
    {
		if(pPlayListNode->ulPlayListId == ulPlayListId)
		{
			break;
		}
		pPlayListNode = pPlayListNode->pNext;
	}

    return pPlayListNode;
}

void    C_M3U_Manager::ResetSessionContext()
{
    memset(&m_sPlaySession, 0, sizeof(S_PLAY_SESSION));
    m_sPlaySession.eMainStreamInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;
    m_sPlaySession.eMainStreamInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;    
    m_sPlaySession.eMainStreamInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;
    m_sPlaySession.eMainStreamInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;
}


void    C_M3U_Manager::ReleaseAllPlayList()
{
    S_PLAYLIST_NODE*   pPlayListNode = NULL;
    
    pPlayListNode = m_pPlayListNodeHeader;
    if(pPlayListNode == NULL)
    {
        return;
    }
    
    while(pPlayListNode != NULL)
    {
		m_pPlayListNodeHeader = pPlayListNode->pNext;
        ReleasePlayList(pPlayListNode);
        delete pPlayListNode;
		pPlayListNode = m_pPlayListNodeHeader;
    }    
	return;
}

void    C_M3U_Manager::ReleasePlayList(S_PLAYLIST_NODE*   pPlayListNode)
{
    S_CHUNCK_ITEM*     pChunkItem = NULL;
    pChunkItem = pPlayListNode->pChunkItemHeader;

    if(pChunkItem == NULL)
    {
        return;
    }

    while(pChunkItem != NULL)
    {
        pPlayListNode->pChunkItemHeader = pChunkItem->pNext;
        delete pChunkItem;
        pChunkItem = pPlayListNode->pChunkItemHeader;
    }

	return;
}

unsigned int    C_M3U_Manager::CreatePlayList(S_PLAYLIST_NODE**   ppPlayListNode)
{
    S_PLAYLIST_NODE*    pPlayListNode = NULL;
    
	pPlayListNode = new S_PLAYLIST_NODE;
    if(pPlayListNode == NULL)
    {
		return HLS_ERR_EMPTY_POINTER;
	}

	memset(pPlayListNode, 0, sizeof(S_PLAYLIST_NODE));
	pPlayListNode->eManifestType = M3U_UNKNOWN_PLAYLIST;
    pPlayListNode->eChuckPlayListType = M3U_INVALID_CHUNK_PLAYLIST_TYPE;
	pPlayListNode->eChunkPlayListTypeEx = M3U_NORMAL;
	pPlayListNode->ePlayListType = E_UNKNOWN_STREAM;

	*ppPlayListNode = pPlayListNode;
    return 0;
}

unsigned int    C_M3U_Manager::AssembleChunkItem(S_TAG_NODE* pTagInf, S_TAG_NODE* pURI, S_KEY_TAG_ARRAY* pKeyArray, unsigned long long  illProgramValue, 
							long long illOffset, long long illLength, unsigned int ulSequenceId, unsigned int  ulDisSequence,
							bool  bDisOccur, unsigned int  ulPlayListId, char*  pPlayListURI, S_TAG_NODE* pXMapTag)
{
    S_PLAYLIST_NODE*    pPlayListNode = NULL;
    S_CHUNCK_ITEM*    pChunkItem = NULL;
    unsigned int            ulLength = 0;

	if(pTagInf == NULL || pURI == NULL)
	{
        return HLS_ERR_UNKNOWN;
	}

    if(pTagInf->ppAttrArray[INF_DURATION_ATTR_ID] == NULL || pURI->ppAttrArray[URI_LINE_ATTR_ID] == NULL)
    {
		return HLS_ERR_UNKNOWN;
	}

    pPlayListNode = FindPlayListById(ulPlayListId);
    if(pPlayListNode == NULL)
    {
        return HLS_ERR_UNKNOWN;
    }

	pChunkItem = new S_CHUNCK_ITEM;
    if(pChunkItem == NULL)
    {
		return HLS_ERR_UNKNOWN;
    }

	memset(pChunkItem, 0, sizeof(S_CHUNCK_ITEM));

	pChunkItem->ulPlayListId = ulPlayListId;
	pChunkItem->ulSequenceIDForKey = ulSequenceId;
    pChunkItem->ullProgramDateTime = illProgramValue;
    pChunkItem->ulDisSequenceId = ulDisSequence;
    pChunkItem->ulDurationInMsec = (unsigned int)(1000*(pTagInf->ppAttrArray[INF_DURATION_ATTR_ID]->fFloatValue));

	if((pURI->ulAttrSet & (1<<URI_LINE_ATTR_ID)) != 0)
	{
		ulLength = (strlen(pURI->ppAttrArray[URI_LINE_ATTR_ID]->pString)<1024)?(strlen(pURI->ppAttrArray[URI_LINE_ATTR_ID]->pString)):1023;
		memcpy(pChunkItem->strChunckItemURL, pURI->ppAttrArray[URI_LINE_ATTR_ID]->pString, ulLength);
	}

    if((pTagInf->ulAttrSet & (1<<INF_DESC_ATTR_ID)) != 0)
	{
		ulLength = (strlen(pTagInf->ppAttrArray[INF_DESC_ATTR_ID]->pString)<64)?(strlen(pTagInf->ppAttrArray[INF_DESC_ATTR_ID]->pString)):63;
		memcpy(pChunkItem->strChunckItemTitle, pTagInf->ppAttrArray[INF_DESC_ATTR_ID]->pString, ulLength);
	}
    
	if(illLength == INAVALIBLEU64)
    {
		pChunkItem->eChunckContentType = E_NORMAL_WHOLE_CHUNCK_NODE;
    }
	else
	{
		pChunkItem->eChunckContentType = E_NORMAL_PART_CHUNCK_NODE;
	}

	if(bDisOccur == true)
    {
		pChunkItem->bDisOccur = true;
	}
	else
	{
		pChunkItem->bDisOccur = false;
	}
    
    pChunkItem->ullChunckLen = illLength;
    pChunkItem->ullChunckOffset = illOffset;
    
	ulLength = (strlen(pPlayListURI)<1024)?(strlen(pPlayListURI)):1023;
	memcpy(pChunkItem->strChunkParentURL, pPlayListURI, ulLength);

	if(pKeyArray != NULL && pKeyArray->ulCurrentKeyTagCount != 0)
	{
        GenerateCombinedKeyLineContent(pKeyArray, pChunkItem->strEXTKEYLine, 1024);
	}

    if(pPlayListNode->pChunkItemHeader == NULL)
    {
		pPlayListNode->pChunkItemHeader = pPlayListNode->pChunkItemTail = pChunkItem;
    }
	else
	{
        pPlayListNode->pChunkItemTail->pNext = pChunkItem;
		pPlayListNode->pChunkItemTail = pChunkItem;
	}

	return 0;
}

unsigned int    C_M3U_Manager::GetMediaTypeFromTagNode(E_PLAYLIST_TYPE* pePlayListType, S_TAG_NODE*pTagNode)
{
    if(pTagNode == NULL || pePlayListType == NULL)
    {
		return HLS_ERR_UNKNOWN;
    }

	switch(pTagNode->ulTagIndex)
    {
        case STREAM_INF_NAME_INDEX:
		{
			*pePlayListType = E_MAIN_STREAM;
			return 0;
		}
		case MEDIA_NAME_INDEX:
	    {
			if(pTagNode->ppAttrArray[MEDIA_TYPE_ATTR_ID] != NULL &&
			   pTagNode->ppAttrArray[MEDIA_TYPE_ATTR_ID]->pString != NULL)
			{
				if(strcmp(pTagNode->ppAttrArray[MEDIA_TYPE_ATTR_ID]->pString, "AUDIO") == 0)
				{
					*pePlayListType = E_X_MEDIA_AUDIO_STREAM;
					return 0;
				}

				if(strcmp(pTagNode->ppAttrArray[MEDIA_TYPE_ATTR_ID]->pString, "VIDEO") == 0)
				{
					*pePlayListType = E_X_MEDIA_VIDEO_STREAM;
					return 0;
				}

				if(strcmp(pTagNode->ppAttrArray[MEDIA_TYPE_ATTR_ID]->pString, "SUBTITLES") == 0)
				{
					*pePlayListType = E_X_MEDIA_SUBTITLE_STREAM;
					return 0;
				}

				if(strcmp(pTagNode->ppAttrArray[MEDIA_TYPE_ATTR_ID]->pString, "CLOSED-CAPTIONS") == 0)
				{
					*pePlayListType = E_X_MEDIA_CAPTION_STREAM;
					return 0;
				}
			}
			break;
		}
		case I_FRAME_STREAM_INF_NAME_INDEX:
	    {
			*pePlayListType = E_I_FRAME_STREAM;
			return 0;
		}
    }

	return HLS_ERR_UNKNOWN;
}

void    C_M3U_Manager::AddPlayListNode(S_PLAYLIST_NODE*  pPlayList)
{
	if(pPlayList == NULL )
    {
		return;
    }

	if(m_pPlayListNodeHeader == NULL)
	{
		m_pPlayListNodeHeader = m_pPlayListNodeTail = pPlayList;
	}
	else
	{
		m_pPlayListNodeTail->pNext = pPlayList;
		m_pPlayListNodeTail = pPlayList;
	}
}

void    C_M3U_Manager::FillPlayListInfo(S_PLAYLIST_NODE*  pPlayList, S_TAG_NODE* pTagNode)
{
    unsigned int   ulLength = 0;


	switch(pPlayList->ePlayListType)
    {
	    case E_MAIN_STREAM:
		{
            FillMainStreamPlayListInfo(pPlayList, pTagNode);
			break;
		}
		case E_I_FRAME_STREAM:
		{
			FillIFramePlayListInfo(pPlayList, pTagNode);
			break;
		}

        case E_X_MEDIA_AUDIO_STREAM:
        case E_X_MEDIA_VIDEO_STREAM:
		case E_X_MEDIA_SUBTITLE_STREAM:
		case E_X_MEDIA_CAPTION_STREAM:
		{
			FillXMediaPlayListInfo(pPlayList, pTagNode);
			break;
		}
    }

	return;
}

void    C_M3U_Manager::FillIFramePlayListInfo(S_PLAYLIST_NODE*  pPlayList, S_TAG_NODE* pTagNode)
{
	unsigned int  ulLength = 0;
    char*   pDesc = NULL;

	if(pTagNode->ppAttrArray[IFRAME_STREAM_URI_ATTR_ID] != NULL &&
       pTagNode->ppAttrArray[IFRAME_STREAM_URI_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[IFRAME_STREAM_URI_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<1023)?(strlen(pDesc)):1023;
		memcpy(pPlayList->strShortURL, pDesc, ulLength);
	}

	if(pTagNode->ppAttrArray[IFRAME_STREAM_BANDWIDTH_ATTR_ID] != NULL )
	{
		pPlayList->sVarIFrameSteamAttr.ulBitrate = (unsigned int)(pTagNode->ppAttrArray[IFRAME_STREAM_BANDWIDTH_ATTR_ID]->illIntValue);
	}

	if(pTagNode->ppAttrArray[IFRAME_STREAM_CODECS_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[IFRAME_STREAM_CODECS_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[IFRAME_STREAM_CODECS_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<(MAX_X_STREAM_CODEC_DESC_LEN-1))?(strlen(pDesc)):(MAX_X_STREAM_CODEC_DESC_LEN-1);
		memcpy(pPlayList->sVarIFrameSteamAttr.strCodecDesc, pDesc, ulLength);
	}

	return;
}

void    C_M3U_Manager::FillMainStreamPlayListInfo(S_PLAYLIST_NODE*  pPlayList, S_TAG_NODE* pTagNode)
{
    unsigned int  ulLength = 0;
    char*   pDesc = NULL;
    
	if(pTagNode->ppAttrArray[STREAM_INF_BANDWIDTH_ATTR_ID] != NULL)
    {
		pPlayList->sVarMainStreamAttr.ulBitrate = (unsigned int)(pTagNode->ppAttrArray[STREAM_INF_BANDWIDTH_ATTR_ID]->illIntValue);
    }

	if(pTagNode->ppAttrArray[STREAM_INF_CODECS_ATTR_ID] != NULL &&
	   pTagNode->ppAttrArray[STREAM_INF_CODECS_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[STREAM_INF_CODECS_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<63)?(strlen(pDesc)):63;
		memcpy(pPlayList->sVarMainStreamAttr.strCodecDesc, pDesc, ulLength);
	}

	if(pTagNode->ppAttrArray[STREAM_INF_VIDEO_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[STREAM_INF_VIDEO_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[STREAM_INF_VIDEO_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<63)?(strlen(pDesc)):63;
		memcpy(pPlayList->sVarMainStreamAttr.strVideoAlterGroup, pDesc, ulLength);
	}

	if(pTagNode->ppAttrArray[STREAM_INF_AUDIO_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[STREAM_INF_AUDIO_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[STREAM_INF_AUDIO_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<63)?(strlen(pDesc)):63;
		memcpy(pPlayList->sVarMainStreamAttr.strAudioAlterGroup, pDesc, ulLength);
	}

	if(pTagNode->ppAttrArray[STREAM_INF_SUBTITLE_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[STREAM_INF_SUBTITLE_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[STREAM_INF_SUBTITLE_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<63)?(strlen(pDesc)):63;
		memcpy(pPlayList->sVarMainStreamAttr.strSubTitleAlterGroup, pDesc, ulLength);
	}
	
	if(pTagNode->ppAttrArray[STREAM_INF_CLOSED_CAPTIONS_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[STREAM_INF_CLOSED_CAPTIONS_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[STREAM_INF_CLOSED_CAPTIONS_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<63)?(strlen(pDesc)):63;
		memcpy(pPlayList->sVarMainStreamAttr.strClosedCaptionGroup, pDesc, ulLength);
	}

	if(pTagNode->ppAttrArray[STREAM_INF_RESOLUTION_ATTR_ID] != NULL &&
	   pTagNode->ppAttrArray[STREAM_INF_RESOLUTION_ATTR_ID]->pResolution != NULL	)
	{
		pPlayList->sVarMainStreamAttr.sResolution.ulWidth = pTagNode->ppAttrArray[STREAM_INF_RESOLUTION_ATTR_ID]->pResolution->ulWidth;
		pPlayList->sVarMainStreamAttr.sResolution.ulHeight = pTagNode->ppAttrArray[STREAM_INF_RESOLUTION_ATTR_ID]->pResolution->ulHeight;
	}
}

void    C_M3U_Manager::FillXMediaPlayListInfo(S_PLAYLIST_NODE*  pPlayList, S_TAG_NODE* pTagNode)
{
	unsigned int  ulLength = 0;
	char*   pDesc = NULL;
    
    pPlayList->sVarXMediaStreamAttr.eStreamType = pPlayList->ePlayListType;

	if(pTagNode->ppAttrArray[MEDIA_GROUP_ID_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[MEDIA_GROUP_ID_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[MEDIA_GROUP_ID_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<63)?(strlen(pDesc)):63;
		memcpy(pPlayList->sVarXMediaStreamAttr.strGroupId, pDesc, ulLength);
	}

	if(pTagNode->ppAttrArray[MEDIA_NAME_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[MEDIA_NAME_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[MEDIA_NAME_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<63)?(strlen(pDesc)):63;
		memcpy(pPlayList->sVarXMediaStreamAttr.strName, pDesc, ulLength);
	}

	if(pTagNode->ppAttrArray[MEDIA_LANGUAGE_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[MEDIA_LANGUAGE_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[MEDIA_LANGUAGE_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<63)?(strlen(pDesc)):63;
		memcpy(pPlayList->sVarXMediaStreamAttr.strLanguage, pDesc, ulLength);
	}

	if(pTagNode->ppAttrArray[MEDIA_ASSOC_LANGUAGE_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[MEDIA_ASSOC_LANGUAGE_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[MEDIA_ASSOC_LANGUAGE_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<63)?(strlen(pDesc)):63;
		memcpy(pPlayList->sVarXMediaStreamAttr.strAssocLanguage, pDesc, ulLength);
	}

	if(pTagNode->ppAttrArray[MEDIA_DEFAULT_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[MEDIA_DEFAULT_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[MEDIA_DEFAULT_ATTR_ID]->pString;
		if(strcmp(pDesc, "YES") == 0)
		{
			pPlayList->sVarXMediaStreamAttr.ulDefault = 1;
		}
		else
		{
			pPlayList->sVarXMediaStreamAttr.ulDefault = 0;
		}
	}

	if(pTagNode->ppAttrArray[MEDIA_AUTOSELECT_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[MEDIA_AUTOSELECT_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[MEDIA_AUTOSELECT_ATTR_ID]->pString;
		if(strcmp(pDesc, "YES") == 0)
		{
			pPlayList->sVarXMediaStreamAttr.ulDefault = 1;
		}
		else
		{
			pPlayList->sVarXMediaStreamAttr.ulDefault = 0;
		}
	}

	if(pTagNode->ppAttrArray[MEDIA_FORCED_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[MEDIA_FORCED_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[MEDIA_FORCED_ATTR_ID]->pString;
		if(strcmp(pDesc, "YES") == 0)
		{
			pPlayList->sVarXMediaStreamAttr.ulDefault = 1;
		}
		else
		{
			pPlayList->sVarXMediaStreamAttr.ulDefault = 0;
		}
	}


	if(pTagNode->ppAttrArray[MEDIA_URI_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[MEDIA_URI_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[MEDIA_URI_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<1023)?(strlen(pDesc)):1023;
		memcpy(pPlayList->strShortURL, pDesc, ulLength);
	}

	if(pTagNode->ppAttrArray[MEDIA_CHARACTERISTICS_ATTR_ID] != NULL &&
		pTagNode->ppAttrArray[MEDIA_CHARACTERISTICS_ATTR_ID]->pString != NULL)
	{
		pDesc = pTagNode->ppAttrArray[MEDIA_CHARACTERISTICS_ATTR_ID]->pString;
		ulLength = (strlen(pDesc)<1023)?(strlen(pDesc)):1023;
		memcpy(pPlayList->sVarXMediaStreamAttr.strCharacteristics, pDesc, ulLength);
	}

	if(pTagNode->ppAttrArray[MEDIA_INSTREAM_ATTR_ID] != NULL)
	{
		pPlayList->sVarXMediaStreamAttr.ulInStreamId = (unsigned int)(pTagNode->ppAttrArray[MEDIA_INSTREAM_ATTR_ID]->illIntValue);
	}
}


bool    C_M3U_Manager::IsPlaySessionReady()
{
	if(m_sPlaySession.pStreamPlayListNode != NULL && m_sPlaySession.pStreamPlayListNode->pChunkItemHeader != NULL)
	{
		if( m_sPlaySession.pAlterAudioPlayListNode != NULL && 
			strlen(m_sPlaySession.pAlterAudioPlayListNode->strShortURL) != 0)
		{
			if(m_sPlaySession.pAlterAudioPlayListNode->pChunkItemHeader == NULL )
			{
				return false;
			}
		}

		if( m_sPlaySession.pAlterVideoPlayListNode != NULL && 
			strlen(m_sPlaySession.pAlterVideoPlayListNode->strShortURL) != 0)
		{
			if(m_sPlaySession.pAlterVideoPlayListNode->pChunkItemHeader == NULL )
			{
				return false;
			}
		}

		if( m_sPlaySession.pAlterSubTitlePlayListNode != NULL && 
			strlen(m_sPlaySession.pAlterSubTitlePlayListNode->strShortURL) != 0)
		{
			if(m_sPlaySession.pAlterSubTitlePlayListNode->pChunkItemHeader == NULL )
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

unsigned int    C_M3U_Manager::GetCurReadyPlaySession(S_PLAY_SESSION**  ppPlaySession)
{
    if(ppPlaySession == NULL)
    {
		return HLS_ERR_EMPTY_POINTER;
    }

    if(IsPlaySessionReady() == false)
    {
		return HLS_ERR_FAIL;
    }

	*ppPlaySession = &m_sPlaySession;
    return 0;
}

unsigned int    C_M3U_Manager::SetStartPosForLiveStream()
{
    S_PLAYLIST_NODE*    pPlayList = NULL;
    unsigned int              ulOffset = 0;
    unsigned int              ulNewSequenceId = 0;
    unsigned int              ulRet = 0;
    unsigned int              ulNewOffset = 0;
    if(m_sPlaySession.pStreamPlayListNode == NULL)
    {
        return HLS_ERR_FAIL;
    }

    pPlayList = m_sPlaySession.pStreamPlayListNode;
    ulOffset = GetPlayListStartOffset(pPlayList);

    ulRet = FindPosInPlayList(ulOffset, pPlayList, &ulNewSequenceId, &ulNewOffset, E_ADAPTIVESTREAMPARSER_SEEK_MODE_NORMAL);
    m_sPlaySession.ulMainStreamSequenceId = ulNewSequenceId;

    m_sPlaySession.ulAlterAudioSequenceId = ulNewSequenceId;
    m_sPlaySession.ulAlterVideoSequenceId = ulNewSequenceId;
    m_sPlaySession.ulAlterSubTitleSequenceId = ulNewSequenceId;

    QCLOGI("Set the New SequenceId:%d at start!");
    return 0;
}

unsigned int    C_M3U_Manager::GetRootManifestType(M3U_MANIFEST_TYPE*  pRootManfestType)
{
    if(pRootManfestType == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }

    *pRootManfestType = m_eRootPlayListType;
    return 0;
}

S_PLAYLIST_NODE*    C_M3U_Manager::GetPlayListNeedParseForSessionReady()
{

    if(m_sPlaySession.pStreamPlayListNode == NULL)
    {
        m_sPlaySession.pStreamPlayListNode = FindTheFirstMainStream();
		PrepareSessionByMainStreamDefaultSetting(m_sPlaySession.pStreamPlayListNode);
        return m_sPlaySession.pStreamPlayListNode;
    }

    if(m_sPlaySession.pStreamPlayListNode != NULL && m_sPlaySession.pStreamPlayListNode->pChunkItemHeader == NULL)
    {
		return m_sPlaySession.pStreamPlayListNode; 
    }

	if(m_sPlaySession.pAlterAudioPlayListNode != NULL && (strlen(m_sPlaySession.pAlterAudioPlayListNode->strShortURL) != 0)  
	   && m_sPlaySession.pAlterAudioPlayListNode->pChunkItemHeader == NULL)
	{
		return m_sPlaySession.pAlterAudioPlayListNode; 
	}

	if(m_sPlaySession.pAlterVideoPlayListNode != NULL && (strlen(m_sPlaySession.pAlterVideoPlayListNode->strShortURL) != 0)  
		&& m_sPlaySession.pAlterVideoPlayListNode->pChunkItemHeader == NULL)
	{
		return m_sPlaySession.pAlterVideoPlayListNode; 
	}

	if(m_sPlaySession.pAlterSubTitlePlayListNode != NULL && (strlen(m_sPlaySession.pAlterSubTitlePlayListNode->strShortURL) != 0)  
		&& m_sPlaySession.pAlterSubTitlePlayListNode->pChunkItemHeader == NULL)
	{
		return m_sPlaySession.pAlterSubTitlePlayListNode; 
	}
    
	return NULL;
}

unsigned int    C_M3U_Manager::SetThePos(unsigned int   ulTime, bool*   pbNeedResetParser, unsigned int* pulTimeChunkOffset, E_ADAPTIVESTREAMPARSER_SEEK_MODE_FLAG eSeekMode)
{
    unsigned int   ulRet = 0;
    unsigned int   ulNewSequenceId = 0;
    unsigned int   ulNewOffset = 0;
    S_PLAYLIST_NODE*  pPlayListNode = NULL;

    if(pulTimeChunkOffset == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }

    pPlayListNode = m_sPlaySession.pStreamPlayListNode;
	ulRet = FindPosInPlayList(ulTime, pPlayListNode, &ulNewSequenceId, &ulNewOffset, eSeekMode);
    if(ulRet == 0)
    {
        m_sPlaySession.ulMainStreamSequenceId = ulNewSequenceId;
        m_sPlaySession.eMainStreamInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;
        *pulTimeChunkOffset = ulNewOffset;
    }
    else
    {
        return HLS_ERR_FAIL;
    }

    pPlayListNode = m_sPlaySession.pAlterAudioPlayListNode;
    if(pPlayListNode != NULL && strlen(pPlayListNode->strShortURL) != 0)
    {
		ulRet = FindPosInPlayList(ulTime, pPlayListNode, &ulNewSequenceId, &ulNewOffset, eSeekMode);
        if(ulRet == 0)
        {
            m_sPlaySession.ulAlterAudioSequenceId = ulNewSequenceId;
        }
    }
    m_sPlaySession.eAlterAudioInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;
    

    pPlayListNode = m_sPlaySession.pAlterVideoPlayListNode;
    if(pPlayListNode != NULL && strlen(pPlayListNode->strShortURL) != 0)
    {
		ulRet = FindPosInPlayList(ulTime, pPlayListNode, &ulNewSequenceId, &ulNewOffset, eSeekMode);
        if(ulRet == 0)
        {
            m_sPlaySession.ulAlterVideoSequenceId = ulNewSequenceId;
        }
    }
    m_sPlaySession.eAlterVideoInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;

    pPlayListNode = m_sPlaySession.pAlterSubTitlePlayListNode;
    if(pPlayListNode != NULL && strlen(pPlayListNode->strShortURL) != 0)
    {
		ulRet = FindPosInPlayList(ulTime, pPlayListNode, &ulNewSequenceId, &ulNewOffset, eSeekMode);
        if(ulRet == 0)
        {
            m_sPlaySession.ulAlterSubTitleSequenceId = ulNewSequenceId;
        }
    }
    m_sPlaySession.eAlterSubTitleInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;
    
    return 0;
}

unsigned int    C_M3U_Manager::GetTheDuration(unsigned int* pTimeDuration)
{
    if(m_sPlaySession.pStreamPlayListNode == NULL)
    {
		return HLS_ERR_FAIL;
	}

	if(pTimeDuration == NULL)
	{
		return HLS_ERR_UNKNOWN;
	}

    if(m_sPlaySession.pStreamPlayListNode->eChuckPlayListType == M3U_VOD)
    {
	    *pTimeDuration = m_sPlaySession.pStreamPlayListNode->ulCurrentDvrDuration;
    }
    else
    {
        *pTimeDuration = 0;
    }
    return 0;
}

unsigned int    C_M3U_Manager::GetTheEndTimeForLiveStream()
{
    return 0;
}

unsigned int    C_M3U_Manager::GetTheDvrDurationForLiveStream()
{
    if(m_sPlaySession.pStreamPlayListNode == NULL)
    {
	    return 0;
    }
    else
    {
        return m_sPlaySession.pStreamPlayListNode->ulCurrentDvrDuration;
    }
}

unsigned int    C_M3U_Manager::GetTheLiveTimeForLiveStream()
{
	return 0;
}

unsigned int    C_M3U_Manager::GetChunkOffsetValueBySequenceId(unsigned int  ulSequenceId, unsigned int* pTimeOffset)
{
    S_PLAYLIST_NODE*   pPlayListNode = NULL;
    unsigned int   ulRet= 0;
    if(pTimeOffset == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }

    pPlayListNode = m_sPlaySession.pStreamPlayListNode;
    return GetPlayListChunkOffsetValueBySequenceId(pPlayListNode, ulSequenceId, pTimeOffset);
}

unsigned int    C_M3U_Manager::GetPlayListChunkOffsetValueBySequenceId(S_PLAYLIST_NODE*   pPlayListNode, unsigned int  ulSequenceId, unsigned int* pTimeOffset)
{
    S_CHUNCK_ITEM*   pChunkItem = NULL;
    unsigned int   ulOffset = 0;
    if(pPlayListNode == NULL || pTimeOffset == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }

    if((ulSequenceId < pPlayListNode->ulCurrentMinSequenceIdInDvrWindow) ||
       (ulSequenceId > pPlayListNode->ulCurrentMaxSequenceIdInDvrWindow) )
    {
        QCLOGI("Invalid sequence id value:%d, not in the Window %d to %d", ulSequenceId, pPlayListNode->ulCurrentMinSequenceIdInDvrWindow,
                pPlayListNode->ulCurrentMaxSequenceIdInDvrWindow);
    }

    pChunkItem = pPlayListNode->pChunkItemHeader;
    while(pChunkItem != NULL)
    {
    
        if(pChunkItem->ulSequenceIDForKey == ulSequenceId)
        {
            *pTimeOffset = ulOffset;
            return 0;
        }
        else
        {
            ulOffset += pChunkItem->ulDurationInMsec; 
        }
        
        pChunkItem = pChunkItem->pNext;
    }

    return 0;
}

unsigned int    C_M3U_Manager::GetTheDvrEndLengthForLiveStream(unsigned long long*   pEndLength)
{
    S_CHUNCK_ITEM*      pChunkItem = NULL;
    S_PLAYLIST_NODE*    pPlayList = NULL;
    unsigned int              ulTotalTime = 0;
    unsigned int              ulCurrentSequenceId = m_sPlaySession.ulMainStreamSequenceId;
    unsigned int        ulRet = 0;
    
    pPlayList = m_sPlaySession.pStreamPlayListNode;
    if(pPlayList == NULL || pEndLength == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }

    pChunkItem = pPlayList->pChunkItemHeader;
    while(pChunkItem != NULL)
    {
        if(pChunkItem->ulSequenceIDForKey >= ulCurrentSequenceId)
        {
            ulTotalTime += pChunkItem->ulDurationInMsec;
        }

        pChunkItem = pChunkItem->pNext;
    }

    QCLOGI("The Current SequenceId:%d, the End Length:%d", ulCurrentSequenceId, ulTotalTime);
    *pEndLength = (unsigned long long)ulTotalTime;
    return 0;
}

unsigned int    C_M3U_Manager::GetCurrentProgreamStreamType(E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE*   peProgramType)
{
	if(m_sPlaySession.pStreamPlayListNode != NULL)
	{
		if(m_sPlaySession.pStreamPlayListNode->eChuckPlayListType == M3U_VOD)
		{
			QCLOGI("Program Type VOD Stream!")
			*peProgramType = E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_VOD;
		}

		if(m_sPlaySession.pStreamPlayListNode->eChuckPlayListType == M3U_LIVE)
		{        
			QCLOGI("Program Type Live Stream!")
			*peProgramType = E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_LIVE;
		}
		return 0;
	}
	else
	{
		return HLS_ERR_NOT_ENOUGH_PLAYLIST_PARSED;
	}
}

void       C_M3U_Manager::SetUTCTime(unsigned long long*   pUTCTime)
{
	QCLOGI("the UTC Time:%lld", *pUTCTime);
	m_ullUTCTime = (*pUTCTime);
	m_ulSystemtimeForUTC = qcGetSysTime();
	QCLOGI("the System for UTC:%d", m_ulSystemtimeForUTC);
}

void       C_M3U_Manager::SetLiveLatencyValue(unsigned int*  pLiveLatencyValue)
{
    return;
}

unsigned int       C_M3U_Manager::GetChunckItemIntervalTime()
{
    if(m_sPlaySession.pStreamPlayListNode != NULL)
    {
        return  m_sPlaySession.pStreamPlayListNode->ulLastChunkDuration;
    }
    else
    {
        QCLOGI("session is not ready, return 10 second!");
        return 10000;
    }
}

unsigned int       C_M3U_Manager::SetPlayListToSession(unsigned int ulPlayListId)
{
    S_PLAYLIST_NODE*   pPlayListNode = NULL;

    pPlayListNode = FindPlayListById(ulPlayListId);
    if(pPlayListNode == NULL)
    {
		return HLS_ERR_UNKNOWN;
	}

    switch(pPlayListNode->ePlayListType)
    {
        case E_MAIN_STREAM:
        {
            if(m_sPlaySession.pStreamPlayListNode == NULL)
            {            
			    m_sPlaySession.eMainStreamInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;
            }
            else
            {                
                m_sPlaySession.eMainStreamInAdaptionStreamState = E_CHUNCK_SMOOTH_ADAPTION_EX;
            }
			m_sPlaySession.pStreamPlayListNode = pPlayListNode;
            break;
        }
		case E_X_MEDIA_AUDIO_STREAM:
        {
            m_sPlaySession.pAlterAudioPlayListNode = pPlayListNode;
            m_sPlaySession.eAlterAudioInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;
			break;
        }
		case E_X_MEDIA_VIDEO_STREAM:
		{
			m_sPlaySession.pAlterVideoPlayListNode = pPlayListNode;
			m_sPlaySession.eAlterVideoInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;
            break;
		}
		case E_X_MEDIA_SUBTITLE_STREAM:
        {
			m_sPlaySession.pAlterSubTitlePlayListNode = pPlayListNode;
			m_sPlaySession.eAlterSubTitleInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;
			break;
		}
		case E_I_FRAME_STREAM:
		{
            m_sPlaySession.pIFramePlayListNode = pPlayListNode;
            break;
		}
		default:
        {
			return HLS_ERR_UNKNOWN;
		}
	}
	
    return 0;
}

unsigned int       C_M3U_Manager::AdjustXMedia()
{
    S_PLAYLIST_NODE*   pPlayListNode = NULL;
    unsigned int             ulRet = 0;
    char*           pGroupInStream = NULL;
	char*           pGroupInXMedia = NULL;

    if(m_sPlaySession.pStreamPlayListNode == NULL)
    {
        return HLS_ERR_UNKNOWN;
    }

    pGroupInStream = m_sPlaySession.pStreamPlayListNode->sVarMainStreamAttr.strAudioAlterGroup;
    if(strlen(pGroupInStream) != 0 )
    {
		if(m_sPlaySession.pAlterAudioPlayListNode == NULL || strcmp(pGroupInStream, m_sPlaySession.pAlterAudioPlayListNode->sVarXMediaStreamAttr.strGroupId) != 0)
        {
            m_sPlaySession.pAlterAudioPlayListNode = FindPreferXMediaPlayListInGroup(pGroupInStream, E_X_MEDIA_AUDIO_STREAM);
		}
    }

	pGroupInStream = m_sPlaySession.pStreamPlayListNode->sVarMainStreamAttr.strVideoAlterGroup;
	if(strlen(pGroupInStream) != 0)
	{
		if(m_sPlaySession.pAlterVideoPlayListNode == NULL || strcmp(pGroupInStream, m_sPlaySession.pAlterVideoPlayListNode->sVarXMediaStreamAttr.strGroupId) != 0)
		{
			m_sPlaySession.pAlterVideoPlayListNode = FindPreferXMediaPlayListInGroup(pGroupInStream, E_X_MEDIA_VIDEO_STREAM);
		}
	}

	pGroupInStream = m_sPlaySession.pStreamPlayListNode->sVarMainStreamAttr.strSubTitleAlterGroup;
	if(strlen(pGroupInStream) != 0)
	{
		if(m_sPlaySession.pAlterSubTitlePlayListNode == NULL  || strcmp(pGroupInStream, m_sPlaySession.pAlterSubTitlePlayListNode->sVarXMediaStreamAttr.strGroupId) != 0)
		{
			m_sPlaySession.pAlterSubTitlePlayListNode = FindPreferXMediaPlayListInGroup(pGroupInStream, E_X_MEDIA_SUBTITLE_STREAM);
		}
	}
	
    return 0;
}


unsigned int       C_M3U_Manager::AdjustIFrameOnly()
{
    S_PLAYLIST_NODE*   pPlayListNode = NULL;
    unsigned int             ulRet = 0;

    if(m_sPlaySession.pStreamPlayListNode == NULL)
    {
        return HLS_ERR_UNKNOWN;
    }

    if(m_sPlaySession.pStreamPlayListNode->ulExtraIFramePlayListId != 0)
    {
        pPlayListNode = FindPlayListById(m_sPlaySession.pStreamPlayListNode->ulExtraIFramePlayListId);
        m_sPlaySession.pIFramePlayListNode = pPlayListNode;        
        QCLOGI("Set IFrame Only %d for Main PlayList:%d", pPlayListNode->ulPlayListId, m_sPlaySession.pStreamPlayListNode->ulPlayListId);
    }
    else
    {
        QCLOGI("No IFrame Only for Main PlayList:%d", m_sPlaySession.pStreamPlayListNode->ulPlayListId);
    }

    return 0;
}


unsigned int       C_M3U_Manager::AdjustChunkPosInListForBA(E_ADAPTIVESTREAMING_CHUNKPOS sPrepareChunkPos)
{
    switch(sPrepareChunkPos)
    {
        case E_ADAPTIVESTREAMING_CHUNKPOS_PRESENT:
        {            
            if(m_sPlaySession.pStreamPlayListNode != NULL)
            {
                if(m_sPlaySession.ulMainStreamSequenceId != 0)
                {
                    m_sPlaySession.ulMainStreamSequenceId = m_sPlaySession.ulMainStreamSequenceId-1;
                }
                else
                {
                    m_sPlaySession.ulMainStreamSequenceId = 0;                
                }
            }
            break;
        }
    }


    return 0;
}

S_PLAYLIST_NODE*       C_M3U_Manager::FindPreferXMediaPlayListInGroup(char*  pGroupId, E_PLAYLIST_TYPE ePlayListType)
{
	S_PLAYLIST_NODE*   pPlayList = NULL;
    pPlayList = m_pPlayListNodeHeader;
	S_PLAYLIST_NODE*   pPreferPlayList = NULL;

    while(pPlayList != NULL)
	{
		if(pPlayList->ePlayListType == ePlayListType && strcmp(pPlayList->sVarXMediaStreamAttr.strGroupId, pGroupId) == 0)
		{
			if(pPreferPlayList == NULL)
			{
				pPreferPlayList = pPlayList;
			}
			else
			{
				if(GetPreferValueForPlayList(pPreferPlayList) < GetPreferValueForPlayList(pPlayList))
				{
					pPreferPlayList = pPlayList;
				}
			}
		}

		pPlayList = pPlayList->pNext;
	}

	return pPreferPlayList;
}

unsigned int       C_M3U_Manager::GetPreferValueForPlayList(S_PLAYLIST_NODE*   pPlayList)
{
    if(pPlayList == NULL)
    {
		return 0;
    }

	switch(pPlayList->ePlayListType)
    {
        case E_X_MEDIA_AUDIO_STREAM:
        case E_X_MEDIA_SUBTITLE_STREAM:
        case E_X_MEDIA_VIDEO_STREAM:
        case E_X_MEDIA_CAPTION_STREAM:
        {
			return pPlayList->sVarXMediaStreamAttr.ulDefault*100 + pPlayList->sVarXMediaStreamAttr.ulAutoSelect*10 + pPlayList->sVarXMediaStreamAttr.ulForced;
		}
    }
	return 0;
}

unsigned int       C_M3U_Manager::GetMainStreamArray(S_PLAYLIST_NODE**  pPlayListNodeArray, unsigned int ulArrayMaxSize, unsigned int*   pulArraySize)
{
    S_PLAYLIST_NODE*   pPlayListNode = NULL;
	unsigned int             ulPlayListCount = 0;
    if(pPlayListNodeArray == NULL || pulArraySize == NULL)
    {
		return HLS_ERR_EMPTY_POINTER;
    }

	pPlayListNode = m_pPlayListNodeHeader;
	while(pPlayListNode != NULL)
	{
		if(pPlayListNode->ePlayListType == E_MAIN_STREAM)
        {
			if(ulPlayListCount<ulArrayMaxSize)
			{
				pPlayListNodeArray[ulPlayListCount] = pPlayListNode;
				ulPlayListCount++;
			}
			else
			{
				//VOLOGE("The MainStream Count is larger than he Array Size!");
				ulPlayListCount++;
			}
		}

		pPlayListNode = pPlayListNode->pNext;
	}

    *pulArraySize = ulPlayListCount;
    return 0;
}

unsigned int       C_M3U_Manager::GetMainStreamCount(unsigned int*   pulArraySize)
{
	S_PLAYLIST_NODE*   pPlayListNode = NULL;
	unsigned int             ulPlayListCount = 0;
	if(pulArraySize == NULL)
	{
		return HLS_ERR_EMPTY_POINTER;
	}

	pPlayListNode = m_pPlayListNodeHeader;
	while(pPlayListNode != NULL)
	{
		if(pPlayListNode->ePlayListType == E_MAIN_STREAM)
		{
			ulPlayListCount++;
		}

		pPlayListNode = pPlayListNode->pNext;
	}

	*pulArraySize = ulPlayListCount;
	return 0;
}

unsigned int       C_M3U_Manager::GetXMediaStreamArrayWithGroupAndType(S_PLAYLIST_NODE**  pPlayListNodeArray, char* pGroupId, E_PLAYLIST_TYPE  ePlayListType, unsigned int ulArrayMaxSize, unsigned int*   pulArraySize)
{
	S_PLAYLIST_NODE*   pPlayListNode = NULL;
	unsigned int             ulPlayListCount = 0;
	if(pulArraySize == NULL)
	{
		return HLS_ERR_EMPTY_POINTER;
	}

	pPlayListNode = m_pPlayListNodeHeader;
	while(pPlayListNode != NULL)
	{
		if(pPlayListNode->ePlayListType == ePlayListType && strcmp(pPlayListNode->sVarXMediaStreamAttr.strGroupId, pGroupId) == 0)
		{
			if(ulPlayListCount<ulArrayMaxSize)
			{
				pPlayListNodeArray[ulPlayListCount] = pPlayListNode;
			}
			ulPlayListCount++;
		}

		pPlayListNode = pPlayListNode->pNext;
	}

	*pulArraySize = ulPlayListCount;
	return 0;
}

unsigned int       C_M3U_Manager::GetXMediaStreamCountWithGroupAndType(char* pGroupId, E_PLAYLIST_TYPE  ePlayListType, unsigned int*   pulArraySize)
{
	S_PLAYLIST_NODE*   pPlayListNode = NULL;
	unsigned int             ulPlayListCount = 0;
	if(pulArraySize == NULL)
	{
		return HLS_ERR_EMPTY_POINTER;
	}

	pPlayListNode = m_pPlayListNodeHeader;
	while(pPlayListNode != NULL)
	{
		if(pPlayListNode->ePlayListType == ePlayListType && strcmp(pPlayListNode->sVarXMediaStreamAttr.strGroupId, pGroupId) == 0)
		{
			ulPlayListCount++;
		}

		pPlayListNode = pPlayListNode->pNext;
	}

	*pulArraySize = ulPlayListCount;
	return 0;
}

unsigned int       C_M3U_Manager::GetCurrentChunk(E_PLAYLIST_TYPE  ePlayListType, S_CHUNCK_ITEM*   pChunkItems)
{
    unsigned int   ulRet = 0;
	unsigned int   ulSequenceId = 0;
    S_PLAYLIST_NODE*   pPlayListNode = NULL;
	switch(ePlayListType)
    {
	    case E_MAIN_STREAM:
		{
			pPlayListNode = m_sPlaySession.pStreamPlayListNode;
			ulSequenceId = m_sPlaySession.ulMainStreamSequenceId;
			break;
		}
		case E_X_MEDIA_AUDIO_STREAM:
		{
            pPlayListNode = m_sPlaySession.pAlterAudioPlayListNode;
            ulSequenceId = m_sPlaySession.ulAlterAudioSequenceId;
            break;
		}
		case E_X_MEDIA_VIDEO_STREAM:
		{
            pPlayListNode = m_sPlaySession.pAlterVideoPlayListNode;
            ulSequenceId = m_sPlaySession.ulAlterVideoSequenceId;
            break;
		}
		case E_X_MEDIA_SUBTITLE_STREAM:
		{
            pPlayListNode = m_sPlaySession.pAlterSubTitlePlayListNode;
            ulSequenceId = m_sPlaySession.ulAlterSubTitleSequenceId;
            break;
		}
		case E_I_FRAME_STREAM:
		{
            pPlayListNode = m_sPlaySession.pIFramePlayListNode;
            ulSequenceId = m_sPlaySession.ulIFrameSequenceId;
            break;
		}
    }

    ulRet = GetChunkItem(pPlayListNode, pChunkItems, ulSequenceId);
    if(ulRet == 0)
	{
		switch(ePlayListType)
		{
		    case E_MAIN_STREAM:
			{
				pChunkItems->eChunkState = m_sPlaySession.eMainStreamInAdaptionStreamState;

                if(m_sPlaySession.ulCurrentDirectionForMainStream == NORMAL_DIRECTION)
                {
                    if(m_sPlaySession.eMainStreamInAdaptionStreamState != E_CHUNCK_NORMAL)
                    {
                        m_sPlaySession.eMainStreamInAdaptionStreamState = E_CHUNCK_NORMAL;
                    }
                    m_sPlaySession.ulMainStreamSequenceId = pChunkItems->ulSequenceIDForKey+1;
                }
                else
                {
                    m_sPlaySession.ulMainStreamSequenceId = pChunkItems->ulSequenceIDForKey-1;                    
                }
				break;
			}
			case E_X_MEDIA_AUDIO_STREAM:
            {
				pChunkItems->eChunkState = m_sPlaySession.eAlterAudioInAdaptionStreamState;
				if(m_sPlaySession.eAlterAudioInAdaptionStreamState != E_CHUNCK_NORMAL)
				{
					m_sPlaySession.eAlterAudioInAdaptionStreamState = E_CHUNCK_NORMAL;
				}
				m_sPlaySession.ulAlterAudioSequenceId = pChunkItems->ulSequenceIDForKey+1;
				break;
			}
			case E_X_MEDIA_VIDEO_STREAM:
			{
				pChunkItems->eChunkState = m_sPlaySession.eAlterVideoInAdaptionStreamState;
				if(m_sPlaySession.eAlterVideoInAdaptionStreamState != E_CHUNCK_NORMAL)
				{
					m_sPlaySession.eAlterVideoInAdaptionStreamState = E_CHUNCK_NORMAL;
				}
				m_sPlaySession.ulAlterVideoSequenceId = pChunkItems->ulSequenceIDForKey+1;
				break;
			}
			case E_X_MEDIA_SUBTITLE_STREAM:
			{
				pChunkItems->eChunkState = m_sPlaySession.eAlterSubTitleInAdaptionStreamState;
				if(m_sPlaySession.eAlterSubTitleInAdaptionStreamState != E_CHUNCK_NORMAL)
				{
					m_sPlaySession.eAlterSubTitleInAdaptionStreamState = E_CHUNCK_NORMAL;
				}
				m_sPlaySession.ulAlterSubTitleSequenceId = pChunkItems->ulSequenceIDForKey+1;
				break;
			}
			case E_I_FRAME_STREAM:
			{
                if(m_sPlaySession.ulCurrentDirectionForIFrameOnly == NORMAL_DIRECTION)
                {
                    m_sPlaySession.ulIFrameSequenceId = pChunkItems->ulSequenceIDForKey+1;
                }
                else
                {
                    m_sPlaySession.ulIFrameSequenceId = pChunkItems->ulSequenceIDForKey-1;
                }
				break;
			}
		}
	}

	return ulRet;
}

unsigned int       C_M3U_Manager::GetChunkItem(S_PLAYLIST_NODE*  pPlayListNode, S_CHUNCK_ITEM*   pChunkItem, unsigned int ulSeqenceId)
{
	unsigned int           ulRet = 0;
    S_CHUNCK_ITEM*   pChuckItemInList = NULL;
	if(pPlayListNode == NULL || pChunkItem == NULL)
    {
		return HLS_ERR_EMPTY_POINTER;
	}

    QCLOGI("the PlayList Id:%d, the sequenceId:%d", pPlayListNode->ulPlayListId, ulSeqenceId);
    pChuckItemInList = pPlayListNode->pChunkItemHeader;
    while(pChuckItemInList != NULL)
    {
		if(pChuckItemInList->ulSequenceIDForKey >= ulSeqenceId)
		{
			break;
		}

		pChuckItemInList = pChuckItemInList->pNext;
    }

	if(pChuckItemInList == NULL)
    {
		QCLOGI("Can't get the sequence id:%d in playlist:%d", ulSeqenceId, pPlayListNode->ulPlayListId);
		switch(pPlayListNode->eChuckPlayListType)
		{
		    case M3U_VOD:
			{
                ulRet = HLS_ERR_VOD_END;
				break;
			}

			case M3U_LIVE:
			case M3U_EVENT:
			{
				ulRet = HLS_PLAYLIST_END;
				break;
			}
		}
		return ulRet;
    }

    memset(pChunkItem, 0, sizeof(S_CHUNCK_ITEM));
    memcpy(pChunkItem, pChuckItemInList, sizeof(S_CHUNCK_ITEM));
    return 0;
}

unsigned int       C_M3U_Manager::FindPosInPlayList(unsigned int  ulTimeOffset, S_PLAYLIST_NODE*   pPlayList, unsigned int*  pulNewSequenceId, unsigned int* pNewOffset, E_ADAPTIVESTREAMPARSER_SEEK_MODE_FLAG eSeekMode)
{
    unsigned int   ulTotalTime = 0;
    unsigned int   ulSequenceNum = 0;
	unsigned int   ulLastChunkDuration = 0;
    S_CHUNCK_ITEM*    pChunkItem = NULL;
    bool           bFindPos = false;
    
    if(pPlayList == NULL || pulNewSequenceId == NULL || pNewOffset == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }

    pChunkItem = pPlayList->pChunkItemHeader;
    ulSequenceNum = pChunkItem->ulSequenceIDForKey;
    while(pChunkItem != NULL)
    {
		ulLastChunkDuration = pChunkItem->ulDurationInMsec;
		if((ulTotalTime+pChunkItem->ulDurationInMsec)>ulTimeOffset)
        {
            bFindPos = true;            
            ulSequenceNum = pChunkItem->ulSequenceIDForKey;
            break;
        }

        ulTotalTime += pChunkItem->ulDurationInMsec;
        pChunkItem = pChunkItem->pNext;
    }

    if(bFindPos == true)
    {
        *pulNewSequenceId = ulSequenceNum;
        *pNewOffset = ulTotalTime;
        QCLOGI("Set PlayList:%d to the SequenceId:%d", pPlayList->ulPlayListId, ulSequenceNum);
		if (eSeekMode == E_ADAPTIVESTREAMPARSER_SEEK_MODE_FOR_SWITCH_STREAM)
		{
			QCLOGI("input pos:%d, first seek pos:%d, last duration:%d", ulTimeOffset, ulTotalTime, ulLastChunkDuration);
			// 20% duration position is the threshold 
			if ((ulTimeOffset - ulTotalTime) >= (ulLastChunkDuration /5))
			{
				*pulNewSequenceId = ulSequenceNum+1;
				*pNewOffset = ulTotalTime + ulLastChunkDuration;
			}
		}

        return 0;
    }
    else
    {
        QCLOGI("TimeOffset:%d beyond the PlayList Duration:", ulTimeOffset, pPlayList->ulCurrentDvrDuration);
        return HLS_ERR_FAIL;
    }
}

S_PLAYLIST_NODE*       C_M3U_Manager::FindTargetPlayListWithTrackTypeAndId(E_ADAPTIVESTREAMPARSER_TRACK_TYPE nType, unsigned int ulTrackId)
{
    char*   pDescGroup = NULL;
    E_PLAYLIST_TYPE  ePlayListType = E_MAIN_STREAM;
    
    S_PLAYLIST_NODE*   pStreamPlayList = NULL;    
    S_PLAYLIST_NODE*   pXMediaPlayList = NULL;

    pStreamPlayList = m_sPlaySession.pStreamPlayListNode;
    if(pStreamPlayList->ulPlayListId == ulTrackId)
    {
        switch(nType)
        {
            case E_ADAPTIVE_TT_AUDIOGROUP:
            case E_ADAPTIVE_TT_AUDIO:
            {
                pDescGroup = pStreamPlayList->sVarMainStreamAttr.strAudioAlterGroup;
                ePlayListType = E_X_MEDIA_AUDIO_STREAM;
                break;
            }
            case E_ADAPTIVE_TT_VIDEOGROUP:
            case E_ADAPTIVE_TT_VIDEO:
            {            
                pDescGroup = pStreamPlayList->sVarMainStreamAttr.strVideoAlterGroup;
                ePlayListType = E_X_MEDIA_VIDEO_STREAM;
                break;
            }
            case E_ADAPTIVE_TT_SUBTITLEGROUP:
            case E_ADAPTIVE_TT_SUBTITLE: 
            {
                pDescGroup = pStreamPlayList->sVarMainStreamAttr.strSubTitleAlterGroup;                
                ePlayListType = E_X_MEDIA_SUBTITLE_STREAM;
                break;
            }
            default:
            {
                return NULL;
            }
        }

        pXMediaPlayList = m_pPlayListNodeHeader;
        while(pXMediaPlayList != NULL)
        {
            if(pXMediaPlayList->ePlayListType == ePlayListType &&
               strcmp(pDescGroup, pXMediaPlayList->sVarXMediaStreamAttr.strGroupId)== 0 &&
               strlen(pXMediaPlayList->strShortURL) == 0)
            {
                return pXMediaPlayList;
            }

            pXMediaPlayList = pXMediaPlayList->pNext;
        }

        return pXMediaPlayList;
    }
    else
    {
        pXMediaPlayList = FindPlayListById(ulTrackId);
        return pXMediaPlayList;
    }
}

S_PLAYLIST_NODE*       C_M3U_Manager::FindTheFirstMainStream()
{
    S_PLAYLIST_NODE*   pPlayList = NULL;
    pPlayList = m_pPlayListNodeHeader;

    while(pPlayList != NULL)
    {
        if(pPlayList->ePlayListType == E_MAIN_STREAM)
        {
            return pPlayList;
        }

        pPlayList = pPlayList->pNext;
    }

    return NULL;
}

void       C_M3U_Manager::ResetPlayListContentForLiveUpdate(S_PLAYLIST_NODE* pPlayList)
{
    if(pPlayList == NULL)
    {
        return;
    }

    pPlayList->ulItemCount = 0;
    pPlayList->ulCurrentMinSequenceIdInDvrWindow = 0;
    pPlayList->ulCurrentMaxSequenceIdInDvrWindow = 0;
    pPlayList->ulCurrentDvrDuration = 0;
    pPlayList->ulTargetDuration = 0;
    pPlayList->ulXStartExist = 0;
    pPlayList->ilXStartValue = 0;	
}

unsigned int       C_M3U_Manager::GetPlayListStartOffset(S_PLAYLIST_NODE* pPlayList)
{
    unsigned int  ulOffset = 0;
    if(pPlayList == NULL)
    {
        return 0;
    }
    
    if(pPlayList->ulXStartExist > 0)
    {
        if(pPlayList->ilXStartValue > 0)
        {
            ulOffset = pPlayList->ilXStartValue;
        }
        else
        {
            if((unsigned int)(-pPlayList->ilXStartValue)>pPlayList->ulCurrentDvrDuration)
            {
                ulOffset = 0;
            }
            else
            {
                ulOffset = pPlayList->ulCurrentDvrDuration+pPlayList->ilXStartValue;         
            }
        }
    }
    else
    {
        if(pPlayList->ulCurrentDvrDuration > (2*pPlayList->ulTargetDuration))
        {
            ulOffset = pPlayList->ulCurrentDvrDuration-(2*pPlayList->ulTargetDuration);
        }
        else
        {
            ulOffset = 0;
        }
    }

    return ulOffset;
}

unsigned int       C_M3U_Manager::GetCurrentSessionDurationByChapterId(unsigned int uChapterId, unsigned int*   pTimeOutput)
{
    if(m_sPlaySession.pStreamPlayListNode == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }
    else
    {
        return GetPlayListDurationByChapterId(m_sPlaySession.pStreamPlayListNode, uChapterId, pTimeOutput);
    }
}

unsigned int       C_M3U_Manager::GetPlayListDurationByChapterId(S_PLAYLIST_NODE* pPlayList, unsigned int uChapterId, unsigned int*   pTimeOutput)
{
    unsigned int          ulTotalTime = 0;
    S_CHUNCK_ITEM*  pChunkItem = NULL; 
    if(pPlayList == NULL || pTimeOutput == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }

    QCLOGI("PlayList Id:%d, ChapterId:%d", pPlayList->ulPlayListId, uChapterId);
    pChunkItem = pPlayList->pChunkItemHeader;
    while(pChunkItem != NULL)
    {
        if(pChunkItem->ulDisSequenceId <= uChapterId)
        {
            ulTotalTime +=  pChunkItem->ulDurationInMsec;
        }
        else
        {
            break;
        }
        pChunkItem = pChunkItem->pNext;
    }

    *pTimeOutput = ulTotalTime;
    return 0;
}

S_SESSION_CONTEXT*       C_M3U_Manager::GetSessionContext()
{
    return &m_sSessionContext;
}

void       C_M3U_Manager::PrepareSessionByMainStreamDefaultSetting(S_PLAYLIST_NODE* pPlayList)
{
	if(pPlayList == NULL ||  pPlayList->ePlayListType != E_MAIN_STREAM)
	{
		return;
	}

	if(strlen(pPlayList->sVarMainStreamAttr.strAudioAlterGroup) != 0)
	{
		m_sPlaySession.pAlterAudioPlayListNode = FindPreferXMediaPlayListInGroup(pPlayList->sVarMainStreamAttr.strAudioAlterGroup, E_X_MEDIA_AUDIO_STREAM);
	}

	if(strlen(pPlayList->sVarMainStreamAttr.strVideoAlterGroup) != 0)
	{
		m_sPlaySession.pAlterVideoPlayListNode = FindPreferXMediaPlayListInGroup(pPlayList->sVarMainStreamAttr.strVideoAlterGroup, E_X_MEDIA_VIDEO_STREAM);
	}

	if(strlen(pPlayList->sVarMainStreamAttr.strSubTitleAlterGroup) != 0)
	{
		m_sPlaySession.pAlterSubTitlePlayListNode = FindPreferXMediaPlayListInGroup(pPlayList->sVarMainStreamAttr.strSubTitleAlterGroup, E_X_MEDIA_SUBTITLE_STREAM);
	}

    if(pPlayList->ulExtraIFramePlayListId != 0)
    {
		m_sPlaySession.pIFramePlayListNode = FindPlayListById(pPlayList->ulExtraIFramePlayListId);    
    }
}

unsigned int       C_M3U_Manager::AdjustSequenceIdInSession()
{
    if(m_sPlaySession.pStreamPlayListNode != NULL)
    {
        AdjustSequenceIdByPlayListContext(E_MAIN_STREAM, m_sPlaySession.pStreamPlayListNode);
    }
    
    if(m_sPlaySession.pAlterAudioPlayListNode!= NULL)
    {
        AdjustSequenceIdByPlayListContext(E_X_MEDIA_AUDIO_STREAM, m_sPlaySession.pAlterAudioPlayListNode);
    }
    
    if(m_sPlaySession.pAlterVideoPlayListNode!= NULL)
    {
        AdjustSequenceIdByPlayListContext(E_X_MEDIA_VIDEO_STREAM, m_sPlaySession.pAlterVideoPlayListNode);
    }
    
    if(m_sPlaySession.pAlterSubTitlePlayListNode!= NULL)
    {
        AdjustSequenceIdByPlayListContext(E_X_MEDIA_SUBTITLE_STREAM, m_sPlaySession.pAlterSubTitlePlayListNode);
    }
    
    return 0;
}

unsigned int       C_M3U_Manager::AdjustSequenceIdByPlayListContext(E_PLAYLIST_TYPE  ePlayListType, S_PLAYLIST_NODE*  pPlayList)
{
    unsigned int*   pulCurrentSequenceId = NULL;

    if(pPlayList == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }

    if(ePlayListType != pPlayList->ePlayListType || strlen(pPlayList->strShortURL) == 0)
    {
        return HLS_ERR_FAIL;
    }
    switch(ePlayListType)
    {
        case E_MAIN_STREAM:
        {
            pulCurrentSequenceId = &(m_sPlaySession.ulMainStreamSequenceId);
            break;
        }
        case E_X_MEDIA_AUDIO_STREAM:
        {
            pulCurrentSequenceId = &(m_sPlaySession.ulAlterAudioSequenceId);
            break;
        }
        case E_X_MEDIA_VIDEO_STREAM:
        {
            pulCurrentSequenceId = &(m_sPlaySession.ulAlterVideoSequenceId);
            break;
        }
        case E_X_MEDIA_SUBTITLE_STREAM:
        {
            pulCurrentSequenceId = &(m_sPlaySession.ulAlterSubTitleSequenceId);            
            break;
        }
        default:
        {
            return HLS_ERR_FAIL;
        }
    }

    QCLOGI("PlayList Id:%d, Current Sequence Id:%d, Current PlayList Min Sequence:%d, Current PlayList Max Sequence:%d", 
           pPlayList->ulPlayListId, *pulCurrentSequenceId, pPlayList->ulCurrentMinSequenceIdInDvrWindow, pPlayList->ulCurrentMaxSequenceIdInDvrWindow);
    if((*pulCurrentSequenceId) < pPlayList->ulCurrentMinSequenceIdInDvrWindow)
    {
        *pulCurrentSequenceId = pPlayList->ulCurrentMinSequenceIdInDvrWindow;
    }

    //Max SequenceId + PlayList Item Count
    if((*pulCurrentSequenceId) > (2*pPlayList->ulCurrentMaxSequenceIdInDvrWindow-pPlayList->ulCurrentMinSequenceIdInDvrWindow))
    {
        *pulCurrentSequenceId = pPlayList->ulCurrentMaxSequenceIdInDvrWindow-1;
    }

    return 0;
}

unsigned int    C_M3U_Manager::SeekForOneTrackOnly(unsigned int  ulPlaylistId, unsigned long long ullTimeOffset, unsigned int* pNewOffset)
{
    S_PLAYLIST_NODE*   pPlaylistNode = NULL;
	S_PLAYLIST_NODE*   pCurWorkPlaylistNode = NULL;
	unsigned int             ulOffset = 0;
    unsigned int             ulNewSequenceId = 0;
    unsigned int             ulNewOffset = 0;
    unsigned int             ulRet = 0;
    
    if(pNewOffset == NULL)
    {
		return HLS_ERR_EMPTY_POINTER;
    }

	pPlaylistNode = FindPlayListById(ulPlaylistId);
    if(pPlaylistNode == NULL)
	{
		return HLS_ERR_FAIL;
	}

	switch(pPlaylistNode->ePlayListType)
	{
	case E_X_MEDIA_AUDIO_STREAM:
		{
			pCurWorkPlaylistNode = m_sPlaySession.pAlterAudioPlayListNode;
		}
		break;

	case E_X_MEDIA_VIDEO_STREAM:
		{
			pCurWorkPlaylistNode = m_sPlaySession.pAlterVideoPlayListNode;
		}
		break;

	case E_X_MEDIA_SUBTITLE_STREAM:
		{
			pCurWorkPlaylistNode = m_sPlaySession.pAlterSubTitlePlayListNode;
		}
		break;

	default:
		{
			QCLOGI("Invalid PlayList type value:%d", pPlaylistNode->ePlayListType);
			return HLS_ERR_FAIL;
		}
	}

	if(pPlaylistNode != pCurWorkPlaylistNode)
    {
		QCLOGI("PlayList:%d doesn't work now", pPlaylistNode->ulPlayListId);
		return HLS_ERR_FAIL;
	}

    if(strlen(pPlaylistNode->strShortURL) == 0)
    {
		QCLOGI("The PlayList:%d media data in Muxed Stream!");
		return 0;
    }

	ulOffset = (unsigned int)ullTimeOffset;
	ulRet = FindPosInPlayList(ulOffset, pPlaylistNode, &ulNewSequenceId, &ulNewOffset, E_ADAPTIVESTREAMPARSER_SEEK_MODE_NORMAL);
    if(ulRet != 0)
	{
		return HLS_ERR_FAIL;
	}

	switch(pPlaylistNode->ePlayListType)
	{
	    case E_X_MEDIA_AUDIO_STREAM:
		{
			m_sPlaySession.ulAlterAudioSequenceId = ulNewSequenceId;
			m_sPlaySession.eAlterAudioInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;
			break;
		}
	    case E_X_MEDIA_SUBTITLE_STREAM:
		{
			m_sPlaySession.ulAlterSubTitleSequenceId = ulNewSequenceId;
			m_sPlaySession.eAlterSubTitleInAdaptionStreamState = E_CHUNCK_FORCE_NEW_STREAM;
			break;
		}
	    default:
		{
			break;
		}
	}

	return 0;
}

unsigned int      C_M3U_Manager::GetSegmentCountByASId(unsigned int  ulASId, unsigned int* pSegmentCount)
{
    S_PLAYLIST_NODE*   pPlayListNode = NULL;
    if(pSegmentCount == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }

    pPlayListNode = FindPlayListById(ulASId);
    if(pPlayListNode == NULL)
    {
        return HLS_ERR_FAIL;
    }

    *pSegmentCount = pPlayListNode->ulCurrentMaxSequenceIdInDvrWindow-pPlayListNode->ulCurrentMinSequenceIdInDvrWindow+1;
    return 0;
}

unsigned int      C_M3U_Manager::GetCurrentIFrameOnlyPlaylistId(unsigned int* pIFrameASId)
{
    S_PLAYLIST_NODE* pPlayListNode = NULL;
    if(m_sPlaySession.pStreamPlayListNode == NULL || pIFrameASId == NULL)
    {
        return HLS_ERR_UNKNOWN;
    }

    if(m_sPlaySession.pStreamPlayListNode->ulExtraIFramePlayListId == 0)
    {
        return HLS_ERR_UNKNOWN;
    }

    *pIFrameASId = m_sPlaySession.pStreamPlayListNode->ulExtraIFramePlayListId;
    return 0;
}

void      C_M3U_Manager::AddKeyTagNodeToKeyList(S_KEY_TAG_ARRAY*   pKeyArray, S_TAG_NODE*  pNewKeyTag)
{
    unsigned int   ulIndex = 0;
    bool  bReplaced = false;
    if(pKeyArray == NULL || pNewKeyTag == NULL)
    {
        return;
    }

	for(ulIndex=0; ulIndex<pKeyArray->ulCurrentKeyTagCount; ulIndex++)
    {
        if(pKeyArray->pKeyTagArray[ulIndex] != NULL && pKeyArray->pKeyTagArray[ulIndex]->ppAttrArray != NULL)
        {
            if(pKeyArray->pKeyTagArray[ulIndex]->ppAttrArray[KEY_METHOD_KEYFORMAT] == NULL)
            {
                if(pNewKeyTag->ppAttrArray != NULL && pNewKeyTag->ppAttrArray[KEY_METHOD_KEYFORMAT] == NULL)
                {
                    pKeyArray->pKeyTagArray[ulIndex] = pNewKeyTag;
                    bReplaced = true;
                    break;
                }
            }
            else
            {
                if(pKeyArray->pKeyTagArray[ulIndex]->ppAttrArray[KEY_METHOD_KEYFORMAT]->pString != NULL)
                {
                    if(pNewKeyTag->ppAttrArray != NULL && pNewKeyTag->ppAttrArray[KEY_METHOD_KEYFORMAT] != NULL &&
                       pNewKeyTag->ppAttrArray[KEY_METHOD_KEYFORMAT]->pString != NULL &&
                       strcmp(pKeyArray->pKeyTagArray[ulIndex]->ppAttrArray[KEY_METHOD_KEYFORMAT]->pString, pNewKeyTag->ppAttrArray[KEY_METHOD_KEYFORMAT]->pString) == 0)
                    {
                        pKeyArray->pKeyTagArray[ulIndex] = pNewKeyTag;                    
                        bReplaced = true;
                        break;
                    }            
                
                }
            }
        }
    }

    if(bReplaced == false)
    {
        if(pKeyArray->ulCurrentKeyTagCount < MAX_KEY_ENTRY_COUNT)
        {
            pKeyArray->pKeyTagArray[pKeyArray->ulCurrentKeyTagCount] = pNewKeyTag;
            pKeyArray->ulCurrentKeyTagCount++;
        }
    }

    return;
}

void      C_M3U_Manager::GenerateCombinedKeyLineContent(S_KEY_TAG_ARRAY*   pKeyArray, char*  pKeyContent, unsigned int  ulKeyContentMax)
{
    char*   pLineContent = NULL;
    unsigned int  ulIndex = 0;
    unsigned int  ulCurrentLength = 0;
    if(pKeyArray == NULL || pKeyContent == NULL)
    {
        return ;
    }

    for(ulIndex=0; ulIndex<pKeyArray->ulCurrentKeyTagCount; ulIndex++)
    {
        if(pKeyArray->pKeyTagArray[ulIndex] != NULL && pKeyArray->pKeyTagArray[ulIndex]->ppAttrArray != NULL &&
           pKeyArray->pKeyTagArray[ulIndex]->ppAttrArray[KEY_LINE_CONTENT] != NULL &&
           pKeyArray->pKeyTagArray[ulIndex]->ppAttrArray[KEY_LINE_CONTENT]->pString != NULL)
        {
            if((ulCurrentLength + strlen(pKeyArray->pKeyTagArray[ulIndex]->ppAttrArray[KEY_LINE_CONTENT]->pString) +2) < ulKeyContentMax)
            {
                pLineContent = pKeyArray->pKeyTagArray[ulIndex]->ppAttrArray[KEY_LINE_CONTENT]->pString;
                memcpy(pKeyContent+ulCurrentLength, pLineContent, strlen(pLineContent));
                ulCurrentLength += strlen(pLineContent);
                memcpy(pKeyContent+ulCurrentLength, "\r\n", 2);
                ulCurrentLength += 2;
            }
        }
    }

    if(ulCurrentLength>2)
    {
        pKeyContent[ulCurrentLength-1] = '\0';        
        pKeyContent[ulCurrentLength-2] = '\0';
    }

    return ;
}

unsigned int      C_M3U_Manager::GetUTCTimeFromString(char*   pProgramDateTimeString, unsigned long long*  pUTCTimeValue)
{
    int   iYear = 0;
    int   iMonth = 0;
    int   iDay = 0;
    int   iHour = 0;
    int   iMin = 0;
    int   iSecond = 0;
    int   iMSecond = 0;
    unsigned long long  ulUTCTime = 0;

    if(pProgramDateTimeString == NULL || pUTCTimeValue == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }

    //#EXT-X-PROGRAM-DATE-TIME:2010-02-19T14:54:23.031+08:00
    sscanf(pProgramDateTimeString, "%d-%d-%dT%d:%d:%d.%d", 
           &iYear, &iMonth, &iDay, &iHour, &iMin, &iSecond, &iMSecond);

    if(iMonth>12)
    {
        return HLS_ERR_WRONG_MANIFEST_FORMAT;
    }

    
    QCLOGI("iYear:%d, iMonth:%d, iDay:%d, iHour:%d, iMin:%d, iSecond:%d", 
            iYear, iMonth, iDay, iHour, iMin, iSecond);
    ulUTCTime = (unsigned long long)GetUTCTime((unsigned int)iYear, (unsigned int)iMonth, (unsigned int)iDay, (unsigned int)iHour, (unsigned int)iMin, (unsigned int)iSecond);
    QCLOGI("UTC Second:%llu", ulUTCTime);
    *pUTCTimeValue = (ulUTCTime*1000);
    return 0;
}

unsigned long long      C_M3U_Manager::GetUTCTime(unsigned int iYear, unsigned int iMonth, unsigned int iDay, unsigned int iHour, unsigned int iMin, unsigned int iSecond)
{
    unsigned int iDayTotal = 0;
	unsigned long long iTimeSecond = 0;
	unsigned int   iYearTotalDays[4] = {0, 365, 365+365, 365+365+366};
	unsigned int   iMonthTotalDays[12] = {0, 31, 31+28, 31+28+31,
		31+28+31+30, 31+28+31+30+31, 31+28+31+30+31+30,31+28+31+30+31+30+31,
		31+28+31+30+31+30+31+31 ,31+28+31+30+31+30+31+31+30, 31+28+31+30+31+30+31+31+30+31,31+28+31+30+31+30+31+31+30+31+30};
	unsigned int   iIsLeap[4] = {0, 0, 1, 0};


	iDayTotal = (iYear-1970)/4*(365+365+366+365) + iYearTotalDays[(iYear-1970)%4] + iMonthTotalDays[iMonth-1]+iIsLeap[(iYear-1970)%4] + (iDay-1);
	iTimeSecond = (iDayTotal*86400) + iHour*3600 + iMin*60 + iSecond;
	return iTimeSecond;
}
