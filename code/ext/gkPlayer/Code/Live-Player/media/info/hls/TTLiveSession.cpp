/**
* File : TTLiveSession.cpp
* Created on : 2015-5-12
* Author : yongping.lin
* Description : CLiveSession
*/

#include "TTLiveSession.h"
#include "TTLog.h"

CLiveSession::CLiveSession(ITTStreamBufferingObserver* aObserver, PlaylistManager* aPlayList)
:mObserver(aObserver)
,mPlayListManager(aPlayList)
,mCancel(0)
,mContentSize(0)
,mDownLoadSize(0)
,mCurSeq(0)
,mOffset(0)
,mAudioSeek(false)
,mVideoSeek(false)
,mPause(false)
,mMediaType(0)
,mTSParser(0)
,mPackedAudio(0)
,mReconNum(0)
,mChunkRecNum(0)
{
	mMediaThread   = new TTEventThread("TTHLSSession Thread");
	mMediaIO = new CTTIOClient(mObserver);

	mCritical.Create();
	mCriticalSrc.Create();
	mCriticalEvent.Create();

	memset(&mListItem, 0, sizeof(ListItem));
	memset(&mSegmentItem, 0, sizeof(SegmentItem));
}

CLiveSession::~CLiveSession()
{
	stop();

	SAFE_DELETE(mMediaThread);
	SAFE_DELETE(mMediaIO);
	mCriticalSrc.Lock();
	SAFE_DELETE(mTSParser);
	SAFE_DELETE(mPackedAudio);
	mCriticalSrc.UnLock();

	mCriticalSrc.Destroy();
	mCriticalEvent.Destroy();
	mCritical.Destroy();
}


int	 CLiveSession::setUrlListItem(ListItem* aListItem)
{
	memcpy(&mListItem, aListItem, sizeof(ListItem));
	return TTKErrNone;	
}
	
int  CLiveSession::start(int nseqNum)
{
	stop();

	memset(&mSegmentItem, 0, sizeof(SegmentItem));	
	TTInt nErr = mPlayListManager->getSegmentItemBySeqNumFromItem(&mListItem, nseqNum, &mSegmentItem);
	if(nErr < 0) {
		return nErr;
	}

	mCurSeq = nseqNum;

	mMediaThread->start();

	GKCAutoLock Lock(&mCriticalEvent);
	postMediaMsgEvent(0, EMediaDownLoadStart, 0, 0, (void *)&mSegmentItem);

	return TTKErrNone;	
}

int  CLiveSession::pause()
{
	GKCAutoLock Lock(&mCritical);
	mPause = true;
	return TTKErrNone;		
}

int  CLiveSession::resume()
{
	GKCAutoLock Lock(&mCritical);
	mPause = false;
	return TTKErrNone;	
}
	
int  CLiveSession::stop()
{
	mMediaIO->Cancel();
	mMediaThread->stop();
	mMediaIO->Close();

	GKCAutoLock Lock(&mCriticalSrc);
	SAFE_DELETE(mTSParser);
	SAFE_DELETE(mPackedAudio);

	mCancel = 0;
	mOffset = 0;
	mReconNum = 0;
	mDownLoadSize = 0;
	mAudioSeek = false;
	mVideoSeek = false;
	mPause = false;
	return TTKErrNone;	
}

void  CLiveSession::cancel()
{
	mMediaIO->Cancel();
	mCancel = 1;
}

