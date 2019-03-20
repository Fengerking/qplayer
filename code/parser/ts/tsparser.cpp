/*******************************************************************************
File:		tsparser.cpp

Contains:	ts parse implement file.

Written by:	Qichao Shen

Change History (most recent first):
2016-12-22		Qichao			Create file

*******************************************************************************/
#include "stdio.h"
#include "string.h"
#include "cmbasetype.h"
#include "tsparser.h"
#include "stdlib.h"

#include "tsparser.h"
#include "tsbase.h"

int static ParseQiniuPrivateDescTag(unsigned char*  pDescData, int iDescSize, void*  pOut)
{
	int iRet = 0;
	int  iSampleRate = 0;
	char  uAudioType = 0;
	int iValid = 0;
	S_Track_Info_From_Desc*  pAudioTrackInfo;

	do
	{
		pAudioTrackInfo = (S_Track_Info_From_Desc*)pOut;
		uAudioType = pDescData[0];
		iSampleRate = (pDescData[1] << 8) | (pDescData[2]);

		if (uAudioType == 'a')
		{
			pAudioTrackInfo->iCodecID = STREAM_TYPE_AUDIO_G711_A;
			iValid = 1;
		}

		if (uAudioType == 'u')
		{
			pAudioTrackInfo->iCodecID = STREAM_TYPE_AUDIO_G711_U;
			iValid = 1;
		}

		if (iValid == 0)
		{
			iRet = 1;
			break;
		}

		pAudioTrackInfo->iSampleRate = iSampleRate;
		pAudioTrackInfo->iChannelCount = 1;
	} while (0);

	return iRet;
}


int static ParseDesc(unsigned char uDescType, unsigned char*  pDescData, int iDescSize, void*  pOut)
{
	int iRet = 0;

	do 
	{
		if (pDescData == NULL || iDescSize < 0)
		{
			break;
		}

		switch (uDescType)
		{
		case 0xf:
		{
			iRet = ParseQiniuPrivateDescTag(pDescData, iDescSize, pOut);
			break;
		}
		default:
		{
			break;
		}
		}

	} while (0);

	return iRet;
}





typedef struct MpegTSSectionFilter 
{
	int section_index;
	int section_h_size;
	uint8 *section_buf;
	uint32 check_crc : 1;
	uint32 end_of_section_reached : 1;
} MpegTSSectionFilter;

uint8* FindPacketHeader(uint8* pData, int cbData, int packetSize)
{
	uint8* p = pData;
	uint8* p2 = pData + cbData - packetSize;
	while (p < p2)
	{
		if ( (*p == TransportPacketSyncByte) && (*(p + packetSize) == TransportPacketSyncByte) )
			return p;
		++p;
	}
	return 0;
}

uint8* FindPacketHeader2(uint8* pData, int cbData, int packetSize)
{
	uint8* p = FindPacketHeader(pData, cbData, packetSize);
	if (p)
	{
		uint8* p2 = p + packetSize * 2;
		if (p2 < pData + cbData)
			if (*p2 == TransportPacketSyncByte)
				return p;
	}
	return 0;
}

int IsIFrameForH264(uint8* pData, uint32 ulSize)
{
	uint8 nalhead[3] = {0, 0, 1};
	int ioffset = 0;
	uint8 * pScan = pData;
	uint8 * pScanEnd = pData+ulSize-4;
	uint8  naluType = 0;
	while(pScan<pScanEnd)
	{
		if(memcmp(pScan, nalhead, 3) == 0)
		{
			naluType = (*(pScan+3))&0x0f;
			if(naluType == 5)
			{
				return 1;
			}
			else
			{
				pScan++;
			}
		}
		else
		{
			pScan++;
		}
	}

	return 0;
}

