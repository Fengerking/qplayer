#include "string.h"
#include "CHls_entity.h"
#include "USystemFunc.h"
#include "ULogFunc.h"
#include "CDataBox.h"
#include "qcErr.h"

C_HLS_Entity::C_HLS_Entity(CBaseInst * pBaseInst)
	: CBaseObject(pBaseInst)
	, m_sM3UManager(pBaseInst)
{
	SetObjectName("C_HLS_Entity");
    m_pEventCallbackFunc = NULL;    
    memset(&m_sChunkContainer, 0, sizeof(S_CHUNK_CONTAINER));    
    m_bUpdateRunning = false;
    memset(&m_sCurrentAdaptiveStreamItemForPlayList, 0, sizeof(S_ADAPTIVESTREAM_PLAYLISTDATA));
	m_eProgramType = E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_UNKNOWN;
	m_ppStreamArray = NULL;
	m_ulStreamCount = 0;	
}

C_HLS_Entity::~C_HLS_Entity()
{
    ResetAllContext();
}

unsigned int     C_HLS_Entity::Init_HLS(S_ADAPTIVESTREAM_PLAYLISTDATA* pData, S_SOURCE_EVENTCALLBACK*   pEventCallback)
{
    unsigned int   ulRet = 0;
    m_pEventCallbackFunc = pEventCallback;
    ulRet = CommitPlayListData(pData);
    return ulRet;
}
    
unsigned int     C_HLS_Entity::Uninit_HLS()
{
    unsigned int   ulRet = 0;
    ResetAllContext();
    return ulRet;
}
    
unsigned int     C_HLS_Entity::Close_HLS()
{
    unsigned int    ulRet = 0;
    ResetAllContext();
    StopPlaylistUpdate();
    return ulRet;
}

unsigned int     C_HLS_Entity::Start_HLS()
{
    unsigned int   ulRet = 0;
    S_PLAY_SESSION*    pPlaySession = NULL;
    unsigned int   ulIndex = 0;

    if(m_sM3UManager.IsPlaySessionReady() != true)
	{
        ulRet = PreparePlaySession();
        if(ulRet != 0)
        {
            return QC_ERR_FAILED;
        }
	}

	ulRet = m_sM3UManager.GetCurReadyPlaySession(&pPlaySession);
	if(ulRet == 0)
	{
		switch(pPlaySession->pStreamPlayListNode->eChuckPlayListType)
		{
		    case M3U_LIVE:
			{
				m_eProgramType = E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_LIVE;
                m_sM3UManager.SetStartPosForLiveStream();
                QCLOGI("Current Program is LIVE!");
				break;
			}
		    case M3U_VOD:
			{
				m_eProgramType = E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_VOD;
                QCLOGI("Current Program is VOD!");
				break;
			}
		    default:
			{
				m_eProgramType = E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_UNKNOWN;
				break;
			}
		}
	}

    if(m_eProgramType == E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_LIVE)
    {
        m_sM3UManager.SetStartPosForLiveStream();
    }

	ulRet = m_sM3UManager.GetCurReadyPlaySession(&pPlaySession);
	if(ulRet == 0)
	{    
		if(pPlaySession!= NULL && pPlaySession->pStreamPlayListNode != NULL && pPlaySession->pStreamPlayListNode->eChuckPlayListType == M3U_LIVE)
		{
			//begin();
		}
	}

    QCLOGI("Set the Program Changed!");
    return ulRet;
}

unsigned int     C_HLS_Entity::Stop_HLS()
{
    unsigned int   ulRet = 0;
    StopPlaylistUpdate();
    return ulRet;
}

/*
void    C_HLS_Entity::begin()
{
	m_bUpdateRunning = true;
	vo_thread::begin();
}

void   C_HLS_Entity::thread_function()
{
	set_threadname((char*) "Playlist Update" );
    PlayListUpdateForLive();
	QCLOGI( "Update Thread Exit!" );
}
*/

unsigned int     C_HLS_Entity::Open_HLS()
{
    unsigned int   ulRet = 0;
    char  strURL[2048] = {0};
    S_PLAY_SESSION*     pPlaySession = NULL;
    M3U_MANIFEST_TYPE   eRootPlaylistType = M3U_UNKNOWN_PLAYLIST;
    
    ulRet = ParseHLSPlaylist(&m_sCurrentAdaptiveStreamItemForPlayList, HLS_ROOT_MANIFEST_ID);
    if(ulRet != 0)
    {
		QCLOGI("Open Fail!");
		return QC_ERR_FAILED;
	}

	ulRet = PreparePlaySession();
	if (ulRet != 0)
	{
		return QC_ERR_FAILED;
	}

    if(m_sCurrentAdaptiveStreamItemForPlayList.pData != NULL)
    {
        delete[] m_sCurrentAdaptiveStreamItemForPlayList.pData;
        m_sCurrentAdaptiveStreamItemForPlayList.pData = NULL;
    }
    
    ulRet = m_sM3UManager.GetRootManifestType(&eRootPlaylistType);
    if(ulRet != 0)
    {
        return QC_ERR_FAILED;
    }

	m_sM3UManager.GetCurrentProgreamStreamType(&m_eProgramType);
	if (m_eProgramType == E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE_LIVE)
	{
		m_sM3UManager.SetStartPosForLiveStream();
	}


	ulRet = GenerateTheProgramInfo();
    if(ulRet != 0)
    {
        QCLOGE("Generate the Program Info Failed!");
    }
    return ulRet;
}


