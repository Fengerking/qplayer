/*
* File : TTFLVParser.cpp
* Created on : 2015-9-8
* Author : yongping.lin
* Description : CTTFLVParser
*/
#include "TTFLVParser.h"
#include "TTIntReader.h"
#include "GKOsalConfig.h"
#include "TTSysTime.h"
#include "TTLog.h"

CTTFLVParser::CTTFLVParser(ITTDataReader& iDataReader, ITTMediaParserObserver& aObserver)
	: CTTMediaParser(iDataReader, aObserver)
	, iDuration(1)
	, iTotalBitrate(0)
	, iOffset(0)
	, iLastBufferTime(0)
	, iNowBufferTime(0)
	, iReBufferCount(0)
	, iCountAMAX(20)
	, iCountVMAX(15)
	, iFirstTime(1)
	, iAudioStream(NULL)
	, iVideoStream(NULL)
	, iBufferStatus(-1)
	, iTagBuffer(NULL)
	, iMaxSize(NULL)
{
	mMsgThread = new TTEventThread("Status Thread");

	iCritical.Create();
	
	mMsgThread->start();
}

CTTFLVParser::~CTTFLVParser()
{
	if(mMsgThread) {
		mMsgThread->stop();
	}
	if(iAudioStream != NULL) {
		delete iAudioStream;
		iAudioStream = NULL;
	}

	if(iVideoStream != NULL) {
		delete iVideoStream;
		iVideoStream = NULL;
	}

	if(iTagBuffer) {
		free(iTagBuffer);
		iTagBuffer = NULL;
	}

	iCritical.Destroy();

	SAFE_DELETE(mMsgThread);
}

TTInt CTTFLVParser::Parse(TTMediaInfo& aMediaInfo)
{
	iParserMediaInfoRef = &aMediaInfo;

	TTUint8 nHeaderBuffer[16];

	TTInt nErr = TTKErrNone;
	
	TTInt nRead = iDataReader.ReadWait(nHeaderBuffer, 0, 9);
	
	if(nRead != 9){
		return TTKErrNotFound;
	}

	if(nHeaderBuffer[0] != 'F' || nHeaderBuffer[1] != 'L' || nHeaderBuffer[2] != 'V') {
		return TTKErrNotSupported;
	}

	TTInt nFlag = nHeaderBuffer[4];
	iOffset = CTTIntReader::ReadUint32BE(nHeaderBuffer + 5);

	if (nFlag & FLV_HEADER_FLAG_HASVIDEO) {
        iVideoStream = new CTTFlvTagStream(FLV_STREAM_TYPE_VIDEO);
	}

	if (nFlag & FLV_HEADER_FLAG_HASAUDIO) {
        iAudioStream = new CTTFlvTagStream(FLV_STREAM_TYPE_AUDIO);
	}

	TTInt nTagCount = 0;
	
	while(nTagCount < 500)
	{
		nRead = iDataReader.ReadWait(nHeaderBuffer, iOffset, 4);
		if(nRead != 4) {
			return TTKErrUnderflow;
		}
		iOffset += 4;

		nRead = iDataReader.ReadWait(nHeaderBuffer, iOffset, 11);
		if(nRead != 11) {
			return TTKErrUnderflow;
		}

		iOffset += 11;

		TTInt nStreamType = nHeaderBuffer[0];
		TTInt nDataSize = CTTIntReader::ReadBytesNBE(nHeaderBuffer + 1, 3);
		TTInt64 nTimeStamp = CTTIntReader::ReadBytesNBE(nHeaderBuffer + 4, 3);
		nTimeStamp |= (nHeaderBuffer[7] << 24);

		if(nDataSize > iMaxSize) {
			free(iTagBuffer);
			iTagBuffer = (unsigned char*)malloc(nDataSize);
			iMaxSize = nDataSize;
		}

		nRead = iDataReader.ReadWait(iTagBuffer, iOffset, nDataSize);
		if(nRead != nDataSize) {
			return TTKErrUnderflow;
		}

		iOffset += nDataSize;

		//LOGI("TTFLVParser::nStreamType %d, nTimeStamp: %d", nStreamType, (int)nTimeStamp);

		switch( nStreamType )
		{
		case FLV_TAG_TYPE_AUDIO:
			if(iAudioStream == NULL) {
				iAudioStream = new CTTFlvTagStream(FLV_STREAM_TYPE_AUDIO);
			}
			iAudioStream->addTag(iTagBuffer, nDataSize, nTimeStamp);
			break;
		case FLV_TAG_TYPE_VIDEO:
			if(iVideoStream == NULL) {
				iVideoStream = new CTTFlvTagStream(FLV_STREAM_TYPE_VIDEO);
			}
			iVideoStream->addTag(iTagBuffer, nDataSize, nTimeStamp);
			break;
		case FLV_TAG_TYPE_META:
			nRead = ReadMetaData(iTagBuffer,  nDataSize);
			break;
		default:
			break;
		}
			
		if(CheckHead(nFlag)) {
			break;
		}

		nTagCount++;
	}

	TTBufferManager* source = NULL;
	TTBuffer buffer;
	if(iAudioStream) {
		source = iAudioStream->getSource();
		if(source) {
			nErr = source->dequeueAccessUnit(&buffer);
			if(buffer.nFlag & TT_FLAG_BUFFER_NEW_FORMAT) {
				TTAudioInfo* pAudioInfo = new TTAudioInfo(*((TTAudioInfo*)buffer.pData));
				aMediaInfo.iAudioInfoArray.Append(pAudioInfo);
				iStreamAudioCount++;
			}
		}
	}

	if(iVideoStream) {
		source = iVideoStream->getSource();
		if(source) {
			nErr = source->dequeueAccessUnit(&buffer);
			if(buffer.nFlag & TT_FLAG_BUFFER_NEW_FORMAT) {
				TTVideoInfo* pVideoInfo = new TTVideoInfo(*((TTVideoInfo*)buffer.pData));
				aMediaInfo.iVideoInfo = pVideoInfo;
			}
		}
	}

	if(iDataReader.Id() != ITTDataReader::ETTDataReaderIdFile) {
		iFirstTime = 1;
		iCountAMAX = 12;
		iCountVMAX = 5;
		SendBufferStartEvent();
		postInfoMsgEvent(5, ECheckBufferStatus);
	}

	LOGI("TTFLVParser::Parse return: %d, nTagCount %d", nErr, nTagCount);
	return nErr;
}

