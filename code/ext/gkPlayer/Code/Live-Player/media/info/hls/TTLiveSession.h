/**
* File : TTLiveSession.h 
* Created on : 2015-5-12
* Author : yongping.lin
* Description : CLiveSessionþ
*/

#ifndef _TT_LIVE_SESSION_H_
#define _TT_LIVE_SESSION_H_

#include "GKTypedef.h"
#include "TTMediainfoDef.h"
#include "TTStreamBufferingItf.h"
#include "GKCritical.h"
#include "TTEventThread.h"
#include "TTSemaphore.h"
#include "TTTSParserProxy.h"
#include "TTPackedaudioParser.h"
#include "TTPlaylistManager.h"
#include "TTBufferManager.h"
#include "TTIOClient.h"
#include "TTList.h"


enum TTMediaMsg
{
	EMediaDownLoadPrepared = 0
	, EMediaDownLoadStart = 1
	, EMediaDownLoadContinue = 2
	, EMediaNetReConnect = 4
	, EMediaSeekToSeqNum = 5
};


class CLiveSession 
{
public:
	CLiveSession(ITTStreamBufferingObserver* aObserver, PlaylistManager* aPlayList);
	virtual ~CLiveSession();

	int	 setUrlListItem(ListItem* aListItem);
	
	int  start(TTInt nIndex);
	int  stop();
	int  pause();
	int  resume();

	int  bandPercent(int nDuration);
	int	 bufferedPercent(TTInt& aBufferedPercent);
	int	 setStartTime(TTInt64 aStartTime,TTInt aOption);
	TTInt64  seek(TTUint64 aPosMS,TTInt aOption, TTInt64 aFirstPTS);
	TTInt64  seekTime(TTUint64 aPosMS,TTInt aOption, TTInt64 aFirstPTS);
	TTInt64  seekSource(TTBufferManager* source, TTUint64 aPosMS, TTInt64 aFirstPTS);
	TTInt64  seektoTime(bool isAudio, TTInt64 aSeekPoint);
	
	void cancel();

	int	 isHeaderReady();
	int	 isBufferReady(TTInt nDuration, TTInt audioID, TTInt videoID);
	int	 GetMediaSample(TTMediaType aStreamType, TTBuffer* pMediaBuffer, TTInt elementID);
	int	 getProgramNum();
	int	 getBufferStatus(bool isAudio, TTInt64 *startTime, TTInt* nBufferDuration);
	int  getProgramStreamNum(int nProgram);
	int  getBandWidth();
	int  getCurrentSeqNum();
	int  getCurChunkPercent();
	TTBufferManager* getStreamSource(int nProgram, int nStream);

protected:
	TTInt	onMediaPrepared(TTInt nVar1, TTInt nVar2, void* nVar3);
	TTInt	onMediaStart(TTInt nVar1, TTInt nVar2, void* nVar3);
	TTInt	onMediaContinue(TTInt nVar1, TTInt nVar2, void* nVar3);
	TTInt	onMediaReconnect(TTInt nVar1, TTInt nVar2, void* nVar3);
	TTInt	onMediaSeekToSeqNum(TTInt nVar1, TTInt nVar2, void* nVar3);

	TTInt	onMediaHandle(TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3);

	int		getNextSegment(int nSeqNum, SegmentItem *nSegment);

	int		updateBuffer(TTInt nSize);

	void	clearBuffer();

	virtual TTInt postMediaMsgEvent(TTInt  nDelayTime, TTInt32 nMsg, int nParam1, int nParam2, void * pParam3);

private:
	ITTStreamBufferingObserver*	mObserver;
	TTEventThread*			mMediaThread;
	CTTIOClient*			mMediaIO;

	RGKCritical				mCritical;
	RGKCritical				mCriticalSrc;
	RGKCritical				mCriticalEvent;

	PlaylistManager*		mPlayListManager;
	
	
	ListItem				mListItem;
	SegmentItem				mSegmentItem;

	char					mBuffer[4*1024];
	TTInt					mOffset;
	TTInt					mCancel;

	TTInt					mContentSize;
	TTInt					mDownLoadSize;
	TTInt					mCurSeq;	

	TTInt					mReconNum;
	TTInt					mChunkRecNum;

	bool					mAudioSeek;
	bool					mVideoSeek;
	bool					mPause;

	TTInt					mMediaType;
	ATSParser*				mTSParser;
	APackedAudioParser*		mPackedAudio;
};

class TTCSessionEvent : public TTBaseEventItem
{
public:
    TTCSessionEvent(CLiveSession * pSession, TTInt (CLiveSession::* method)(TTInt, TTInt, TTInt, void*),
					    TTInt nType, TTInt nMsg = 0, TTInt nVar1 = 0, TTInt nVar2 = 0, void* nVar3 = 0)
		: TTBaseEventItem (nType, nMsg, nVar1, nVar2, nVar3)
	{
		mSession = pSession;
		mMethod = method;
    }

    virtual ~TTCSessionEvent()
	{
	}

    virtual void fire (void) 
	{
        (mSession->*mMethod)(mMsg, mVar1, mVar2, mVar3);
    }

protected:
    CLiveSession *		mSession;
    int (CLiveSession::* mMethod) (TTInt, TTInt, TTInt, void*);
};


#endif  // _TT_LIVE_SESSION_H_
