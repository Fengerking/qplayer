/*******************************************************************************
File:		CHls_parser.cpp

Contains:	HLS Parser implement file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-03		Qichao			Create file

*******************************************************************************/

#include "string.h"
#include "stdio.h"
#include "CHls_parser.h"
#include "AdaptiveStreamParser.h"
#include "ULogFunc.h"

C_M3U_Parser::C_M3U_Parser(CBaseInst * pBaseInst)
	: CBaseObject(pBaseInst)
{  
	SetObjectName("C_M3U_Parser");
	m_pManifestData = NULL;
	m_ulManifestDataMaxLength = 0;
	m_pTagNodeHeader = NULL;
	m_pTagNodeTail = NULL;
	m_eCurrentManifestType = M3U_UNKNOWN_PLAYLIST;
	m_eCurrentChuckPlayListType = M3U_INVALID_CHUNK_PLAYLIST_TYPE;
	m_eCurrentChuckPlayListTypeEx = M3U_NORMAL;

    m_ppTagName = NULL;
    m_pAttrMaxCountSet = NULL;
    m_ulTagTypeCount = 0;
    InitParseContext();
}

C_M3U_Parser::~C_M3U_Parser()
{
    if(m_pManifestData != NULL)
	{
        delete[] m_pManifestData;
        m_pManifestData = NULL;
	}

    ReleaseParseContext();
	ReleaseAllTagNode();
}


void   C_M3U_Parser::ReleaseAllTagNode()
{
    S_TAG_NODE*  pTagNode = NULL;

    if(m_pTagNodeHeader == NULL)
	{
        return;
	}

	pTagNode = m_pTagNodeHeader;
	while(pTagNode != NULL)
	{
		m_pTagNodeHeader = pTagNode->pNext;
		ReleaseTagNode(pTagNode);
		pTagNode = m_pTagNodeHeader;
	}

	m_pTagNodeHeader = m_pTagNodeTail = NULL;
}

void   C_M3U_Parser::ReleaseTagNode(S_TAG_NODE*  pTagNode)
{
    unsigned int   ulIndex = 0;
	if(pTagNode == NULL)
    {
		return;
    }
    
    for(ulIndex=0; ulIndex<pTagNode->ulAttrMaxCount; ulIndex++)
    {
		if(pTagNode->ppAttrArray != NULL && pTagNode->ppAttrArray[ulIndex] != NULL)
		{
            switch(pTagNode->ppAttrArray[ulIndex]->ulDataValueType)
            {
			    case M3U_STRING:
				{
					delete[]  pTagNode->ppAttrArray[ulIndex]->pString;
					pTagNode->ppAttrArray[ulIndex]->pString = NULL;
					break;
				}
				case M3U_DECIMAL_RESOLUTION:
			    {
					delete  pTagNode->ppAttrArray[ulIndex]->pResolution;
					pTagNode->ppAttrArray[ulIndex]->pResolution = NULL;
					break;
				}
				case M3U_BYTE_RANGE:
				{
                    delete  pTagNode->ppAttrArray[ulIndex]->pResolution;
                    pTagNode->ppAttrArray[ulIndex]->pResolution = NULL;
                    break;
				}

				case M3U_INT:
			    case M3U_FLOAT:
				case M3U_HEX_DATA:
				case M3U_UNKNOWN:
				{
					break;
				}
			}

			delete pTagNode->ppAttrArray[ulIndex];
            pTagNode->ppAttrArray[ulIndex] = NULL;
		}
	}

	if(pTagNode->ulAttrMaxCount >0)
	{
		delete[]    pTagNode->ppAttrArray;
	}

	delete pTagNode;
}

unsigned int   C_M3U_Parser::ParseManifest(unsigned char*   pManifestData, unsigned int ulDataLength)
{
    unsigned int ulRet = 0;
    char*   pLine = NULL;
    char*   pNext = NULL;
    char*   pEnd = NULL;
    char*   pCur = NULL;
    char    strOutput[8192] = {0};
    if(pManifestData == NULL)
    {
		return HLS_ERR_EMPTY_POINTER;
	}

    memcpy(strOutput, pManifestData, (ulDataLength>8191)?8191:ulDataLength);


    ulRet = VerifyHeader(pManifestData);
    if(ulRet != 0)
    {
		return ulRet;
    }

    ulRet = CheckWorkMemory(ulDataLength);
    if(ulRet != 0)
    {
		return HLS_ERR_LACK_MEMORY;
    }

	memset(m_pManifestData, 0, m_ulManifestDataMaxLength);
    memcpy(m_pManifestData, pManifestData, ulDataLength);
    ResetContext();
	pCur = (char*)m_pManifestData;
    pEnd = (char*)(m_pManifestData+ulDataLength);
	ulRet = ReadNextLineWithoutCopy(pCur, pEnd, &pLine, &pNext);
	while(ulRet == 0)
	{
		ParseLine(pLine);
		pCur = pNext;
		ulRet = ReadNextLineWithoutCopy(pCur, pEnd, &pLine, &pNext);
	}

    return 0;
}