int IsIFrameForHEVC(uint8* pData, uint32 ulSize)
{
	uint8 nalhead[3] = { 0, 0, 1 };
	int ioffset = 0;
	uint8 * pScan = pData;
	uint8 * pScanEnd = pData + ulSize - 4;
	uint8  naluType = 0;
	while (pScan < pScanEnd)
	{
		if (memcmp(pScan, nalhead, 3) == 0)
		{
			naluType = ((*(pScan + 3))>>1) & 0x3f;
			if (naluType >= 19 && naluType <= 21)
			{
				return 1;
			}
			else
			{
				pScan++;
			}
		}
		else
		{
			pScan++;
		}
	}

	return 0;
}

int CheckPacketSize(uint8* pData, int cbData)
{
	if (cbData <= 204) //only one packet
		return cbData;

	if (cbData <= 408) //less than 2 packets
	{
		if (FindPacketHeader(pData, cbData, 188))
			return 188;
		if (FindPacketHeader(pData, cbData, 204))
			return 204;
		if (FindPacketHeader(pData, cbData, 192))
			return 192;
		return 0;
	}

	if (FindPacketHeader2(pData, cbData, 188))
		return 188;
	if (FindPacketHeader2(pData, cbData, 204))
		return 204;
	if (FindPacketHeader2(pData, cbData, 192))
		return 192;
	return 0;
}

static int FindPIDInContext(uint16 usPID, S_Ts_Parser_Context*  pTsContext)
{
	int   iIndex = 0;

	for(iIndex=0; iIndex<pTsContext->usPesPIDCount; iIndex++)
	{
		if(pTsContext->aPesPID[iIndex] == usPID)
		{
			return iIndex;
		}
	}

	return 0xffff;
}

static void DoTheCallbackForFrameData(int iArrayIndex, S_Ts_Parser_Context*  pTsContext)
{
	S_Ts_Media_Sample    sMediaSample = { 0 };

	sMediaSample.pSampleBuffer = pTsContext->aPesBufferData[iArrayIndex];
	sMediaSample.ulSampleBufferSize = pTsContext->aPesBufferSize[iArrayIndex];
	sMediaSample.ullTimeStamp = pTsContext->aPesTimeStamp[iArrayIndex];
	sMediaSample.pUserData = pTsContext->sCallbackProcInfo.pUserData;
	sMediaSample.usTrackId = pTsContext->aPesPID[iArrayIndex];
	switch(pTsContext->aPesElementType[iArrayIndex])
	{
	    case STREAM_TYPE_VIDEO_H264:
		{
			sMediaSample.usMediaType = MEDIA_VIDEO_IN_TS;
			sMediaSample.ulMediaCodecId = STREAM_TYPE_VIDEO_H264;
			break;
		}

		case STREAM_TYPE_VIDEO_HEVC:
		{
			sMediaSample.usMediaType = MEDIA_VIDEO_IN_TS;
			sMediaSample.ulMediaCodecId = STREAM_TYPE_VIDEO_HEVC;
			break;
		}

		case STREAM_TYPE_AUDIO_MPEG1:
		{
			sMediaSample.usMediaType = MEDIA_AUDIO_IN_TS;
			sMediaSample.ulMediaCodecId = STREAM_TYPE_AUDIO_MPEG1;
			break;
		}
		case STREAM_TYPE_AUDIO_AAC:
		{
			sMediaSample.usMediaType = MEDIA_AUDIO_IN_TS;
			sMediaSample.ulMediaCodecId = STREAM_TYPE_AUDIO_AAC;
			break;
		}
		case STREAM_TYPE_PES_PRIVATE:
		{
			if (pTsContext->sProInfo.aSEsInfo[iArrayIndex].iCodecID == STREAM_TYPE_AUDIO_G711_A || 
				pTsContext->sProInfo.aSEsInfo[iArrayIndex].iCodecID == STREAM_TYPE_AUDIO_G711_U)
			sMediaSample.usMediaType = MEDIA_AUDIO_IN_TS;
			sMediaSample.ulMediaCodecId = pTsContext->sProInfo.aSEsInfo[iArrayIndex].iCodecID;
			break;
		}
		default:
		{
			return;
		}
	}

	pTsContext->sCallbackProcInfo.pProc(&sMediaSample);
}

