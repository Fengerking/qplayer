/*
* File : TTMP4Parser.cpp
* Description : CTTMP4Parserþ
*/
#include "TTMP4Parser.h"
#include "TTIntReader.h"
#include "GKOsalConfig.h"
#include "TTSysTime.h"
#include "TTLog.h"
#ifdef __TT_OS_IOS__
#include "TTIPodLibraryFileReader.h"
extern TTInt gIos8Above;
#endif

#define SKIP_BYTES(pos, len, skips)	{	\
	(pos) += (skips);					\
	(len) -= (skips);					\
	}

#define KBoxHeaderSize 16


enum {
	KTag_ESDescriptor            = 0x03,
	KTag_DecoderConfigDescriptor = 0x04,
	KTag_DecoderSpecificInfo     = 0x05,
	KTag_SLConfigDescriptor 	 = 0x06,
};

//static const TTInt KMaxM4AFrameSize = 128 * KILO;
static const TTInt KMaxALACFrameSize = 2 * KILO * KILO;

CTTMP4Parser::~CTTMP4Parser()
{
	TTInt nCount = iAudioTrackInfoTab.Count();
	for(TTInt i = 0; i < nCount; i++)
	{
		removeTrackInfo(iAudioTrackInfoTab[i]);
	}
	
	removeTrackInfo(iVideoTrackInfo);
}


CTTMP4Parser::CTTMP4Parser(ITTDataReader& iDataReader, ITTMediaParserObserver& aObserver)
	: CTTMediaParser(iDataReader, aObserver)
	, iDuration(1)
	, iTotalBitrate(0)
	, iAudioTrackInfoTab(2)
	, iVideoTrackInfo(NULL)
	, iCurTrackInfo(NULL)
	, iCurAudioInfo(NULL)
	, iCurVideoInfo(NULL)
{

}

TTInt CTTMP4Parser::Parse(TTMediaInfo& aMediaInfo)
{
	//LOGI("TTMP4Parser::Parse");

	iParserMediaInfoRef = &aMediaInfo;

	TTUint64 nReadPos = 0;
	TTUint64 nBoxSize = 0;
	TTInt    nHeadSize = 0;

	//LOGI("0000TTMP4Parser::Parse %lld", GetTimeOfDay());
	if((nHeadSize = LocationBox(nReadPos, nBoxSize, "moov")) < 0)
	{
		LOGI("TTMP4Parser::Parse return TTKErrNotSupported");
		return TTKErrNotSupported;
	}

	if ((TTInt)(nBoxSize + nReadPos) > iDataReader.Size())
	{
		LOGI("TTMP4Parser::Parse return TTKErrOverflow");
		return TTKErrOverflow;
	}

	//LOGI("1111TTMP4Parser::Parse %lld", GetTimeOfDay());

	TTInt nFlag = TTREADER_CACHE_SYNC;
	TTInt nErr = iDataReader.PrepareCache(nReadPos, nBoxSize, nFlag);
	if(nErr != TTKErrNone) return nErr;

	//LOGI("2222TTMP4Parser::Parse %lld", GetTimeOfDay());

	nErr = ReadBoxMoov(nReadPos + nHeadSize, nBoxSize - nHeadSize);

	//LOGI("3333TTMP4Parser::Parse %lld", GetTimeOfDay());

	if(nErr == TTKErrNone)
	{
		if(iRawDataBegin == 0) 
		{
			nReadPos += nBoxSize;
			nHeadSize = LocationBox(nReadPos, nBoxSize, "mdat");
			if(nHeadSize < 0)
			{
				LOGI("TTMP4Parser::Parse return TTKErrNotSupported");
				//return TTKErrNotSupported;
				iRawDataBegin = nReadPos;
				iRawDataEnd = iDataReader.Size();
				//nHeadSize = 0;
			} else {
				iRawDataBegin = nReadPos + nHeadSize;
				iRawDataEnd = nReadPos + nBoxSize;
			}
		}

		nFlag = TTREADER_CACHE_ASYNC;
		nErr = iDataReader.PrepareCache(iRawDataBegin, iTotalBitrate/2, nFlag);
	}

	//LOGI("4444TTMP4Parser::Parse  Time %lld, Size %d, iTotalBitrate/2 %d", GetTimeOfDay(), iDataReader.BufferedSize(), iTotalBitrate/2);

	LOGI("TTMP4Parser::Parse return: %d", nErr);
	return nErr;
}

TTUint CTTMP4Parser::MediaDuration(TTInt aStreamId)
{
	TTUint64 nDuration = 0;

	if(aStreamId < 100 && aStreamId < iStreamAudioCount) {
		nDuration = iAudioTrackInfoTab[aStreamId]->iDuration;
	}

	if(aStreamId == 100) {
		nDuration = iVideoTrackInfo->iDuration;
	}
	
	if(nDuration == 0)
		nDuration = iDuration;

	return (TTUint)nDuration;
}

TTInt CTTMP4Parser::GetMediaSample(TTMediaType tMediaType, TTBuffer* pMediaBuffer)
{
	TTInt64 nTime = pMediaBuffer->llTime;
	TTUint8* avBuffer = NULL;
	TTUint32 nFrameSize = 0;
	TTInt64 nFramePos = -1;
	TTInt nFlag = 0;
	TTInt64 nTimeStamp = 0;
	TTSampleInfo* pNextInfo = NULL;

	if(tMediaType == EMediaTypeAudio){
		if(iCurAudioInfo == NULL && iAudioTrackInfoTab[iAudioStreamId]->iSampleInfoTab != NULL) {
			iCurAudioInfo = iAudioTrackInfoTab[iAudioStreamId]->iSampleInfoTab;
		} else if(iCurAudioInfo == NULL && iAudioTrackInfoTab[iAudioStreamId]->iSampleInfoTab == NULL){
			return TTKErrEof;
		}

		if(iAudioSeek) {
			if(pMediaBuffer->nFlag & TT_FLAG_BUFFER_SEEKING) {
				iAudioSeek = false;
				//LOGI("CTTMP4Parser::GetMediaSample. iAudioSeek %d", iAudioSeek);
			} else {
				//LOGI("CTTMP4Parser::GetMediaSample not ready. iAudioSeek %d", iAudioSeek);
				return TTKErrInUse;
			}
		}
		
		if(iCurAudioInfo->iSampleIdx == 0x7fffffff)
			return TTKErrEof;

		nFrameSize = iCurAudioInfo->iSampleEntrySize;
		if (nFrameSize > 0)	{
			if(nFrameSize > iAudioSize)	{
				SAFE_FREE(iAudioBuffer);

				iAudioBuffer = (TTPBYTE)malloc(nFrameSize + 32);
				iAudioSize = nFrameSize + 32;
			}
		}
		avBuffer = iAudioBuffer;
		nFramePos = iCurAudioInfo->iSampleFileOffset;
		nTimeStamp = iCurAudioInfo->iSampleTimeStamp;
		nFlag = iCurAudioInfo->iFlag;
		pNextInfo = iCurAudioInfo + 1;
	} else if(tMediaType == EMediaTypeVideo){
		if(iCurVideoInfo == NULL && iVideoTrackInfo->iSampleInfoTab != NULL) {
			iCurVideoInfo = iVideoTrackInfo->iSampleInfoTab;
		} else if(iCurVideoInfo == NULL && iVideoTrackInfo->iSampleInfoTab == NULL){
			return TTKErrEof;
		}

		if(iVideoSeek) {
			if(pMediaBuffer->nFlag & TT_FLAG_BUFFER_SEEKING) {
				iVideoSeek = false;
				//LOGI("CTTMP4Parser::GetMediaSample. iVideoSeek %d", iVideoSeek);
			} else {
				//LOGI("CTTMP4Parser::GetMediaSample not ready. iVideoSeek %d", iVideoSeek);
				return TTKErrInUse;
			}
		}
		
		if(iCurVideoInfo->iSampleIdx == 0x7fffffff)
			return TTKErrEof;

		TTInt nIndex = iCurVideoInfo->iSampleIdx;
		nIndex = findNextSyncFrame(iVideoTrackInfo->iSampleInfoTab, iCurVideoInfo, nTime);
		//LOGI("nIndex %d, nFlag %d, Time %lld, playTime %lld",iCurVideoInfo->iSampleIdx, iCurVideoInfo->iFlag, iCurVideoInfo->iSampleTimeStamp, nTime);

		iCurVideoInfo = &(iVideoTrackInfo->iSampleInfoTab[nIndex - 1]);		
		nFrameSize = iCurVideoInfo->iSampleEntrySize;
		if (nFrameSize > 0)	{
			if(nFrameSize > iVideoSize)	{
				SAFE_FREE(iVideoBuffer);

				iVideoBuffer = (TTPBYTE)malloc(nFrameSize + 32);
				iVideoSize = nFrameSize + 32;
			}
		}
		avBuffer = iVideoBuffer;
		nFramePos = iCurVideoInfo->iSampleFileOffset;
		nTimeStamp = iCurVideoInfo->iSampleTimeStamp;
		nFlag = iCurVideoInfo->iFlag;

		pNextInfo = iCurVideoInfo + 1;
	}

	TTInt nReadSize = iDataReader.ReadSync(avBuffer, nFramePos, nFrameSize);

	if (nReadSize == nFrameSize) {
		pMediaBuffer->nSize = nFrameSize;
		pMediaBuffer->pBuffer = avBuffer;
		pMediaBuffer->llTime = nTimeStamp;
		pMediaBuffer->nFlag = nFlag;

		if(tMediaType == EMediaTypeAudio){
			iCurAudioInfo = pNextInfo;
		} else if(tMediaType == EMediaTypeVideo){
			iCurVideoInfo = pNextInfo;

			if(iVideoTrackInfo->iCodecType == TTVideoInfo::KTTMediaTypeVideoCodeH264 || 
				iVideoTrackInfo->iCodecType == TTVideoInfo::KTTMediaTypeVideoCodeHEVC) {
					TTUint32 nFrame = 0;
					TTInt	 nKeyFrame = 0;
					TTInt nErr = ConvertAVCFrame(avBuffer, nFrameSize, nFrame, nKeyFrame);
					if(nErr) return nErr;

					
					if(iVideoTrackInfo->iCodecType == TTVideoInfo::KTTMediaTypeVideoCodeH264)
						pMediaBuffer->nFlag = nKeyFrame;

					if(iNALLengthSize < 3) {
						pMediaBuffer->nSize = nFrame;
						pMediaBuffer->pBuffer = iVideoBuffer;
					}
			}
		}
	}
#ifdef __TT_OS_IOS__
    else if (iDataReader.Id() == ITTDataReader::ETTDataReaderIdIPodLibraryFile)
    {
        /*if (nReadSize == TTKErrEof)
         {
         nErr = TTKErrEof;
         }
         else
         {
         FileException(nReadSize);
         } */
        return TTKErrEof;
    }
#endif
	else if(nReadSize == TTKErrAccessDenied)
	{
		return TTKErrEof;
	}
	else if(nReadSize < 0)
	{
		if(tMediaType == EMediaTypeAudio){
			iCurAudioInfo = pNextInfo;
		} else if(tMediaType == EMediaTypeVideo){
			iCurVideoInfo = pNextInfo;
		}
		return nReadSize;
	}
	else
	{
		//online m4a file may read fail, but that doesn't mean socket read fail ,just need to bufferring more
		if (iDataReader.Id() == ITTDataReader::ETTDataReaderIdHttp || iDataReader.Id() == ITTDataReader::ETTDataReaderIdBuffer) {
			iDataReader.CheckOnLineBuffering();
			return TTKErrNotReady;
		} else {
			return TTKErrEof;
		}
	}

	return TTKErrNone;
}