unsigned int     C_HLS_Entity::GetChunk_HLS(E_ADAPTIVESTREAMPARSER_CHUNKTYPE uID ,  S_ADAPTIVESTREAMPARSER_CHUNK **ppChunk)
{
	CAutoLock lock( &m_sListLock );
    unsigned int   ulRet = QC_ERR_NONE;
    S_PLAYLIST_NODE*   pPlayListNode = NULL;
    E_PLAYLIST_TYPE    ePlayListType = E_UNKNOWN_STREAM;
    bool            bInAdaption = false;
    bool            bDrmEnable = false;

    if(ppChunk == NULL)
    {
        return QC_ERR_EMPTYPOINTOR;
    }

    ulRet = GetChunckItem(uID, ppChunk);
    if(ulRet == 0)
    {
        (*ppChunk)->ullTimeScale = 1000;
    }
    
    if(ulRet == 0)
    {
        if((*ppChunk)->pChunkDRMInfo != NULL)
        {
            bDrmEnable = true;
        }
        
		QCLOGI("the start time:%d, the duration:%d, the drm type:%d, the ulFlag:%d, the deadtime:%lld, the url:%s, the root url:%s, the playlist id:%d, the seq id:%d,the chapter id:%d", (unsigned int)((*ppChunk)->ullStartTime), (unsigned int)((*ppChunk)->ullDuration), bDrmEnable, ((*ppChunk)->ulFlag),
			(*ppChunk)->ullChunkDeadTime, (*ppChunk)->strUrl, (*ppChunk)->strRootUrl, (*ppChunk)->ulPlaylistId, (*ppChunk)->ulChunkID, (*ppChunk)->ulPeriodSequenceNumber);
    }
    
    return ulRet;
}


unsigned int     C_HLS_Entity::ParseHLSPlaylist(void*  pPlaylistData, unsigned int ulPlayListId)
{   
    CAutoLock lock( &m_sListLock );
    unsigned int   ulRet = 0;
    S_ADAPTIVESTREAM_PLAYLISTDATA*   pPlaylistItem = NULL;

    if(pPlaylistData == NULL)
    {
        return QC_ERR_EMPTYPOINTOR;
    }

    pPlaylistItem = (S_ADAPTIVESTREAM_PLAYLISTDATA*)pPlaylistData;
    QCLOGI("NewURL:%s", pPlaylistItem->strNewUrl);
	QCLOGI("URL:%s", pPlaylistItem->strUrl);
    ulRet = m_sM3UManager.ParseManifest(pPlaylistItem->pData, pPlaylistItem->ulDataSize, (char*)pPlaylistItem->strNewUrl, ulPlayListId);            
	return  ulRet;
}

unsigned int     C_HLS_Entity::PreparePlaySession()
{
    unsigned int   ulRet = 0;
    char  strURL[2048] = {0};
	char  strRoot[2048] = {0};
    M3U_MANIFEST_TYPE   ePlaylistType = M3U_UNKNOWN_PLAYLIST;
    S_PLAY_SESSION*     pPlaySession = NULL;
    unsigned int              ulIndex = 0;
    unsigned int              ulCurrentParseTime = 0;
    unsigned int              ulParseRet = 0;
    unsigned int              ulPlayListId = 0xffffffff;
    S_PLAYLIST_NODE*    pPlayListNode = NULL;

    ulRet = m_sM3UManager.GetRootManifestType(&ePlaylistType);
    if(ulRet != 0)
    {
        return QC_ERR_FAILED;
    }

    while(m_sM3UManager.IsPlaySessionReady() == false && ulCurrentParseTime<(HLS_MAX_MANIFEST_RETRY_COUNT))
    {
        switch(ePlaylistType)
        {
            case M3U_CHUNK_PLAYLIST:
            {
                break;
            }
            case M3U_STREAM_PLAYLIST:
            {
                pPlayListNode = m_sM3UManager.GetPlayListNeedParseForSessionReady();
                if(pPlayListNode != NULL)
                {
                    ulParseRet = NotifyToParse(pPlayListNode->strRootURL, pPlayListNode->strShortURL, pPlayListNode->ulPlayListId);
                    ulCurrentParseTime++;
                }
                break;
            }
        }
    }

    return 0;
}

unsigned int    C_HLS_Entity::CommitPlayListData(void*  pPlaylistData)
{
    unsigned int   ulRet = QC_ERR_NONE;
    S_ADAPTIVESTREAM_PLAYLISTDATA*   pPlaylistItem = NULL;
    unsigned char*    pData = NULL;

    if(pPlaylistData == NULL)
    {
        QCLOGI("The pPlaylistData is NULL!");
        return QC_ERR_EMPTYPOINTOR;
    }

    memset(&m_sCurrentAdaptiveStreamItemForPlayList, 0, sizeof(m_sCurrentAdaptiveStreamItemForPlayList));
    pPlaylistItem = (S_ADAPTIVESTREAM_PLAYLISTDATA*)pPlaylistData;
    pData = new unsigned char[pPlaylistItem->ulDataSize+1];
    if(pData == NULL)
    {
        QCLOGI("Lack of memory!");
        return QC_ERR_EMPTYPOINTOR;
    }

	memset(pData, 0, pPlaylistItem->ulDataSize + 1);
	memcpy(pData, pPlaylistItem->pData, pPlaylistItem->ulDataSize);
    m_sCurrentAdaptiveStreamItemForPlayList.pData = pData;
	m_sCurrentAdaptiveStreamItemForPlayList.ulDataSize = pPlaylistItem->ulDataSize;
	memcpy(m_sCurrentAdaptiveStreamItemForPlayList.strRootUrl, pPlaylistItem->strRootUrl, strlen(pPlaylistItem->strRootUrl));
	memcpy(m_sCurrentAdaptiveStreamItemForPlayList.strUrl, pPlaylistItem->strNewUrl, strlen(pPlaylistItem->strNewUrl));
	memcpy(m_sCurrentAdaptiveStreamItemForPlayList.strNewUrl, pPlaylistItem->strNewUrl, strlen(pPlaylistItem->strNewUrl));
    return   ulRet;
}