TTInt64  CLiveSession::seek(TTUint64 aPosMS,TTInt aOption, TTInt64 aFirstPTS)
{
	mAudioSeek = true;
	mVideoSeek = true;
	TTInt64 nTimeUs = seekTime(aPosMS, aOption, aFirstPTS);
	if(nTimeUs > 0) {
//        if(aOption) {
//            nTimeUs = aPosMS;
//        }
//		//nTimeUs -= aFirstPTS;
//		if(nTimeUs < 0) {
//			nTimeUs = 0;
//		}
		return nTimeUs;
	}
	
	cancel();
	mMediaThread->cancelAllEvent();
	mMediaIO->Close();

	mCancel = 0;
	nTimeUs = aPosMS;

	TTInt nSeqNum = mPlayListManager->getSeqNumberForTimeFromItem(&mListItem, &nTimeUs);
	if(nSeqNum < 0) {
		mAudioSeek = false;
		mVideoSeek = false;
		return nSeqNum;
	}

	mCurSeq = nSeqNum;

//	if(aOption) {
//		nTimeUs = aPosMS;
//	}

	mCriticalSrc.Lock();
	if(mMediaType == 0) {
		if(mTSParser) {
			mTSParser->signalEOS(0);
		}
	} else {
		if(mPackedAudio) {
			mPackedAudio->signalEOS(0);
		}
	}
	//setStartTime(aFirstPTS + aPosMS, aOption);
	clearBuffer();
	mCriticalSrc.UnLock();

	GKCAutoLock Lock(&mCriticalEvent);
	postMediaMsgEvent(0, EMediaSeekToSeqNum, nSeqNum, 0, (void *)&mSegmentItem);

	return nTimeUs;	
}

void	 CLiveSession::clearBuffer()
{
	TTBufferManager* source = NULL;	
	GKCAutoLock Lock(&mCriticalSrc);
	if(mMediaType == 0) {
		if(mTSParser == NULL)
			return;
		
		if(mTSParser->isHeadReady() == 0)
			return;

		TTInt nNum = mTSParser->getProgramStreamNum(1);
		for(TTInt n = 0; n < nNum; n++) {
			source = mTSParser->getStreamSource(1, n);
			if(source) {
				source->clear();
			}
		}
	} else if (mMediaType == 1) {
		source = mPackedAudio->getStreamSource();
		if(source) {
			source->clear();
		} 
	}
}

int	 CLiveSession::setStartTime(TTInt64 aStartTime,TTInt aOption)
{
	TTBufferManager* source = NULL;	
	GKCAutoLock Lock(&mCriticalSrc);
	if(mMediaType == 0) {
		if(mTSParser == NULL)
			return TTKErrNotReady;
		
		if(mTSParser->isHeadReady() == 0)
			return TTKErrNotReady;

		TTInt nNum = mTSParser->getProgramStreamNum(1);
		for(TTInt n = 0; n < nNum; n++) {
			source = mTSParser->getStreamSource(1, n);
			if(source && source->isAudio()) {
				source->setStartTime(aStartTime, aOption);
			}
		}
	} else if (mMediaType == 1) {
		source = mPackedAudio->getStreamSource();
		if(source) {
			source->setStartTime(aStartTime, aOption);
		} else {
			return TTKErrNotReady;
		}
	}

	return 0;
}

TTInt64	 CLiveSession::seekTime(TTUint64 aPosMS,TTInt aOption, TTInt64 aFirstPTS)
{
	TTBufferManager* source = NULL;	
	TTInt64 nTimeUs = aPosMS;

	GKCAutoLock Lock(&mCriticalSrc);
	if(mMediaType == 0) {
		if(mTSParser == NULL)
			return TTKErrNotReady;
		
		if(mTSParser->isHeadReady() == 0)
			return TTKErrNotReady;

		TTInt nNum = mTSParser->getProgramStreamNum(1);
		for(TTInt n = 0; n < nNum; n++) {
			source = mTSParser->getStreamSource(1, n);
			if(source && source->isVideo()) {
				nTimeUs = seekSource(source, nTimeUs, aFirstPTS);
				if(nTimeUs < 0) {
					return nTimeUs;
				}
			}
		}

//		if(aOption) {
//			nTimeUs = aPosMS;
//		}

		for(TTInt n = 0; n < nNum; n++) {
			source = mTSParser->getStreamSource(1, n);
			if(source && source->isAudio()) {
				nTimeUs = seekSource(source, nTimeUs, aFirstPTS);
				if(nTimeUs < 0) {
					return nTimeUs;
				}
			}
		}
	} else if (mMediaType == 1) {
		source = mPackedAudio->getStreamSource();
		if(source) {
			nTimeUs = seekSource(source, nTimeUs, aFirstPTS);
		} else {
			return TTKErrNotReady;
		}
	}

	return nTimeUs;
}

