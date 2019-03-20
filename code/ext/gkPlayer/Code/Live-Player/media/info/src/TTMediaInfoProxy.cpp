/**
* File : TTMediaInfoProxy.cpp
* Created on : 2011-3-23
* Author : lin.yongping
* Description : CTTMediaInfoProxyʵ���ļ�
*/

//INCLUDES
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "GKOsalConfig.h"
#include "TTLog.h"
#include "GKTypedef.h"
#include "GKMacrodef.h"
#include "TTUrlParser.h"
#include "TTIntReader.h"
#include "TTMediainfoProxy.h"
#include "TTFileReader.h"
#include "TTHttpReader.h"
#include "TTBufferReader.h"
#include "TTID3Tag.h"
#include "TTAACHeader.h"
#include "TTAACParser.h"
#include "TTMP4Parser.h"
#include "TTFLVParser.h"
#include "TTHttpAACParser.h"

#include "TTHttpReaderProxy.h"
#include "TTHttpClient.h"
#include "TTSysTime.h"

extern const bool gGetCacheFileEnble();

typedef struct MediaFormatMap_s
{
	const TTChar*					iMediaFormatExt;
	CTTMediaInfoProxy::TTMediaFormatId	iMediaFormatId;
} MediaFormatMap_t;

static const MediaFormatMap_t KMediaFormatMap[] = { {"AAC", CTTMediaInfoProxy::EMediaExtIdAAC},
													{"MP4", CTTMediaInfoProxy::EMediaExtIdM4A},
													{"M4A", CTTMediaInfoProxy::EMediaExtIdM4A},
													{"MP3", CTTMediaInfoProxy::EMediaExtIdMP3},
													{"FLV", CTTMediaInfoProxy::EMediaExtIdFLV},
												  };

static const TTInt KMAXExtSize = 16;

static const TTInt KFLVFlagSize = 3;
static const TTUint8 KFLVFlag[KFLVFlagSize] =
{
	0x46,0x4c,0x56//"FLV "
};

static const TTInt KM4AFlagSize = 4;
static const TTUint8 KM4AFlag[KM4AFlagSize] =
{
	0x66,0x74,0x79,0x70//"ftyp"
};


CTTMediaInfoProxy::CTTMediaInfoProxy(TTObserver* aObserver)
: iMediaParser(NULL)
, iDataReader(NULL)
, iObserver(aObserver)
{
	iCriEvent.Create();
}

CTTMediaInfoProxy::~CTTMediaInfoProxy()
{
	SAFE_RELEASE(iDataReader);
	iCriEvent.Destroy();
}

TTInt CTTMediaInfoProxy::Open(const TTChar* aUrl, TTInt aFlag)
{
    if (iMediaParser != NULL) {
        SAFE_DELETE(iMediaParser);
        if (iDataReader != NULL)
        {
            iDataReader->Close();
        }
        iMediaInfo.Reset();
    }
	GKASSERT(iMediaParser == NULL);

	AdaptSrcReader(aUrl, aFlag);

	if (iDataReader == NULL) 
	{
		return TTKErrAccessDenied;
	}

	ITTDataReader::TTDataReaderId tReaderId = iDataReader->Id();
	LOGI("CTTMediaInfoProxy::Open ReaderId: %d", tReaderId);

	if (tReaderId == ITTDataReader::ETTDataReaderIdHttp)
	{
		//LOGI("CTTMediaInfoProxy::Open SetStreamBufferingObserver");
		((CTTHttpReader*)iDataReader)->SetStreamBufferingObserver(this);
		//LOGI("CTTMediaInfoProxy::Open SetStreamBufferingObserver ok");
	}
    
	if (tReaderId == ITTDataReader::ETTDataReaderIdBuffer)
	{
		//LOGI("CTTMediaInfoProxy::Open SetStreamBufferingObserver");
		((CTTBufferReader*)iDataReader)->SetStreamBufferingObserver(this);
		//LOGI("CTTMediaInfoProxy::Open SetStreamBufferingObserver ok");
	}

    TTInt nErr = iDataReader->Open(aUrl);
    
	if (nErr == TTKErrNone)
	{
		switch (tReaderId)
		{
		case ITTDataReader::ETTDataReaderIdFile:
			nErr = AdaptLocalFileParser(aUrl);
			break;

		case ITTDataReader::ETTDataReaderIdBuffer:
		case ITTDataReader::ETTDataReaderIdHttp:
			nErr = AdaptHttpFileParser(aUrl);
			break;
		default:
			GKASSERT(ETTFalse);
			break;
		}

		//nErr = (nErr != TTKErrNone) ? nErr : ((iMediaParser == NULL) ? TTKErrNoMemory : TTKErrNone);
		if (nErr == TTKErrNone)
		{
			nErr = (iMediaParser == NULL) ? TTKErrNoMemory : TTKErrNone;
		}
	}
	LOGI("CTTMediaInfoProxy::Open return: %d", nErr);
	return nErr;
}