TTInt CTTMP4Parser::findNextSyncFrame(TTSampleInfo* pTrackInfo, TTSampleInfo* pCurInfo, TTInt64 nTimeStamp)
{
	TTInt nIndex = pCurInfo->iSampleIdx;
	while(1) {
		if(pCurInfo->iSampleIdx == 0x7fffffff)
			break;

		if(pCurInfo->iSampleTimeStamp > nTimeStamp && (pCurInfo->iFlag&TT_FLAG_BUFFER_KEYFRAME))
			break;

		if(pCurInfo->iSampleTimeStamp < nTimeStamp && (pCurInfo->iFlag&TT_FLAG_BUFFER_KEYFRAME))
			nIndex = pCurInfo->iSampleIdx;

		pCurInfo++;
	}

	return nIndex;
}

TTInt64 CTTMP4Parser::Seek(TTUint64 aPosMS, TTInt aOption)
{
#ifdef __TT_OS_IOS__
    if (iDataReader.Id() == ITTDataReader::ETTDataReaderIdIPodLibraryFile)
    {
        if ((aPosMS + KMinSeekOverflowOffset) < ((CTTIPodLibraryFileReader*)(&iDataReader))->Duration())
        {   
            ((CTTIPodLibraryFileReader*)(&iDataReader))->Seek(aPosMS);
        }
        else if (!iReadEof)
        {
			iReadEof = true;
        }
        
        return aPosMS;
    }        
#endif

	TTUint64 TempPos = aPosMS;
	TTInt64 nPos = -1;	
	TTInt nDuration = 0;
	TTInt nFrameIdx = 0;
	TTInt nSampleCount = 0;
	TTInt nKeyIdx = 0;
	TTInt i = 0;
	TTInt iVideoEOS = 0;
	//TTInt nErr = TTKErrNotSupported;
	TTSampleInfo* SampleInfo = NULL;
	
	if(iVideoTrackInfo && iVideoTrackInfo->iSampleInfoTab) {
		SampleInfo = iVideoTrackInfo->iSampleInfoTab;
		nSampleCount = iVideoTrackInfo->iSampleCount;

		if(TempPos >= iVideoTrackInfo->iDuration) {
			iVideoEOS = 1;
			iCurVideoInfo = &SampleInfo[nSampleCount];
		} else {
			if(iVideoTrackInfo->iKeyFrameSampleEntryNum > 0 && iVideoTrackInfo->iKeyFrameSampleTab != NULL)	{
				for(i = 0; i < iVideoTrackInfo->iKeyFrameSampleEntryNum; i++) {
					nFrameIdx = iVideoTrackInfo->iKeyFrameSampleTab[i];
					if(SampleInfo[nFrameIdx - 1].iSampleTimeStamp > TempPos) {
						break;
					}
					nKeyIdx = nFrameIdx - 1;
				}

				nPos = SampleInfo[nKeyIdx].iSampleFileOffset;
				//if(iDataReader.Id() == ITTDataReader::ETTDataReaderIdHttp) {
				//	if(nPos > iDataReader.BufferedSize() + iTotalBitrate/4)
				//		return -1;
				//}

				iCurVideoInfo = &SampleInfo[nKeyIdx];
				TempPos = iCurVideoInfo->iSampleTimeStamp;
				nPos = iCurVideoInfo->iSampleFileOffset;
			} else {
				return -1;
			}
		}
	}

	if(aOption)
		TempPos = aPosMS;
	//LOGI("Seek aPosMS %lld,TempPos %lld, iCurVideoInfo->iSampleTimeStamp %lld, nKeyIdx %d, nFlag %d",aPosMS,TempPos, iCurVideoInfo->iSampleTimeStamp, nKeyIdx, iCurVideoInfo->iFlag);

	if(iAudioStreamId >= 0)
	{
		SampleInfo = iAudioTrackInfoTab[iAudioStreamId]->iSampleInfoTab;
		nDuration = iAudioTrackInfoTab[iAudioStreamId]->iDuration;
		nSampleCount = iAudioTrackInfoTab[iAudioStreamId]->iSampleCount;

		if(TempPos >= nDuration) {
			iCurAudioInfo = &SampleInfo[nSampleCount];
			if(iVideoEOS || iVideoTrackInfo == NULL) 
				return TTKErrEof;
		}

		nFrameIdx = 0;
		if(nDuration)
			nFrameIdx = (TTInt)((TempPos * nSampleCount + nDuration / 2) / nDuration);

		if(nFrameIdx > nSampleCount) {
			nFrameIdx = nSampleCount; 
			if(iVideoEOS || iVideoTrackInfo == NULL) 
				return TTKErrEof;
		}

		if(iVideoTrackInfo == NULL) {
			nPos = SampleInfo[nFrameIdx].iSampleFileOffset;
			//if(iDataReader.Id() == ITTDataReader::ETTDataReaderIdHttp) {
			//	if(nPos > iDataReader.BufferedSize() + iTotalBitrate/4)
			//		return -1;
			//}
		}
		
		iCurAudioInfo = &SampleInfo[nFrameIdx];
		//LOGI("Seek different. %lld", TempPos - iCurAudioInfo->iSampleTimeStamp);

		TempPos = iCurAudioInfo->iSampleTimeStamp;

		if(nPos > iCurAudioInfo->iSampleFileOffset || nPos == -1)
		{
			nPos = iCurAudioInfo->iSampleFileOffset;
		}
	}

	TTInt nFlag = TTREADER_CACHE_ASYNC;
	iDataReader.PrepareCache(nPos, iTotalBitrate/4, nFlag);
	iVideoSeek = true;
	iAudioSeek = true;

	return TempPos;
}