unsigned int   C_M3U_Parser::ParseLine(char* pManifestLine)
{
    unsigned int   ulRet = 0;
    char*  pFirst = NULL;
    unsigned int   ulLineLength = 0;
    unsigned int   ulTagType = 0xffffffff;

    if(pManifestLine == NULL || strlen(pManifestLine) == 0)
    {
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
    }

    ulLineLength = strlen(pManifestLine);
    
	if((*pManifestLine) == '#')
    {
        if((ulLineLength < 4) ||
		   *(pManifestLine+1) != 'E'||
		   *(pManifestLine+2) != 'X'||
		   *(pManifestLine+3) != 'T')
		{
			return HLS_ERR_WRONG_MANIFEST_FORMAT;
		}

		ulRet = ParseTagLine(pManifestLine);
	}
	else
	{
		ulRet = AddURILine(pManifestLine);
	}

	return ulRet;
}


unsigned int   C_M3U_Parser::AddURILine(char*   pLine)
{
    unsigned int      ulRet = 0;
    S_ATTR_VALUE*    pAttrValue = NULL;
	S_TAG_NODE*  pTagNodeNew = NULL;
    char*     pLineContent = NULL;
    unsigned int       ulLineContentLength = 0;
    
    ulRet = CreateTagNode(&pTagNodeNew, NORMAL_URI_NAME_INDEX);
    if(ulRet != 0)
    {
		return ulRet;
    }

	ParseTotalLine(pLine, pTagNodeNew, URI_LINE_ATTR_ID);
	AddTag(pTagNodeNew);
	return 0;
}


unsigned int   C_M3U_Parser::GetTagType(char*   pLine)
{
    unsigned int  ulIndex = 0;

    for(ulIndex=0; ulIndex<V13_SPEC_TAG_TYPE_COUNT; ulIndex++)
    {
		if(memcmp(pLine, m_ppTagName[ulIndex], strlen(m_ppTagName[ulIndex])) == 0)
		{
			break;
		}
	}
    
	if(ulIndex<V13_SPEC_TAG_TYPE_COUNT)
	{
		return ulIndex;
	}
	else
	{
		return 0xffffffff;
	}
}


unsigned int   C_M3U_Parser::ParseTagLine(char*   pLine)
{
    unsigned int  ulRet = 0;
    unsigned int  ulTagIndex = 0xffffffff;

    ulTagIndex = GetTagType(pLine);
    switch(ulTagIndex)
    {
        case TARGETDURATION_NAME_INDEX:
        {
			ulRet = ParseTargeDuration(pLine);
            break;
        }
		case MEDIA_SEQUENCE_NAME_INDEX:
		{
            ulRet = ParseMediaSequence(pLine);
			break;
		}
		case BYTERANGE_NAME_INDEX:
		{
            ulRet = ParseByteRange(pLine);
			break;
		}
		case INF_NAME_INDEX:
		{
            ulRet = ParseInf(pLine);
			break;
		}
		case KEY_NAME_INDEX:
		{
            ulRet = ParseKey(pLine);
			break;
		}
		case STREAM_INF_NAME_INDEX:
		{
            ulRet = ParseStreamInf(pLine);
			break;
		}
		case PROGRAM_DATE_TIME_NAME_INDEX:
		{
            ulRet = ParseProgramDataTime(pLine);
			break;
		}
		case I_FRAME_STREAM_INF_NAME_INDEX:
		{
            ulRet = ParseIFrameStreamInf(pLine);
			break;
		}
		case ALLOW_CACHE_NAME_INDEX:
		{
			ulRet = ParseAllowCache(pLine);
			break;
		}
		case MEDIA_NAME_INDEX:
		{
            ulRet = ParseXMedia(pLine);
			break;
		}
		case PLAYLIST_TYPE_NAME_INDEX:
		{
            ulRet = ParsePlayListType(pLine);
			break;
		}
		case I_FRAMES_ONLY_NAME_INDEX:         
		{
            ulRet = ParseIFrameOnly(pLine);
			break;
		}
		case DISCONTINUITY_NAME_INDEX:
		{
            ulRet = ParseDisContinuity(pLine);
			break;
		}
		case ENDLIST_NAME_INDEX:
		{
			ulRet = ParseEndList(pLine);
			break;
		}
		case VERSION_NAME_INDEX:
		{
            ulRet = ParseVersion(pLine);
			break;
		}
		case MAP_NAME_INDEX:
		{
            ulRet = ParseXMap(pLine);
			break;
		}
		case START_NAME_INDEX:
		{
            ulRet = ParseXStart(pLine);
			break;
		}
		case DISCONTINUITY_SEQUENCE_NAME_INDEX:
		{
            ulRet = ParseDisSequence(pLine);
			break;
		}
		case INDEPENDENT_NAME_INDEX:
		{
            ulRet = ParseDisSequence(pLine);
			break;
		}
		default:
        {
			break;
		}
	}
    return 0;
}


void   C_M3U_Parser::AddTag(S_TAG_NODE*  pTagNode)
{
    if(m_pTagNodeTail == NULL)
	{
		m_pTagNodeHeader = m_pTagNodeTail = pTagNode;
	}
	else
	{
		m_pTagNodeTail->pNext = pTagNode;
		m_pTagNodeTail = pTagNode;
	}
}