TTInt CTTMediaInfoProxy::AdaptHttpFileParser(const TTChar* aUrl)
{
	TTMediaFormatId tMediaFormatId = IdentifyMedia(*iDataReader, aUrl);

	if (tMediaFormatId == EMediaExtIdAAC)
	{
		iMediaParser = new CTTHttpAACParser(*iDataReader, *this);
	}
	else if (tMediaFormatId == EMediaExtIdM4A)
	{
		iMediaParser = new CTTMP4Parser(*iDataReader, *this);
	}
	else if(tMediaFormatId == EMediaExtIdFLV) 
	{
		iMediaParser = new CTTFLVParser(*iDataReader, *this);
	}
	else
	{
		LOGE("HttpSource Error:url = %s, formatId = %d", aUrl, tMediaFormatId);
		return TTKErrOnLineFormatNotSupport;
	}

	return TTKErrNone;
}


TTInt CTTMediaInfoProxy::AdaptLocalFileParser(const TTChar* aUrl)
{
	TTInt nErr = TTKErrNone;

	TTMediaFormatId tMediaExtId = IdentifyMedia(*iDataReader, aUrl);
	switch (tMediaExtId)
	{
	case EMediaExtIdAAC:
		iMediaParser = new CTTAACParser(*iDataReader, *this);
		break;
	case EMediaExtIdM4A:
		iMediaParser = new CTTMP4Parser(*iDataReader, *this);
		break;
	case EMediaExtIdFLV:
		iMediaParser = new CTTFLVParser(*iDataReader, *this);
		break;
	default:
		GKASSERT(ETTFalse);
		nErr = TTKErrNotSupported;
		break;
	}

	//GKASSERT(iMediaParser != NULL);
	LOGI("AdaptLocalFileParser return: %d", nErr);
	return nErr;
}

TTInt CTTMediaInfoProxy::Parse()
{
 	GKASSERT(iMediaParser != NULL);
 	TTInt nErr = iMediaParser->Parse(iMediaInfo);

	if(iDataReader->Id() == ITTDataReader::ETTDataReaderIdFile) {
		BufferingDone();
	}
	return nErr;
}

TTInt CTTMediaInfoProxy::Parse(TTInt aErrCode)
{
	return TTKErrNotSupported;
}

void CTTMediaInfoProxy::Close()
{	
	LOGI("CTTMediaInfoProxy::Close");
	//LOGI("CTTMediaInfoProxy::Close close Reader use time %lld", nEnd - nStart);
	//nStart = GetTimeOfDay();
	SAFE_DELETE(iMediaParser);
	//nEnd = GetTimeOfDay();
	//LOGI("CTTMediaInfoProxy::Close release iMediaParser use time %lld", nEnd - nStart);
	//nStart = GetTimeOfDay();
	if (iDataReader != NULL)
	{
		iDataReader->Close();
	}
	//nEnd = GetTimeOfDay();

	iMediaInfo.Reset();
}


TTBool CTTMediaInfoProxy::IsFLV(const TTUint8* aHeader)
{
	return (memcmp(aHeader, KFLVFlag, KFLVFlagSize) == 0);
}

TTBool CTTMediaInfoProxy::IsM4A(const TTUint8* aHeader)
{
	return (memcmp(aHeader + 4, KM4AFlag, KM4AFlagSize) == 0);
}

TTBool CTTMediaInfoProxy::IdentifyAACByBuffer(ITTDataReader& aDataReader)
{
	TTUint8 tHeader[KMaxMediaBufferSize];
	TTInt nID3V2Size = ID3v2TagSize(aDataReader);

	AAC_HEADER mh;
	AAC_FRAME_INFO mi;
	AAC_FRAME_INFO mi_1;
	TTBool isAAC = ETTFalse;
	TTInt nCount = 0;
	TTInt i = 0;

	do {
		if (aDataReader.ReadSync((TTUint8*)tHeader, nID3V2Size + nCount*(KMaxMediaBufferSize - 8), KMaxMediaBufferSize) != KMaxMediaBufferSize)
			return EMediaExtIdNone;

		for (i = 0; i < KMaxMediaBufferSize - 8; i++) 
		{
			if ( (tHeader[i+0] & 0xFF) == 0xff && (tHeader[i+1] & 0xF0) == 0xF0) {
				isAAC = CTTAACHeader::AACCheckHeader(tHeader + i, mh) && CTTAACHeader::AACParseFrame(mh, mi);
			}
		
			if(isAAC) {
				if(i + mi.nFrameSize < KMaxMediaBufferSize - 8) {
					isAAC = CTTAACHeader::AACCheckHeader(tHeader + i + mi.nFrameSize, mh) && CTTAACHeader::AACParseFrame(mh, mi_1);
					if(isAAC && mi.nChannels == mi_1.nChannels && mi.nSampleRate == mi_1.nSampleRate 
						&& mi.nAACType == mi_1.nAACType) {
						break;
					}
				}			
			}

			isAAC = 0;
		}

		if(isAAC) {
			break;
		}

		nCount++;
	} while(nCount < 10);

	return isAAC;	
}