static void CommitFrameData(uint8*  pData,  uint32 ulDataSize, int iArrayIndex, S_Ts_Parser_Context*  pTsContext)
{
	uint8*  pNewBuffer = NULL;
	if(pTsContext->aPesBufferMaxSize[iArrayIndex] < (pTsContext->aPesBufferSize[iArrayIndex]) + ulDataSize)
	{
		pNewBuffer = (uint8*)realloc(pTsContext->aPesBufferData[iArrayIndex], (pTsContext->aPesBufferSize[iArrayIndex]) + ulDataSize + DEFAULT_MAX_BUFFER_SIZE_FOR_PES);
		if (pNewBuffer == NULL)
		{
			pNewBuffer = pTsContext->aPesBufferData[iArrayIndex];
			printf("not enough memory!");
		}
		else
		{
			pTsContext->aPesBufferData[iArrayIndex] = pNewBuffer;
			pTsContext->aPesBufferMaxSize[iArrayIndex] = (pTsContext->aPesBufferSize[iArrayIndex]) + ulDataSize + DEFAULT_MAX_BUFFER_SIZE_FOR_PES;
		}
	}

	if (pTsContext->aPesBufferMaxSize[iArrayIndex] >= (pTsContext->aPesBufferSize[iArrayIndex]) + ulDataSize)
	{
		memcpy(pTsContext->aPesBufferData[iArrayIndex] + pTsContext->aPesBufferSize[iArrayIndex],
			pData, ulDataSize);
		pTsContext->aPesBufferSize[iArrayIndex] += ulDataSize;
	}

}


static void ParseElementData(uint8*  pTsData, uint32  ulTsSize, int iStartFlag, int iArrayIndex, S_Ts_Parser_Context*  pTsContext)
{
	bool  bPesLoad = false;
	if(iStartFlag == 1)
	{
		if(pTsContext->aPesBufferSize[iArrayIndex] != 0)
		{
			DoTheCallbackForFrameData(iArrayIndex, pTsContext);
		}

		pTsContext->aPesBufferSize[iArrayIndex] = 0;
		pTsContext->aPesTimeStamp[iArrayIndex] = 0;
		PESPacket   sPes;
		bPesLoad = sPes.Load(pTsData, ulTsSize);
		if(bPesLoad == true)
		{
			pTsContext->aPesLengthInPesInfo[iArrayIndex] = sPes.payloadsize;
			pTsContext->aPesTimeStamp[iArrayIndex] = (uint32) (sPes.PTS*1000/90000);
			CommitFrameData(sPes.data, sPes.datasize, iArrayIndex, pTsContext);
		}
	}
	else
	{
		CommitFrameData(pTsData, ulTsSize, iArrayIndex, pTsContext);
	}
}