unsigned int   C_M3U_Parser::CreateTagNode(S_TAG_NODE**  ppTagNode, unsigned int  ulTagType)
{
    S_TAG_NODE*    pTagNode = NULL;
    S_ATTR_VALUE**  ppAttrArray = NULL;

	if(ulTagType >= V13_SPEC_TAG_TYPE_COUNT)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

    pTagNode = new S_TAG_NODE;
    if(pTagNode == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }

    memset(pTagNode, 0, sizeof(S_TAG_NODE));

	if(m_pAttrMaxCountSet[ulTagType] != 0)
    {
		ppAttrArray = new S_ATTR_VALUE*[m_pAttrMaxCountSet[ulTagType]];
	}

    if(m_pAttrMaxCountSet[ulTagType] != 0 && ppAttrArray == NULL)
    {
		delete pTagNode;
		return HLS_ERR_EMPTY_POINTER;
	}

    memset(ppAttrArray, 0, sizeof(S_ATTR_VALUE*)*m_pAttrMaxCountSet[ulTagType]);
	pTagNode->ulAttrMaxCount = m_pAttrMaxCountSet[ulTagType];
	pTagNode->ulTagIndex = ulTagType;
    pTagNode->ulAttrSet = 0;
    pTagNode->pNext = NULL;
    pTagNode->ppAttrArray = ppAttrArray;

    *ppTagNode = pTagNode;
	return 0;
}


unsigned int   C_M3U_Parser::GetManifestType(M3U_MANIFEST_TYPE*  peManifestType, M3U_CHUNCK_PLAYLIST_TYPE* peChucklistType, M3U_CHUNCK_PLAYLIST_TYPE_EX*  peChunklistTypeEx)
{
    S_TAG_NODE*    pTagNode = NULL;
	int            iExistEndlist = 0;
	pTagNode = m_pTagNodeHeader;
    while(pTagNode != NULL)
    {
		switch(pTagNode->ulTagIndex)
        {
		    case STREAM_INF_NAME_INDEX:
			{
				m_eCurrentManifestType = M3U_STREAM_PLAYLIST;
				break;
			}
			case INF_NAME_INDEX:
            {
				m_eCurrentManifestType = M3U_CHUNK_PLAYLIST;
				if (iExistEndlist == 0)
				{
					m_eCurrentChuckPlayListType = M3U_LIVE;
				}
                break;
            }
			case ENDLIST_NAME_INDEX:
			{
				m_eCurrentChuckPlayListType = M3U_VOD;
				iExistEndlist = 1;
                break;
			}
			case PLAYLIST_TYPE_NAME_INDEX:
            {
				if(pTagNode->ppAttrArray[PALYLIST_TYPE_VALUE_ATTR_ID] != NULL && 
				   pTagNode->ppAttrArray[PALYLIST_TYPE_VALUE_ATTR_ID]->pString != NULL)
				{
					if(memcmp(pTagNode->ppAttrArray[PALYLIST_TYPE_VALUE_ATTR_ID]->pString, "VOD", 3) == 0)
					{
						m_eCurrentChuckPlayListType = M3U_VOD;
					}
					if(memcmp(pTagNode->ppAttrArray[PALYLIST_TYPE_VALUE_ATTR_ID]->pString, "EVENT", 5) == 0)
					{
						m_eCurrentChuckPlayListType = M3U_EVENT;
					}
				}
				break;
			}
		}

		pTagNode = pTagNode->pNext;
    }

	*peManifestType = m_eCurrentManifestType;
    *peChucklistType = m_eCurrentChuckPlayListType;
    *peChunklistTypeEx = m_eCurrentChuckPlayListTypeEx;
    return 0;
}