TTInt CTTMP4Parser::GetFrameLocation(TTInt aStreamId, TTInt& aFrmIdx, TTUint64 aTime)
{
	TTInt iFrameTime = 0;
	TTUint32 nSampleEntryNum = 0;
	TTUint64 nDuration = 0;

	if(aStreamId < 100) {
		nSampleEntryNum = iAudioTrackInfoTab[aStreamId]->iSampleCount;
		iFrameTime = iAudioTrackInfoTab[aStreamId]->iFrameTime;
		nDuration = iAudioTrackInfoTab[aStreamId]->iDuration;
	} else {
		nSampleEntryNum = iVideoTrackInfo->iSampleCount;
		iFrameTime = iVideoTrackInfo->iFrameTime;
		nDuration = iVideoTrackInfo->iDuration;
	}

	if(nDuration)
		aFrmIdx = (TTInt)(((TTInt64)aTime * nSampleEntryNum + nDuration / 2) / nDuration);	
	else
		return TTKErrGeneral;

	return TTKErrNone;
}

TTInt CTTMP4Parser::SeekWithinFrmPosTab(TTInt aStreamId, TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo)
{
	//TTInt trackType = 0;

	TTSampleInfo* SampleInfo = NULL;
	TTUint32 nSampleEntryNum;
	TTUint32 iFrameTime = 0;

	if(aStreamId < 100) {
		SampleInfo = iAudioTrackInfoTab[aStreamId]->iSampleInfoTab;
		nSampleEntryNum = iAudioTrackInfoTab[aStreamId]->iSampleCount;
		iFrameTime = iAudioTrackInfoTab[aStreamId]->iFrameTime;
	} else {
		SampleInfo = iVideoTrackInfo->iSampleInfoTab;
		nSampleEntryNum = iVideoTrackInfo->iSampleCount;
		iFrameTime = iAudioTrackInfoTab[aStreamId]->iFrameTime;
	}

	if(SampleInfo == NULL)
		return TTKErrEof;

	if (aFrmIdx < (nSampleEntryNum - 2))
	{
		aFrameInfo.iFrameLocation = (TTInt64)(SampleInfo[aFrmIdx].iSampleFileOffset);
		aFrameInfo.iFrameSize = (TTInt)(SampleInfo[aFrmIdx + 1].iSampleFileOffset) - aFrameInfo.iFrameLocation;
		aFrameInfo.iFrameStartTime = SampleInfo[aFrmIdx].iSampleIdx * iFrameTime / 1000;
		return TTKErrNone;
	}

	return TTKErrEof;
}

TTInt CTTMP4Parser::ReadBoxMoov(TTUint64 aBoxPos, TTUint64 aBoxLen)
{
	LOGI("TTMP4Parser::ReadBoxMoov. [%d, %d]", aBoxPos, aBoxLen);
	TTUint64 nBoxLenLeft = aBoxLen;
	TTUint64 nBoxPos = aBoxPos;
	TTUint64 aOffset = 8;
	TTMP4TrackInfo *pCurrentTrack = NULL;
	TTUint8 version = 0;
	TTUint32 nTimeScale = 0;
	TTUint64 nDuration = 0;

	while(nBoxLenLeft > 0)	
	{
		if(nBoxLenLeft < 8) {
			break;
		}

		TTUint32 nBoxSize = iDataReader.ReadUint32BE(nBoxPos);
		TTUint32 nBoxId = iDataReader.ReadUint32BE(nBoxPos + 4);
		if(nBoxSize == 1) {
			if(nBoxLenLeft < 16) {
				break;
			}
			nBoxSize = iDataReader.ReadUint64BE(nBoxPos + 8);
			aOffset = 16;
		} 
		
		if(nBoxSize < 8){
			return TTKErrNotSupported;
		}

		if(nBoxLenLeft < nBoxSize) {
			break;
		}

		switch(nBoxId) {
			case MAKEFOURCC('m','v','h','d'): 
				{
					iDataReader.ReadSync(&version, nBoxPos + aOffset, sizeof(TTUint8));

					if(version == 0) {
						nTimeScale = iDataReader.ReadUint32BE(nBoxPos + 12 + aOffset);
						nDuration = iDataReader.ReadUint32BE(nBoxPos + 16 + aOffset);
					} else if(version == 1)	{
						nTimeScale = iDataReader.ReadUint32BE(nBoxPos + 20 + aOffset);
						nDuration = iDataReader.ReadUint64BE(nBoxPos + 24 + aOffset);
					}

					if(nTimeScale) iDuration = (TTUint)((nDuration * 1000) / nTimeScale);

					nBoxPos += nBoxSize;
					nBoxLenLeft -= nBoxSize;
				}
				break;

		case MAKEFOURCC('t','r','a','k'):
			{
				updateTrackInfo();

				pCurrentTrack = new TTMP4TrackInfo;
				memset(pCurrentTrack, 0, sizeof(TTMP4TrackInfo));

				iCurTrackInfo = pCurrentTrack;

				nBoxPos += aOffset;
				nBoxLenLeft -= aOffset;
			}
			break;

		case MAKEFOURCC('t','k','h','d'):
			{
				iDataReader.ReadSync(&version, nBoxPos + aOffset, sizeof(TTUint8));
				if(version == 0) {
					nDuration = iDataReader.ReadUint32BE(nBoxPos + 20 + aOffset);
					pCurrentTrack->iWidth = iDataReader.ReadUint32BE(nBoxPos + 76 + aOffset) >> 16;
					pCurrentTrack->iHeight = iDataReader.ReadUint32BE(nBoxPos + 80 + aOffset) >> 16;
				} else if(version == 1)	{
					nDuration = iDataReader.ReadUint64BE(nBoxPos + 28 + aOffset);
					pCurrentTrack->iWidth = iDataReader.ReadUint32BE(nBoxPos + 84 + aOffset) >> 16;
					pCurrentTrack->iHeight = iDataReader.ReadUint32BE(nBoxPos + 88 + aOffset) >> 16;
				}

				pCurrentTrack->iDuration = nDuration;
				pCurrentTrack->iScale = nTimeScale;

				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
			}
			break;

		case MAKEFOURCC('m','d','i','a'):
			{
				nBoxPos += aOffset;
				nBoxLenLeft -= aOffset;
			}
			break;

		case MAKEFOURCC('m','d','h','d'):
			{
				iDataReader.ReadSync(&version, nBoxPos + aOffset, sizeof(TTUint8));
				TTUint32 nScale = 0;
				TTUint64 Duration = 0;
				TTUint16 lang16;
				TTUint8	 lang[2];

				if(version == 0)
				{
					nScale = iDataReader.ReadUint32BE(nBoxPos + 20 + aOffset - 8);
					Duration = iDataReader.ReadUint32BE(nBoxPos + 24 + aOffset - 8);
					lang16 = iDataReader.ReadUint16BE(nBoxPos + 28 + aOffset - 8);
				}
				else
				{
					nScale = iDataReader.ReadUint32BE(nBoxPos + 28 + aOffset - 8);
					Duration = iDataReader.ReadUint32BE(nBoxPos + 32 + aOffset - 8);
					lang16 = iDataReader.ReadUint16BE(nBoxPos + 40 + aOffset - 8);
				}

				if(nScale && Duration != 0xffffffff) 
					pCurrentTrack->iDuration = Duration * 1000/nScale;

				if(nScale)
					pCurrentTrack->iScale = nScale;
				
				lang[0] = lang16 >> 8; lang[1] = lang16 & 0xff; 
				pCurrentTrack->iLang_code[0] = ((lang[0] >> 2) & 0x1f) + 0x60;
				pCurrentTrack->iLang_code[1] = ((lang[0] & 0x3) << 3 | (lang[1] >> 5)) + 0x60;
				pCurrentTrack->iLang_code[2] = (lang[1] & 0x1f) + 0x60;
				pCurrentTrack->iLang_code[3] = '\0';

				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
			}
			break;

		case MAKEFOURCC('h','d','l','r'):
			{
				TTInt nStreamID = -1;
				TTUint32 nSubType = iDataReader.ReadUint32BE(nBoxPos + aOffset + 8);

				if (nSubType == MAKEFOURCC('s','o','u','n'))
				{ 
					if(pCurrentTrack->iAudio == 0) {
						nStreamID = EMediaStreamIdAudioL + iStreamAudioCount;
						pCurrentTrack->iAudio = 1;
						iStreamAudioCount++;
					} else {
						nStreamID = pCurrentTrack->iStreamID;
					}
				} else if (nSubType == MAKEFOURCC('v','i','d','e'))	{ 
					if(pCurrentTrack->iStreamID > 0) {
						nStreamID = pCurrentTrack->iStreamID;
					}else {
						if(iStreamVideoCount == 0) {
							nStreamID = EMediaStreamIdVideo + iStreamVideoCount;
							iStreamVideoCount++;
						}else {
							pCurrentTrack->iErrorTrackInfo = 1;
						}
					}
				} 

				if(nStreamID >= 0) {
					pCurrentTrack->iStreamID = nStreamID;
				}
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
			}
			break;

		case MAKEFOURCC('m','i','n','f'):
			{
				if(pCurrentTrack->iErrorTrackInfo == 1) {
					nBoxPos += nBoxSize;
					nBoxLenLeft -= nBoxSize;
				}else {
					nBoxPos += aOffset;
					nBoxLenLeft -= aOffset;
				}
			}
			break;

		case MAKEFOURCC('s','t','b','l'):
			{
				nBoxPos += aOffset;
				nBoxLenLeft -= aOffset;
			}
			break;

		case MAKEFOURCC('s','t','s','d'):
			{
				ReadBoxStsd(nBoxPos + aOffset, nBoxSize - aOffset);
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= TT_BOX_FLAG_STSD;
			}
			break;

		case MAKEFOURCC('c','t','t','s'):
			{
				ReadBoxCtts(nBoxPos + aOffset, nBoxSize - aOffset);
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= TT_BOX_FLAG_CTTS;
			}
			break;

		case MAKEFOURCC('s','t','t','s'):
			{
				ReadBoxStts(nBoxPos + aOffset, nBoxSize - aOffset);
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= TT_BOX_FLAG_STTS;
			}
			break;

		case MAKEFOURCC('s','t','s','s'):
			{
				ReadBoxStss(nBoxPos + aOffset, nBoxSize - aOffset);
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= TT_BOX_FLAG_STSS;
			}
			break;

		case MAKEFOURCC('s','t','s','z'):
			{				
				ReadBoxStsz(nBoxPos + aOffset, nBoxSize - aOffset);
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= TT_BOX_FLAG_STSZ;
			}
			break;

		case MAKEFOURCC('s','t','s','c'):
			{
				ReadBoxStsc(nBoxPos + aOffset, nBoxSize - aOffset);				
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= TT_BOX_FLAG_STSC;
			}
			break;

		case MAKEFOURCC('s','t','c','o'):
			{
				ReadBoxStco(nBoxPos + aOffset, nBoxSize - aOffset);
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= TT_BOX_FLAG_STCO;
			}
			break;

		case MAKEFOURCC('c','o','6','4'):
			{
				ReadBoxCo64(nBoxPos + aOffset, nBoxSize - aOffset);
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= TT_BOX_FLAG_STCO;
			}
			break;

		default:
			{
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				break;
			}
		}
	}

	updateTrackInfo();

	iFrmPosTabDone = ETTTrue;

	if(iStreamAudioCount > 0 || iStreamVideoCount > 0)
		return TTKErrNone;

	return TTKErrNotSupported;
}