TTInt64	 CLiveSession::seekSource(TTBufferManager* source, TTUint64 aPosMS, TTInt64 aFirstPTS)
{
	TTInt64 nTimeUs = aPosMS;
	TTInt64 nNextTimeUS = 0;
	if(source->nextBufferTime(&nNextTimeUS) < 0) {
		return TTKErrNotReady;
	}

	if(mPlayListManager->isLive()) {
		return nNextTimeUS;
	}

	int nEOS = 0;
	int nDuration = source->getBufferedDurationUs(&nEOS);				
	if(aFirstPTS < 0) {
		aFirstPTS = nNextTimeUS;
	}

	if(nTimeUs + aFirstPTS >= nNextTimeUS - 500 && nTimeUs + aFirstPTS <= nNextTimeUS + nDuration + mPlayListManager->getTargetDuration()) {
		nTimeUs = source->seek(nTimeUs + aFirstPTS);
	} else {
		return TTKErrNotReady;
	}

	return nTimeUs;
}

TTInt64	 CLiveSession::seektoTime(bool isAudio, TTInt64 aSeekPoint)
{
	TTBufferManager* source = NULL;

	GKCAutoLock Lock(&mCriticalSrc);
	if(mMediaType == 0) {
		if(mTSParser == NULL)
			return TTKErrNotReady;
		
		if(mTSParser->isHeadReady() == 0) {
			return TTKErrNotReady;
		}

		TTInt nNum = mTSParser->getProgramStreamNum(1);
		TTInt hasFound = 0;
		for(TTInt n = 0; n < nNum; n++) {
			source = mTSParser->getStreamSource(1, n);
			if(source && isAudio == source->isAudio()) {
				hasFound = 1;
				break;
			}
		}

		if(hasFound == 0) {
			return TTKErrNotFound;
		}
	} else if (mMediaType == 1) {
		source = mPackedAudio->getStreamSource();
		if(source == NULL ||  isAudio != source->isAudio()) {
			return TTKErrNotFound;
		} 
	}

	TTInt64 nTime = source->seek(aSeekPoint);
	

	return nTime;
}

int	 CLiveSession::isHeaderReady()
{
	TTBufferManager* source = NULL;	

	GKCAutoLock Lock(&mCriticalSrc);
	if(mMediaType == 0) {
		if(mTSParser == NULL)
			return TTKErrNone;
		
		if(mTSParser->isHeadReady() == 0)
			return TTKErrNone;

		TTInt nNum = mTSParser->getProgramStreamNum(1);
		TTInt nReady = 0;
		TTInt hasAudio = 0;
		TTInt hasVideo = 0;
		for(TTInt n = 0; n < nNum; n++) {
			source = mTSParser->getStreamSource(1, n);
			if(source) {
				if(source->getBufferCount() >= 1) {
					nReady++;					
				}

				if(source->isAudio()) {
					hasAudio++;
				}

				if(source->isVideo()) {
					hasVideo++;
				}
			}
		}

		if(nReady >= nNum || (hasAudio && hasVideo)) { 
			return nReady;
		}
	} else if (mMediaType == 1) {
		source = mPackedAudio->getStreamSource();
		if(source && source->getBufferCount() >= 1) {
			return 1;
		}
	}

	return 0;
}

