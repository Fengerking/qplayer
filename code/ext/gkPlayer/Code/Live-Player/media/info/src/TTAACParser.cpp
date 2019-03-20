/*
============================================================================
 Name        : TTAACParser.cpp
 Author      : ²Üºú
 Version     :
 Copyright   : Your copyright notice
 Description : CTTAACParser implementation
============================================================================
*/
#include "TTID3Tag.h"
#include "TTAPETag.h"
#include "TTAACParser.h"
#include "TTFileReader.h"
#include "TTLog.h"

CTTAACParser::~CTTAACParser()
{
}

CTTAACParser::CTTAACParser(ITTDataReader& aDataReader, ITTMediaParserObserver& aObserver)
: CTTMediaParser(aDataReader, aObserver)
, iTotalFrameSize(0)
, iFrameNum(0)
, iAVEFrameSize(0)
{

}

TTInt CTTAACParser::RawDataEnd()
{
	TTInt nID3v1TagSize = ID3v1TagSize(iDataReader);
	TTInt nAPETagSize = APETagSize(iDataReader);

	return iDataReader.Size() - nID3v1TagSize - nAPETagSize;
}

TTInt CTTAACParser::Parse(TTMediaInfo& aMediaInfo)
{
	TTInt nReadTagSize = 0;
	TTInt nReadPos = 0;

	do
	{
		nReadTagSize = ID3v2TagSize(iDataReader, nReadPos);
		nReadPos += nReadTagSize;
	} while (nReadTagSize > 0);

	iParserMediaInfoRef = &aMediaInfo;

	TTInt nMaxFirstFrmOffset = KMaxFirstFrmOffset + nReadPos;

	iRawDataEnd = RawDataEnd();

	TTFrmSyncResult tResult;
	TTInt nOffSet = 0;
	TTInt nProcessedSize = 0;
	TTInt nErr = TTKErrNotSupported;
	TTInt nCount = 0;

	do
	{
		if (nReadPos >= nMaxFirstFrmOffset)
		{
			LOGI("TTAACParser::Parse. ReadPos >= nMaxFirstFrmOffset");
			nErr = TTKErrNotSupported;
			break;
		}
		else
		{
			tResult = FrameSyncWithPos(nReadPos, nOffSet, nProcessedSize, iFirstFrmInfo, ETTTrue);
			LOGI("TTAACParser::FrameSyncWithPos : %d", tResult);
			if ((EFrmSyncComplete == tResult) || (EFrmSyncEofComplete == tResult))
			{
				iFrameTime = TTUint(((TTInt64)iFirstFrmInfo.nSamplesPerFrame * 1000000) / iFirstFrmInfo.nSampleRate);
				iAVEFrameSize = iFirstFrmInfo.nFrameSize;
				iRawDataBegin = nReadPos + nOffSet;

				TTAudioInfo* pAudioInfo = new TTAudioInfo();			
				pAudioInfo->iBitRate = iFirstFrmInfo.nBitRate;
				pAudioInfo->iChannel = iFirstFrmInfo.nChannels;			
				pAudioInfo->iSampleRate = iFirstFrmInfo.nSampleRate;
				pAudioInfo->iMediaTypeAudioCode = TTAudioInfo::KTTMediaTypeAudioCodeAAC;
				if(iFirstFrmInfo.nAACType == AACTYPE_ADTS) {
					pAudioInfo->iFourCC = MAKEFOURCC('A','D','T','S');
				} else if(iFirstFrmInfo.nAACType == AACTYPE_ADIF) {
					pAudioInfo->iFourCC = MAKEFOURCC('A','D','I','F');
				}
				pAudioInfo->iStreamId = EMediaStreamIdAudioL;
				GKASSERT(aMediaInfo.iAudioInfoArray.Count() == 0);
				aMediaInfo.iAudioInfoArray.Append(pAudioInfo);
				iStreamAudioCount++;

				nErr = TTKErrNone;
				break;
			}

			nReadPos += nProcessedSize;

			if(nProcessedSize == 0) {
				if(iDataReader.Id() == ITTDataReader::ETTDataReaderIdFile || tResult == EFrmSyncReadErr)
					nCount++;

				if (iDataReader.Id() == ITTDataReader::ETTDataReaderIdHttp || iDataReader.Id() == ITTDataReader::ETTDataReaderIdBuffer) {
					mSemaphore.Wait(5);
				}
			} else {
				nCount = 0;
			}

			if(nCount > 100)
				break;
		}
	} while (tResult != EFrmSyncReadErr && tResult != EFrmSyncEofInComplete);


	LOGI("TTAACParser::Parse return: %d", nErr);
	return nErr;
}