TTBool CTTMediaInfoProxy::ShouldIdentifiedByHeader(TTMediaFormatId aMediaFormatId)
{
	return (aMediaFormatId == EMediaExtIdM4A)
		|| (aMediaFormatId == EMediaExtIdFLV);
}

CTTMediaInfoProxy::TTMediaFormatId CTTMediaInfoProxy::IdentifyMedia(ITTDataReader& aDataReader, const TTChar* aUrl)
{
	TTMediaFormatId tMediaFormatId = IdentifyMediaByHeader(aDataReader);
	if (tMediaFormatId != EMediaExtIdNone)
	{
		return tMediaFormatId;
	}

	if(IdentifyAACByBuffer(aDataReader)) {
		return EMediaExtIdAAC;
	}
	
	tMediaFormatId = IdentifyMediaByExtension(aUrl);

	return tMediaFormatId;
}

CTTMediaInfoProxy::TTMediaFormatId CTTMediaInfoProxy::IdentifyMediaByHeader(ITTDataReader& aDataReader)
{
	TTUint8 tHeader[KMaxMediaFlagSize];
	TTInt64 nID3V2Size = ID3v2TagSize(aDataReader);

	if (aDataReader.ReadSync((TTUint8*)tHeader, nID3V2Size, KMaxMediaFlagSize) != KMaxMediaFlagSize)
		return EMediaExtIdNone;

	TTMediaFormatId tMediaFormatId = EMediaExtIdNone;
	
    if (IsM4A(tHeader))
	{
		tMediaFormatId = EMediaExtIdM4A;
	}
	else if(IsFLV(tHeader))
	{
		tMediaFormatId = EMediaExtIdFLV;
	}

	return tMediaFormatId;
}

CTTMediaInfoProxy::TTMediaFormatId CTTMediaInfoProxy::IdentifyMediaByExtension(const TTChar* aUrl)
{
	TTChar tExtStr[KMAXExtSize];
	CTTUrlParser::ParseExtension(aUrl, tExtStr);

	for (TTInt i = sizeof(KMediaFormatMap) / sizeof(MediaFormatMap_t) - 1; i >= 0; --i)
	{
		if (strcmp(tExtStr, KMediaFormatMap[i].iMediaFormatExt) == 0)
		{
			return KMediaFormatMap[i].iMediaFormatId;			
		}
	}

	return EMediaExtIdNone;
}

const TTMediaInfo& CTTMediaInfoProxy::GetMediaInfo()
{
	return iMediaInfo;
}

TTUint CTTMediaInfoProxy::MediaSize()
{
	return (iDataReader != NULL) ? iDataReader->Size() : 0;
}

TTBool CTTMediaInfoProxy::IsSeekAble()
{
	return ETTTrue;
}

void CTTMediaInfoProxy::CreateFrameIndex()
{
	iMediaParser->StartFrmPosScan();
}

TTInt CTTMediaInfoProxy::GetFrameLocation(TTInt aId, TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo)
{
	if(iMediaParser == NULL)
		return  TTKErrNotFound;

	return iMediaParser->GetFrameLocation(aId, aFrmIdx, aFrameInfo);
}

TTInt CTTMediaInfoProxy::GetFrameLocation(TTInt aStreamId, TTInt& aFrmIdx, TTUint64 aTime)
{
	if(iMediaParser == NULL)
		return  TTKErrNotFound;

	return iMediaParser->GetFrameLocation(aStreamId, aFrmIdx, aTime);
}

TTInt CTTMediaInfoProxy::GetMediaSample(TTMediaType aStreamType, TTBuffer* pMediaBuffer)
{
	if(iMediaParser == NULL)
		return  TTKErrNotFound;

	GKASSERT((iMediaParser != NULL));
	return iMediaParser->GetMediaSample(aStreamType, pMediaBuffer);
}