int	 CLiveSession::isBufferReady(TTInt nDuration, TTInt audioID, TTInt videoID)
{
	GKCAutoLock Lock(&mCriticalSrc);

	TTInt nErr = TTKErrNone;
	TTInt nNum = getProgramStreamNum(1);
	TTBufferManager* source = NULL;
	TTInt isAudio = 0;
	TTInt isVideo = 0;

	if(mDownLoadSize == 0) {
		return TTKErrNone;
	}
		
	for(TTInt n = 0; n < nNum; n++) {
		source = getStreamSource(1, n);
		if(source) {
			if(source->isAudio()) {
				isAudio = 1;
				int isEOS = 0;
				TTInt64 nBufferedDurationUs = source->getBufferedDurationUs(&isEOS);
				if(nBufferedDurationUs >= nDuration || isEOS) {
					isAudio = 2;
				}
			}

			if(source->isVideo()) {
				isVideo = 1;
				int isEOS = 0;
				TTInt64 nBufferedDurationUs = source->getBufferedDurationUs(&isEOS);

				if(nBufferedDurationUs >= nDuration || isEOS) {
					isVideo = 2;
				}
			}
		}
	}

	if((isAudio == 2 && isVideo == 2) || (isAudio == 0 && isVideo == 2) || (isAudio == 2 && isVideo == 0)) {
		nErr = TTKErrNone;
	} else {
		nErr = TTKErrNotReady;
	}
	return nErr;
}

int	 CLiveSession::GetMediaSample(TTMediaType aStreamType, TTBuffer* pMediaBuffer, TTInt elementID)
{
	GKCAutoLock Lock(&mCriticalSrc);
	TTInt nErr = TTKErrNone;
	TTInt nNum = getProgramStreamNum(1);
	TTBufferManager* source = NULL;
	TTBuffer buffer;
	TTInt nFound = 0;

	if(mVideoSeek && aStreamType == EMediaTypeVideo) {
		if(pMediaBuffer->nFlag & TT_FLAG_BUFFER_SEEKING) {
			mVideoSeek = false;
		} else {
			return TTKErrInUse;
		}
	}

	if(mAudioSeek && aStreamType == EMediaTypeAudio) {
		if(pMediaBuffer->nFlag & TT_FLAG_BUFFER_SEEKING) {
			mAudioSeek = false;
		} else {
			return TTKErrInUse;
		}
	}

	memset(&buffer, 0, sizeof(TTBuffer));
	buffer.llTime = pMediaBuffer->llTime;	
	for(TTInt n = 0; n < nNum; n++) {
		source = getStreamSource(1, n);
		if(source) {
			if(source->isAudio() && aStreamType == EMediaTypeAudio) {
				nErr = source->dequeueAccessUnit(&buffer);
				if(nErr ==  TTKErrNone) {
					memcpy(pMediaBuffer, &buffer, sizeof(TTBuffer));
					nFound = 1;
					break;
				} else if(nErr == TTKErrEof){
					nFound = 2;
				} 
			}

			if(source->isVideo() && aStreamType == EMediaTypeVideo) {
				nErr = source->dequeueAccessUnit(&buffer);
				if(nErr ==  TTKErrNone) {
					memcpy(pMediaBuffer, &buffer, sizeof(TTBuffer));
					nFound = 1;
					break;
				} else if(nErr == TTKErrEof){
					nFound = 2;
				} 
			}
		}
	}

	if(nFound == 1) {
		return TTKErrNone;
	} else if(nFound == 2) {
		return TTKErrEof;
	}

	return TTKErrNotReady;
}

int	 CLiveSession::bandPercent(int nDuration)
{
	GKCAutoLock Lock(&mCriticalSrc);
	TTInt nErr = TTKErrNone;
	TTInt nNum = getProgramStreamNum(1);
	TTBufferManager* source = NULL;
	TTBuffer buffer;
	TTInt nPercent = 0;
	if(nDuration == 0) {
		nDuration = 5000;
	}

	for(TTInt n = 0; n < nNum; n++) {
		source = getStreamSource(1, n);
		if(source) {
			int nEOS;
			int percent = source->getBufferedDurationUs(&nEOS)*100/nDuration;
			if(percent > 100 || nEOS) {
				percent = 100;
			}

			if(percent < nPercent || nPercent == 0) {
				nPercent = percent;
			}
		}
	}

	return nPercent;
}

int	 CLiveSession::getBandWidth()
{
	GKCAutoLock Lock(&mCriticalSrc);
	return  mMediaIO->GetBandWidth();
}