TTUint CTTAACParser::MediaDuration(TTInt /*aStreamId*/)
{
	if (iFrmPosTabDone)
	{
		return iFrameTime * iFrmPosTabCurIndex / 1000;
	}
	else
	{
        if(iFrmPosTabCurIndex > 0)
        {
            TTInt nFramesParsed = iFrmPosTabCurIndex - 1;
            if (nFramesParsed != 0)
            {
                TTInt32 *pFrmPosTab = (TTInt32 *)(iFrmPosTab);
                iAVEFrameSize = (*(pFrmPosTab + nFramesParsed) - iRawDataBegin) / nFramesParsed;
            }
        }

		TTUint64 nStreamSize = iRawDataEnd - iRawDataBegin;
		//LOGI("AACParser::MediaDuration. iFrmPosTabDone = %d, iFrmPosTabCurIndex = %d", iFrmPosTabDone, iFrmPosTabCurIndex);
		//LOGI("AACParser::MediaDuration. iRawDataEnd = %d, iRawDataBegin = %d, nStreamSize = %llu", iRawDataEnd, iRawDataBegin, nStreamSize);
		//LOGI("AACParser::MediaDuration. iFrameTime = %d, iAVEFrameSize = %d, return = %u", iFrameTime, iAVEFrameSize, (TTUint)(nStreamSize * iFrameTime / (iAVEFrameSize * 1000)));
		return (TTUint)(nStreamSize * iFrameTime / (iAVEFrameSize * 1000));
	}
}

void CTTAACParser::StartFrmPosScan()
{
	if(iDataReader.Size() > 1024*1024*20)
		return;

	CTTMediaParser::StartFrmPosScan();
}

void CTTAACParser::ParseFrmPos(const TTUint8 *aData, TTInt aParserSize)
{
	if ((aData != NULL) && (aParserSize >= 4))
	{
		AAC_HEADER mh;
		AAC_FRAME_INFO mi;
		TTInt32 *pFrmPosTab = (TTInt32 *)(iFrmPosTab);
		TTInt nPos = iFrmPosTabCurOffset;

		do
		{
			if (CTTAACHeader::AACCheckHeader(aData, mh) && CTTAACHeader::AACParseFrame(mh, mi) && (mi.nFrameSize > 0) && (mi.nFrameSize < KAACMaxFrameSize))
			{
				*(pFrmPosTab + (iFrmPosTabCurIndex++)) = nPos;
				nPos         += mi.nFrameSize;
				aData        += mi.nFrameSize;
				aParserSize  -= mi.nFrameSize;
			}
			else
			{
				nPos        ++;
				aData       ++;
				aParserSize --;
			}
		} while ((aParserSize >= KAACMinFrameSize) && (iFrmPosTabCurIndex < iFrmTabSize));

		iFrmPosTabCurOffset = nPos;

		if (iFrmPosTabCurIndex >= iFrmTabSize)
		{
			FrmIdxTabReAlloc();
		}
	}
}

TTInt CTTAACParser::SeekWithinFrmPosTab(TTInt aStreamId, TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo)
{
	TTInt nErr = TTKErrNotFound;
	TTInt64& nFramePos = aFrameInfo.iFrameLocation;
	TTInt& nFrameSize = aFrameInfo.iFrameSize;

	TTUint *pFrmPosTab = iFrmPosTab + aFrmIdx;

	if (aFrmIdx < (iFrmPosTabCurIndex - 1))
	{
		nFramePos = *pFrmPosTab++;
		nFrameSize = *pFrmPosTab - nFramePos;
		nErr = (nFrameSize > KAACMaxFrameSize) ? TTKErrTooBig : TTKErrNone;
	}
	else if (iFrmPosTabDone && aFrmIdx == iFrmPosTabCurIndex - 1)
	{
		nFramePos = *pFrmPosTab;

		TTInt nOffset;
		TTInt nReadSize;
		AAC_FRAME_INFO tFrameInfo;

		FrameSyncWithPos(nFramePos, nOffset, nReadSize, tFrameInfo);

		nFramePos += nOffset;
		nFrameSize = tFrameInfo.nFrameSize;

		nErr = TTKErrEof;
	}

	if ((nErr == TTKErrNone) || (nErr == TTKErrEof))
	{
		UpdateFrameInfo(aFrameInfo, aFrmIdx);
	}

	return nErr;
}

void CTTAACParser::UpdateFrameInfo(TTMediaFrameInfo& aFrameInfo,  TTInt aFrameIdx)
{
	TTUint64 nTmp64 = aFrameIdx;
	aFrameInfo.iFrameStartTime = (TTUint)(nTmp64 * iFrameTime / 1000);
}

TTInt CTTAACParser::SeekWithoutFrmPosTab(TTInt aStreamId, TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo)
{
	TTInt nErr = CTTMediaParser::SeekWithoutFrmPosTab(aStreamId, aFrmIdx, aFrameInfo);

	if ((nErr == TTKErrNone) || (nErr == TTKErrEof))
	{
		UpdateFrameInfo(aFrameInfo, aFrmIdx);

		iTotalFrameSize += aFrameInfo.iFrameSize;
		iFrameNum++;

		iAVEFrameSize = iTotalFrameSize/iFrameNum;		
	}

	return nErr;
}