unsigned int   C_M3U_Parser::InitParseContext()
{
    char**     ppAttrNameArray = NULL;
    M3U_DATA_TYPE*   pAttrTypeArray = NULL;
    unsigned int           ulTagMaxAttrCount = 0;


    m_ulTagTypeCount = V13_SPEC_TAG_TYPE_COUNT;
    m_ppTagName = new char*[m_ulTagTypeCount];
	m_pAttrMaxCountSet = new unsigned int[m_ulTagTypeCount];

    if(m_ppTagName == NULL || m_pAttrMaxCountSet == NULL)
    {
		return HLS_ERR_UNKNOWN;
    }

    memset(m_ppTagName, 0, sizeof(char*)*m_ulTagTypeCount);
	memset(m_pAttrMaxCountSet, 0, sizeof(unsigned int)*m_ulTagTypeCount);

    //Add every Tag information;

    //Add BEGIN_NAME_INDEX;
	m_ppTagName[BEGIN_NAME_INDEX] =(char*) "#EXTM3U";
    m_pAttrMaxCountSet[BEGIN_NAME_INDEX] = 0;

    //Add TARGETDURATION_NAME_INDEX
	m_ppTagName[TARGETDURATION_NAME_INDEX] = (char*) "#EXT-X-TARGETDURATION";
    m_pAttrMaxCountSet[TARGETDURATION_NAME_INDEX] = TARGETDURATION_MAX_ATTR_COUNT;

	//Add MEDIA_SEQUENCE_NAME_INDEX
	m_ppTagName[MEDIA_SEQUENCE_NAME_INDEX] = (char*)"#EXT-X-MEDIA-SEQUENCE";
	m_pAttrMaxCountSet[MEDIA_SEQUENCE_NAME_INDEX] = MEDIA_SEQUENCE_MAX_ATTR_COUNT;

	//Add BYTERANGE_NAME_INDEX
	m_ppTagName[BYTERANGE_NAME_INDEX] =(char*)"#EXT-X-BYTERANGE";
	m_pAttrMaxCountSet[BYTERANGE_NAME_INDEX] = BYTERANGE_MAX_ATTR_COUNT;

	//Add INF_NAME_INDEX
	m_ppTagName[INF_NAME_INDEX] = (char*) "#EXTINF";
	m_pAttrMaxCountSet[INF_NAME_INDEX] = INF_MAX_ATTR_COUNT;

	//Add KEY_NAME_INDEX
	m_ppTagName[KEY_NAME_INDEX] = (char*) "#EXT-X-KEY";
	m_pAttrMaxCountSet[KEY_NAME_INDEX] = KEY_MAX_ATTR_COUNT;

	//Add STREAM_INF_NAME_INDEX
	m_ppTagName[STREAM_INF_NAME_INDEX] =(char*) "#EXT-X-STREAM-INF";
	m_pAttrMaxCountSet[STREAM_INF_NAME_INDEX] = STREAM_MAX_ATTR_COUNT;

	//Add PROGRAM_DATE_TIME_NAME_INDEX
	m_ppTagName[PROGRAM_DATE_TIME_NAME_INDEX] = (char*) "#EXT-X-PROGRAM-DATE-TIME";
	m_pAttrMaxCountSet[PROGRAM_DATE_TIME_NAME_INDEX] = PROGRAM_MAX_ATTR_COUNT;

	//Add I_FRAME_STREAM_INF_NAME_INDEX
	m_ppTagName[I_FRAME_STREAM_INF_NAME_INDEX] = (char*) "#EXT-X-I-FRAME-STREAM-INF";
	m_pAttrMaxCountSet[I_FRAME_STREAM_INF_NAME_INDEX] = IFRAME_STREAM_MAX_ATTR_COUNT;

	//Add ALLOW_CACHE_NAME_INDEX
	m_ppTagName[ALLOW_CACHE_NAME_INDEX] = (char*) "#EXT-X-ALLOW-CACHE";
	m_pAttrMaxCountSet[ALLOW_CACHE_NAME_INDEX] = ALLOW_CACHE_MAX_ATTR_COUNT;

	//Add MEDIA_NAME_INDEX
	m_ppTagName[MEDIA_NAME_INDEX] = (char*) "#EXT-X-MEDIA";
	m_pAttrMaxCountSet[MEDIA_NAME_INDEX] = MEDIA_MAX_ATTR_COUNT;

	//Add PLAYLIST_TYPE_NAME_INDEX
	m_ppTagName[PLAYLIST_TYPE_NAME_INDEX] = (char*) "#EXT-X-PLAYLIST-TYPE";
	m_pAttrMaxCountSet[PLAYLIST_TYPE_NAME_INDEX] = PALYLIST_TYPE_MAX_ATTR_COUNT;

	//Add I_FRAMES_ONLY_NAME_INDEX
	m_ppTagName[I_FRAMES_ONLY_NAME_INDEX] = (char*) "#EXT-X-I-FRAMES-ONLY";
	m_pAttrMaxCountSet[I_FRAMES_ONLY_NAME_INDEX] = 0;

	//Add DISCONTINUITY_NAME_INDEX
	m_ppTagName[DISCONTINUITY_NAME_INDEX] = (char*) "#EXT-X-DISCONTINUITY";
	m_pAttrMaxCountSet[DISCONTINUITY_NAME_INDEX] = 0;

	//Add ENDLIST_NAME_INDEX
	m_ppTagName[ENDLIST_NAME_INDEX] = (char*) "#EXT-X-ENDLIST";
	m_pAttrMaxCountSet[ENDLIST_NAME_INDEX] = 0;

	//Add VERSION_NAME_INDEX
	m_ppTagName[VERSION_NAME_INDEX] = (char*) "#EXT-X-VERSION";
	m_pAttrMaxCountSet[VERSION_NAME_INDEX] = VERSION_MAX_ATTR_COUNT;

	//Add MAP_NAME_INDEX
	m_ppTagName[MAP_NAME_INDEX] = (char*) "#EXT-X-MAP";
	m_pAttrMaxCountSet[MAP_NAME_INDEX] = XMAP_MAX_ATTR_COUNT;

	//Add START_NAME_INDEX
	m_ppTagName[START_NAME_INDEX] = (char*) "#EXT-X-START";
	m_pAttrMaxCountSet[START_NAME_INDEX] = X_START_MAX_ATTR_COUNT;

	//Add DISCONTINUITY_SEQUENCE_NAME_INDEX
	m_ppTagName[DISCONTINUITY_SEQUENCE_NAME_INDEX] = (char*) "#EXT-X-DISCONTINUITY-SEQUENCE";
	m_pAttrMaxCountSet[DISCONTINUITY_SEQUENCE_NAME_INDEX] = DISCONTINUITY_SEQUENCE_MAX_ATTR_COUNT;

    //Add EXT-X-INDEPENDENT-SEGMENTS   
    m_ppTagName[INDEPENDENT_NAME_INDEX] = (char*) "#EXT-X-INDEPENDENT-SEGMENTS";
    m_pAttrMaxCountSet[INDEPENDENT_NAME_INDEX] = 0;

	//Add NORMAL_URI_NAME_INDEX
	m_ppTagName[NORMAL_URI_NAME_INDEX] = (char*) "";
	m_pAttrMaxCountSet[NORMAL_URI_NAME_INDEX] = URI_LINE_MAX_ATTR_COUNT;
	return 0;
}

void  C_M3U_Parser::ReleaseParseContext()
{
    if(m_ppTagName != NULL)
    {
        delete []m_ppTagName;
        m_ppTagName = NULL;
    }

	if(m_pAttrMaxCountSet != NULL)
	{
        delete []m_pAttrMaxCountSet;
        m_pAttrMaxCountSet = NULL;
	}
}