int	 CLiveSession::getCurrentSeqNum() 
{
	TTInt nSeqNum;
	GKCAutoLock Lock(&mCriticalSrc);
	nSeqNum = mCurSeq;
	return nSeqNum;
}

int	 CLiveSession::getBufferStatus(bool isAudio, TTInt64 *startTime, TTInt* nBufferDuration)
{
	TTBufferManager* source = NULL;	

	GKCAutoLock Lock(&mCriticalSrc);
	if(mMediaType == 0) {
		if(mTSParser == NULL)
			return TTKErrNotReady;
		
		if(mTSParser->isHeadReady() == 0) {
			return TTKErrNotReady;
		}

		TTInt nNum = mTSParser->getProgramStreamNum(1);
		TTInt hasFound = 0;
		for(TTInt n = 0; n < nNum; n++) {
			source = mTSParser->getStreamSource(1, n);
			if(source && isAudio == source->isAudio()) {
				hasFound = 1;
				break;
			}
		}

		if(hasFound == 0) {
			return TTKErrNotFound;
		}
	} else if (mMediaType == 1) {
		source = mPackedAudio->getStreamSource();
		if(source == NULL ||  isAudio != source->isAudio()) {
			return TTKErrNotFound;
		} 
	}
	
	TTInt nEOS = 0;

	if(startTime) {
		nEOS = source->nextBufferTime(startTime);
		if(nEOS < 0) {
			return TTKErrNotReady;
		}
	}

	if(nBufferDuration) {
		*nBufferDuration = source->getBufferedDurationUs(&nEOS);
	}

	if(nEOS) {
		return TTKErrEof;
	}

	return TTKErrNone;
}

int	 CLiveSession::bufferedPercent(TTInt& aBufferedPercent)
{
	aBufferedPercent = mPlayListManager->getPercentFromSeqNum(&mListItem, mCurSeq);

	return TTKErrNone;
}

int	 CLiveSession::getNextSegment(int nSeqNum, SegmentItem *pSegment)
{
	TTInt nErr = mPlayListManager->getSegmentItemBySeqNumFromItem(&mListItem, nSeqNum, pSegment);
	if(nErr > 0 && mPlayListManager->isLive()) {
		mCriticalSrc.Lock();
		if(mTSParser) {
			mTSParser->signalDiscontinuity(ATSParser::DISCONTINUITY_SEGMENT_SKIP, 0);
		}
		mCriticalSrc.UnLock();
		nErr = mPlayListManager->getSegmentItemBySeqNumFromItem(&mListItem, nErr, pSegment);
	}

	return nErr;
}

int	 CLiveSession::getProgramNum()
{
	TTInt nNum = 0;
	GKCAutoLock Lock(&mCriticalSrc);
	if(mMediaType == 0) {
		if(mTSParser) {
			nNum = mTSParser->getProgramNum();
		}
	} else if(mMediaType == 1){
		if(mPackedAudio) {
			nNum = 1;
		}
	}

	return nNum;	
}

int	 CLiveSession::getCurChunkPercent()
{
	GKCAutoLock Lock(&mCriticalSrc);
	TTInt nContentSize = mMediaIO->ContentLength();
	TTInt nOffset = mMediaIO->GetOffset();

	if(nContentSize == 0) {
		return 100;
	}

	return nOffset*100/nContentSize;
}

int	 CLiveSession::getProgramStreamNum(int nProgram)
{
	TTInt nNum = 0;
	GKCAutoLock Lock(&mCriticalSrc);
	if(mMediaType == 0) {
		if(mTSParser) {
			nNum = mTSParser->getProgramStreamNum(nProgram);
		}
	} else if(mMediaType == 1){
		if(mPackedAudio) {
			nNum = 1;
		}
	}

	return nNum;	
}

TTBufferManager* CLiveSession::getStreamSource(int nProgram, int nStream)
{
	TTInt nNum = 0;
	GKCAutoLock Lock(&mCriticalSrc);
	if(mMediaType == 0) {
		if(mTSParser) {
			return mTSParser->getStreamSource(nProgram, nStream);
		}
	} else if(mMediaType == 1){
		if(mPackedAudio) {
			return mPackedAudio->getStreamSource();
		}
	}

	return NULL;	
}