TTInt CTTAACParser::SeekWithPos(TTInt aStreamId, TTInt64 aPos, TTInt64 &aFrmPos, TTInt &aFrmSize)
{
	TTInt nOffset = 0;
	TTInt nProcessSize = 0;
	AAC_FRAME_INFO tAACFrameInfo;

	TTInt nErr = TTKErrNotFound;

	TTFrmSyncResult tResult = FrameSyncWithPos(aPos, nOffset, nProcessSize, tAACFrameInfo);

	if ( EFrmSyncInComplete == tResult)
	{
		return TTKErrUnderflow;
	}
	else if (EFrmSyncReadErr == tResult)
	{
		return TTKErrOverflow;
	}

	if (SYNC_COMPLETED(tResult))
	{
		aFrmSize = tAACFrameInfo.nFrameSize;
		aFrmPos = aPos + nOffset;
		nErr = TTKErrNone;
	}

	if (SYNC_EOF(tResult))
	{
		nErr = TTKErrEof;
	}
	return nErr;
}

TTInt CTTAACParser::SeekWithIdx(TTInt aStreamId, TTInt aFrmIdx, TTInt64 &aFrmPos, TTInt &aFrmSize)
{
    if (iFrmPosTabCurIndex > 0) 
    {
        TTInt32* pFrmPosTab = (TTInt32 *)(iFrmPosTab);
        TTInt nFramesParsed = iFrmPosTabCurIndex - 1;
        
        if (nFramesParsed != 0)
            iAVEFrameSize = (*(pFrmPosTab + nFramesParsed) - iRawDataBegin) / nFramesParsed;        
    }
    
    TTInt nPos = iRawDataBegin + iAVEFrameSize * aFrmIdx;
	return SeekWithPos(aStreamId, nPos, aFrmPos, aFrmSize);
}

TTFrmSyncResult CTTAACParser::FrameSyncWithPos(TTInt aReadPos, TTInt &aOffSet, TTInt &aProcessedSize,
										AAC_FRAME_INFO& aFrameInfo, TTBool aCheckNextFrameHeader)
{
	TTInt nTotalOffset = 0;
	TTUint8* pData = NULL;
	TTInt nReadSize = KSyncReadSize;

	TTBool bFoundAnySync = ETTFalse;

	TTReadResult nReadReturnStatus = ReadStreamData(aReadPos, pData, nReadSize);
	switch(nReadReturnStatus)
	{
	case EReadEof:
	case EReadOK:
		{
			TTInt nRemainSize = nReadSize;
			TTBool bFound = ETTFalse;
			AAC_FRAME_INFO tFrameInfo;
			tFrameInfo.nAACType = AACTYPE_ADTS;

			while (!bFound)
			{
				TTInt nOffset = 0;
				if (!CTTAACHeader::AACSyncFrameHeader(pData, nRemainSize, nOffset, tFrameInfo))
				{
					break;
				}

				bFoundAnySync = ETTTrue;
				bFound = ETTTrue;
				pData += nOffset;
				nTotalOffset += nOffset;
				nRemainSize -= nOffset;

				if (aCheckNextFrameHeader && nRemainSize > (KAACMinFrameSize + tFrameInfo.nFrameSize))
				{
					AAC_HEADER mh;
					AAC_FRAME_INFO mi;
					bFound = CTTAACHeader::AACCheckHeader(pData + tFrameInfo.nFrameSize, mh) && CTTAACHeader::AACParseFrame(mh, mi) && (mi.nFrameSize > 0);
					if (!bFound)
					{
						pData++;
						nTotalOffset++;
						nRemainSize--;
					}
				}
			}

			aProcessedSize = bFoundAnySync ? nTotalOffset : nReadSize;

			if (bFound)
			{
				aFrameInfo = tFrameInfo;
				aOffSet = nTotalOffset;
			}

			TTBool bEof = EReadEof == nReadReturnStatus;
			return (TTFrmSyncResult)((bFound << SYNC_COMPLETED_SHIFT) | (bEof << SYNC_EOF_SHIFT));
		}
		break;

	case EReadUnderflow:
		return EFrmSyncInComplete;
		break;

	case EReadOverflow:
	case EReadErr:
	default:
		return EFrmSyncReadErr;
	}
}

TTInt CTTAACParser::GetFrameLocation(TTInt aStreamId, TTInt& aFrmIdx, TTUint64 aTime)
{
	GKASSERT(iFrameTime > 0);
	aFrmIdx = ((TTInt64)aTime * 1000 + iFrameTime / 2) / iFrameTime;
	return TTKErrNone;
}