void  C_M3U_Parser::ResetContext()
{
	m_eCurrentManifestType = M3U_UNKNOWN_PLAYLIST;
	m_eCurrentChuckPlayListType = M3U_INVALID_CHUNK_PLAYLIST_TYPE;
	m_eCurrentChuckPlayListTypeEx = M3U_NORMAL;
	ReleaseAllTagNode();
}

unsigned int  C_M3U_Parser::CheckWorkMemory(unsigned int  ulNewDataLength)
{
	if(m_pManifestData == NULL)
	{
		m_pManifestData = new unsigned char[ulNewDataLength*2];
		if(m_pManifestData == NULL)
		{
			return HLS_ERR_LACK_MEMORY;
		}

		memset(m_pManifestData, 0, ulNewDataLength*2);
		m_ulManifestDataMaxLength = 2*ulNewDataLength;
	}
	else
	{
		if(ulNewDataLength > m_ulManifestDataMaxLength)
		{
			delete []m_pManifestData;
			m_pManifestData = new unsigned char[ulNewDataLength*2];
			if(m_pManifestData == NULL)
			{
				return HLS_ERR_LACK_MEMORY;
			}

			memset(m_pManifestData, 0, ulNewDataLength*2);
			m_ulManifestDataMaxLength = 2*ulNewDataLength;
		}
	}
	return 0;
}


unsigned int   C_M3U_Parser::VerifyHeader(unsigned char*   pManifestData)
{
    char*   pFind = NULL;
	pFind = strstr((char*)pManifestData, "#EXT");
	if(pFind == NULL)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}
	else
	{
		if(memcmp(pFind, "#EXTM3U", strlen("#EXTM3U")) != 0)
		{
			QCLOGI("can't find the M3U Begin!");
			return HLS_ERR_WRONG_MANIFEST_FORMAT;
		}

		return 0;
	}
}


unsigned int   C_M3U_Parser::ReadNextLineWithoutCopy(char* pSrc, char* pEnd, char** ppLine, char** pNext)
{
	char*   pLineStart = NULL;
	char*   pLineEnd = NULL;
	char*   pCur = NULL;

	if(pSrc == NULL || pSrc>=pEnd)
	{
		*ppLine = NULL;
		return 1;
	}

	pCur = pSrc;
	while(pCur<pEnd)
	{
		if((*pCur) != '\0' && (*pCur) != '\r' && (*pCur) != '\n')
		{
			if(pLineStart == NULL)
			{
				pLineStart = pCur;
				pLineEnd = pCur;
			}
			else
			{
				pLineEnd = pCur;
			}
		}
		else
		{
			if(pLineStart != NULL)
			{
				*ppLine = pLineStart;
				*pCur = '\0';
				*pNext = pCur+1;
				return 0;
			}
		}

		pCur++;
	}


	if(pLineStart == NULL)
	{
		*ppLine = NULL;
	}
	else
	{
		if(pCur == pEnd )
		{
            *ppLine = pLineStart;
			*pNext = pEnd;
			return 0;
		}
	}

    

	*pNext = pEnd;
	return 1;
}

unsigned int   C_M3U_Parser::FindAttrValueByName(char*   pOriginalXMediaLine, char*  pAttrValue, unsigned int ulAttrValueSize, char*   pAttrName)
{
	char*  pStart = NULL;
    char*  pEnd = NULL;
    bool   bFindQuoteStringStart = false;    

	if(pOriginalXMediaLine == NULL || pAttrValue == NULL)
	{
		return 1;
	}

	pStart = strstr(pOriginalXMediaLine, pAttrName);
	if(pStart == NULL)
	{
		return 1;
	}
	else
	{
		pEnd = pOriginalXMediaLine+strlen(pOriginalXMediaLine);
		pStart = pStart+strlen(pAttrName);
		if(*pStart == '\"')
		{
			bFindQuoteStringStart = true;
			pStart++;
		}

		while((*pStart)!= '\"' &&
			(*pStart)!= '\0' &&
             pStart< pEnd )
		{
			if( *(pStart) ==',')
			{
                if(bFindQuoteStringStart == true)
			    {
					*(pAttrValue++) = *(pStart++);
			    }
			    else
			    {
					break;
			    }
			}
			else
			{
				*(pAttrValue++) = *(pStart++);
			}
		}

		return 0;
	}
}

unsigned int   C_M3U_Parser::ParseTargeDuration(char*   pLine)
{
    S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

    ulRet = CreateTagNode(&pTagNode, TARGETDURATION_NAME_INDEX);
    if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseInt(pLine, (char*)":", pTagNode, TARGETDURATION_VALUE_ATTR_ID);
    AddTag(pTagNode);
    return 0;
}