TTUint CTTFLVParser::MediaDuration(TTInt aStreamId)
{
	TTUint64 nDuration = 0;
	nDuration = iDuration;
	return (TTUint)nDuration;
}

TTInt CTTFLVParser::CheckEOS(int nOffset, int nRead)
{
	TTInt nErr = TTKErrUnderflow;
	
	TTInt nSize = nOffset;
	if(nRead > 0) {
		nSize += nRead;
	}

	if(iDataReader.Size() > 0 && nSize >= iDataReader.Size()) {
		nErr = TTKErrEof;
	}

	return nErr;
}

TTInt CTTFLVParser::ParserTag(TTInt64  aOffset)
{
	TTUint8 nHeaderBuffer[16];
	TTInt nOffset = 0;
	//TTInt nErr = TTKErrNone;
	TTInt nRead = 0;

	if(iDataReader.BufferedSize() < aOffset) {
		iOffset = 0;
		return TTKErrUnderflow;
	}
	
	if(iDataReader.BufferedSize() < aOffset + 15) {
		if(iDataReader.Id() == ITTDataReader::ETTDataReaderIdFile) {
			return TTKErrEof;
		} else {
			if(iDataReader.Size() > 0 && iDataReader.BufferedSize() == iDataReader.Size()) {
				return TTKErrEof; 
			} else {
				return TTKErrUnderflow;
			}
		}
	}

	nRead = iDataReader.ReadSync(nHeaderBuffer, aOffset + nOffset, 4);
	nOffset += 4;

	if(nHeaderBuffer[0] == 'F' && nHeaderBuffer[1] == 'L' && nHeaderBuffer[2] == 'V') {
		nRead = iDataReader.ReadSync(nHeaderBuffer, aOffset + nOffset, 11);
		nOffset = CTTIntReader::ReadUint32BE(nHeaderBuffer + 1);
		return nOffset;
	}

	nRead = iDataReader.ReadSync(nHeaderBuffer, aOffset + nOffset, 11);
	nOffset += 11;

	if(nHeaderBuffer[0] == 'F' && nHeaderBuffer[1] == 'L' && nHeaderBuffer[2] == 'V') {
		nOffset = 4 + CTTIntReader::ReadUint32BE(nHeaderBuffer + 5);
		return nOffset;
	}

	TTInt nStreamType = nHeaderBuffer[0];
	TTInt nDataSize = CTTIntReader::ReadBytesNBE(nHeaderBuffer + 1, 3);
	TTInt64 nTimeStamp = CTTIntReader::ReadBytesNBE(nHeaderBuffer + 4, 3);
	nTimeStamp |= (nHeaderBuffer[7] << 24);

	if(nDataSize > iMaxSize) {
		free(iTagBuffer);
		iTagBuffer = (unsigned char*)malloc(nDataSize);
		iMaxSize = nDataSize;
	}

	if(iDataReader.BufferedSize() < aOffset + nOffset + nDataSize) {
		if(iDataReader.Id() == ITTDataReader::ETTDataReaderIdFile) {
			return TTKErrEof;
		} else {
			if(iDataReader.Size() > 0 && iDataReader.BufferedSize() == iDataReader.Size()) {
				return TTKErrEof; 
			} else {
				return TTKErrUnderflow;
			}
		}
	}

	if(nDataSize == 0) {
		return nOffset;
	}

	nRead = iDataReader.ReadSync(iTagBuffer, aOffset + nOffset, nDataSize);
	nOffset += nDataSize;

	//LOGI("TTFLVParser::nStreamType %d, nTimeStamp: %d", nStreamType, (int)nTimeStamp);

	switch( nStreamType )
	{
	case FLV_TAG_TYPE_AUDIO:
		if(iAudioStream == NULL) {
			return TTKErrNotFound;
		}
		iAudioStream->addTag(iTagBuffer, nDataSize, nTimeStamp);
		break;
	case FLV_TAG_TYPE_VIDEO:
		if(iVideoStream == NULL) {
			return TTKErrNotFound;
		}
		iVideoStream->addTag(iTagBuffer, nDataSize, nTimeStamp);
		break;
	case FLV_TAG_TYPE_META:
		nRead = ReadMetaData(iTagBuffer,  nDataSize);
		break;
	default:
		break;
	}

	return nOffset;
}