static int  ParsePAT(uint8*  pTsData, uint32  ulTsSize, S_Ts_Parser_Context*  pTsContext)
{
	uint16  uProgramNum = 0;
    uint16  uPID = 0;
	uint8  uTableId = 0;
	uint16 usSectionLength = 0;
	uint16 transport_stream_id = 0;
	uint8  version_number = 0;
	uint8  current_next_indicator;
	uint8   section_number;
	uint8   last_section_number;
    uint8*   pEnd = NULL;

	if(ulTsSize < 3)
	{
		return 1;
	}
	
	uint8* p = pTsData;
	BitStream bs(p);

	pEnd = pTsData+ulTsSize;

	bit1 section_syntax_indicator; 
	bs.ReadBits(8, uTableId);
	bs.ReadBits(1, section_syntax_indicator);
	bs.SkipBits(3);
	bs.ReadBits(12, usSectionLength);

	if ((bs.Position() + usSectionLength) < pEnd)
	{
		pEnd = bs.Position() + usSectionLength;
	}

	bs.ReadBits(16, transport_stream_id);
	bs.SkipBits(2); //reserved
	bs.ReadBits(5, version_number);
	bs.ReadBits(1, current_next_indicator);
	bs.ReadBits(8, section_number);
	bs.ReadBits(8, last_section_number);

	//Only One Program (the Program Num should not be 0)
    while((bs.Position()+4) < pEnd)
	{
		bs.ReadBits(16, uProgramNum);
		bs.SkipBits(3);
		bs.ReadBits(13, uPID);
		if(uProgramNum != 0)
		{
			break;
		}
	}

	if (pTsContext->eState != MPEGTS_PAYLOAD)
	{
		pTsContext->sProInfo.usPMTPID = uPID;
		pTsContext->sProInfo.usProNum = uProgramNum;
		pTsContext->eState = MPEGTS_PMT;
	}
	else
	{
		pTsContext->sProInfoLast.usPMTPID = uPID;
		pTsContext->sProInfoLast.usProNum = uProgramNum;
	}

	return 0;
}

static int    ParsePMT(uint8*  pTsData, uint32  ulTsSize, S_Ts_Parser_Context*  pTsContext)
{
	uint16  uProgramNum = 0;
	uint16  uPID = 0;
    uint16  uProgram_info_length = 0;
	uint16  uPCRPID = 0;
	uint8  uTableId = 0;
	uint16 usSectionLength = 0;
	uint16 transport_stream_id = 0;
	uint8  version_number = 0;
	uint8  current_next_indicator;
	uint8   section_number;
	uint8   last_section_number;
	uint8   uStreamType = 0;
    uint16  usElementPID = 0;
    uint16  ulElementInfoLength = 0;
	uint8*   pEnd = 0;
	uint8      uDescType = 0;
	uint8      uDescSize = 0;
	int iRet = 0;
	S_Program_Info*   pProInfo = NULL;
	S_Track_Info_From_Desc   sTrackInfoFromDesc = { 0 };



	if(ulTsSize < 3)
	{
		return 1;
	}

	uint8* p = pTsData;
	BitStream bs(p);

	pEnd = pTsData+ulTsSize;

	bit1 section_syntax_indicator; 
	bs.ReadBits(8, uTableId);
	bs.ReadBits(1, section_syntax_indicator);
	bs.SkipBits(3);
	bs.ReadBits(12, usSectionLength);
	
	if ((bs.Position() + usSectionLength) < pEnd)
	{
		pEnd = bs.Position() + usSectionLength;
	}

	if (pTsContext->eState != MPEGTS_PAYLOAD)
	{
		pProInfo = &(pTsContext->sProInfo);
	}
	else
	{
		pProInfo = &(pTsContext->sProInfoLast);
		pProInfo->usPIDCount = 0;
	}

	bs.ReadBits(16, uProgramNum);
	if (pProInfo->usProNum != uProgramNum)
	{
		return 1;
	}

	bs.SkipBits(2); //reserved
	bs.ReadBits(5, version_number);
	bs.ReadBits(1, current_next_indicator);
	bs.ReadBits(8, section_number);
	bs.ReadBits(8, last_section_number);
	bs.SkipBits(3); //reserved
	bs.ReadBits(13, uPCRPID);
	bs.SkipBits(4); //reserved
	bs.ReadBits(12, uProgram_info_length);
	
	//Skip the Program Infor
	bs.SkipBytes(uProgram_info_length);
	

	//Only One Program (the Program Num should not be 0)
	while((bs.Position()+4) < pEnd)
	{
		bs.ReadBits(8, uStreamType);
		bs.SkipBits(3);
		bs.ReadBits(13, usElementPID);
		bs.SkipBits(4);
		bs.ReadBits(12, ulElementInfoLength);
		if (uStreamType == STREAM_TYPE_PES_PRIVATE)
		{
			bs.ReadBits(8, uDescType);
			bs.ReadBits(8, uDescSize);
			iRet = ParseDesc(uDescType, bs.Position(), uDescSize, &sTrackInfoFromDesc);
			bs.SkipBytes(ulElementInfoLength-2);
		}
		else
		{
			bs.SkipBytes(ulElementInfoLength);
		}
		//Add the Stream Type and Stream PID to the Array
		pProInfo->aPIDs[pProInfo->usPIDCount] = usElementPID;
		pProInfo->aStreamTypes[pProInfo->usPIDCount] = uStreamType;
		pProInfo->aSEsInfo[pProInfo->usPIDCount] = sTrackInfoFromDesc;
		pProInfo->usPIDCount++;
	}

	return 0;
}