unsigned int     C_HLS_Entity::Seek_HLS(unsigned long long*  pTimeStamp, E_ADAPTIVESTREAMPARSER_SEEK_MODE_FLAG eSeekMode)
{
	CAutoLock lock( &m_sListLock );
    
    unsigned int     ulRet = 0;
    bool    bNeedResetParser = false;
    unsigned int     ulTimeInput = 0;
    unsigned int     ulTimeChunkOffset = 0;

    if(pTimeStamp == NULL)
    {
        QCLOGI("Input Param Invalid!");
        return QC_ERR_EMPTYPOINTOR;
    }

    ulTimeInput = (unsigned int)(*pTimeStamp);
    
    ulRet = m_sM3UManager.SetThePos(ulTimeInput, &bNeedResetParser, &ulTimeChunkOffset, eSeekMode);
    if(ulRet == 0)
    {
        QCLOGI("Seek time:%d  OK! New Chunk Offset:%d", ulTimeInput, ulTimeChunkOffset);
        *pTimeStamp = ulTimeChunkOffset;
    }
    else
    {
        QCLOGI("Seek time:%d  Failed!", ulTimeInput);
    }

    QCLOGI("return value for seek:%d", ulRet);
	return ulRet;
}
    
unsigned int     C_HLS_Entity::GetDuration_HLS(unsigned long long * pDuration)
{
    unsigned int    ulRet = 0;
    unsigned int    ulDuration = 0;

    if(pDuration == NULL)
    {
        return QC_ERR_EMPTYPOINTOR;
    }
    
    ulRet = m_sM3UManager.GetTheDuration(&ulDuration);
    if(ulRet != 0)
    {
        return QC_ERR_RETRY;
    }

    *pDuration = ulDuration;
    return 0;
}
    
unsigned int     C_HLS_Entity::GetProgramCounts_HLS(unsigned int*  pProgramCounts)
{
    unsigned int    ulRet = 0;
    
    if(pProgramCounts == NULL)
    {
        return QC_ERR_EMPTYPOINTOR;
    }

    *pProgramCounts = 1;
    return 0;
}

unsigned int     C_HLS_Entity::SelectStream_HLS(unsigned int uStreamId, E_ADAPTIVESTREAMING_CHUNKPOS sPrepareChunkPos)
{
    unsigned int   ulRet = 0;
    S_PLAY_SESSION*    pPlaySession = NULL;
    S_PLAYLIST_NODE*   pPlayListNode = NULL;

	QCLOGI("The New Stream Id:%d", uStreamId);
    pPlayListNode = m_sM3UManager.FindPlayListById(uStreamId);
    if(pPlayListNode == NULL)
    {
		QCLOGI("Stream:%d doesn't exist!", uStreamId);
        return QC_ERR_FAILED;
    }

    ulRet = m_sM3UManager.GetCurReadyPlaySession(&pPlaySession);
    if(ulRet == 0)
    {
		if(pPlaySession->pStreamPlayListNode != NULL && pPlaySession->pStreamPlayListNode->ulPlayListId == uStreamId)
        {
			QCLOGI("The Stream:%d already selected!", uStreamId);
			return 0;
        }
    }

    ulRet = NotifyToParse(pPlayListNode->strRootURL, pPlayListNode->strShortURL, pPlayListNode->ulPlayListId);
    if(ulRet != 0)
    {
		QCLOGI("Parse the PlayList:%d fail!", pPlayListNode->ulPlayListId);
        return QC_ERR_FAILED;
    }

    m_sM3UManager.AdjustChunkPosInListForBA(sPrepareChunkPos);
    m_sM3UManager.SetPlayListToSession(uStreamId);
    m_sM3UManager.AdjustXMedia();    
    m_sM3UManager.AdjustIFrameOnly();

    ulRet = PreparePlaySession();
    if(ulRet != 0)
    {
        QCLOGI("Can't start current play session!");
        return QC_ERR_FAILED;
    }
    
    m_sM3UManager.AdjustSequenceIdInSession();
    return 0;
}

unsigned int     C_HLS_Entity::SelectTrack_HLS(E_ADAPTIVESTREAMPARSER_TRACK_TYPE nType, unsigned int nTrackID)
{
	unsigned int   ulRet = 0;
	S_PLAY_SESSION*    pPlaySession = NULL;
	S_PLAYLIST_NODE*   pPlayListNode = NULL;
    unsigned int             ulStreamIndex = 0xffffffff;    
    unsigned int             ulStreamId = 0xffffffff;
    unsigned int             ulTrackIndex = 0;
    unsigned int             ulLoop = 0;
    unsigned int             ulConvertTrackId = nTrackID;
    S_PLAYLIST_NODE*   pCurPlayListWithNewType = NULL;
    S_PLAYLIST_NODE*   pCurMainStreamPlayList = NULL;
    S_PLAYLIST_NODE*   pNewTargetPlayList = NULL;

    ulRet = m_sM3UManager.GetCurReadyPlaySession(&pPlaySession);
    if(ulRet != 0)
    {
        return QC_ERR_FAILED;
    }

	switch(nType)
	{
	    case E_ADAPTIVE_TT_AUDIO:
		{
            pCurPlayListWithNewType = pPlaySession->pAlterAudioPlayListNode;
			break;
		}
		case E_ADAPTIVE_TT_VIDEO:
		{
			pCurPlayListWithNewType = pPlaySession->pAlterVideoPlayListNode;
			break;
		}
		case E_ADAPTIVE_TT_SUBTITLE:
		{
			pCurPlayListWithNewType = pPlaySession->pAlterSubTitlePlayListNode;
			break;
		}
	}

    QCLOGI("new Type:%d, new Id:%d", nType, nTrackID);
    pCurMainStreamPlayList = pPlaySession->pStreamPlayListNode;
    
    if(pCurPlayListWithNewType == NULL)
    {
        QCLOGI("no %d type XMedia Track", nType);
        return 0;
    }

    pNewTargetPlayList = m_sM3UManager.FindTargetPlayListWithTrackTypeAndId(nType, nTrackID);

    if(pNewTargetPlayList == NULL)
    {
        QCLOGI("No target Track!");
        return 0;
    }

    if(pNewTargetPlayList->ulPlayListId == pCurPlayListWithNewType->ulPlayListId)
    {
        QCLOGI("already selected!");
        return 0;
    }
    
    ulRet = NotifyToParse(pNewTargetPlayList->strRootURL, pNewTargetPlayList->strShortURL, pNewTargetPlayList->ulPlayListId);
	if(ulRet != 0)
    {
			QCLOGI("nTrackID:%d parse fail!", pNewTargetPlayList->ulPlayListId);
			return QC_ERR_FAILED;
	}

	m_sM3UManager.SetPlayListToSession(pNewTargetPlayList->ulPlayListId);
    m_sM3UManager.AdjustSequenceIdInSession();
    return 0;
}