TTInt CTTFLVParser::FillBuffer()
{
	TTInt nTagCount = 0;
	TTInt nOffset = 0;
	//TTBufferManager* source = NULL;
	
	while(nTagCount < 100) {
		nOffset = ParserTag(iOffset);
		if(nOffset > 0) {
			iOffset += nOffset;
		}

		if(nOffset == TTKErrEof) {
			if(iAudioStream) {
				iAudioStream->signalEOS(1);
			}

			if(iVideoStream) {
				iVideoStream->signalEOS(1);
			}

			iCritical.Lock();
			TTInt eBufferStatus = iBufferStatus;
			iCritical.UnLock();

			if(eBufferStatus == 0) {
				iObserver.BufferingReady();
				iLastBufferTime = GetTimeOfDay();

				iCritical.Lock();
				iBufferStatus = 1;
				iCritical.UnLock();
			}
		}

		if(nOffset < 0) {
			return nOffset;
		}

		nTagCount++;
	}

	return TTKErrNone;
}

TTInt CTTFLVParser::GetMediaSample(TTMediaType tMediaType, TTBuffer* pMediaBuffer)
{
	if(iVideoSeek && tMediaType == EMediaTypeVideo) {
		if(pMediaBuffer->nFlag & TT_FLAG_BUFFER_SEEKING) {
			iVideoSeek = false;
		} else {
			return TTKErrInUse;
		}
	}

	if(iAudioSeek && tMediaType == EMediaTypeAudio) {
		if(pMediaBuffer->nFlag & TT_FLAG_BUFFER_SEEKING) {
			iAudioSeek = false;
		} else {
			return TTKErrInUse;
		}
	}

	if(iDataReader.Id() == ITTDataReader::ETTDataReaderIdFile) {
		TTBufferManager* sourceA = NULL;
		TTBufferManager* sourceV = NULL;
		TTInt nCountA = -1;
		TTInt nCountV = -1;

		if(iAudioStream != NULL) {
			sourceA = iAudioStream->getSource();
			if(sourceA != NULL) {
				nCountA = sourceA->getBufferCount();
			}
		}

		if(iVideoStream != NULL) {
			sourceV = iVideoStream->getSource();
			if(sourceV != NULL) {
				nCountV = sourceV->getBufferCount();
			}
		}

		if((nCountA != -1 && nCountA < iCountAMAX) || (nCountV != -1 && nCountV < iCountVMAX)) {
			FillBuffer();
		}
	}

	TTBufferManager* source = NULL;
	TTBuffer buffer;
	//TTInt nFound = 0;

	memset(&buffer, 0, sizeof(TTBuffer));
	buffer.llTime = pMediaBuffer->llTime;
	if(tMediaType == EMediaTypeAudio) {
		if(iAudioStream == NULL) {
			return TTKErrNotReady;
		}
		
		source = iAudioStream->getSource();
		if(source == NULL) {
			return TTKErrNotReady;
		}		
	} else if(tMediaType == EMediaTypeVideo) {
		if(iVideoStream == NULL) {
			return TTKErrNotReady;
		}
		
		source = iVideoStream->getSource();
		if(source == NULL) {
			return TTKErrNotReady;
		}
	}

	TTInt nErr = TTKErrNotReady;
	if(source) {
		nErr = source->dequeueAccessUnit(&buffer);
		
		int bufferCount = source->getBufferCount();
		//LOGI("GetMediaSample: tMediaType %d, bufferCount %d, nErr %d, Time %d", tMediaType, bufferCount, nErr, (int)GetTimeOfDay());

		if(nErr ==  TTKErrNone) {
			memcpy(pMediaBuffer, &buffer, sizeof(TTBuffer));
			return TTKErrNone;
		} else if(nErr == TTKErrEof){
			return TTKErrEof;
		} else if(nErr == TTKErrNotReady) {
			if(iDataReader.Id() != ITTDataReader::ETTDataReaderIdFile && tMediaType == EMediaTypeAudio) {
				SendBufferStartEvent();
			}
		}
	}

	return nErr;
}