TTInt	 CLiveSession::onMediaPrepared(TTInt nVar1, TTInt nVar2, void* nVar3)
{
	TTBufferManager* source = NULL;	
	TTInt nNum = 0;
	TTInt nBufferOverflow = 0;
	TTInt final = 0;
	if(mMediaType == 0) {
		GKCAutoLock Lock(&mCriticalSrc);
		if(mTSParser) {
			nNum = mTSParser->getProgramStreamNum(1);
			for(TTInt n = 0; n < nNum; n++) {
				source = mTSParser->getStreamSource(1, n);
				if(source && (source->getBufferCount() >= 600 || source->getBufferedDurationUs(&final) > 24000)) {
					TTInt nCount = source->getBufferCount();
					nBufferOverflow++;
				}
			}

			if(nBufferOverflow >= nNum) {
				GKCAutoLock Lock(&mCriticalEvent);
				postMediaMsgEvent(2000, EMediaDownLoadPrepared, nVar1, nVar2, nVar3);
				return TTKErrNone;								
			}
		}
	} else if(mMediaType == 1){
		GKCAutoLock Lock(&mCriticalSrc);
		if(mPackedAudio) {
			source = mPackedAudio->getStreamSource();
			if(source && (source->getBufferCount() >= 600 || source->getBufferedDurationUs(&final) > 24000)) {
				GKCAutoLock Lock(&mCriticalEvent);
				postMediaMsgEvent(2000, EMediaDownLoadPrepared, nVar1, nVar2, nVar3);
				return TTKErrNone;
			}
		}
	}

	if(nVar1 > 0) {
		GKCAutoLock Lock(&mCriticalSrc);
		if(mTSParser) {
			mTSParser->signalDiscontinuity(ATSParser::DISCONTINUITY_SEGMENT_SKIP, 0);
		}

		if(mPackedAudio) {
			mPackedAudio->signalDiscontinuity(0, 0);
		}
	}
	
	SegmentItem* segment = (SegmentItem*)nVar3;
	SegmentItem	tSegmentItem;
	memset(&tSegmentItem, 0, sizeof(SegmentItem));
	TTInt nErr = getNextSegment(segment->nSeqNum + 1, &tSegmentItem);
	if(nErr == 0) {
		memcpy(&mSegmentItem, &tSegmentItem, sizeof(SegmentItem));
		if(mSegmentItem.nDisflag) {
			GKCAutoLock Lock(&mCriticalSrc);
			if(mTSParser) {
				mTSParser->signalDiscontinuity(ATSParser::DISCONTINUITY_NEW_PROGRAM_FLAG, 0);
			}
		}
		GKCAutoLock Lock(&mCriticalEvent);
		postMediaMsgEvent(0, EMediaDownLoadStart, 0, 0, (void *)&mSegmentItem);
	} else if(nErr == TTKErrEof){
		GKCAutoLock Lock(&mCriticalSrc);
		if(mTSParser) {
			mTSParser->signalEOS(1);
		}	
	} else {
		GKCAutoLock Lock(&mCriticalEvent);
		postMediaMsgEvent(200, EMediaDownLoadPrepared, nVar1, nVar2, nVar3);
	}

	return TTKErrNone;	
}

TTInt	 CLiveSession::onMediaSeekToSeqNum(TTInt nVar1, TTInt nVar2, void* nVar3)
{
	GKCAutoLock Lock(&mCriticalSrc);
	TTInt nErr = TTKErrNone;
	TTInt nNum = getProgramStreamNum(1);
	TTBufferManager* source = NULL;
	for(TTInt n = 0; n < nNum; n++) {
		source = getStreamSource(1, n);
		if(source) {
			source->clear(false);
		}
	}

	if(mTSParser) {
		mTSParser->signalDiscontinuity(ATSParser::DISCONTINUITY_SEGMENT_SKIP, 0);
	}

	SegmentItem	tSegmentItem;
	memset(&tSegmentItem, 0, sizeof(SegmentItem));
	nErr = getNextSegment(nVar1, &tSegmentItem);
	if(nErr < 0) {
		GKCAutoLock Lock(&mCriticalEvent);
		postMediaMsgEvent(0, EMediaDownLoadPrepared, 1, 0, (void *)nVar3);
		return nErr;
	}
	
	memcpy(&mSegmentItem, &tSegmentItem, sizeof(SegmentItem));
	if(!mCancel) {
		GKCAutoLock Lock(&mCriticalEvent);
		postMediaMsgEvent(0, EMediaDownLoadStart, 0, 0, (void *)&mSegmentItem);
	}

	return TTKErrNone;	
}