unsigned int     C_HLS_Entity::GetParam_HLS(unsigned int nParamID, void* pParam )
{
    unsigned long long*    pullTimeValue = NULL;
    unsigned int     ulRet = 0;

    if(pParam == NULL)
    {
        return QC_ERR_EMPTYPOINTOR;
    }


	return 0;
}

unsigned int     C_HLS_Entity::SetParam_HLS(unsigned int nParamID, void* pParam )
{
    unsigned int     ulRet = 0;
    unsigned long long*    pullTimeValue = NULL;
    
    if(pParam == NULL)
    {
        return QC_ERR_EMPTYPOINTOR;
    }
    return ulRet;
}

unsigned int    C_HLS_Entity::GetProgramInfo_HLS(QC_STREAM_FORMAT**   &ppStreamInfo, int* piStreamCount)
{
	ppStreamInfo = m_ppStreamArray;
	*piStreamCount = (int)m_ulStreamCount;
	return 0;
}

void    C_HLS_Entity::ResetAllContext()
{
    m_pEventCallbackFunc = NULL;
    m_bUpdateRunning = false;    
    DeleteAllProgramInfo();
    m_sM3UManager.ReleaseAllPlayList();
}


unsigned int  C_HLS_Entity::GenerateTheProgramInfo()
{
	S_PLAYLIST_NODE* pPlayListArray[HLS_MAX_STREAM_COUNT_IN_MASTER] = {0};
	unsigned int           ulStreamMAXCount = HLS_MAX_STREAM_COUNT_IN_MASTER;
    unsigned int           ulStreamId = 0;
	QC_STREAM_FORMAT*      pStreamInfo = NULL;
	unsigned int           ulStreamCount = 0;
	unsigned int           ulRet = 0;
    unsigned int           ulLoop = 0;
    unsigned int           ulTrackCount = 0;

    ulRet = m_sM3UManager.GetMainStreamArray(pPlayListArray, ulStreamMAXCount, &ulStreamCount);
    if(ulRet != 0)
    {
		return QC_ERR_EMPTYPOINTOR;
	}

	//
	if(ulStreamCount != 0)
	{
		m_ppStreamArray  = new QC_STREAM_FORMAT*[ulStreamCount];
		if(m_ppStreamArray != NULL)
		{
			memset(m_ppStreamArray, 0, sizeof(QC_STREAM_FORMAT*)*ulStreamCount);		
		}
	}

    for(ulLoop=0; ulLoop<ulStreamCount; ulLoop++)
    {
    	pStreamInfo = new QC_STREAM_FORMAT;
		if(pStreamInfo == NULL)
		{
			break;
		}
		
        memset(pStreamInfo, 0, sizeof(QC_STREAM_FORMAT));
		pStreamInfo->nBitrate = pPlayListArray[ulLoop]->sVarMainStreamAttr.ulBitrate;
        pStreamInfo->nID = pPlayListArray[ulLoop]->ulPlayListId;
		pStreamInfo->pPrivateData = pPlayListArray[ulLoop]->strShortURL;
		m_ppStreamArray[ulLoop] = pStreamInfo;
	}

	m_ulStreamCount = ulStreamCount;
    return 0;
}

unsigned int    C_HLS_Entity::NotifyToParse(char*  pURLRoot, char*   pURL, unsigned int ulPlayListId)
{
    unsigned int                            ulEventCallbackCount = 0;
    S_ADAPTIVESTREAM_PLAYLISTDATA    sVarPlayListData = {0};
    unsigned int                            ulRet = 0;
    S_PLAYLIST_NODE*                  pPlayListNode = NULL;
    if(strlen(pURL) == 0)
    {
        QCLOGI("empty URL, return ok!")
        return 0;
    }

    pPlayListNode = m_sM3UManager.FindPlayListById(ulPlayListId);
    if(pPlayListNode != NULL && pPlayListNode->eChuckPlayListType == M3U_VOD && pPlayListNode->pChunkItemHeader != NULL)
    {
        QCLOGI("the Playlist id:%d is already parsed! And the Playlist type is VOD!", ulPlayListId);
        return 0;
    }

    if(m_pEventCallbackFunc != NULL && m_pEventCallbackFunc->SendEvent != NULL)
    {

        while(ulEventCallbackCount < HLS_MAX_MANIFEST_RETRY_COUNT)
        {
            ulRet = RequestManfestAndParsing(&sVarPlayListData, pURLRoot, pURL, ulPlayListId);
            if(ulRet == 0)
            {
                QCLOGI("RequestManfestAndParsing ok!");
                break;
            }
            else
            {
                ulEventCallbackCount++;
            }
        }
    }
    
    return ulRet;
}