TTInt CTTMP4Parser::LocationBox(TTUint64& aLocation, TTUint64& aBoxSize, const TTChar* aBoxType)
{
	//LOGI("TTMP4Parser::LocationBox. [%lu, %lu, %s]", aLocation, aBoxSize, aBoxType);

	TTUint8 nBoxHeaderBuffer[KBoxHeaderSize];

	TTUint64 nReadPos = aLocation;
	TTUint64 nBoxSize = 0;
	TTInt nHeadSize = 0;
	while(ETTTrue)
	{
		if (KBoxHeaderSize != iDataReader.ReadWait(nBoxHeaderBuffer, nReadPos, KBoxHeaderSize))
		{
			LOGI("TTMP4Parser::LocationBox return TTKErrOverflow");
			return TTKErrOverflow;
		}

		nBoxSize = CTTIntReader::ReadUint32BE(nBoxHeaderBuffer);
		if(nBoxSize == 1)
		{
			nBoxSize = CTTIntReader::ReadUint64BE(nBoxHeaderBuffer + 8);
			if(nBoxSize < 16)
				return TTKErrArgument;

			nHeadSize = 16;
		}
		else
		{
			if(nBoxSize < 8)
				return TTKErrArgument;

			nHeadSize = 8;
		}

		if (memcmp(nBoxHeaderBuffer + 4, aBoxType, 4) == 0)
		{
			break;
		}

		if(memcmp(nBoxHeaderBuffer + 4, "mdat", 4) == 0)
		{
			iRawDataBegin = nReadPos + nHeadSize;
			iRawDataEnd = nReadPos + nBoxSize;
		}

		nReadPos += nBoxSize;
	}

	aLocation = nReadPos;
	aBoxSize = nBoxSize;

	//LOGI("TTMP4Parser::LocationBox return TTKErrNone");
	return nHeadSize;
}

TTInt CTTMP4Parser::updateTrackInfo()
{
	if(iCurTrackInfo == NULL)
		return TTKErrNone;

	if(iCurTrackInfo->iSampleInfoTab != NULL && iCurTrackInfo->iSampleCount > 0)
		return TTKErrNone; 

	if(iCurTrackInfo->iErrorTrackInfo || iCurTrackInfo->iCodecType == 0 || (iCurTrackInfo->iReadBoxFlag&0x1f) != 0x1f) {
		if(iCurTrackInfo->iStreamID >= 0) {
			if(iCurTrackInfo->iAudio)		
				 iStreamAudioCount--;
			else
				 iStreamVideoCount--;
		}
		removeTrackInfo(iCurTrackInfo);
		iCurTrackInfo = NULL;
		return TTKErrNotSupported;
	}
	
	TTInt nBitrate = 0;
	TTInt nFrameRate = 0;
	TTInt64 nDuration = iDuration;

	buildSampleTab(iCurTrackInfo);

	if(iCurTrackInfo->iDuration) {
		nDuration = iCurTrackInfo->iDuration;
	}

	if(nDuration == 0) {
		if(iCurTrackInfo->iSampleCount > 0) {
			nDuration = (iCurTrackInfo->iSampleInfoTab[1].iSampleTimeStamp - 
					iCurTrackInfo->iSampleInfoTab[0].iSampleTimeStamp)*iCurTrackInfo->iSampleCount;
		}
	}

	if(nDuration) {
		nBitrate = iCurTrackInfo->iTotalSize * 8 * 1000/nDuration;
		nFrameRate = iCurTrackInfo->iSampleCount*1000/nDuration;
	}
	
	iTotalBitrate += nBitrate;
	
	if(iCurTrackInfo->iAudio == 0) {
		TTVideoInfo* pVideoInfo = new TTVideoInfo();
		pVideoInfo->iBitRate = nBitrate;
		pVideoInfo->iFrameRate = nFrameRate;
		pVideoInfo->iWidth = iCurTrackInfo->iWidth;
		pVideoInfo->iHeight = iCurTrackInfo->iHeight;
		pVideoInfo->iStreamId = iCurTrackInfo->iStreamID;
		pVideoInfo->iDuration = nDuration;
		pVideoInfo->iMediaTypeVideoCode = iCurTrackInfo->iCodecType;
		pVideoInfo->iFourCC = iCurTrackInfo->iFourCC;

		if(iCurTrackInfo->iCodecType == TTVideoInfo::KTTMediaTypeVideoCodeH264) {
			pVideoInfo->iDecInfo = iCurTrackInfo->iAVCDecoderSpecificInfo;
		} else if(iCurTrackInfo->iCodecType == TTVideoInfo::KTTMediaTypeVideoCodeMPEG4) {
			pVideoInfo->iDecInfo = iCurTrackInfo->iMP4DecoderSpecificInfo;
		} else if(iCurTrackInfo->iCodecType == TTVideoInfo::KTTMediaTypeVideoCodeHEVC) {
			pVideoInfo->iDecInfo = iCurTrackInfo->iAVCDecoderSpecificInfo;
		}

		if(iCurTrackInfo->iSampleCount)
			iCurTrackInfo->iFrameTime = TTUint((nDuration * 1000) / iCurTrackInfo->iSampleCount);

		if(iParserMediaInfoRef)
			iParserMediaInfoRef->iVideoInfo = pVideoInfo;

		iVideoTrackInfo = iCurTrackInfo;
	} else {
		TTAudioInfo* pAudioInfo = new TTAudioInfo();
		pAudioInfo->iSampleRate = iCurTrackInfo->iM4AWaveFormat->iSampleRate;
		pAudioInfo->iChannel = iCurTrackInfo->iM4AWaveFormat->iChannels;
		pAudioInfo->iBitRate = nBitrate;
		pAudioInfo->iStreamId = iCurTrackInfo->iStreamID;
		pAudioInfo->iDuration = nDuration;
		pAudioInfo->iMediaTypeAudioCode = iCurTrackInfo->iCodecType;
		pAudioInfo->iFourCC = iCurTrackInfo->iFourCC;
		memcpy(pAudioInfo->iLanguage, iCurTrackInfo->iLang_code, 4);

		TTInt64 nTemp = 1024;
		if(iCurTrackInfo->iCodecType == TTAudioInfo::KTTMediaTypeAudioCodeAAC) {
			pAudioInfo->iDecInfo = iCurTrackInfo->iMP4DecoderSpecificInfo;
		}

		TTInt nSampleRate = pAudioInfo->iSampleRate;
		iCurTrackInfo->iFrameTime = TTUint((nTemp * 1000000) / nSampleRate);

		if(iParserMediaInfoRef)
		{
			iParserMediaInfoRef->iAudioInfoArray.Append(pAudioInfo);
		}

		iAudioTrackInfoTab.Append(iCurTrackInfo);
	}

	return TTKErrNone;
}