static void AllocBufferForPes(S_Ts_Parser_Context*  pTsContext)
{
	uint8*   pBuffer = NULL;
	int   iIndex = 0;
    if(pTsContext->sProInfo.usPIDCount > 0)
	{
		pTsContext->usPesPIDCount = pTsContext->sProInfo.usPIDCount;
		for(iIndex=0; iIndex<pTsContext->usPesPIDCount; iIndex++)
		{
			pTsContext->aPesBufferData[iIndex] = pBuffer = (uint8*)malloc(DEFAULT_MAX_BUFFER_SIZE_FOR_PES);
			pTsContext->aPesBufferMaxSize[iIndex] = DEFAULT_MAX_BUFFER_SIZE_FOR_PES;
			pTsContext->aPesBufferSize[iIndex] = 0;
			pTsContext->aPesPID[iIndex] = pTsContext->sProInfo.aPIDs[iIndex];
			pTsContext->aPesElementType[iIndex] =  pTsContext->sProInfo.aStreamTypes[iIndex];
		} 
	}
}

static int ProcessInnerForStateNo(S_Ts_Parser_Context*  pTsContext)
{
	int   iPacketSize = 0;
	if(pTsContext->ulCurSizeForCacheHeader >= DEFAULT_FIND_PACKET_SIZE_LEN)
	{
		iPacketSize = CheckPacketSize(pTsContext->pBufferForCacheHeader, pTsContext->ulCurSizeForCacheHeader);
		if(iPacketSize != 0)
		{
			pTsContext->ulTsPacketSize = iPacketSize;
			return 0;
		}
	}

	return 1;
}


static void ProcessInnerForPAT(uint8*  pTsData, uint32  ulTsSize, S_Ts_Parser_Context*  pTsContext)
{
	RawPacket   sPacket = {0};
	uint8*  pCur = NULL;
    uint8*  pEnd = NULL;

	if((pTsContext->ulCurSizeForCacheHeader+ulTsSize) < pTsContext->ulMaxSizeForCacheHeader)
	{
		memcpy(pTsContext->pBufferForCacheHeader+pTsContext->ulCurSizeForCacheHeader, pTsData, ulTsSize);
		pTsContext->ulCurSizeForCacheHeader += ulTsSize;
	}

	//The Cache Size must large than pTsContext->ulTsPacketSize*4
	pCur = FindPacketHeader(pTsContext->pBufferForCacheHeader, pTsContext->ulTsPacketSize*4, pTsContext->ulTsPacketSize);
    pEnd = pTsContext->pBufferForCacheHeader + pTsContext->ulCurSizeForCacheHeader;
	while(pCur < pEnd)
	{
		ParseOnePacket(&sPacket, pCur, pTsContext->ulTsPacketSize);
		if(sPacket.PID == PAT_TID)
		{
			ParsePAT(sPacket.data, sPacket.datasize, pTsContext);
			break;
		}
		pCur += pTsContext->ulTsPacketSize;
	}
}