unsigned int     C_HLS_Entity::PlayListUpdateForLive()
{
    S_PLAY_SESSION*     pPlaySession = NULL;
    unsigned int              ulRet = 0;
    unsigned int              ulTimeInterval = 0;
    unsigned long long              ullStartTime = 0;
    char             strRoot[4096] = {0};
    bool             bStop = m_bUpdateRunning;

    ulRet = m_sM3UManager.GetCurReadyPlaySession(&pPlaySession);
    if(ulRet != 0)
    {
        m_bUpdateRunning = false;
        return QC_ERR_NONE;
    }

    if(pPlaySession->pStreamPlayListNode != NULL && pPlaySession->pStreamPlayListNode->eChuckPlayListType == M3U_LIVE)
    {
        while(m_bUpdateRunning == true)
        {
            ullStartTime = qcGetSysTime();
            ulTimeInterval = m_sM3UManager.GetChunckItemIntervalTime();            
            QCLOGI("The update interval:%d", ulTimeInterval);

            UpdateThePlayListForLive();
            m_sM3UManager.AdjustSequenceIdInSession();

            while(m_bUpdateRunning == true && ( (qcGetSysTime() - ullStartTime) <= ulTimeInterval ) )
            {
                ulTimeInterval = m_sM3UManager.GetChunckItemIntervalTime();
				bStop = (m_bUpdateRunning?false:true);
                qcSleepEx (100*1000, &m_pBaseInst->m_bForceClose);
            }
            
			bStop = (m_bUpdateRunning?false:true);
            qcSleepEx (50*1000, &m_pBaseInst->m_bForceClose);
        }
    }

    QCLOGI("Update Thread End!");
	return 0;
}

void    C_HLS_Entity::StopPlaylistUpdate()
{   
	QCLOGI( "+stop_updatethread" );
    m_bUpdateRunning = false;
	/*
	vo_thread::stop();
	*/
	QCLOGI( "-stop_updatethread" );
}