TTInt CTTMP4Parser::buildSampleTab(TTMP4TrackInfo*	pTrackInfo)
{
	TTSampleInfo* iSampleInfoTab = new TTSampleInfo[pTrackInfo->iSampleCount + 1];
	TTChunkToSample* iChunkToSampleTab = pTrackInfo->iChunkToSampleTab;
	TTimeToSample*  iTimeToSampleTab = pTrackInfo->iTimeToSampleTab;
	TTInt64* iChunkOffsetTab = pTrackInfo->iChunkOffsetTab;
	TTInt* iKeyFrameSampleTab = pTrackInfo->iKeyFrameSampleTab;
	TTInt  iConstantSampleSize = pTrackInfo->iConstantSampleSize;
	TTInt*  iVariableSampleSizeTab = pTrackInfo->iVariableSampleSizeTab;
	
	TTInt nIdx = 0;
	TTInt nSampleIdx = 1;
	TTInt* nKeySampleIdx = NULL;
	TTInt nOldFirstChunkIdx;
	TTInt nOldSampleIdx ;
	TTInt nNewFirstChunkIdx;
	TTInt IsVideo_I_Frame=0;
	TTInt64 nChunkOffset;
	TTInt nChunkCount=1;
	TTInt entrySizeFront=0;
	TTInt k, m;

	if(iKeyFrameSampleTab != NULL)
		nKeySampleIdx = iKeyFrameSampleTab;

	memset(iSampleInfoTab, 0, sizeof(TTSampleInfo)*(pTrackInfo->iSampleCount + 1));

	iChunkToSampleTab[pTrackInfo->iChunkToSampleEntryNum].iFirstChunk = iCurTrackInfo->iChunkOffsetEntryNum + 1;

	for (k=1; k <=pTrackInfo->iChunkToSampleEntryNum; k++)
	{
		/* Only access sample_to_chunk[] if new data is required. */
		nOldFirstChunkIdx = iChunkToSampleTab[k-1].iFirstChunk;
		nOldSampleIdx = iChunkToSampleTab[k-1].iSampleNum;
		nNewFirstChunkIdx = iChunkToSampleTab[k].iFirstChunk;

		for(m=nOldFirstChunkIdx; m<nNewFirstChunkIdx; m++){
			nChunkOffset = iChunkOffsetTab[nChunkCount];

			for(int h=0; h<nOldSampleIdx; h++){
				IsVideo_I_Frame = 0;
				if(iKeyFrameSampleTab != NULL){   
					if(nSampleIdx == *nKeySampleIdx)
					{
						IsVideo_I_Frame = TT_FLAG_BUFFER_KEYFRAME;
						nKeySampleIdx++;
					}
				}

				if(nIdx >= pTrackInfo->iSampleCount) {
					break;
				}

				iSampleInfoTab[nIdx].iSampleIdx = nSampleIdx;
				iSampleInfoTab[nIdx].iFlag |= IsVideo_I_Frame;
				if(iConstantSampleSize) {					
					iSampleInfoTab[nIdx].iSampleEntrySize = iConstantSampleSize;					
					iSampleInfoTab[nIdx].iSampleFileOffset = nChunkOffset + entrySizeFront;
					entrySizeFront += iConstantSampleSize;
				} else {
					iSampleInfoTab[nIdx].iSampleEntrySize = iVariableSampleSizeTab[nSampleIdx];
					iSampleInfoTab[nIdx].iSampleFileOffset = nChunkOffset + entrySizeFront;
					entrySizeFront+=iVariableSampleSizeTab[nSampleIdx];
				}

				nSampleIdx++;
				nIdx++;
			}

			nChunkCount++;
			entrySizeFront=0;
		}
	}

	iSampleInfoTab[pTrackInfo->iSampleCount].iSampleIdx = 0x7fffffff;

	TTInt nCurIndex = 0;
	TTInt nCurCom = 0;
	TTInt64 nTimeStamp = 0;
	TTInt   nScale = pTrackInfo->iScale;
	
	if(nScale == 0)  nScale = 1000;
	
	nIdx = 0;
	TTInt64 nSampleDelta = 0;
	TTInt nComTime = 0;
	for (k = 0; k <pTrackInfo->iTimeToSampleEntryNum; k++) {
		TTInt nCount = iTimeToSampleTab[k].iSampleCount;
		nSampleDelta = iTimeToSampleTab[k].iSampleDelta;
		nComTime = 0;
		for(m = 0; m < nCount; m++) {
			if(nIdx < pTrackInfo->iSampleCount) {
				nComTime = getCompositionTimeOffset(pTrackInfo, nIdx, nCurIndex, nCurCom);
				iSampleInfoTab[nIdx].iSampleTimeStamp = (nTimeStamp + nComTime) * 1000/nScale;
			}

			nTimeStamp += nSampleDelta;
			nIdx++;
		}		
	}

	if(pTrackInfo->iDuration == 0)
		pTrackInfo->iDuration = iSampleInfoTab[pTrackInfo->iSampleCount - 1].iSampleTimeStamp + nSampleDelta * 1000/nScale;

	pTrackInfo->iSampleInfoTab = iSampleInfoTab;

	return TTKErrNone;
}

TTInt CTTMP4Parser::getCompositionTimeOffset(TTMP4TrackInfo* pTrackInfo, TTInt aIndex, TTInt& aCurIndex, TTInt& aCurCom)
{
	TCompositionTimeSample*	iComTimeSampleTab = pTrackInfo->iComTimeSampleTab;
	TTInt					iComTimeSampleEntryNum = pTrackInfo->iComTimeSampleEntryNum;

	if (iComTimeSampleEntryNum == 0 || iComTimeSampleTab == NULL) {
        return 0;
    }

    while (aCurCom < iComTimeSampleEntryNum) {
		TTUint32 sampleCount = iComTimeSampleTab[aCurCom].iSampleCount;
        if (aIndex < aCurIndex + sampleCount) {
			return iComTimeSampleTab[aCurCom].iSampleOffset;
        }

        aCurIndex += sampleCount;
        ++aCurCom;
    }

	return 0;
}

static TTInt ParseDescriptorLength(ITTDataReader& aDataReader, TTUint64& aDesPos, TTUint32& aDesLen, TTUint32& aDescriptorLen)
{
	TTBool bMore;
	TTUint8 n8Bits;

	aDescriptorLen = 0;

	do 
	{
		if (aDesLen == 0) 
		{
			return TTKErrNotSupported;
		}

		aDataReader.ReadSync(&n8Bits, aDesPos, sizeof(TTUint8));
		SKIP_BYTES(aDesPos, aDesLen, sizeof(TTUint8));

		aDescriptorLen = (aDescriptorLen << 7) | (n8Bits & 0x7F);
		bMore = (n8Bits & 0x80) != 0;
	}while (bMore);

	//LOGI("TTMP4Parser::ParseDescriptorLength. length = %lu", aDescriptorLen);

	return TTKErrNone;
}

static TTInt ParseM4AWaveFormat(TTMP4DecoderSpecificInfo* aDecoderSpecificInfo, TTM4AWaveFormat* aWaveFormat)
{
	TTUint8* pData = aDecoderSpecificInfo->iData;

	TTUint32 nFreqIndex = (pData[0] & 7) << 1 | (pData[1] >> 7);

	if (nFreqIndex == 15) 
	{
		if (aDecoderSpecificInfo->iSize < 5) 
		{
			return TTKErrNotSupported;;
		}

		aWaveFormat->iSampleRate = ((pData[1] & 0x7f) << 17)
			| (pData[2] << 9)
			| (pData[3] << 1)
			| (pData[4] >> 7);

		aWaveFormat->iChannels = (pData[4] >> 3) & 15;
	} 
	else 
	{
		static TTUint32 KSamplingRate[] = {
			96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
			16000, 12000, 11025, 8000, 7350
		};

		if (nFreqIndex == 13 || nFreqIndex == 14) 
		{
			return TTKErrNotSupported;
		}

		aWaveFormat->iSampleRate = KSamplingRate[nFreqIndex];
		aWaveFormat->iChannels = (pData[1] >> 3) & 15;
	}

	if (aWaveFormat->iChannels == 0) 
	{
		return TTKErrNotSupported;
	}

	return TTKErrNone;
}