TTInt	 CLiveSession::onMediaStart(TTInt nVar1, TTInt nVar2, void* nVar3)
{
	SegmentItem* segment = (SegmentItem*)nVar3;
	
	mOffset = 0;
	mContentSize = 0;
	mReconNum = 0;

	TTInt nErr = mMediaIO->Open(segment->url);
	if(nErr < 0) {
		nErr = mMediaIO->Open(segment->url);
		if(nErr <  0) {
			mChunkRecNum++;
			if(mChunkRecNum > 10 && mObserver) {
				TTUint nHostIP = mMediaIO->GetHostIP();
				char *pParam3 = inet_ntoa(*(struct in_addr*)&nHostIP);
				TTInt nStatusCode = mMediaIO->GetStatusCode();
				mObserver->DownLoadException(TTKErrDisconnected, nStatusCode, pParam3);
				return TTKErrAccessDenied;
			}
			GKCAutoLock Lock(&mCriticalEvent);
			postMediaMsgEvent(0, EMediaDownLoadPrepared, 1, 0, nVar3);
			return nErr;
		}
	}

	mCriticalSrc.Lock();
	mContentSize = mMediaIO->ContentLength();
	mCurSeq = segment->nSeqNum;
	mCriticalSrc.UnLock();

	mChunkRecNum = 0;
	GKCAutoLock Lock(&mCriticalEvent);
	postMediaMsgEvent(0, EMediaDownLoadContinue, 0, 0, nVar3);

	return TTKErrNone;	
}
	
TTInt	 CLiveSession::onMediaContinue(TTInt nVar1, TTInt nVar2, void* nVar3)
{
	mCritical.Lock();
	bool bPause = mPause;
	mCritical.UnLock();

	char* pBuffer = mBuffer + mOffset;
	int   nLength = 4096 - mOffset;
	if(bPause) {
		if(nLength > 8) {
			nLength = 8;
		}
	}
	TTInt nSize = mMediaIO->Read(pBuffer, nLength);	
	if(nSize == TTKErrAccessDenied) {
		mMediaIO->Close();
		GKCAutoLock Lock(&mCriticalEvent);
		postMediaMsgEvent(0, EMediaDownLoadPrepared, 0, 0, nVar3);
		return nSize;
	}

	if(nSize < 0) {
		GKCAutoLock Lock(&mCriticalEvent);
		postMediaMsgEvent(0, EMediaNetReConnect, mReconNum, 0, nVar3);
		return nSize;
	}

	updateBuffer(nSize);
	mReconNum = 0;

	if(mMediaIO->IsEnd()) {
		mMediaIO->Close();
		GKCAutoLock Lock(&mCriticalEvent);
		postMediaMsgEvent(0, EMediaDownLoadPrepared, 0, 0, nVar3);
	} else {
		GKCAutoLock Lock(&mCriticalEvent);
		TTInt nDelayTime = 0;
		if(nSize == 0 || bPause) {
			nDelayTime = 10;
		}
		postMediaMsgEvent(nDelayTime, EMediaDownLoadContinue, 1, 0, nVar3);
	}

	return TTKErrNone;	
}
	