unsigned int  C_HLS_Entity::GetChunckItem(E_ADAPTIVESTREAMPARSER_CHUNKTYPE uID,S_ADAPTIVESTREAMPARSER_CHUNK **ppChunk)
{
    unsigned int ulRet = 0;
	S_PLAY_SESSION*   pPlaySession = NULL;
	S_ADAPTIVESTREAMPARSER_CHUNK*    pAdaptiveItem = NULL;
	S_DRM_HSL_PROCESS_INFO*         pDrmItem = NULL;
	S_CHUNCK_ITEM                     sChunkItem = {0};
	E_PLAYLIST_TYPE                   ePlayListTypeForChunk = E_UNKNOWN_STREAM;

	ulRet = m_sM3UManager.GetCurReadyPlaySession(&pPlaySession);
    if(ulRet != 0)
    {
		QCLOGI("The PlaySession is not ready now!");
		return ulRet;
	}

	switch(uID)
	{
	    case E_ADAPTIVESTREAMPARSER_CHUNKTYPE_AUDIOVIDEO:
		{
			if(pPlaySession->pStreamPlayListNode != NULL)
			{
				if(strlen(pPlaySession->pStreamPlayListNode->strShortURL) != 0)
				{
					InitChunkNode(&(m_sChunkContainer.sMainStreamChunk));
					pAdaptiveItem = &(m_sChunkContainer.sMainStreamChunk.sCurrentAdaptiveStreamItem);
					pAdaptiveItem->eType = E_ADAPTIVESTREAMPARSER_CHUNKTYPE_AUDIOVIDEO;
					ePlayListTypeForChunk = E_MAIN_STREAM;
				}
				else
				{
					return QC_ERR_FAILED;
				}
			}
			else
			{
				return QC_ERR_FINISH;
			}
			break;
		}
	    case E_ADAPTIVESTREAMPARSER_CHUNKTYPE_AUDIO:
		{
			if(pPlaySession->pAlterAudioPlayListNode != NULL)
			{
				if(strlen(pPlaySession->pAlterAudioPlayListNode->strShortURL) != 0)
				{
					InitChunkNode(&(m_sChunkContainer.sAltrerAudioChunk));
					pAdaptiveItem = &(m_sChunkContainer.sAltrerAudioChunk.sCurrentAdaptiveStreamItem);
					pAdaptiveItem->eType = E_ADAPTIVESTREAMPARSER_CHUNKTYPE_AUDIO;
					ePlayListTypeForChunk = E_X_MEDIA_AUDIO_STREAM;
				}
			}
			else
			{
				return QC_ERR_IO_EOF;
			}
			break;
		}
	    case E_ADAPTIVESTREAMPARSER_CHUNKTYPE_VIDEO:
		{
			if(pPlaySession->pAlterVideoPlayListNode != NULL)
			{
				if(strlen(pPlaySession->pAlterVideoPlayListNode->strShortURL) != 0)
				{
					InitChunkNode(&(m_sChunkContainer.sAltrerVideoChunk));
					pAdaptiveItem = &(m_sChunkContainer.sAltrerVideoChunk.sCurrentAdaptiveStreamItem);
					pAdaptiveItem->eType = E_ADAPTIVESTREAMPARSER_CHUNKTYPE_VIDEO;
					ePlayListTypeForChunk = E_X_MEDIA_VIDEO_STREAM;
				}
				else
				{
					return QC_ERR_IO_EOF;
				}
			}
			else
			{
				if(pPlaySession->pStreamPlayListNode != NULL && strlen(pPlaySession->pStreamPlayListNode->strShortURL) != 0 )
				{
					InitChunkNode(&(m_sChunkContainer.sMainStreamChunk));
					pAdaptiveItem = &(m_sChunkContainer.sMainStreamChunk.sCurrentAdaptiveStreamItem);                
					if(pPlaySession->pAlterAudioPlayListNode != NULL && strlen(pPlaySession->pAlterAudioPlayListNode->strShortURL) != 0)
					{
						pAdaptiveItem->eType = E_ADAPTIVESTREAMPARSER_CHUNKTYPE_VIDEO; 
						ePlayListTypeForChunk = E_MAIN_STREAM;
					}
					else
					{
						pAdaptiveItem->eType = E_ADAPTIVESTREAMPARSER_CHUNKTYPE_AUDIOVIDEO;
						ePlayListTypeForChunk = E_MAIN_STREAM;
					}
				}
				else
				{
					return QC_ERR_IO_EOF;                    
				}
			}
			break;
		}

	    case E_ADAPTIVESTREAMPARSER_CHUNKTYPE_SUBTITLE:
		{
			if(pPlaySession->pAlterSubTitlePlayListNode != NULL)
			{
				if(strlen(pPlaySession->pAlterSubTitlePlayListNode->strShortURL) != 0)
				{
					InitChunkNode(&(m_sChunkContainer.sAltrerSubTitleChunk));
					pAdaptiveItem = &(m_sChunkContainer.sAltrerSubTitleChunk.sCurrentAdaptiveStreamItem);
					pAdaptiveItem->eType = E_ADAPTIVESTREAMPARSER_CHUNKTYPE_SUBTITLE;
					ePlayListTypeForChunk = E_X_MEDIA_SUBTITLE_STREAM;
				}
				else
				{
					return QC_ERR_IO_EOF;
				}
			}
			else
			{
				return QC_ERR_IO_EOF;
			}
			break;
		}

	    case E_ADAPTIVESTREAMPARSER_CHUNKTYPE_IFRAME_ONLY:
		{
			if(pPlaySession->pIFramePlayListNode!= NULL)
			{
				if(strlen(pPlaySession->pIFramePlayListNode->strShortURL) != 0)
				{
					InitChunkNode(&(m_sChunkContainer.sIFrameOnlyChunk));
					pAdaptiveItem = &(m_sChunkContainer.sIFrameOnlyChunk.sCurrentAdaptiveStreamItem);
					pAdaptiveItem->eType = E_ADAPTIVESTREAMPARSER_CHUNKTYPE_IFRAME_ONLY;
					ePlayListTypeForChunk = E_I_FRAME_STREAM;
				}
				else
				{
					return QC_ERR_IO_EOF;
				}
			}
			else
			{
				return QC_ERR_IO_EOF;
			}
			break;
		}
        

	    default:
		{
			return QC_ERR_IO_EOF;
		}
	}

	memset(&sChunkItem, 0, sizeof(S_CHUNCK_ITEM));
	ulRet = m_sM3UManager.GetCurrentChunk(ePlayListTypeForChunk, &sChunkItem);
	if(ulRet != 0)
	{
		ulRet = ConvertErrorCodeToSource2(ulRet);
		return ulRet;
	}

	pDrmItem = (S_DRM_HSL_PROCESS_INFO*)(pAdaptiveItem->pChunkDRMInfo);
	memset(pAdaptiveItem->pChunkDRMInfo, 0, sizeof(S_DRM_HSL_PROCESS_INFO));
	memcpy(pDrmItem->strCurURL, sChunkItem.strChunkParentURL, strlen(sChunkItem.strChunkParentURL));
	memcpy(pDrmItem->strKeyString, sChunkItem.strEXTKEYLine, strlen(sChunkItem.strEXTKEYLine));
	pDrmItem->ulSequenceNum = sChunkItem.ulSequenceIDForKey;
	QCLOGI("Key sequence Id value:%d", sChunkItem.ulSequenceIDForKey);

	pAdaptiveItem->ulChunkID = sChunkItem.ulSequenceIDForKey;
	memset(pAdaptiveItem->strUrl, 0, MAX_URL_SIZE);
	memcpy(pAdaptiveItem->strUrl, sChunkItem.strChunckItemURL, strlen(sChunkItem.strChunckItemURL));
	memset(pAdaptiveItem->strRootUrl, 0, MAX_URL_SIZE);
	memcpy(pAdaptiveItem->strRootUrl, sChunkItem.strChunkParentURL, strlen(sChunkItem.strChunkParentURL));
	if(sChunkItem.ullChunckOffset != INAVALIBLEU64 && sChunkItem.ullChunckLen != INAVALIBLEU64)
	{
	    QCLOGI("chunk offset:%d", (unsigned int)sChunkItem.ullChunckOffset);        
	    QCLOGI("chunk length:%d", (unsigned int)sChunkItem.ullChunckLen);
		pAdaptiveItem->ullChunkOffset = sChunkItem.ullChunckOffset;
		pAdaptiveItem->ullChunkSize = sChunkItem.ullChunckLen;
	}
	else
	{
		pAdaptiveItem->ullChunkOffset = INAVALIBLEU64;
		pAdaptiveItem->ullChunkSize = INAVALIBLEU64;
	}
	pAdaptiveItem->ullChunkDeadTime = sChunkItem.ullEndTime;
	pAdaptiveItem->ullChunkLiveTime = sChunkItem.ullBeginTime;
	pAdaptiveItem->ullStartTime = sChunkItem.ullTimeStampOffset;
	pAdaptiveItem->ullDuration = sChunkItem.ulDurationInMsec;
	pAdaptiveItem->ulPlaylistId = sChunkItem.ulPlayListId;
	pAdaptiveItem->ulPeriodSequenceNumber = sChunkItem.ulDisSequenceId;

	if(sChunkItem.bDisOccur == true)
	{
		pAdaptiveItem->ulFlag = E_ADAPTIVESTREAMPARSER_CHUNKFLAG_FORMATCHANGE;
	}

	if(sChunkItem.bIndependent == true)
	{
		pAdaptiveItem->ulFlag = pAdaptiveItem->ulFlag | E_ADAPTIVESTREAMPARSER_CHUNKFLAG_INDEPENDENT;
	}

	switch(sChunkItem.eChunkState)
	{
	    case E_CHUNCK_NORMAL:
		{
			break;
		}
	    case E_CHUNCK_NEW_STREAM:
		{
			pAdaptiveItem->ulFlag |= E_ADAPTIVESTREAMPARSER_CHUNKFLAG_FORMATCHANGE;
			break;
		}
	    case E_CHUNCK_SMOOTH_ADAPTION:
		{
			pAdaptiveItem->ulFlag |= E_ADAPTIVESTREAMPARSER_CHUNKFLAG_FORMATCHANGE ;
			break;
		}
	    case E_CHUNCK_FORCE_NEW_STREAM:
		{
			pAdaptiveItem->ulFlag |= E_ADAPTIVESTREAMPARSER_CHUNKFLAG_FORMATCHANGE;
			break;
		}
	    case E_CHUNCK_SMOOTH_ADAPTION_EX:
		{
			pAdaptiveItem->ulFlag |= E_ADAPTIVESTREAMPARSER_CHUNKFLAG_FORMATCHANGE ;
			break;
		}
	}

	*ppChunk = pAdaptiveItem;
	return ulRet;
}