TTInt CTTMP4Parser::ParseDecoderSpecificInfo(TTUint64 aDesPos, TTUint32 aDesLen)
{
	//LOGI("TTMP4Parser::ParseDecoderSpecificInfo. [%lu, %lu]", aDesPos, aDesLen);

	if (aDesLen > 0) 
	{
		TTMP4DecoderSpecificInfo* MP4DecoderSpecificInfo = (TTMP4DecoderSpecificInfo*)malloc(sizeof(TTMP4DecoderSpecificInfo));
		MP4DecoderSpecificInfo->iData = (TTUint8*)malloc(aDesLen* sizeof(TTUint8));
		iDataReader.ReadSync(MP4DecoderSpecificInfo->iData, aDesPos, aDesLen);
		MP4DecoderSpecificInfo->iSize = aDesLen;
		iCurTrackInfo->iMP4DecoderSpecificInfo = MP4DecoderSpecificInfo;

		if(iCurTrackInfo->iAudio)
			return ParseM4AWaveFormat(MP4DecoderSpecificInfo, iCurTrackInfo->iM4AWaveFormat);
		else
			return TTKErrNone;
	}

	return TTKErrNotSupported;
}

TTInt CTTMP4Parser::ParseDecoderConfigDescriptor(TTUint64 aDesPos, TTUint32 aDesLen)
{
	//LOGI("TTMP4Parser::ParseDecoderConfigDescriptor. [%lu, %lu]", aDesPos, aDesLen);

	if (aDesLen < 13)
	{
		return TTKErrNotSupported;
	}

	TTUint8 n8Bits;
	TTUint8 ObjectTypeIndication = 0;
	iDataReader.ReadSync(&ObjectTypeIndication, aDesPos, sizeof(TTUint8));	
	SKIP_BYTES(aDesPos, aDesLen, 13*sizeof(TTUint8));

	if(iCurTrackInfo->iAudio)
	{
		if (ObjectTypeIndication == 0xe1) {
			// it's QCELP 14k...
			return  TTKErrNotSupported;
		}

		if (ObjectTypeIndication  == 0x6b || ObjectTypeIndication  == 0x69) {
			iCurTrackInfo->iCodecType = TTAudioInfo::KTTMediaTypeAudioCodeMP3;
		}
	}

	if (aDesLen > 0)
	{
		iDataReader.ReadSync(&n8Bits, aDesPos, sizeof(TTUint8));
		SKIP_BYTES(aDesPos, aDesLen, sizeof(TTUint8));
		if (n8Bits == KTag_DecoderSpecificInfo)
		{
			TTUint32 nDescriptorLen;
			TTInt nErr = ParseDescriptorLength(iDataReader, aDesPos, aDesLen, nDescriptorLen);
			if (nErr == TTKErrNone)
			{
				return ParseDecoderSpecificInfo(aDesPos, aDesLen);
			}
		}
	}

	return TTKErrNotSupported;
}

TTInt CTTMP4Parser::ParseSLConfigDescriptor(TTUint64 aDesPos, TTUint32 aDesLen)
{
	return TTKErrNone;
}

TTInt CTTMP4Parser::ParseEsDescriptor(TTUint64 aDesPos, TTUint32 aDesLen)
{	
	//LOGI("TTMP4Parser::ParseEsDescriptor. [%lu, %lu]", aDesPos, aDesLen);

	SKIP_BYTES(aDesPos, aDesLen, sizeof(TTUint16));	// skip ES_ID

	TTUint8 n8Bits;
	iDataReader.ReadSync(&n8Bits, aDesPos, sizeof(TTUint8));
	SKIP_BYTES(aDesPos, aDesLen, sizeof(TTUint8));

	TTUint8 nStreamDependenceFlag = n8Bits & 0x80;
	TTUint8 nUrlFlag = n8Bits & 0x40;
	TTUint8 nOcrStreamFlag = n8Bits & 0x20;

	if (nStreamDependenceFlag)
	{
		SKIP_BYTES(aDesPos, aDesLen, sizeof(TTUint16));	// skip dependsOn_ES_ID
	}

	if (nUrlFlag)
	{
		iDataReader.ReadSync(&n8Bits, aDesPos, sizeof(TTUint8));
		SKIP_BYTES(aDesPos, aDesLen, n8Bits+sizeof(TTUint8));		// skip URLstring
	}

	if (nOcrStreamFlag)
	{
		SKIP_BYTES(aDesPos, aDesLen, sizeof(TTUint16));	// skip OCR_ES_Id
	}

	TTInt nErr = TTKErrNotSupported;
	while (aDesLen > 0)
	{
		iDataReader.ReadSync(&n8Bits, aDesPos, sizeof(TTUint8));
		SKIP_BYTES(aDesPos, aDesLen, sizeof(TTUint8));
		TTUint32 nDescriptorLen;
		nErr = ParseDescriptorLength(iDataReader, aDesPos, aDesLen, nDescriptorLen);
		if (nErr == TTKErrNone)
		{
			switch (n8Bits)
			{
			case KTag_DecoderConfigDescriptor:
				nErr = ParseDecoderConfigDescriptor(aDesPos, nDescriptorLen);
				SKIP_BYTES(aDesPos, aDesLen, nDescriptorLen);
				break;

			case KTag_SLConfigDescriptor:
				nErr = ParseSLConfigDescriptor(aDesPos, nDescriptorLen);
				SKIP_BYTES(aDesPos, aDesLen, nDescriptorLen);
				break;

			default:
				break;
			}
		}
	}

	return nErr;
}

TTInt CTTMP4Parser::ReadBoxEsds(TTUint64 aBoxPos, TTUint32 aBoxLen)
{
	//LOGI("TTMP4Parser::ReadBoxEsds. [%lu, %lu]", aBoxPos, aBoxLen);

	SKIP_BYTES(aBoxPos, aBoxLen, 1 + 3);	//version + flags

	TTUint8 n8Bits;
	iDataReader.ReadSync(&n8Bits, aBoxPos, sizeof(TTUint8));
	SKIP_BYTES(aBoxPos, aBoxLen, sizeof(TTUint8));

	if (n8Bits == KTag_ESDescriptor)
	{
		TTUint32 nDescriptorLen;
		TTInt nErr = ParseDescriptorLength(iDataReader, aBoxPos, aBoxLen, nDescriptorLen);
		if (nErr == TTKErrNone && nDescriptorLen >= 3)
		{
			return ParseEsDescriptor(aBoxPos, nDescriptorLen);		
		}
	}

	return TTKErrNotSupported;
}

TTInt CTTMP4Parser::ReadBoxStsd(TTUint64 aBoxPos, TTUint32 aBoxLen)
{
	//LOGI("TTMP4Parser::ReadBoxStsd. [%lu, %lu]", aBoxPos, aBoxLen);
	aBoxPos += (1 + 3);//version + flags

	TTInt nEntryNum = iDataReader.ReadUint32BE(aBoxPos);
	aBoxPos += 4;

	TTInt nErr = TTKErrNone;

	for (TTInt i = 0; i < nEntryNum; i++)
	{
		TTUint32 nEntrySize = iDataReader.ReadUint32BE(aBoxPos);
		TTUint32 nForamtId = iDataReader.ReadUint32BE(aBoxPos + 4);
		if (nForamtId == MAKEFOURCC('a','v','c','1')){
			iCurTrackInfo->iCodecType = TTVideoInfo::KTTMediaTypeVideoCodeH264;
			iCurTrackInfo->iFourCC = nForamtId;
			nErr = ReadBoxStsdVide(aBoxPos, nEntrySize);
			if(nErr != TTKErrNone) {
				iCurTrackInfo->iErrorTrackInfo = 1;
			}
		} else if (nForamtId == MAKEFOURCC('h','v','c','1') || nForamtId == MAKEFOURCC('h','e','v','1')){
			iCurTrackInfo->iCodecType = TTVideoInfo::KTTMediaTypeVideoCodeHEVC;
			iCurTrackInfo->iFourCC = nForamtId;
			nErr = ReadBoxStsdVide(aBoxPos, nEntrySize);
			if(nErr != TTKErrNone) {
				iCurTrackInfo->iErrorTrackInfo = 1;
			}
		} else if (nForamtId == MAKEFOURCC('m','p','4','v')){
			iCurTrackInfo->iCodecType = TTVideoInfo::KTTMediaTypeVideoCodeMPEG4;
			iCurTrackInfo->iFourCC = nForamtId;
			nErr = ReadBoxStsdVide(aBoxPos, nEntrySize);
			if(nErr != TTKErrNone) {
				iCurTrackInfo->iErrorTrackInfo = 1;
			}
		}
		else if (nForamtId == MAKEFOURCC('m','p','4','a'))
		{
			iCurTrackInfo->iCodecType = TTAudioInfo::KTTMediaTypeAudioCodeAAC;
			iCurTrackInfo->iFourCC = MAKEFOURCC('R','A','W',' ');
			nErr = ReadBoxStsdSoun(aBoxPos, nEntrySize);
			if(nErr != TTKErrNone) {
				iCurTrackInfo->iErrorTrackInfo = 1;
			}
		}
		else
		{
			iCurTrackInfo->iErrorTrackInfo = 1;
			return TTKErrNotSupported;
		}
	} 

	return nErr;
}