TTUint CTTMediaInfoProxy::MediaDuration()
{
	if(iMediaParser == NULL)
		return  TTKErrNotFound;

	GKASSERT((iMediaParser != NULL));

	return iMediaParser->MediaDuration();
}

void CTTMediaInfoProxy::CreateFrameIdxComplete()
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyUpdateDuration, TTKErrNone, 0, NULL);
}

TTBool CTTMediaInfoProxy::IsCreateFrameIdxComplete()
{
	if(iMediaParser == NULL)
		return  TTKErrNotFound;

	GKASSERT(iMediaParser != NULL);
	return iMediaParser->IsCreateFrameIdxComplete();
}

void CTTMediaInfoProxy::BufferingEmtpy(TTInt nErr, TTInt nStatus, TTUint32 aParam)
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver && iObserver->pObserver) {
		char *pParam3 = NULL;
		if(aParam)
			pParam3 = inet_ntoa(*(struct in_addr*)&aParam);
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyBufferingStart, nErr, nStatus, pParam3);
	}
}

void CTTMediaInfoProxy::BufferingReady()
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyBufferingDone, TTKErrNone, 0, NULL);
}

TTUint CTTMediaInfoProxy::BufferedSize()
{
	if (iDataReader != NULL){
		return iDataReader->BufferedSize();
	} 

	return 0;
}

TTUint CTTMediaInfoProxy::ProxySize()
{
	if (iDataReader != NULL){
		return iDataReader->ProxySize();
	} 

	return 0;
}

TTUint CTTMediaInfoProxy::BandWidth()
{
	if (iDataReader != NULL){
		return iDataReader->BandWidth();
	} 

	return 0;
}

void CTTMediaInfoProxy::SetDownSpeed(TTInt aFast)
{
	if (iDataReader != NULL){
		return iDataReader->SetDownSpeed(aFast);
	} 
}

TTUint CTTMediaInfoProxy::BandPercent()
{
	if (iDataReader != NULL){
		return iDataReader->BandPercent();
	} 

	return 0;
}

TTInt CTTMediaInfoProxy::BufferedPercent(TTInt& aBufferedPercent)
{
	if (iDataReader != NULL) {
		TTInt64 nBufferSize = iDataReader->BufferedSize();
		TTInt64 nTotalSize = iDataReader->Size();
		if(nTotalSize > 0) {
			aBufferedPercent = nBufferSize * 100 / nTotalSize;
		} else {
			aBufferedPercent = 0;
		}
		return TTKErrNone;
	} 

	return TTKErrNotSupported;
}

TTInt CTTMediaInfoProxy::SelectStream(TTMediaType aType, TTInt aStreamId)
{
	if(iMediaParser == NULL)
		return  TTKErrNotFound;

	GKASSERT(iMediaParser != NULL);
	return iMediaParser->SelectStream(aType, aStreamId);
}

TTInt64 CTTMediaInfoProxy::Seek(TTUint64 aPosMS, TTInt aOption)
{
	if(iMediaParser == NULL)
		return  TTKErrNotFound;

	GKASSERT(iMediaParser != NULL);
	return iMediaParser->Seek(aPosMS, aOption);
}

TTInt CTTMediaInfoProxy::SetParam(TTInt aType, TTPtr aParam)
{
	if(iMediaParser == NULL)
		return  TTKErrNotFound;

	GKASSERT(iMediaParser != NULL);
	return iMediaParser->SetParam(aType, aParam);
}

TTInt CTTMediaInfoProxy::GetParam(TTInt aType, TTPtr aParam)
{
	if(aType == TT_PID_COMMON_STATUSCODE) {
		if(iDataReader) {
			*((TTInt *)aParam) = iDataReader->GetStatusCode();
		} else {
			*((TTInt *)aParam) = 0;
		}
		return 0;
	} else if(aType == TT_PID_COMMON_HOSTIP) {
		if(iDataReader) {
			*((TTUint *)aParam) = iDataReader->GetHostIP();
		} else {
			*((TTUint *)aParam) = 0;
		}
		return 0;
	} else if(aType == TT_PID_COMMON_LIVEDMOE) {
		*((TTUint *)aParam) = 0;
		if(iDataReader && iDataReader->Size() == 0) {
			*((TTUint *)aParam) = 1;
		}
		return 0;
	}
	
	if(iMediaParser == NULL)
		return  TTKErrNotFound;

	GKASSERT(iMediaParser != NULL);
	return iMediaParser->GetParam(aType, aParam);
}