unsigned int  C_HLS_Entity::GetTrackCountByMainPlayList(S_PLAYLIST_NODE* pPlayListInfo, unsigned int*  pulCount)
{
    char*   pGroupId = NULL;
    unsigned int   ulMuxCount = 1;
    unsigned int   ulAlterVideoCount = 0;
    unsigned int   ulAlterAudioCount = 0;
    unsigned int   ulAlterSubTitleCount = 0;
    unsigned int   ulAlterClosedCaptionCount = 0;
    unsigned int   ulTrackCount = 0;
    unsigned int   ulRet = 0;


    if(pPlayListInfo == NULL || pulCount == NULL)
	{
		return QC_ERR_EMPTYPOINTOR;
	}

	//Get Video Count
	if(strlen(pPlayListInfo->sVarMainStreamAttr.strVideoAlterGroup) != 0)
	{
		pGroupId = pPlayListInfo->sVarMainStreamAttr.strVideoAlterGroup;
		ulRet = m_sM3UManager.GetXMediaStreamCountWithGroupAndType(pGroupId, E_X_MEDIA_VIDEO_STREAM, &ulAlterVideoCount);
	}

	//Get Audio Count
	if(strlen(pPlayListInfo->sVarMainStreamAttr.strAudioAlterGroup) != 0)
	{
		pGroupId = pPlayListInfo->sVarMainStreamAttr.strAudioAlterGroup;
		ulRet = m_sM3UManager.GetXMediaStreamCountWithGroupAndType(pGroupId, E_X_MEDIA_AUDIO_STREAM, &ulAlterAudioCount);
	}

	//Get SubTitle Count
	if(strlen(pPlayListInfo->sVarMainStreamAttr.strSubTitleAlterGroup) != 0)
	{
		pGroupId = pPlayListInfo->sVarMainStreamAttr.strSubTitleAlterGroup;
		ulRet = m_sM3UManager.GetXMediaStreamCountWithGroupAndType(pGroupId, E_X_MEDIA_SUBTITLE_STREAM, &ulAlterSubTitleCount);
	}

	//Get ClosedCaption Count
	if(strlen(pPlayListInfo->sVarMainStreamAttr.strClosedCaptionGroup) != 0)
	{
		pGroupId = pPlayListInfo->sVarMainStreamAttr.strClosedCaptionGroup;
		ulRet = m_sM3UManager.GetXMediaStreamCountWithGroupAndType(pGroupId, E_X_MEDIA_CAPTION_STREAM, &ulAlterClosedCaptionCount);
	}

	ulTrackCount = ulMuxCount + ulAlterVideoCount + ulAlterAudioCount + ulAlterSubTitleCount + ulAlterClosedCaptionCount;
    *pulCount = ulTrackCount;
    return 0;
}

unsigned int       C_HLS_Entity::ConvertErrorCodeToSource2(unsigned int   ulErrorCodeInHLS)
{
    switch(ulErrorCodeInHLS)
    {
        case HLS_ERR_NONE:
        {
            return QC_ERR_NONE;
        }
        
        case HLS_ERR_VOD_END:
        {
		    QCLOGI("VOD End!");
            return QC_ERR_FINISH;
        }
		case HLS_PLAYLIST_END:
	    {
			QCLOGI("live Playlist End!");
            return QC_ERR_RETRY;
		}
        case HLS_ERR_EMPTY_POINTER:
        case HLS_ERR_WRONG_MANIFEST_FORMAT:
        case HLS_ERR_LACK_MEMORY:
        case HLS_UN_IMPLEMENT:
        case HLS_ERR_NOT_ENOUGH_BUFFER:
        case HLS_ERR_NOT_EXIST:
        case HLS_ERR_NOT_ENOUGH_PLAYLIST_PARSED:
        case HLS_ERR_NEED_DOWNLOAD:
        case HLS_ERR_ALREADY_EXIST:
        {
            return QC_ERR_FAILED;  
        }
        default:
        {
            return QC_ERR_IMPLEMENT;
        }
    }
}

void C_HLS_Entity::SetEventCallbackFunc(void*   pCallbackFunc)
{
    if(pCallbackFunc != NULL)
    {
        m_pEventCallbackFunc = (S_SOURCE_EVENTCALLBACK*)pCallbackFunc;
    }
}

void    C_HLS_Entity::DeleteAllProgramInfo()
{
    if(m_ppStreamArray!= NULL)
    {
		for( int i= 0; i<(int)m_ulStreamCount; i++)
		{
			if(m_ppStreamArray[i] != NULL)
			{
				delete m_ppStreamArray[i];
				m_ppStreamArray[i] = NULL;
			}
		}
        delete[] m_ppStreamArray;
		m_ppStreamArray = NULL;
    }
}