TTInt64 CTTFLVParser::Seek(TTUint64 aPosMS, TTInt aOption)
{
	if(iFrmPosTab == NULL || iFrmTabSize < 1) {
		return TTKErrNotSupported;
	}

	int i = 0;
	int nIndex = 0;
	for(i = 0; i < iFrmTabSize; i++) {
		if(iFrmPosTab[2*i] > aPosMS) {
			break;
		}
		nIndex = i;
	}

	aPosMS = iFrmPosTab[2*i];
	iOffset = iFrmPosTab[2*i + 1];

	if(iAudioStream) {
		iAudioStream->flush();
	}

	if(iVideoStream) {
		iVideoStream->flush();
	}

	iAudioSeek = true;
	iVideoSeek = true;

	return aPosMS;
}

void CTTFLVParser::SendBufferStartEvent()
{
	iCritical.Lock();
	TTInt eBufferStatus = iBufferStatus;
	iCritical.UnLock();

	if (eBufferStatus != 0)
	{
		TTInt nErr = TTKErrNotReady;
		if(iDataReader.GetStatusCode() != 2) {
			nErr = TTKErrCouldNotConnect;
		}
		iObserver.BufferingEmtpy(nErr, iDataReader.GetStatusCode(), iDataReader.GetHostIP());

		iCritical.Lock();
		iBufferStatus = 0;
		iCritical.UnLock();

		iNowBufferTime = GetTimeOfDay();
		if(iNowBufferTime - iLastBufferTime < 2000) {
			iReBufferCount++;
		}

		iCountAMAX = 15;
		iCountVMAX = 10;
		//TTInt nBandWidth = iDataReader.BandWidth();
		if(iReBufferCount > 3 && iReBufferCount <= 8) {
			iCountAMAX = 30;
			iCountVMAX = 20;
		} else if(iReBufferCount > 8 && iReBufferCount <= 15) {
			iCountAMAX = 60;
			iCountVMAX = 30;
		} else if(iReBufferCount > 15){
			iCountAMAX = 100;
			iCountVMAX = 40;
		}

		iCountAMAX = 2*iCountAMAX;
		iCountVMAX = 2*iCountVMAX;
	}
}