static int  FindStreamInfoInTsCtx(short uPID, short uStreamType, S_Ts_Parser_Context*  pTsContext, int* pIndexValue)
{
	int iFind = 0;
	int iIndex = 0;
	for (iIndex = 0; iIndex < pTsContext->usPesPIDCount; iIndex++)
	{
		if (uPID == pTsContext->aPesPID[iIndex])
		{
			if (uStreamType == pTsContext->aPesElementType[iIndex])
			{
				iFind = 1;
			}
			else
			{
				iFind = 2;
			}

			*pIndexValue = iIndex;
			break;
		}
	}

	return iFind;
}

static void UpdatePesContextForNewPMT(S_Ts_Parser_Context*  pTsContext)
{
	uint16  uCurPID = 0;
	uint16  uCurElemType = 0;
	int iFind = 0;
	int iPreIndex = 0;
	int iIndexOri = 0;
	int iIndexNew = 0;
	int    iCurPesCount = pTsContext->usPesPIDCount;

	for (iIndexNew = 0; iIndexNew < pTsContext->sProInfoLast.usPIDCount; iIndexNew++)
	{
		iFind = 0;
		uCurPID = pTsContext->sProInfoLast.aPIDs[iIndexNew];
		uCurElemType = pTsContext->sProInfoLast.aStreamTypes[iIndexNew];

		iFind = FindStreamInfoInTsCtx(uCurPID, uCurElemType, pTsContext, &iPreIndex);
		switch (iFind)
		{
			case 0:
			{
				pTsContext->aPesBufferData[iCurPesCount] = (uint8*)malloc(DEFAULT_MAX_BUFFER_SIZE_FOR_PES);
				pTsContext->aPesBufferMaxSize[iCurPesCount] = DEFAULT_MAX_BUFFER_SIZE_FOR_PES;
				pTsContext->aPesBufferSize[iCurPesCount] = 0;
				pTsContext->aPesPID[iCurPesCount] = uCurPID;
				pTsContext->aPesElementType[iCurPesCount] = uCurElemType;
				iCurPesCount++;
				pTsContext->usPesPIDCount++;
				break;
			}

			case 2:
			{
				pTsContext->aPesBufferSize[iPreIndex] = 0;
				pTsContext->aPesElementType[iPreIndex] = uCurElemType;
				break;
			}
		}

		/*
		if (iFind == 0)
		{

		}

		if (iFind == 2)
		{
			//change the stream type in the same PID, reset context
			pTsContext->aPesBufferSize[iIndexOri] = 0;
			pTsContext->aPesElementType[iIndexOri] = uCurElemType;
		}
		*/
	}
}