unsigned int    C_HLS_Entity::UpdateThePlayListForLive()
{    
    S_PLAY_SESSION*     pPlaySession = NULL;
    unsigned int              ulRet = 0;
    char             strRoot[4096] = {0};
    S_ADAPTIVESTREAM_PLAYLISTDATA    sVarPlayListData = {0};
    char*            pURL = NULL;
    S_PLAYLIST_NODE*    pPlayListNode = NULL;

    ulRet = m_sM3UManager.GetCurReadyPlaySession(&pPlaySession);
    if(ulRet != 0)
    {
        return QC_ERR_NONE;
    }

    pPlayListNode = pPlaySession->pStreamPlayListNode;
    if(pPlayListNode != NULL &&  strlen(pPlayListNode->strShortURL) != 0 &&
	   pPlayListNode->eChuckPlayListType != M3U_VOD)
    {
        memset(&sVarPlayListData, 0, sizeof(S_ADAPTIVESTREAM_PLAYLISTDATA));
        RequestManfestAndParsing(&sVarPlayListData, pPlayListNode->strRootURL, pPlayListNode->strShortURL, pPlayListNode->ulPlayListId);
    }
    
	pPlayListNode = pPlaySession->pAlterAudioPlayListNode;
	if(pPlayListNode != NULL &&  strlen(pPlayListNode->strShortURL) != 0 &&
		pPlayListNode->eChuckPlayListType != M3U_VOD)
	{
		memset(&sVarPlayListData, 0, sizeof(S_ADAPTIVESTREAM_PLAYLISTDATA));
		RequestManfestAndParsing(&sVarPlayListData, pPlayListNode->strRootURL, pPlayListNode->strShortURL, pPlayListNode->ulPlayListId);
	}
    
	pPlayListNode = pPlaySession->pAlterVideoPlayListNode;
	if(pPlayListNode != NULL &&  strlen(pPlayListNode->strShortURL) != 0 &&
		pPlayListNode->eChuckPlayListType != M3U_VOD)
	{
		memset(&sVarPlayListData, 0, sizeof(S_ADAPTIVESTREAM_PLAYLISTDATA));
		RequestManfestAndParsing(&sVarPlayListData, pPlayListNode->strRootURL, pPlayListNode->strShortURL, pPlayListNode->ulPlayListId);
	}

	pPlayListNode = pPlaySession->pAlterSubTitlePlayListNode;
	if(pPlayListNode != NULL &&  strlen(pPlayListNode->strShortURL) != 0 &&
		pPlayListNode->eChuckPlayListType != M3U_VOD)
	{
		memset(&sVarPlayListData, 0, sizeof(S_ADAPTIVESTREAM_PLAYLISTDATA));
		RequestManfestAndParsing(&sVarPlayListData, pPlayListNode->strRootURL, pPlayListNode->strShortURL, pPlayListNode->ulPlayListId);
	}

    return 0;
}

unsigned int    C_HLS_Entity::RequestManfestAndParsing(S_ADAPTIVESTREAM_PLAYLISTDATA*  pPlayListData, char*  pRootPath, char* pManifestURL, unsigned int ulPlayListId)
{
    unsigned int     ulRet = 0;
    S_DATABOX_CALLBACK DataBox_CallBack;
    CDataBox databox;
    if(pPlayListData == NULL || pRootPath == NULL || pManifestURL == NULL)
    {
        QCLOGI("some input parameter point is null!");
        return QC_ERR_EMPTYPOINTOR;
    }

    DataBox_CallBack.MallocData = databox.MallocData;
    DataBox_CallBack.pUserData = (void*)&databox;
    
    memset(pPlayListData, 0, sizeof(S_ADAPTIVESTREAM_PLAYLISTDATA));
    memcpy(pPlayListData->strRootUrl, pRootPath, strlen(pRootPath));
    QCLOGI("event callback the root url:%s", pPlayListData->strRootUrl);
    memcpy(pPlayListData->strUrl, pManifestURL, strlen(pManifestURL));
    QCLOGI("event callback the url:%s", pPlayListData->strUrl);
    pPlayListData->pReserve = (void*)(&DataBox_CallBack);

    if(m_pEventCallbackFunc != NULL && m_pEventCallbackFunc->SendEvent != NULL)
    {
        ulRet = m_pEventCallbackFunc->SendEvent(m_pEventCallbackFunc->pUserData, QC_EVENTID_ADAPTIVESTREAMING_NEEDPARSEITEM, (void*)(pPlayListData), 0);
        if(ulRet == 0)
        {
            if(strlen(pPlayListData->strNewUrl) != 0 && pPlayListData->pData != NULL)
            {
                ulRet = ParseHLSPlaylist(pPlayListData, ulPlayListId);
                if(ulRet != 0)
                {
                    QCLOGE("The PlayList Content error! Parse Playlist Error!");
                    return HLS_ERR_WRONG_MANIFEST_FORMAT;
                }
                else
                {
                    return 0;
                }
            }
            else
            {
                return HLS_ERR_WRONG_MANIFEST_FORMAT;
            }
        }
        else
        {
            QCLOGI("DownLoad File Error!");
            return QC_ERR_RETRY;
        }
    }
    else
    {
        return QC_ERR_EMPTYPOINTOR;
    }    
}

unsigned int        C_HLS_Entity::AdjustTheSequenceIDForMainStream()
{
    unsigned int   ulRet = 0;
    S_PLAY_SESSION*      pPlaySession = NULL;

    ulRet = m_sM3UManager.GetCurReadyPlaySession(&pPlaySession);
    if(ulRet != 0)
    {
        QCLOGI("Play Session is not ready!");
        return QC_ERR_FAILED;
    }

    if(pPlaySession->pAlterAudioPlayListNode != NULL && strlen(pPlaySession->pAlterAudioPlayListNode->strShortURL) != 0)
    {
        QCLOGI("The AlterAudio is available, the url is %s!", pPlaySession->pAlterAudioPlayListNode->strShortURL);
    }
    else
    {
        QCLOGI("The AlterAudio is disable, the url is %s!");
        return 0;
    }

    QCLOGI("Set the MainStream SequenceId to:%d", pPlaySession->ulAlterAudioSequenceId);
    pPlaySession->ulMainStreamSequenceId = pPlaySession->ulAlterAudioSequenceId;
    return 0;
}

E_ADAPTIVESTREAMPARSER_PROGRAM_TYPE        C_HLS_Entity::GetProgramType()
{
	return m_eProgramType;
}

void        C_HLS_Entity::InitChunkNode(S_CHUNK_NODE*  pChunkNode)
{
	if(pChunkNode == NULL)
	{
		return;
	}

	memset(pChunkNode, 0, sizeof(S_CHUNK_NODE));
	pChunkNode->sCurrentAdaptiveStreamItem.pChunkDRMInfo = &(pChunkNode->sCurrentDrm);
	return;
}