TTInt CTTMP4Parser::ReadBoxStsdVide(TTUint64 aBoxPos, TTUint32 aBoxLen)
{
	aBoxPos += 8;
	aBoxLen -= 8;

	TTInt  width = iDataReader.ReadUint16BE(aBoxPos+24);
	TTInt  height = iDataReader.ReadUint16BE(aBoxPos+26);

	if(width != 0 && height != 0)
	{
		iCurTrackInfo->iWidth = width;
		iCurTrackInfo->iHeight  = height;
	}

	if(aBoxLen < 78)
		return TTKErrNotSupported;

	aBoxPos += 78;
	aBoxLen -= 78;
    
    TTUint64 BoxLen64 = aBoxLen;

	if(iCurTrackInfo->iCodecType == TTVideoInfo::KTTMediaTypeVideoCodeH264 && iCurTrackInfo->iFourCC == MAKEFOURCC('a','v','c','1'))
	{
		TTInt nHeaderSize = LocationBox(aBoxPos, BoxLen64, "avcC");
		if(nHeaderSize < 0)
			return TTKErrNotSupported;
        
        aBoxLen = BoxLen64;

		return ReadBoxAvcC(aBoxPos, aBoxLen);
	}

	if(iCurTrackInfo->iCodecType == TTVideoInfo::KTTMediaTypeVideoCodeHEVC)
	{
		TTInt nHeaderSize = LocationBox(aBoxPos, BoxLen64, "hvcC");
		if(nHeaderSize < 0)
			return TTKErrNotSupported;
        
        aBoxLen = BoxLen64;

		return ReadBoxHevC(aBoxPos, aBoxLen);
	}
	
	if (aBoxLen > 0)
	{
		TTInt nHeaderSize = LocationBox(aBoxPos, BoxLen64, "esds");
		if(nHeaderSize < 0)
			return TTKErrNone;
        
        aBoxLen = BoxLen64;

		return ReadBoxEsds(aBoxPos + nHeaderSize, aBoxLen - nHeaderSize);
	}

	return TTKErrNone;
}

TTInt CTTMP4Parser::ReadBoxHevC(TTUint64 aBoxPos,TTUint32 aBoxLen)
{
	TTInt32 AvcCSize=aBoxLen - 8;
	aBoxPos += 8;

	TTAVCDecoderSpecificInfo* AVCDecoderSpecificInfo = (TTAVCDecoderSpecificInfo*)malloc(sizeof(TTAVCDecoderSpecificInfo));
	memset(AVCDecoderSpecificInfo, 0, sizeof(TTAVCDecoderSpecificInfo));

	AVCDecoderSpecificInfo->iData = (unsigned char *)malloc(AvcCSize + 128);

	AVCDecoderSpecificInfo->iConfigSize = AvcCSize;
	AVCDecoderSpecificInfo->iConfigData = (unsigned char *)malloc(AvcCSize + 128);

	TTPBYTE   pConfigDate = AVCDecoderSpecificInfo->iConfigData;
	TTUint32  nSize = AvcCSize;
	
	iDataReader.ReadSync(pConfigDate, aBoxPos, nSize);

	TTInt nErr = ConvertHEVCHead(AVCDecoderSpecificInfo->iData, AVCDecoderSpecificInfo->iSize, pConfigDate, nSize);	

	iCurTrackInfo->iAVCDecoderSpecificInfo = AVCDecoderSpecificInfo;
	
	return nErr;
}

TTInt CTTMP4Parser::ReadBoxAvcC(TTUint64 aBoxPos,TTUint32 aBoxLen)
{
	TTInt32 AvcCSize=aBoxLen - 8;
	aBoxPos += 8;

	TTAVCDecoderSpecificInfo* AVCDecoderSpecificInfo = (TTAVCDecoderSpecificInfo*)malloc(sizeof(TTAVCDecoderSpecificInfo));
	memset(AVCDecoderSpecificInfo, 0, sizeof(TTAVCDecoderSpecificInfo));

	AVCDecoderSpecificInfo->iData = (unsigned char *)malloc(AvcCSize + 128);
	AVCDecoderSpecificInfo->iSpsData = (unsigned char *)malloc(AvcCSize);
	AVCDecoderSpecificInfo->iPpsData = (unsigned char *)malloc(AvcCSize);

	AVCDecoderSpecificInfo->iConfigSize = AvcCSize;
	AVCDecoderSpecificInfo->iConfigData = (unsigned char *)malloc(AvcCSize);

	TTPBYTE   pConfigDate = AVCDecoderSpecificInfo->iConfigData;
	TTUint32  nSize = AvcCSize;
	
	iDataReader.ReadSync(pConfigDate, aBoxPos, nSize);

	TTInt nErr = ConvertAVCHead(AVCDecoderSpecificInfo, pConfigDate, nSize);	

	iCurTrackInfo->iAVCDecoderSpecificInfo = AVCDecoderSpecificInfo;
	
	return nErr;
}

TTInt CTTMP4Parser::ReadBoxStsdSoun(TTUint64 aBoxPos,TTUint32 nRemainLen)
{
	if (iCurTrackInfo->iM4AWaveFormat == NULL)
	{
		iCurTrackInfo->iM4AWaveFormat = (TTM4AWaveFormat*)malloc(sizeof(TTM4AWaveFormat));
	}
	
	iCurTrackInfo->iM4AWaveFormat->iChannels = iDataReader.ReadUint16BE(aBoxPos + 24);
	iCurTrackInfo->iM4AWaveFormat->iSampleBit=iDataReader.ReadUint16BE(aBoxPos+26);
	iCurTrackInfo->iM4AWaveFormat->iSampleRate = iDataReader.ReadUint16BE(aBoxPos + 30);
	if (iCurTrackInfo->iM4AWaveFormat->iSampleRate == 0) //error compatible
		iCurTrackInfo->iM4AWaveFormat->iSampleRate = iDataReader.ReadUint16BE(aBoxPos + 32);
	if(iCurTrackInfo->iM4AWaveFormat->iSampleRate == 0)
		iCurTrackInfo->iM4AWaveFormat->iSampleRate = iCurTrackInfo->iScale;

	aBoxPos += 36;/* reserved + data reference index + sound version + reserved */
	nRemainLen -= 36;

	if (nRemainLen > 0)
	{
        TTUint64 nRemainLen64 = nRemainLen;
        TTInt nHeaderSize = LocationBox(aBoxPos, nRemainLen64, "esds");
		if(nHeaderSize < 0)
			return TTKErrNone;
        nRemainLen = nRemainLen64;
		return ReadBoxEsds(aBoxPos + nHeaderSize, nRemainLen - nHeaderSize);
	}

	return TTKErrNone;
}

TTInt CTTMP4Parser::ReadBoxCtts(TTUint64 aBoxPos, TTUint32 aBoxLen)
{
 	TTInt nEntryNum = iDataReader.ReadUint32BE(aBoxPos + 4);
	TCompositionTimeSample*	ComTimeToSampleTab = new TCompositionTimeSample[nEntryNum];

	aBoxPos += 8;
 	for (TTInt i = 0; i < nEntryNum; i++){
 		ComTimeToSampleTab[i].iSampleCount = iDataReader.ReadUint32BE(aBoxPos);
 		ComTimeToSampleTab[i].iSampleOffset = iDataReader.ReadUint32BE(aBoxPos + 4);
		aBoxPos += 8;
 	}

	iCurTrackInfo->iComTimeSampleEntryNum = nEntryNum;
 	iCurTrackInfo->iComTimeSampleTab = ComTimeToSampleTab;

	return TTKErrNone;
}


TTInt CTTMP4Parser::ReadBoxStts(TTUint64 aBoxPos, TTUint32 aBoxLen)
{
 	TTInt TimeToSampleEntryNum = iDataReader.ReadUint32BE(aBoxPos + 4);
 	TTimeToSample* TimeToSampleTab = new TTimeToSample[TimeToSampleEntryNum];

	aBoxPos += 8;
 	for (TTInt i = 0; i < TimeToSampleEntryNum; i++)
 	{
 		TimeToSampleTab[i].iSampleCount = iDataReader.ReadUint32BE(aBoxPos);
 		TimeToSampleTab[i].iSampleDelta = iDataReader.ReadUint32BE(aBoxPos + 4);
		aBoxPos += 8;
 	}

	iCurTrackInfo->iTimeToSampleEntryNum = TimeToSampleEntryNum;
 	iCurTrackInfo->iTimeToSampleTab = TimeToSampleTab;

	return TTKErrNone;
}