unsigned int   C_M3U_Parser::ParseMediaSequence(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, MEDIA_SEQUENCE_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseInt(pLine, (char*)":", pTagNode, MEDIA_SEQUENCE_VALUE_ATTR_ID);
    AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseByteRange(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, BYTERANGE_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseByteRangeInfo(pLine, (char*)":", pTagNode, BYTERANGE_RANGE_ATTR_ID);
	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseInf(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, INF_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseFloat(pLine, (char*)":", pTagNode, INF_DURATION_ATTR_ID);
    ParseString(pLine, (char*)",", pTagNode, INF_DESC_ATTR_ID);
	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseKey(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int    ulRet = 0;
    unsigned int    ulLineContentLength = 0;
    char*  pLineContent = NULL;
    S_ATTR_VALUE*   pAttrValue = NULL;

	ulRet = CreateTagNode(&pTagNode, KEY_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}
    
	ParseTotalLine(pLine, pTagNode, KEY_LINE_CONTENT);
	ParseString(pLine, (char*)"METHOD=", pTagNode, KEY_METHOD_ID);
	ParseString(pLine, (char*)"KEYFORMAT=", pTagNode, KEY_METHOD_KEYFORMAT);
    
	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseStreamInf(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int    ulRet = 0;
	unsigned int    ulLineContentLength = 0;
	char*  pLineContent = NULL;
	S_ATTR_VALUE*   pAttrValue = NULL;

	ulRet = CreateTagNode(&pTagNode, STREAM_INF_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseInt(pLine, (char*)"BANDWIDTH=", pTagNode, STREAM_INF_BANDWIDTH_ATTR_ID);
	ParseString(pLine, (char*)"CODECS=", pTagNode, STREAM_INF_CODECS_ATTR_ID);
	ParseString(pLine, (char*)"VIDEO=", pTagNode, STREAM_INF_VIDEO_ATTR_ID);
	ParseString(pLine, (char*)"AUDIO=", pTagNode, STREAM_INF_AUDIO_ATTR_ID);
	ParseString(pLine, (char*)"SUBTITLES=", pTagNode, STREAM_INF_SUBTITLE_ATTR_ID);
	ParseString(pLine, (char*)"CLOSED-CAPTIONS=", pTagNode, STREAM_INF_CLOSED_CAPTIONS_ATTR_ID);
	ParseResolution(pLine, (char*)"RESOLUTION=", pTagNode, STREAM_INF_RESOLUTION_ATTR_ID);

	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseProgramDataTime(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, PROGRAM_DATE_TIME_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseString(pLine, (char*)":", pTagNode, PROGRAM_DATE_TIME_ATTR_ID);
	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseIFrameStreamInf(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int    ulRet = 0;
	unsigned int    ulLineContentLength = 0;
	char*  pLineContent = NULL;
	S_ATTR_VALUE*   pAttrValue = NULL;

	ulRet = CreateTagNode(&pTagNode, I_FRAME_STREAM_INF_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseInt(pLine, (char*)"BANDWIDTH=", pTagNode, IFRAME_STREAM_BANDWIDTH_ATTR_ID);
	ParseString(pLine, (char*)"URI=", pTagNode, IFRAME_STREAM_URI_ATTR_ID);
	ParseString(pLine, (char*)"VIDEO=", pTagNode, IFRAME_STREAM_CODECS_ATTR_ID);
	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseAllowCache(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, ALLOW_CACHE_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseString(pLine, (char*)":", pTagNode, ALLOW_CACHE_VALUE_ATTR_ID);
	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseXMedia(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int    ulRet = 0;
	unsigned int    ulLineContentLength = 0;
	char*  pLineContent = NULL;
	S_ATTR_VALUE*   pAttrValue = NULL;

	ulRet = CreateTagNode(&pTagNode, MEDIA_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseString(pLine, (char*)"TYPE=", pTagNode, MEDIA_TYPE_ATTR_ID);
	ParseString(pLine, (char*)"GROUP-ID=", pTagNode, MEDIA_GROUP_ID_ATTR_ID);
	ParseString(pLine, (char*)"NAME=", pTagNode, MEDIA_NAME_ATTR_ID);
	ParseString(pLine, (char*)"DEFAULT=", pTagNode, MEDIA_DEFAULT_ATTR_ID);
	ParseString(pLine, (char*)"URI=", pTagNode, MEDIA_URI_ATTR_ID);
	ParseString(pLine, (char*)"AUTOSELECT=", pTagNode, MEDIA_AUTOSELECT_ATTR_ID);
	ParseString(pLine, (char*)"LANGUAGE=", pTagNode, MEDIA_LANGUAGE_ATTR_ID);
	ParseString(pLine, (char*)"ASSOC-LANGUAGE=", pTagNode, MEDIA_ASSOC_LANGUAGE_ATTR_ID);
	ParseString(pLine, (char*)"FORCED=", pTagNode, MEDIA_FORCED_ATTR_ID);
	ParseString(pLine, (char*)"INSTREAM-ID=", pTagNode, MEDIA_INSTREAM_ATTR_ID);
	ParseString(pLine, (char*)"CHARACTERISTICS=", pTagNode, MEDIA_CHARACTERISTICS_ATTR_ID);

	AddTag(pTagNode);
	return 0;}

unsigned int   C_M3U_Parser::ParsePlayListType(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, PLAYLIST_TYPE_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseString(pLine, (char*)":", pTagNode, PALYLIST_TYPE_VALUE_ATTR_ID);
	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseIFrameOnly(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, I_FRAMES_ONLY_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseDisContinuity(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, DISCONTINUITY_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseEndList(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, ENDLIST_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseVersion(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, VERSION_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseInt(pLine, (char*)":", pTagNode, VERSION_NUMBER_ATTR_ID);
	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseXMap(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, MAP_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseInt(pLine, (char*)"URI=", pTagNode, XMAP_URI_ATTR_ID);
	ParseString(pLine, (char*)"BYTERANGE=", pTagNode, XMAP_BYTERANGE_ATTR_ID);
	AddTag(pTagNode);
	return 0;}

unsigned int   C_M3U_Parser::ParseXStart(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, START_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseInt(pLine, (char*)"TIME-OFFSET=", pTagNode, X_START_TIMEOFFSET_ATTR_ID);
	ParseString(pLine, (char*)"PRECISE=", pTagNode, X_START_PRECISE_ATTR_ID);
	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseDisSequence(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, DISCONTINUITY_SEQUENCE_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	ParseInt(pLine, (char*)":", pTagNode, DISCONTINUITY_SEQUENCE_ATTR_ID);
	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseIndependent(char*   pLine)
{
	S_TAG_NODE*   pTagNode = NULL;
	unsigned int   ulRet = 0;

	ulRet = CreateTagNode(&pTagNode, INDEPENDENT_NAME_INDEX);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	AddTag(pTagNode);
	return 0;
}

unsigned int   C_M3U_Parser::ParseInt(char*   pLine, char* pAttrName, S_TAG_NODE*  pTagNode, unsigned int ulAttrIndex)
{
	long long illValue = 0;
	unsigned int   ulRet = 0;
	S_ATTR_VALUE*   pAttrValue = NULL;
	char    strAttrValue[1024] = {0};

	ulRet = FindAttrValueByName(pLine, strAttrValue, 1024, pAttrName);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	if(sscanf(strAttrValue, "%llu", &illValue) <= 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	pAttrValue = new S_ATTR_VALUE;
	if(pAttrValue == NULL)
	{
		return HLS_ERR_EMPTY_POINTER;
	}

	memset(pAttrValue, 0, sizeof(S_ATTR_VALUE));
	pAttrValue->ulDataLength = 8;
	pAttrValue->ulDataValueType = M3U_INT;
	pAttrValue->illIntValue = illValue;

	pTagNode->ppAttrArray[ulAttrIndex] = pAttrValue;
	pTagNode->ulAttrSet |=(1<<ulAttrIndex);
	return 0;
}

unsigned int   C_M3U_Parser::ParseFloat(char*   pLine, char* pAttrName, S_TAG_NODE*  pTagNode, unsigned int ulAttrIndex)
{
	float   fValue = 0;
	unsigned int   ulRet = 0;
	S_ATTR_VALUE*   pAttrValue = NULL;
	char    strAttrValue[1024] = {0};

	ulRet = FindAttrValueByName(pLine, strAttrValue, 1024, pAttrName);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	if(sscanf(strAttrValue, "%f", &fValue) <= 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	pAttrValue = new S_ATTR_VALUE;
	if(pAttrValue == NULL)
	{
		return HLS_ERR_EMPTY_POINTER;
	}

	memset(pAttrValue, 0, sizeof(S_ATTR_VALUE));
	pAttrValue->ulDataLength = 8;
	pAttrValue->ulDataValueType = M3U_FLOAT;
	pAttrValue->fFloatValue = fValue;

	pTagNode->ppAttrArray[ulAttrIndex] = pAttrValue;
	pTagNode->ulAttrSet |=(1<<ulAttrIndex);
    return 0;
}

unsigned int   C_M3U_Parser::ParseString(char*   pLine, char* pAttrName, S_TAG_NODE*  pTagNode, unsigned int ulAttrIndex)
{
    char*    pStringValue = NULL;
	unsigned int   ulRet = 0;
	S_ATTR_VALUE*   pAttrValue = NULL;
	char    strAttrValue[1024] = {0};
    unsigned int     ulSringValueLength = 0;

	ulRet = FindAttrValueByName(pLine, strAttrValue, 1024, pAttrName);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

    if(strlen(strAttrValue) == 0)
    {
        return 0;
    }

    ulSringValueLength = strlen(strAttrValue)/4*4+8;

	pAttrValue = new S_ATTR_VALUE;
    pStringValue = new char[ulSringValueLength];
    
	if(pAttrValue == NULL || pStringValue == NULL)
	{
		if(pAttrValue != NULL)
		{
			delete pAttrValue;
		}
        
		if(pStringValue != NULL)
        {
			delete []pStringValue;
	    }
		return HLS_ERR_EMPTY_POINTER;
	}


	memset(pAttrValue, 0, sizeof(S_ATTR_VALUE));
    memset(pStringValue, 0, ulSringValueLength);
    memcpy(pStringValue, strAttrValue, strlen(strAttrValue));

	pAttrValue->ulDataLength = ulSringValueLength;
	pAttrValue->ulDataValueType = M3U_STRING;
	pAttrValue->pString = pStringValue;

	pTagNode->ppAttrArray[ulAttrIndex] = pAttrValue;
	pTagNode->ulAttrSet |=(1<<ulAttrIndex);
	return 0;
}

unsigned int   C_M3U_Parser::ParseByteRangeInfo(char*   pLine, char* pAttrName, S_TAG_NODE*  pTagNode, unsigned int ulAttrIndex)
{
	unsigned int      ulRet = 0;
    long long      illLength = 0;
    char*    pFind = NULL;
	S_ATTR_VALUE*   pAttrValue = NULL;
    S_BYTE_RANGE*   pByteRange = NULL;
	char    strAttrValue[1024] = {0};

	ulRet = FindAttrValueByName(pLine, strAttrValue, 1024, pAttrName);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	if(strlen(strAttrValue) == 0)
	{
		return 0;
	}

	pFind = strstr(strAttrValue, "@");
	if(pFind != NULL)
	{
		*pFind = '\0';
		if(sscanf(strAttrValue, "%llu", &illLength) <= 0)
		{
			return HLS_ERR_WRONG_MANIFEST_FORMAT;
		}
	}
	else
	{
		if(sscanf(strAttrValue, "%llu", &illLength) <= 0)
		{
			return HLS_ERR_WRONG_MANIFEST_FORMAT;
		}
	}
	
    pAttrValue = new S_ATTR_VALUE;
    pByteRange = new S_BYTE_RANGE;
	if(pAttrValue == NULL || pByteRange == NULL)
	{
		if(pAttrValue != NULL)
        {
			delete pAttrValue;
		}

		if(pByteRange != NULL)
		{
			delete pAttrValue;
		}
		return HLS_ERR_EMPTY_POINTER;
	}

    memset(pByteRange, 0, sizeof(S_BYTE_RANGE));
	memset(pAttrValue, 0, sizeof(S_ATTR_VALUE));
	pAttrValue->pRangeInfo = pByteRange;
	pAttrValue->ulDataLength = 16;
	pAttrValue->ulDataValueType = M3U_BYTE_RANGE;
    pAttrValue->pRangeInfo->ullOffset = INAVALIBLEU64;
	pAttrValue->pRangeInfo->ullLength = (unsigned long long)illLength;

    if(pFind != NULL)
	{
		memset(strAttrValue, 0, 1024);
		ulRet = FindAttrValueByName(pLine, strAttrValue, 1024, (char*)"@");
		if(ulRet == 0)
		{
			if(sscanf(strAttrValue, "%llu", &illLength) > 0)
			{
				pAttrValue->pRangeInfo->ullOffset = (unsigned long long)illLength;
			}
		}
	}

	pTagNode->ppAttrArray[ulAttrIndex] = pAttrValue;
	pTagNode->ulAttrSet |=(1<<ulAttrIndex);
	return 0;
}

unsigned int   C_M3U_Parser::ParseTotalLine(char*   pLine, S_TAG_NODE*  pTagNode, unsigned int ulAttrIndex)
{
	unsigned int      ulRet = 0;
	S_ATTR_VALUE*    pAttrValue = NULL;
	char*     pLineContent = NULL;
	unsigned int       ulLineContentLength = 0;

	ulLineContentLength = strlen(pLine)/4*4+8;
	pLineContent = new char[ulLineContentLength];
	pAttrValue = new S_ATTR_VALUE;
	if(pAttrValue == NULL || pLineContent == NULL)
	{
		if(pAttrValue != NULL)
		{
			delete pAttrValue;
		}

		if(pLineContent != NULL)
		{
			delete []pLineContent;
		}
		return HLS_ERR_EMPTY_POINTER;
	}

	memset(pAttrValue, 0, sizeof(S_ATTR_VALUE));
	memset(pLineContent, 0, ulLineContentLength);	
	pAttrValue->ulDataValueType = M3U_STRING;
	memcpy(pLineContent, pLine, strlen(pLine));
	pAttrValue->pString = pLineContent;
	pAttrValue->ulDataLength = ulLineContentLength;

	pTagNode->ppAttrArray[ulAttrIndex] = pAttrValue;
	pTagNode->ulAttrSet |=(1<<ulAttrIndex);
	return 0;
}

unsigned int   C_M3U_Parser::ParseResolution(char*   pLine, char* pAttrName, S_TAG_NODE*  pTagNode, unsigned int ulAttrIndex)
{
	unsigned int      ulRet = 0;
	long long      illWidth = 0;
    long long      illHeight = 0;
	char*    pFind = NULL;
	S_ATTR_VALUE*   pAttrValue = NULL;
	S_RESOLUTION*   pResolution = NULL;
	char    strAttrValue[1024] = {0};

	ulRet = FindAttrValueByName(pLine, strAttrValue, 1024, pAttrName);
	if(ulRet != 0)
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	if(strlen(strAttrValue) == 0)
	{
		return 0;
	}

	pFind = strstr(strAttrValue, "x");
	if(pFind != NULL)
	{
		if(sscanf(strAttrValue, "%llux%llu", &illWidth, &illHeight) <= 0)
		{
			return HLS_ERR_WRONG_MANIFEST_FORMAT;
		}
	}
	else
	{
		return HLS_ERR_WRONG_MANIFEST_FORMAT;
	}

	pAttrValue = new S_ATTR_VALUE;
	pResolution = new S_RESOLUTION;
	if(pAttrValue == NULL || pResolution == NULL)
	{
		if(pAttrValue != NULL)
		{
			delete pAttrValue;
		}

		if(pResolution != NULL)
		{
			delete pAttrValue;
		}
		return HLS_ERR_EMPTY_POINTER;
	}

	memset(pResolution, 0, sizeof(S_RESOLUTION));
	memset(pAttrValue, 0, sizeof(S_ATTR_VALUE));
	pAttrValue->pResolution = pResolution;
	pAttrValue->ulDataLength = 16;
	pAttrValue->ulDataValueType = M3U_DECIMAL_RESOLUTION;
	pAttrValue->pResolution->ulWidth = (unsigned int)illWidth;
	pAttrValue->pResolution->ulHeight = (unsigned int)illHeight;

	pTagNode->ppAttrArray[ulAttrIndex] = pAttrValue;
	pTagNode->ulAttrSet |=(1<<ulAttrIndex);
	return 0;
}

unsigned int   C_M3U_Parser::GetTagList(S_TAG_NODE** ppTagNode)
{
    unsigned int   ulRet = 0;
    if(ppTagNode == NULL && m_pTagNodeHeader == NULL)
    {
        return HLS_ERR_EMPTY_POINTER;
    }

	*ppTagNode = m_pTagNodeHeader;
    return 0;
}