TTInt	 CLiveSession::onMediaReconnect(TTInt nVar1, TTInt nVar2, void* nVar3)
{
	if(mPlayListManager->isLive()) {
		if(mReconNum >= 3) {
			mReconNum = 0;
			GKCAutoLock Lock(&mCriticalEvent);
			postMediaMsgEvent(0, EMediaDownLoadPrepared, 1, 0, nVar3);
			return TTKErrNone; 
		}
	} else {
		if(mReconNum > 20) {
			mReconNum = 0;
			GKCAutoLock Lock(&mCriticalEvent);
			postMediaMsgEvent(0, EMediaDownLoadPrepared, 1, 0, nVar3);
			return TTKErrNone; 
		}
	}

	TTInt nErr = mMediaIO->ReOpen(mMediaIO->GetOffset());
	if(nErr == 0) {
		mReconNum = 0;
		GKCAutoLock Lock(&mCriticalEvent);
		postMediaMsgEvent(0, EMediaDownLoadContinue, 0, 0, nVar3);
	} else {
		mReconNum++;
		GKCAutoLock Lock(&mCriticalEvent);
		postMediaMsgEvent(0, EMediaNetReConnect, mReconNum, 0, nVar3);
	}

	return TTKErrNone;	
}

TTInt	 CLiveSession::onMediaHandle(TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3)
{
	TTInt nErr = TTKErrNone;

	if(mCancel) {
		return TTKErrNone;
	}

	switch(nMsg) {
		case EMediaDownLoadPrepared:
			{
				nErr = onMediaPrepared(nVar1, nVar2, nVar3);
			}
			break;
		case EMediaDownLoadStart:
			{
				nErr = onMediaStart(nVar1, nVar2, nVar3);
			}
			break;
		case EMediaDownLoadContinue:
			{
				nErr = onMediaContinue(nVar1, nVar2, nVar3);
			}
			break;
		case EMediaNetReConnect:
			{
				nErr = onMediaReconnect(nVar1, nVar2, nVar3);
			}			
			break;
		case EMediaSeekToSeqNum:
			{
				nErr = onMediaSeekToSeqNum(nVar1, nVar2, nVar3);
			}
			break;
	}

	return nErr;
}

int	 CLiveSession::updateBuffer(TTInt nSize)
{
	if(nSize == 0)
		return 0;

	GKCAutoLock Lock(&mCriticalSrc);
	if(mDownLoadSize == 0) {
		if(mBuffer[0] == 0x47) {
			if (mTSParser == NULL) {
				mTSParser = new ATSParser(ATSParser::TS_TIMESTAMPS_ARE_ABSOLUTE);
			}

			mMediaType = 0;
		} else {
			if (mPackedAudio == NULL) {
				mPackedAudio = new APackedAudioParser(0);
			}
			mMediaType = 1;
		}
	}

	TTInt nBufferSize = nSize + mOffset;
	TTInt nErr = TTKErrNone;

	if(mMediaType == 0) {
		TTInt offset = 0;
		while (offset + 188 <= nBufferSize) {
			TTInt err = mTSParser->feedTSPacket((unsigned char *)(mBuffer + offset), 188);

			offset += 188;
		}

		mOffset = nBufferSize - offset;
		memcpy(mBuffer, mBuffer + offset, mOffset);
	} else if(mMediaType == 1) {
		mPackedAudio->feedAudioPacket(mBuffer, nBufferSize);
		mOffset = 0;
	}

	mDownLoadSize += nSize;

	return TTKErrNone;
}

TTInt	 CLiveSession::postMediaMsgEvent(TTInt  nDelayTime, TTInt32 nMsg, int nParam1, int nParam2, void * pParam3)
{
	if (mMediaThread == NULL)
		return TTKErrNotFound;

	TTBaseEventItem * pEvent = mMediaThread->getEventByType(EEventStream);
	if (pEvent == NULL)
		pEvent = new TTCSessionEvent (this, &CLiveSession::onMediaHandle, EEventStream, nMsg, nParam1, nParam2, pParam3);
	else
		pEvent->setEventMsg(nMsg, nParam1, nParam2, pParam3);
	mMediaThread->postEventWithDelayTime (pEvent, nDelayTime);

	return TTKErrNone;
}