void CTTFLVParser::CheckBufferStatus()
{
	TTBufferManager* sourceA = NULL;
	TTBufferManager* sourceV = NULL;
	TTInt nCountA = -1;
	TTInt nCountV = -1;

	TTInt nBandWidth = iDataReader.BandWidth();

	iCritical.Lock();
	TTInt eBufferStatus = iBufferStatus;
	iCritical.UnLock();

	if(iAudioStream != NULL) {
		sourceA = iAudioStream->getSource();
		if(sourceA != NULL) {
			nCountA = sourceA->getBufferCount();
		}
	}

	if(iVideoStream != NULL) {
		sourceV = iVideoStream->getSource();
		if(sourceV != NULL) {
			nCountV = sourceV->getBufferCount();
		}
	}

	FillBuffer();

	if(iAudioStream != NULL) {
		sourceA = iAudioStream->getSource();
		if(sourceA != NULL) {
			nCountA = sourceA->getBufferCount();
		}
	}

	if(iVideoStream != NULL) {
		sourceV = iVideoStream->getSource();
		if(sourceV != NULL) {
			nCountV = sourceV->getBufferCount();
		}
	}

	//LOGI("TTFLVParser::nCountA %d, nCountV: %d, nBandWidth %d", nCountA, nCountV, nBandWidth);

	if (eBufferStatus == 0){
		if((nCountA == -1 || nCountA >= iCountAMAX) && (nCountV == -1 || nCountV >= iCountVMAX)) {
			if(iFirstTime) {
				TTInt64 nStartTimeV = -1;
				TTInt64 nStartTimeA = -1;

				if(sourceV) {
					sourceV->nextBufferTime(&nStartTimeV);
				} 
				if(sourceA) {
					sourceA->nextBufferTime(&nStartTimeA);
				}

				if(nStartTimeV >= 0 && nStartTimeA >= 0) {
					if(nStartTimeV > nStartTimeA) {
						sourceA->seek(nStartTimeV);
					} else if(nStartTimeV < nStartTimeA){
						sourceV->seek(nStartTimeA);
					}
				}
				iFirstTime = 0;
			}

			iObserver.BufferingReady();
			iLastBufferTime = GetTimeOfDay();

			iCritical.Lock();
			iBufferStatus = 1;
			iCritical.UnLock();
		}
	} else {
		if(iReBufferCount && GetTimeOfDay() - iLastBufferTime > 20000) {
			iReBufferCount = 0;
		}
	}

	
	if(iDataReader.Size() == 0 && eBufferStatus) {
		if(nCountA >= 150 || nCountV >= 50) {
			iDelayStatus = 1;
		}

#ifdef __TT_OS_ANDROID__
		if((nCountA == -1 || nCountA < 70) &&(nCountV == -1 || nCountV < 30)) {
#else
		if((nCountA == -1 || nCountA < 50) &&(nCountV == -1 || nCountV < 20)) {
#endif
			iDelayStatus = 0;
		}
		//LOGI("----TTFLVParser::nCountA %d, nCountV: %d", nCountA, nCountV);
		TTInt64 nStartTime = 0;
		TTInt64 nSeekTime = 0;
		TTInt	nDuration = 0;
		TTInt   nEOS = 0;

		if((nCountA == -1 || nCountA >= 500) && (nCountV == -1 || nCountV >= 150)) {
			if(sourceV) {
				sourceV->nextBufferTime(&nStartTime);
				nDuration = sourceV->getBufferedDurationUs(&nEOS);
				nSeekTime = nStartTime + nDuration - 100;
				nSeekTime = sourceV->seek(nSeekTime);
				nCountV = sourceV->getBufferCount();
			} else {
				if(sourceA) {
					sourceA->nextBufferTime(&nStartTime);
					nDuration = sourceA->getBufferedDurationUs(&nEOS);
					nSeekTime = nStartTime + nDuration - 200;
				}
			}

			if(sourceA) {
				nSeekTime = sourceA->seek(nSeekTime);
				nCountA = sourceA->getBufferCount();
			}
		}

		if(nCountA > 2000) {
			sourceA->nextBufferTime(&nStartTime);
			nDuration = sourceA->getBufferedDurationUs(&nEOS);
			nSeekTime = nStartTime + nDuration - 2000;
			nSeekTime = sourceA->seek(nSeekTime);
		}

		if(nCountV > 300)  {
			sourceV->nextBufferTime(&nStartTime);
			nDuration = sourceV->getBufferedDurationUs(&nEOS);
			nSeekTime = nStartTime + nDuration - 1000;
			nSeekTime = sourceV->seek(nSeekTime);
		}
	}

	if(eBufferStatus) {
		postInfoMsgEvent(20, ECheckBufferStatus);
	} else {
		postInfoMsgEvent(50, ECheckBufferStatus);
	}
}

TTInt CTTFLVParser::GetFrameLocation(TTInt aStreamId, TTInt& aFrmIdx, TTUint64 aTime)
{
	return TTKErrNotSupported;
}

TTInt CTTFLVParser::SeekWithinFrmPosTab(TTInt aStreamId, TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo)
{
	return TTKErrNotSupported;
}

TTInt CTTFLVParser::AmfGetString(unsigned char*	pSrcBuffer, int nLength, char* pDesString)
{
    int length = CTTIntReader::ReadUint16BE(pSrcBuffer);
    if (length >= nLength) {
        return -1;
    }

    memcpy(pDesString, pSrcBuffer + 2, length);

    pDesString[length] = '\0';

    return length;
}

TTInt CTTFLVParser::onInfoHandle(TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3) 
{
	if(nMsg == ECheckBufferStatus) {
		CheckBufferStatus();
	}

	return 0;
}

TTInt CTTFLVParser::postInfoMsgEvent(TTInt  nDelayTime, TTInt32 nMsg, int nParam1, int nParam2, void * pParam3)
{
	if (mMsgThread == NULL)
		return TTKErrNotFound;

	TTBaseEventItem * pEvent = mMsgThread->getEventByType(EEventStream);
	if (pEvent == NULL)
		pEvent = new CTTFLVParserEvent (this, &CTTFLVParser::onInfoHandle, EEventStream, nMsg, nParam1, nParam2, pParam3);
	else
		pEvent->setEventMsg(nMsg, nParam1, nParam2, pParam3);
	mMsgThread->postEventWithDelayTime (pEvent, nDelayTime);

	return TTKErrNone;
}

TTInt CTTFLVParser::ReadMetaData(unsigned char*	pBuffer, unsigned int nLength)
{
	TTInt nOffset = 0;
	TTInt type = pBuffer[0];
	nOffset++;

	char szName[1024];
	
	if(type != AMF_DATA_TYPE_STRING) {
		return TTKErrNotSupported;
	}

	TTInt nRead = AmfGetString(pBuffer + nOffset, nLength - nOffset, szName);
	if(nRead < 0) {
		return TTKErrNotSupported;
	}

	nOffset += 2 + nRead;

	if (!strcmp(szName, "onTextData"))
        return 1;

    if (strcmp(szName, "onMetaData") && strcmp(szName, "onCuePoint"))
        return 2;

	nRead = AmfReadObject(pBuffer + nOffset, nLength - nOffset, szName);
	if(nRead < 0) {
		return TTKErrNotSupported;
	}

	return 0;
}

bool CTTFLVParser::CheckHead(int nFlag) 
{
	TTBufferManager* sourceA = NULL;
	TTInt64 nBufferTimeA = -1;
	TTInt64 nNextTimeA = 0;
	TTBufferManager* sourceV = NULL;
	TTInt64 nBufferTimeV = -1;
	TTInt64 nNextTimeV = 0;

	if(nFlag & FLV_HEADER_FLAG_HASAUDIO) {
		if(iAudioStream == NULL)  {
			nBufferTimeA = 0;
		} else {
			sourceA = iAudioStream->getSource();
			if(sourceA) {
				int nEOS = 0;
				nBufferTimeA = sourceA->getBufferedDurationUs(&nEOS);
				nEOS = sourceA->nextBufferTime(&nNextTimeA);
			} else {
				nBufferTimeA = 0;
			}
		}
	}

	if(nFlag & FLV_HEADER_FLAG_HASVIDEO) {
		if(iVideoStream == NULL)  {
			nBufferTimeV = 0;
		} else {
			sourceV = iVideoStream->getSource();
			if(sourceV) {
				int nEOS = 0;
				nBufferTimeV = sourceV->getBufferedDurationUs(&nEOS);
				nEOS = sourceV->nextBufferTime(&nNextTimeV);
				if(nEOS == 0 && sourceA && nBufferTimeV > 0) {
					sourceA->seek(nNextTimeV);
				}
			} else {
				nBufferTimeV = 0;
			}
		}
	}

	if((nBufferTimeA > 0 || nBufferTimeA == -1) && (nBufferTimeV > 0 || nBufferTimeV == -1)) {
		return true;
	}

	return false;
}

TTInt CTTFLVParser::GetBufferTime(int nStreamType)
{
	TTBufferManager* source = NULL;
	TTInt nBufferTime = 0;
	if(nStreamType == FLV_STREAM_TYPE_AUDIO) {
		if(iAudioStream == NULL)  {
			return 0;
		}

		source = iAudioStream->getSource();
		if(source) {
			int nEOS = 0;
			nBufferTime = source->getBufferedDurationUs(&nEOS);
			return nBufferTime;
		} else {
			return 0;
		}
	}

	if(nStreamType == FLV_STREAM_TYPE_VIDEO) {
		if(iVideoStream == NULL)  {
			return 0;
		}

		source = iVideoStream->getSource();
		if(source) {
			int nEOS = 0;
			nBufferTime = source->getBufferedDurationUs(&nEOS);
			return nBufferTime;
		} else {
			return 0;
		}
	}

	return 0;
}

TTInt CTTFLVParser::AmfReadObject(unsigned char* pBuffer, int nLength, const char* key)
{
	TTInt nOffset = 0;
	TTInt nRead = 0;
	double nVal;
	AMFDataType amf_type = (AMFDataType)pBuffer[0];
	nOffset++;

	if(nLength < 0) {
		return -1;
	}

	char szName[1024];

	switch (amf_type) {
	case AMF_DATA_TYPE_NUMBER:
		nVal = CTTIntReader::ReadDouble64(pBuffer + nOffset);
		nOffset += 8;
		break;
	case AMF_DATA_TYPE_BOOL:
		nVal = pBuffer[nOffset];
		nOffset++;
		break;
	case AMF_DATA_TYPE_STRING:
		nRead = AmfGetString(pBuffer + nOffset, nLength - nOffset, szName);
		if(nRead < 0) {
			return nRead;
		}
		nOffset += 2 + nRead;
		break;
	case AMF_DATA_TYPE_OBJECT:
		if (key && !strcmp(KEYFRAMES_TAG, key)) {
			nRead = KeyFrameIndex(pBuffer + nOffset, nLength - nOffset);
			if(nRead < 0) {
				return nRead;
			}
			nOffset += nRead;
		}

		while (nOffset < nLength - 2) {
			nRead = AmfGetString(pBuffer + nOffset, nLength - nOffset, szName);
			if(nRead < 0) {
				return nRead;
			}
			nOffset += 2 + nRead;
			nRead = AmfReadObject(pBuffer + nOffset, nLength - nOffset, szName);
			if(nRead < 0) {
				return nRead;
			}

			nOffset += nRead;
		}

		if (pBuffer[nOffset] != AMF_END_OF_OBJECT) {
		    return -1;
		}
		break;
	case AMF_DATA_TYPE_NULL:
	case AMF_DATA_TYPE_UNDEFINED:
	case AMF_DATA_TYPE_UNSUPPORTED:
		break;     
	case AMF_DATA_TYPE_MIXEDARRAY:
		unsigned int nLen;
		nLen = CTTIntReader::ReadUint32BE(pBuffer + nOffset);
		nOffset += 4;
		while (nOffset < nLength - 2) {
			nRead = AmfGetString(pBuffer + nOffset, nLength - nOffset, szName);
			if(nRead < 0) {
				return nRead;
			}

			nOffset += 2 + nRead;

			nRead = AmfReadObject(pBuffer + nOffset, nLength - nOffset, szName);
			if(nRead < 0) {
				return nRead;
			}

			nOffset += nRead;
		}

		if (pBuffer[nOffset] != AMF_END_OF_OBJECT) {
		    return -1;
		}
		break;
	case AMF_DATA_TYPE_ARRAY:
		{
			unsigned int arraylen, i;
			arraylen = CTTIntReader::ReadUint32BE(pBuffer + nOffset);
			nOffset += 4;
			for (i = 0; i < arraylen && nOffset < nLength - 1; i++) {
				nRead = AmfReadObject(pBuffer + nOffset, nLength - nOffset, NULL);
				if(nRead < 0) {
					return -1; 
				}
				
				nOffset += nRead;
			}
		}
		break;
	case AMF_DATA_TYPE_DATE:
		nOffset += 8 + 2;  
		break;
	default:                    
		return -1;
	}

	if (key) {
		if (amf_type == AMF_DATA_TYPE_NUMBER || amf_type == AMF_DATA_TYPE_BOOL) {
			if (!strcmp(key, "duration")) {
				iDuration = nVal * 1000;
			} 
		}
	}

	return nOffset;
}

TTInt CTTFLVParser::KeyFrameIndex(unsigned char* pBuffer, int nLength)
{
	unsigned int nTimesLen = 0;
	unsigned int nFileposLen = 0;
	TTInt nRead = 0;
	int i;
    char szName[256];
	TTInt nOffset = 0;

	while (nOffset < nLength - 2) {
		nRead = AmfGetString(pBuffer + nOffset, nLength - nOffset, szName);
		if(nRead < 0) {
			return nRead;
		}

		nOffset += 2 + nRead;

		AMFDataType amf_type = (AMFDataType)pBuffer[nOffset];
		nOffset++;

        if (amf_type != AMF_DATA_TYPE_ARRAY)
            break;

        TTInt nLen = CTTIntReader::ReadUint32BE(pBuffer + nOffset);
		nOffset += 4;
        if (nLen >> 28)
            break;
		
		TTInt nType = 0;
        if(!strcmp(KEYFRAMES_TIMESTAMP_TAG , szName)) {
            nTimesLen      = nLen;
        } else if (!strcmp(KEYFRAMES_BYTEOFFSET_TAG, szName)) {
            nFileposLen    = nLen;
			nType = 1;
		} else {
            break;
		}

		if(nTimesLen != 0 && nFileposLen != 0 && nFileposLen != nTimesLen) {
			break;
		}

		if(iFrmPosTab == NULL) {
			iFrmTabSize = nLen;
			iFrmPosTab = new TTUint[iFrmTabSize*2]; 
		}

        for (i = 0; i < nLen && nOffset < nLength - 1; i++) {
            if (pBuffer[nOffset] != AMF_DATA_TYPE_NUMBER)
                break;
			nOffset++;
            iFrmPosTab[2*i + nType] = (TTInt)CTTIntReader::ReadDouble64(pBuffer + nOffset);
			nOffset += 8;
        }
    }

	if(nTimesLen != 0 && nFileposLen != 0 && nFileposLen == nTimesLen) {
		iFrmPosTabDone = true;;
	}

	return 0;
}