static void ProcessInner(uint8*  pTsData, uint32  ulTsSize, S_Ts_Parser_Context*  pTsContext)
{
	RawPacket   sPacket = {0};
	uint8*  pCur = NULL;
	uint8*  pEnd = NULL;
	uint8*  pStartForHeader = NULL;
	int       iNeedRollback = 0;
	int       iArrayIndex = 0;
	int       iCopySize = 0;
	int       iPSIOffsetValue = 0;


	if(ulTsSize == 0 || *pTsData != 0x47)
	{
		int i=0;
	}

	if(pTsContext == NULL || pTsData == NULL)
	{
		return;
	}

	if(pTsContext->eState != MPEGTS_PAYLOAD)
	{
		if((pTsContext->ulCurSizeForCacheHeader+ulTsSize) < pTsContext->ulMaxSizeForCacheHeader)
		{
			memcpy(pTsContext->pBufferForCacheHeader+pTsContext->ulCurSizeForCacheHeader, pTsData, ulTsSize);
			pTsContext->ulCurSizeForCacheHeader += ulTsSize;
		}
		else
		{
			iCopySize = pTsContext->ulMaxSizeForCacheHeader-pTsContext->ulCurSizeForCacheHeader;
			memcpy(pTsContext->pBufferForCacheHeader+pTsContext->ulCurSizeForCacheHeader, pTsData, iCopySize);
			pTsContext->ulCurSizeForCacheHeader += iCopySize;
		}

		if(pTsContext->eState == MPEGTS_NO)
		{
			if(ProcessInnerForStateNo(pTsContext) != 0)
			{
				return;
			}
			pTsContext->eState = MPEGTS_PAT;
		}

		pCur = FindPacketHeader(pTsContext->pBufferForCacheHeader, pTsContext->ulTsPacketSize*4, pTsContext->ulTsPacketSize);
		pStartForHeader = pCur;
		pEnd = pTsContext->pBufferForCacheHeader + pTsContext->ulCurSizeForCacheHeader;
	}
	else
	{
		pCur = pTsData;
		pEnd = pTsData+ulTsSize;
	}

	while(pCur<pEnd)
    {
		iPSIOffsetValue = 0;
		ParseOnePacket(&sPacket, pCur, pTsContext->ulTsPacketSize);
		switch (pTsContext->eState)
		{
            case  MPEGTS_PAT:
			{
				if(sPacket.PID == PAT_PID)
				{
					iPSIOffsetValue = sPacket.data[0];
					if (ParsePAT(sPacket.data + 1 + iPSIOffsetValue, sPacket.datasize - 1 - iPSIOffsetValue, pTsContext) == 0)
					{
                        iNeedRollback = 1;
						pTsContext->eState = MPEGTS_PMT;
					}
				}
				break;
			}
			 
			case  MPEGTS_PMT:
			{
				if(sPacket.PID == pTsContext->sProInfo.usPMTPID)
				{
					iPSIOffsetValue = sPacket.data[0];
					if (ParsePMT(sPacket.data + 1 + iPSIOffsetValue, sPacket.datasize - 1 - iPSIOffsetValue, pTsContext) == 0)
					{
						iNeedRollback = 1;
						pTsContext->eState = MPEGTS_PAYLOAD;
						AllocBufferForPes(pTsContext);
					}
				}
				break;
			}
			
			case  MPEGTS_PAYLOAD:
			{
				if (sPacket.PID == 0)
				{
					iPSIOffsetValue = sPacket.data[0];
					ParsePAT(sPacket.data + 1 + iPSIOffsetValue, sPacket.datasize - 1 - iPSIOffsetValue, pTsContext);
				}

				if (sPacket.PID == pTsContext->sProInfoLast.usPMTPID)
				{
					iPSIOffsetValue = sPacket.data[0];
					ParsePMT(sPacket.data + 1 + iPSIOffsetValue, sPacket.datasize - 1 - iPSIOffsetValue, pTsContext);
					UpdatePesContextForNewPMT(pTsContext);
					//memset(&(pTsContext->sProInfoLast), 0, sizeof(S_Program_Info));
				}

				iArrayIndex = FindPIDInContext(sPacket.PID, pTsContext);
				if(iArrayIndex != 0xffff)
				{
					ParseElementData(sPacket.data, sPacket.datasize, sPacket.payload_unit_start_indicator, iArrayIndex, pTsContext);
				}
				break;
			}
		}

		if(iNeedRollback == 1)
		{
			pCur = pStartForHeader;
			iNeedRollback = 0;
		}
		else
		{
			pCur += pTsContext->ulTsPacketSize;
		}
	}
}


int   InitTsParser(CM_PARSER_INIT_INFO*   pParserInit, S_Ts_Parser_Context*  pTsContext)
{
    if(pParserInit == NULL || pTsContext == NULL)
	{
		return AVERROR_INVALIDDATA;
	}

	memset(pTsContext, 0, sizeof(S_Ts_Parser_Context));
	pTsContext->pBufferForCacheHeader = (uint8* )malloc(DEFAULT_MAX_CACHE_SIZE_FOR_HEADER);
    pTsContext->ulMaxSizeForCacheHeader = DEFAULT_MAX_CACHE_SIZE_FOR_HEADER;
	pTsContext->ulCurSizeForCacheHeader = 0;

	pTsContext->eState = MPEGTS_NO;
	pTsContext->sCallbackProcInfo.pProc = pParserInit->pProc;
	pTsContext->sCallbackProcInfo.pUserData = pParserInit->pUserData;
    return 0;
}