void CTTMediaInfoProxy::AdaptSrcReader(const TTChar* aUrl, TTInt aFlag, TTBool aHarddecode)
{
	ITTDataReader::TTDataReaderId tReaderId = ITTDataReader::ETTDataReaderIdNone;
    
	LOGI("AdaptSrcReader: aUrl = %s", aUrl);
    if (IsLocalFileSource(aUrl))
	{
		tReaderId = ITTDataReader::ETTDataReaderIdFile;
	}
	else if (IsHttpSource(aUrl))
	{
		if(aFlag & 1) {
			tReaderId = ITTDataReader::ETTDataReaderIdBuffer;
		} else {
			if(gGetCacheFileEnble())
				tReaderId = ITTDataReader::ETTDataReaderIdHttp;
			else
				tReaderId = ITTDataReader::ETTDataReaderIdBuffer;
		}
	} 

   
	if (iDataReader == NULL || iDataReader->Id() != tReaderId)
	{
		SAFE_RELEASE(iDataReader);
		switch (tReaderId)
		{
		case ITTDataReader::ETTDataReaderIdFile:
			{
				iDataReader = new CTTFileReader();
				GKASSERT(iDataReader != NULL);
			}
			break;

		case ITTDataReader::ETTDataReaderIdHttp:
			{
				iDataReader = new CTTHttpReader();
				GKASSERT(iDataReader != NULL);
			}
			break;
		
		case ITTDataReader::ETTDataReaderIdBuffer:
			{
				iDataReader = new CTTBufferReader();
				GKASSERT(iDataReader != NULL);
			}
			break;
		case ITTDataReader::ETTDataReaderIdNone:
		default:
			break;
		}
	}

	LOGI("AdaptSrcReader: return. tReaderId = %d, iDataReader = %p", tReaderId, iDataReader);
}

TTBool CTTMediaInfoProxy::IsLocalFileSource(const TTChar* aUrl)
{
	return CTTFileReader::IsSourceValid(aUrl);
}

TTBool CTTMediaInfoProxy::IsHttpSource(const TTChar* aUrl)
{
#ifndef __TT_OS_WINDOWS__
	return (strncasecmp("http://", aUrl, 7) == 0);
#else
	return (strnicmp("http://", aUrl, 7) == 0);
#endif
}

void CTTMediaInfoProxy::SetNetWorkProxy(TTBool aNetWorkProxy)
{
	if(iDataReader != NULL) {
		iDataReader->SetNetWorkProxy(aNetWorkProxy);
	}
}

ITTDataReader::TTDataReaderId CTTMediaInfoProxy::GetSrcReaderId()
{
	return (iDataReader != NULL) ? iDataReader->Id() : ITTDataReader::ETTDataReaderIdNone;
}

void CTTMediaInfoProxy::CancelReader()
{
	if (iDataReader != NULL)
		iDataReader->CloseConnection();
}

void CTTMediaInfoProxy::SetObserver(TTObserver*	aObserver)
{
	GKCAutoLock lock(&iCriEvent);
	iObserver = aObserver;
}

void CTTMediaInfoProxy::BufferingStart(TTInt nErr, TTInt nStatus, TTUint32 aParam)
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver && iObserver->pObserver) {
		char *pParam3 = NULL;
		if(aParam)
			pParam3 = inet_ntoa(*(struct in_addr*)&aParam);
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyBufferingStart, nErr, nStatus, pParam3);
	}
}

void CTTMediaInfoProxy::BufferingDone()
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyBufferingDone, TTKErrNone, 0, NULL);
}

void CTTMediaInfoProxy::DNSDone()
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyDNSDone, TTKErrNone, 0, NULL);
}

void CTTMediaInfoProxy::ConnectDone()
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyConnectDone, TTKErrNone, 0, NULL);
}

void CTTMediaInfoProxy::HttpHeaderReceived()
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyHttpHeaderReceived, TTKErrNone, 0, NULL);
}

void CTTMediaInfoProxy::PrefetchStart(TTUint32 aParam)
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyPrefetchStart, TTKErrNone, aParam, NULL);
}

void CTTMediaInfoProxy::PrefetchCompleted()
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyPrefetchCompleted, TTKErrNone, 0, NULL);
}

void CTTMediaInfoProxy::CacheCompleted(const TTChar* pFileName)
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyCacheCompleted, TTKErrNone, 0, NULL);
}

void CTTMediaInfoProxy::DownLoadException(TTInt errorCode, TTInt nParam2, void *pParam3)
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyException, errorCode, nParam2, pParam3);
}

void CTTMediaInfoProxy::FileException(TTInt nReadSize)
{
	GKCAutoLock lock(&iCriEvent);
	if(iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyException, nReadSize, 0, NULL);
}