TTInt CTTMP4Parser::ReadBoxStss(TTUint64 aBoxPos,TTUint32 aBoxLen)
{
	TTInt nEntryNum = iDataReader.ReadUint32BE(aBoxPos+4);
	TTInt* nKeyFrameSampleTab = new TTInt[nEntryNum + 1];

	aBoxPos+=8;
	for(TTInt i=0;i<nEntryNum;i++){
		nKeyFrameSampleTab[i]=iDataReader.ReadUint32BE(aBoxPos);
		aBoxPos+=4;
	}

	nKeyFrameSampleTab[nEntryNum] = 0x7fffffff;

	iCurTrackInfo->iKeyFrameSampleTab = nKeyFrameSampleTab;
 	iCurTrackInfo->iKeyFrameSampleEntryNum = nEntryNum;

	return TTKErrNone;
}

TTInt CTTMP4Parser::ReadBoxStsz(TTUint64 aBoxPos, TTUint32 aBoxLen)
{
	//LOGI("TTMP4Parser::ReadBoxStsz. [%lu, %lu]", aBoxPos, aBoxLen);
	TTInt MaxFrameSize = 0;
	TTUint64 TotalSize = 0;
	TTInt ConstantSampleSize = iDataReader.ReadUint32BE(aBoxPos + 4);
	TTInt SampleCount = iDataReader.ReadUint32BE(aBoxPos + 8);
	TTInt* VariableSampleSizeTab = NULL;
	
	if(ConstantSampleSize == 0) {
		VariableSampleSizeTab = new TTInt[SampleCount+1];

		aBoxPos+=12;
		for(TTInt i=1; i<=SampleCount; i++){
			VariableSampleSizeTab[i] = iDataReader.ReadUint32BE(aBoxPos);
			if(MaxFrameSize < VariableSampleSizeTab[i])
				MaxFrameSize = VariableSampleSizeTab[i];
			TotalSize += VariableSampleSizeTab[i];
			aBoxPos+=4;
		}
	} else {
		MaxFrameSize = ConstantSampleSize;
		TotalSize = ConstantSampleSize*SampleCount;
	}

	iCurTrackInfo->iConstantSampleSize = ConstantSampleSize;
	iCurTrackInfo->iSampleCount = SampleCount;
	iCurTrackInfo->iVariableSampleSizeTab = VariableSampleSizeTab;
	iCurTrackInfo->iMaxFrameSize = MaxFrameSize;
	iCurTrackInfo->iTotalSize = TotalSize;

	if(iCurTrackInfo->iAudio) {
		SAFE_FREE(iAudioBuffer);

		iAudioBuffer = (TTPBYTE)malloc(MaxFrameSize + 32);
		iAudioSize = MaxFrameSize + 32;	
	} else {
		SAFE_FREE(iVideoBuffer);

		iVideoBuffer = (TTPBYTE)malloc(MaxFrameSize + 32);
		iVideoSize = MaxFrameSize + 32;

		if(iNALLengthSize < 3) {
			SAFE_DELETE_ARRAY(iAVCBuffer);
			iAVCBuffer = new TTUint8[MaxFrameSize + 512];
			iAVCSize = MaxFrameSize + 512;
		}
	}

	return TTKErrNone;
}

TTInt CTTMP4Parser::ReadBoxStsc(TTUint64 aBoxPos, TTUint32 aBoxLen)
{
	TTInt i = 0;
	//LOGI("TTMP4Parser::ReadBoxStsc. [%lu, %lu]", aBoxPos, aBoxLen);
	TTInt nEntryNum = iDataReader.ReadUint32BE(aBoxPos + 4);
	TTChunkToSample* ChunkToSampleTab = new TTChunkToSample[nEntryNum+1];

	aBoxPos += 8;
	for (i = 0; i < nEntryNum; i++)
	{
		ChunkToSampleTab[i].iFirstChunk = iDataReader.ReadUint32BE(aBoxPos);
		ChunkToSampleTab[i].iSampleNum = iDataReader.ReadUint32BE(aBoxPos + 4);
		aBoxPos += 12;
	}

	ChunkToSampleTab[i].iFirstChunk = ChunkToSampleTab[i-1].iFirstChunk + 1;
	ChunkToSampleTab[i].iSampleNum = 0;

	iCurTrackInfo->iChunkToSampleTab = ChunkToSampleTab;
	iCurTrackInfo->iChunkToSampleEntryNum = nEntryNum;

	return TTKErrNone;
};

TTInt CTTMP4Parser::ReadBoxCo64(TTUint64 aBoxPos, TTUint32 aBoxLen)
{
	//LOGI("TTMP4Parser::ReadBoxCo64. [%lu, %lu]", aBoxPos, aBoxLen);
	TTInt nEntryNum = iDataReader.ReadUint32BE(aBoxPos + 4);
	
	if(iCurTrackInfo->iAudio){
		TTInt nEntrySize = iCurTrackInfo->iTotalSize / nEntryNum;
		TTInt nMaxEntrySize = KMaxALACFrameSize;
		if ((nEntryNum <= 0) || (nEntrySize > nMaxEntrySize))
		{
			iCurTrackInfo->iErrorTrackInfo = 1;
			return TTKErrNotSupported;
		}
	}

	TTInt64* nChunkOffsetTab = new TTInt64[nEntryNum+1];

	aBoxPos += 8;
	//sample to chunkoffset
	for(int i=1;i<=nEntryNum;i++){
		nChunkOffsetTab[i] = iDataReader.ReadUint64BE(aBoxPos);
		aBoxPos += 8;
	}

	iCurTrackInfo->iChunkOffsetTab = nChunkOffsetTab;
	iCurTrackInfo->iChunkOffsetEntryNum = nEntryNum;

	return TTKErrNone;
}

TTInt CTTMP4Parser::ReadBoxStco(TTUint64 aBoxPos, TTUint32 aBoxLen)
{
	//LOGI("TTMP4Parser::ReadBoxStco. [%lu, %lu]", aBoxPos, aBoxLen);
	TTInt nEntryNum = iDataReader.ReadUint32BE(aBoxPos + 4);
	//audio
	if(iCurTrackInfo->iAudio){
		TTInt nEntrySize = iCurTrackInfo->iTotalSize / nEntryNum;
		TTInt nMaxEntrySize = KMaxALACFrameSize;
		if ((nEntryNum <= 2) || (nEntrySize > nMaxEntrySize))
		{
			iCurTrackInfo->iErrorTrackInfo = 1;
			return TTKErrNotSupported;
		}
	}

	TTInt64* nChunkOffsetTab = new TTInt64[nEntryNum+1];

	aBoxPos += 8;
	//sample to chunkoffset
	for(int i=1;i<=nEntryNum;i++){
		nChunkOffsetTab[i]=(TTInt64)iDataReader.ReadUint32BE(aBoxPos);
		aBoxPos+=4;
	}

	iCurTrackInfo->iChunkOffsetTab = nChunkOffsetTab;
	iCurTrackInfo->iChunkOffsetEntryNum = nEntryNum;

	return TTKErrNone;
}

TTInt CTTMP4Parser::removeTrackInfo(TTMP4TrackInfo*	pTrackInfo)
{
	if(pTrackInfo == NULL)
		return TTKErrNone;

	if (pTrackInfo->iMP4DecoderSpecificInfo != NULL)
	{
		SAFE_FREE(pTrackInfo->iMP4DecoderSpecificInfo->iData);
		SAFE_FREE(pTrackInfo->iMP4DecoderSpecificInfo);
	}

	if(pTrackInfo->iAVCDecoderSpecificInfo != NULL)
	{
		SAFE_FREE(pTrackInfo->iAVCDecoderSpecificInfo->iData);
		SAFE_FREE(pTrackInfo->iAVCDecoderSpecificInfo->iConfigData);
		SAFE_FREE(pTrackInfo->iAVCDecoderSpecificInfo->iSpsData);
		SAFE_FREE(pTrackInfo->iAVCDecoderSpecificInfo->iPpsData);
		SAFE_FREE(pTrackInfo->iAVCDecoderSpecificInfo);
	}

	SAFE_FREE(pTrackInfo->iM4AWaveFormat);
	SAFE_DELETE_ARRAY(pTrackInfo->iComTimeSampleTab);
	SAFE_DELETE_ARRAY(pTrackInfo->iTimeToSampleTab);
	SAFE_DELETE_ARRAY(pTrackInfo->iVariableSampleSizeTab);
	SAFE_DELETE_ARRAY(pTrackInfo->iChunkOffsetTab);
	SAFE_DELETE_ARRAY(pTrackInfo->iChunkToSampleTab);
	SAFE_DELETE_ARRAY(pTrackInfo->iKeyFrameSampleTab);
	SAFE_DELETE_ARRAY(pTrackInfo->iSampleInfoTab);

	SAFE_DELETE(pTrackInfo);

	return TTKErrNone; 
}