int   ProcessTs(uint8*  pTsData, uint32  ulTsSize, S_Ts_Parser_Context*  pTsContext)
{
	uint8*   pTsStart = NULL;
	uint8*   pTsEnd = NULL;

    if((pTsContext->ulBufferSizeForPartTs+ulTsSize)<TS_PACKET_SIZE)
	{
		memcpy(pTsContext->aBufferForPartTs+pTsContext->ulBufferSizeForPartTs, pTsData, ulTsSize);
		pTsContext->ulBufferSizeForPartTs += ulTsSize;
		return 0;
	}
	else
	{
		if(pTsContext->ulBufferSizeForPartTs != 0)
		{
			memcpy(pTsContext->aBufferForPartTs+pTsContext->ulBufferSizeForPartTs, pTsData, (TS_PACKET_SIZE-pTsContext->ulBufferSizeForPartTs));
			ProcessInner(pTsContext->aBufferForPartTs, TS_PACKET_SIZE, pTsContext);
			pTsStart = pTsData + (TS_PACKET_SIZE-pTsContext->ulBufferSizeForPartTs);
			pTsEnd = pTsStart + (ulTsSize-(TS_PACKET_SIZE-pTsContext->ulBufferSizeForPartTs))/TS_PACKET_SIZE*TS_PACKET_SIZE;
			pTsContext->ulBufferSizeForPartTs = 0;
		}
		else
		{
			pTsStart = pTsData;
			pTsEnd = pTsStart + ulTsSize/TS_PACKET_SIZE*TS_PACKET_SIZE;
		}
	}



	ProcessInner(pTsStart, pTsEnd-pTsStart, pTsContext);
	if(pTsEnd <(pTsData+ulTsSize))
	{
		memcpy(pTsContext->aBufferForPartTs+pTsContext->ulBufferSizeForPartTs, pTsEnd, (pTsData+ulTsSize-pTsEnd));
		pTsContext->ulBufferSizeForPartTs += (pTsData+ulTsSize-pTsEnd);
	}
	return 0;
}

int   FlushAllCacheData(S_Ts_Parser_Context*  pTsContext)
{
	int iIndex = 0;
	do 
	{
		for (iIndex = 0; iIndex < pTsContext->usPesPIDCount; iIndex++)
		{
			if (pTsContext->aPesBufferSize[iIndex] != 0)
			{
				DoTheCallbackForFrameData(iIndex, pTsContext);
				pTsContext->aPesBufferSize[iIndex] = 0;
				pTsContext->aPesTimeStamp[iIndex] = 0;
			}
		}

	} while (false);

	return 0;
}


int   UnInitTsParser( S_Ts_Parser_Context*  pTsContext)
{
	int iIndex = 0;
	if(pTsContext->pBufferForCacheHeader != NULL)
	{
		free(pTsContext->pBufferForCacheHeader);
		pTsContext->pBufferForCacheHeader = NULL;
	}

	
	for(iIndex=0; iIndex<MAX_PIDS_PER_PROGRAM; iIndex++)
    {
		if(pTsContext->aPesBufferData[iIndex] != NULL)
		{
			free(pTsContext->aPesBufferData[iIndex]);
		}
    }

	return 0;
}

int   GetEsTrackInfoByPID(S_Ts_Parser_Context*  pTsContext, unsigned short ulPID, S_Track_Info_From_Desc*  pDesc)
{
	int iIndex = 0;
	int iRet = 0;

	for (iIndex = 0; iIndex < pTsContext->usPesPIDCount; iIndex++)
	{
		if (pTsContext->sProInfo.aPIDs[iIndex] == ulPID)
		{
			*pDesc = pTsContext->sProInfo.aSEsInfo[iIndex];
			iRet = 1;
			break;
		}
	}

	return iRet;
}
