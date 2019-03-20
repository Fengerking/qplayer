/**
* File : TTPlaylistManager.cpp
* Created on : 2015-5-12
* Author : yongping.lin
* Description : PlaylistManagerþ
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "TTPlaylistManager.h"
#include "TTLog.h"

PlaylistManager::PlaylistManager()
:mMasterM3UParser(NULL),
 mMediaM3UParser(NULL),
 mTagetDuration(0),
 mTotalDuraiton(0),
 mCurBitrateIndex(0),
 mEvent(false),
 mLive(false)
{
	mCritical.Create();
}

PlaylistManager::~PlaylistManager()
{
	stop();
	mCritical.Destroy();
}

int  PlaylistManager::open(const char* aUrl, unsigned char* aBuffer, unsigned int aSize)
{
	GKCAutoLock Lock(&mCritical);
	M3UParser* pM3u = new M3UParser(aUrl, aBuffer, aSize);	
	if(pM3u->initCheck() < 0) {
		delete pM3u;
		return TTKErrNotSupported;
	}

	ListItem* pListItem = new ListItem();
	memset(pListItem, 0, sizeof(ListItem));
	strcpy(pListItem->cUrl, aUrl);

	if(pM3u->isVariantPlaylist()) {
		mMasterM3UParser = pM3u;
		pListItem->nType = EMasterlist;
		TTInt nNum = pM3u->getVariantNum();
		mCurBitrateIndex = 0;
		if(mCurBitrateIndex < 0) {
			mCurBitrateIndex = 0;
		}
	} else {
		mMediaM3UParser = pM3u;
		pListItem->nType = EMediaList;
		mEvent = pM3u->isEvent();
		mTagetDuration = pM3u->getTargetDuration();
		mTotalDuraiton = pM3u->getTotalDuration();
		if(!mEvent && !pM3u->isComplete()) {
			mLive = true;
		} else {
			mLive = false;
		}
	}

	pListItem->pM3UParser = pM3u;
	mListUrlItem.push_back(pListItem);

	return TTKErrNone;
}

int PlaylistManager::initIndex(ListItem *pItem, TTInt64 utime)
{
	GKCAutoLock Lock(&mCritical);
	M3UParser*  pM3u = getM3UParser(pItem);
	if(pM3u == NULL) {
		return TTKErrNotFound;
	}

	TTInt nIndex = 0;

	if(!pM3u->isComplete() && !pM3u->isEvent()) {
		TTInt nNum = pM3u->getSegmentNum();
		nIndex = nNum - 3;
		if(nIndex < 0) nIndex = 0;
	}

	return nIndex;
}

int PlaylistManager::initSeqNum(ListItem *pItem, TTInt64 utime)
{
	GKCAutoLock Lock(&mCritical);
	M3UParser*  pM3u = getM3UParser(pItem);
	if(pM3u == NULL) {
		return TTKErrNotFound;
	}

	TTInt nIndex = 0;

	if(!pM3u->isComplete() && !pM3u->isEvent()) {
		TTInt nNum = pM3u->getSegmentNum();
		nIndex = nNum - 2;
		if(nIndex < 0) nIndex = 0;
	}

	TTInt seqNum = nIndex + pM3u->getSequenceNum();

	return seqNum;
}

int PlaylistManager::addPlayList(ListItem *pItem, unsigned char* aBuffer, unsigned int aSize)
{
	GKCAutoLock Lock(&mCritical);
	M3UParser* pM3u = new M3UParser(pItem->cUrl, aBuffer, aSize);
	
	if(pM3u->initCheck() < 0) {
		delete pM3u;
		return TTKErrNotSupported;
	}
	
	if(pItem->pM3UParser != NULL) {
		delete pItem->pM3UParser;
		pItem->pM3UParser = NULL;
	}

	pItem->pM3UParser = pM3u;

	mEvent = pM3u->isEvent();
	if(!mEvent && !pM3u->isComplete()) {
		mLive = true;
	} else {
		mLive = false;
	}

	mTagetDuration = pM3u->getTargetDuration();
	mTotalDuraiton = pM3u->getTotalDuration();

	return 0;
}

ListItem* PlaylistManager::getListItem(int nIndex, int nType, int nXMediaIndex)
{
	GKCAutoLock Lock(&mCritical);
	ListItem *pList = NULL;
	bool bFoundList = false;

	List<ListItem * >::iterator it = mListUrlItem.begin();
	while (it != mListUrlItem.end()) {
		pList = *it;
		if(pList->nType == nType && pList->nBitrateIndex == nIndex && pList->nXMediaIndex == nXMediaIndex) {
			bFoundList = true;
			break;
		}
		++it;
	}

	if(bFoundList) {
		return pList;
	}

	if(nType == EMasterlist || !isVariantPlaylist() || mMasterM3UParser == NULL) {
		return NULL;
	}

	pList = new ListItem();
	memset(pList, 0, sizeof(ListItem));
	
	if(nType == EMediaList) {
		if(nIndex < 0 || nIndex >= mMasterM3UParser->getVariantNum()) {
			delete pList;
			return NULL;
		}

		VariantItem* pVItem = mMasterM3UParser->getVariantItem(nIndex);
		if(pVItem == NULL) {
			delete pList;
			return NULL;
		}

		strcpy(pList->cUrl, pVItem->url);
		pList->nBitrateIndex = nIndex;
		pList->nType = EMediaList;
	} else if(nType == EXMediaRenditionList) {
		if(nIndex < 0 || nIndex >= mMasterM3UParser->getVariantNum()) {
			delete pList;
			return NULL;
		}

		VariantItem* pVItem = mMasterM3UParser->getVariantItem(nIndex);
		if(pVItem == NULL) {
			delete pList;
			return NULL;
		}

		if(nXMediaIndex < 0 || nXMediaIndex >= pVItem->nMediaVideoNum) {
			delete pList;
			return NULL;
		}

		MediaItem* pMItem = mMasterM3UParser->getMediaVideoItem(nIndex, nXMediaIndex);
		if(pMItem == NULL) {
			delete pList;
			return NULL;
		}

		strcpy(pList->cUrl, pMItem->url);
		pList->nBitrateIndex = nIndex;
		pList->nType = EXMediaRenditionList;
		pList->nXMediaIndex = nXMediaIndex;
	} else if(nType == EXMediaAudioList) {
		if(nIndex < 0 || nIndex >= mMasterM3UParser->getVariantNum()) {
			delete pList;
			return NULL;
		}

		VariantItem* pVItem = mMasterM3UParser->getVariantItem(nIndex);
		if(pVItem == NULL) {
			delete pList;
			return NULL;
		}

		if(nXMediaIndex < 0 || nXMediaIndex >= pVItem->nMediaVideoNum) {
			delete pList;
			return NULL;
		}

		MediaItem* pMItem = mMasterM3UParser->getMediaAudioItem(nIndex, nXMediaIndex);
		if(pMItem == NULL) {
			delete pList;
			return NULL;
		}

		strcpy(pList->cUrl, pMItem->url);
		pList->nBitrateIndex = nIndex;
		pList->nType = EXMediaAudioList;
		pList->nXMediaIndex = nXMediaIndex;
	} else if(nType == EXMediaSubtitleList) {
		if(nIndex < 0 || nIndex >= mMasterM3UParser->getVariantNum()) {
			delete pList;
			return NULL;
		}

		VariantItem* pVItem = mMasterM3UParser->getVariantItem(nIndex);
		if(pVItem == NULL) {
			delete pList;
			return NULL;
		}

		if(nXMediaIndex < 0 || nXMediaIndex >= pVItem->nMediaVideoNum) {
			delete pList;
			return NULL;
		}

		MediaItem* pMItem = mMasterM3UParser->getMediaSubtitleItem(nIndex, nXMediaIndex);
		if(pMItem == NULL) {
			delete pList;
			return NULL;
		}

		strcpy(pList->cUrl, pMItem->url);
		pList->nBitrateIndex = nIndex;
		pList->nType = EXMediaSubtitleList;
		pList->nXMediaIndex = nXMediaIndex;	
	} 

	mListUrlItem.push_back(pList);

	return pList;
}

int PlaylistManager::stop()
{
	GKCAutoLock Lock(&mCritical);
	List<ListItem * >::iterator it = mListUrlItem.begin();
	while (it != mListUrlItem.end()) {
		ListItem *pList = *it;
		delete pList->pM3UParser;
		delete pList;
		it = mListUrlItem.erase(it);
    }

	mMasterM3UParser = NULL;
	mMediaM3UParser = NULL;

	return 0;	
}

int PlaylistManager::setCurBitrateIndex(int nIndex)
{
	GKCAutoLock Lock(&mCritical);
	mCurBitrateIndex = nIndex;
	return 0;
}

int PlaylistManager::getCurBitrateIndex()
{
	GKCAutoLock Lock(&mCritical);
	int nIndex = mCurBitrateIndex;
	return nIndex;
}

int  PlaylistManager::getCurMediaAudioItemIndex(int nBitrate)
{
	return -1;
}
	
int  PlaylistManager::getCurMediaVideoItemIndex(int nBitrate)
{
	return -1;
}
	
int  PlaylistManager::getCurMediaSubtitleItemIndex(int nBitrate)
{
	return -1;
}

int  PlaylistManager::setCurMediaAudioItemIndex(int nBitrate, int nIndex)
{
	return 0;
}
	
int  PlaylistManager::setCurMediaVideoItemIndex(int nBitrate, int nIndex)
{
	return 0;
}
	
int  PlaylistManager::setCurMediaSubtitleItemIndex(int nBitrate, int nIndex)
{
	return 0;
}

int PlaylistManager::getTargetDuration() const
{
	return mTagetDuration;
}

int PlaylistManager::getTotalDuration() const
{
	if(isLive()) {
		return -1;
	}
	return mTotalDuraiton;
}

int PlaylistManager::switchUp(int nBitrate)
{
	GKCAutoLock Lock(&mCritical);
	if(mMasterM3UParser == NULL) {
		return mCurBitrateIndex;
	}

	TTInt nNum = mMasterM3UParser->getVariantNum();
	TTInt nAdapterBitrate = nBitrate*7/10;
	TTInt nAdapterIndex = mCurBitrateIndex;

	for(TTInt n = nNum - 1; n >= mCurBitrateIndex; n--) {
		VariantItem* pVariantItem = mMasterM3UParser->getVariantItem(n);
		nAdapterIndex = n;
		if(pVariantItem->nBandwidth < nAdapterBitrate) {
			break;
		}
	}

	return nAdapterIndex;
}

int PlaylistManager::checkBitrate(int nBitrate)
{
	GKCAutoLock Lock(&mCritical);
	if(mMasterM3UParser == NULL) {
		return mCurBitrateIndex;
	}

	TTInt nNum = mMasterM3UParser->getVariantNum();
	TTInt nAdapterBitrate = nBitrate;
	TTInt nAdapterIndex = 0;

	for(TTInt n = nNum - 1; n >= 0; n--) {
		VariantItem* pVariantItem = mMasterM3UParser->getVariantItem(n);
		nAdapterIndex = n;
		if(pVariantItem->nBandwidth < nAdapterBitrate) {
			break;
		}		
	}

	return nAdapterIndex;
}

int PlaylistManager::switchDown(int nBitrate)
{
	GKCAutoLock Lock(&mCritical);
	if(mMasterM3UParser == NULL) {
		return mCurBitrateIndex;
	}

	TTInt nNum = mMasterM3UParser->getVariantNum();
	TTInt nAdapterBitrate = nBitrate*8/10;
	TTInt nAdapterIndex = 0;

	for(TTInt n = mCurBitrateIndex; n >= 0; n--) {
		VariantItem* pVariantItem = mMasterM3UParser->getVariantItem(n);
		nAdapterIndex = n;
		if(pVariantItem->nBandwidth < nAdapterBitrate) {
			break;
		}		
	}

	return nAdapterIndex;
}

bool PlaylistManager::isVariantPlaylist() const
{
	if(mMasterM3UParser == NULL)
		return false;

	return mMasterM3UParser->isVariantPlaylist();
}

bool PlaylistManager::isComplete(ListItem *pItem)
{
	GKCAutoLock Lock(&mCritical);
	M3UParser* pParser = getM3UParser(pItem);
	if(pParser == NULL) {
		return false;
	}
	
	return pParser->isComplete();
}
    
bool PlaylistManager::isEvent() const
{
	return mEvent;
}

bool  PlaylistManager::isLive() const
{
	return mLive;
}

int  PlaylistManager::getVariantNum() const
{
	if(mMasterM3UParser == NULL) {
		return TTKErrNotFound;
	}

	return mMasterM3UParser->getVariantNum();
}
	
VariantItem*  PlaylistManager::getVariantItem(int nIndex) const
{
	if(mMasterM3UParser == NULL) {
		return NULL;
	}

	return mMasterM3UParser->getVariantItem(nIndex);
}
	
MediaItem*  PlaylistManager::getMediaAudioItem(int nStream, int nIndex)
{
	if(mMasterM3UParser == NULL) {
		return NULL;
	}

	return mMasterM3UParser->getMediaAudioItem(nStream, nIndex);
}
	
MediaItem*  PlaylistManager::getMediaVideoItem(int nStream, int nIndex)
{
	if(mMasterM3UParser == NULL) {
		return NULL;
	}

	return mMasterM3UParser->getMediaVideoItem(nStream, nIndex);
}
	
MediaItem*  PlaylistManager::getMediaSubtitleItem(int nStream, int nIndex)
{
	if(mMasterM3UParser == NULL) {
		return NULL;
	}

	return mMasterM3UParser->getMediaSubtitleItem(nStream, nIndex);
}

unsigned int  PlaylistManager::getSegmentNum(int nStream)
{
	ListItem iItem;
	memset(&iItem, 0, sizeof(ListItem));
	iItem.nType = EMediaList;
	iItem.nBitrateIndex = nStream;

	return getSegmentNumFromItem(&iItem);
}
	
int  PlaylistManager::getSegmentItemByIndex(int nStream, int nIndex, SegmentItem* sItem)
{
	ListItem iItem;
	memset(&iItem, 0, sizeof(ListItem));
	iItem.nType = EMediaList;
	iItem.nBitrateIndex = nStream;

	return getSegmentItemByIndexFromItem(&iItem, nIndex, sItem);
}	

int PlaylistManager::getSegmentItemBySeqNum(int nStream, int seqNum, SegmentItem* sItem)
{
	ListItem iItem;
	memset(&iItem, 0, sizeof(ListItem));
	iItem.nType = EMediaList;
	iItem.nBitrateIndex = nStream;

	return getSegmentItemBySeqNumFromItem(&iItem, seqNum, sItem);
}

int  PlaylistManager::estimateSeqNumFromSeqNum(ListItem* pNewItem, ListItem* pOldItem, TTInt seqLastNum, int nPercent)
{
	GKCAutoLock Lock(&mCritical);
	M3UParser* pParser = getM3UParser(pNewItem);
	if(pParser == NULL) {
		return TTKErrNotFound;
	}

	M3UParser* pLastParser = getM3UParser(pOldItem);
	if(pLastParser == NULL) {
		return TTKErrNotFound;
	}

	TTInt nSeqNum = pParser->getSequenceNum();
	TTInt nSegCount = pParser->getSegmentNum();
	TTInt nLastSeqNum = pLastParser->getSequenceNum();
	TTInt nLastSeqCount = pLastParser->getSegmentNum();
	TTInt nErr = TTKErrNotFound;

	TTInt nSegDuration = 0;
	TTInt n;
	SegmentItem* pSeg = NULL;

	seqLastNum = (seqLastNum < nLastSeqNum + nLastSeqCount) ? seqLastNum : (nLastSeqNum + nLastSeqCount);

	for(n = nLastSeqNum; n < seqLastNum; n++) {
		TTInt nIndex = n - nLastSeqNum;
		pSeg = pLastParser->getSegmentItem(nIndex);
		nSegDuration += pSeg->nDuration;
	}
	
	nSegDuration += pLastParser->getTargetDuration()*nPercent/100;

	TTInt nTargetDuration = 0;
	TTInt nTargetNum = nSeqNum;
	for(n = nSeqNum; n < nSeqNum + nSegCount; n++) {
		TTInt nIndex = n - nSeqNum;
		pSeg = pParser->getSegmentItem(nIndex);
		nTargetNum = n;	
		if(nTargetDuration + pSeg->nDuration >= nSegDuration) {			
			break;
		}
		nTargetDuration += pSeg->nDuration;
	}

	return nTargetNum;
}
	
int  PlaylistManager::getSeqNumberWithAnchorTime(int nStream, TTInt64 anchorTimeUs) 
{
	return 0;
}
    
int  PlaylistManager::getSeqNumberForDiscontinuity(int nStream, unsigned int discontinuitySeq) 
{
	return 0;
}
    
int  PlaylistManager::getSeqNumberForTime(int nStream, TTInt64 timeUs) 
{
	return 0;
}

unsigned int  PlaylistManager::getSegmentNumFromItem(ListItem* pItem)
{
	GKCAutoLock Lock(&mCritical);
	M3UParser* pParser = getM3UParser(pItem);
	if(pParser == NULL) {
		return TTKErrNotFound;
	}

	return pParser->getSegmentNum();
}

int  PlaylistManager::getSegmentItemByIndexFromItem(ListItem* pItem, int nIndex, SegmentItem* sItem)
{
	GKCAutoLock Lock(&mCritical);
	M3UParser* pParser = getM3UParser(pItem);
	if(pParser == NULL) {
		return TTKErrNotFound;
	}

	if(nIndex < 0) {
		if(isLive()) {
			return initIndex(pItem, 0);
		}
		return TTKErrNotFound;
	}

	if(nIndex >= pParser->getSegmentNum()) {
		if(pParser->isComplete()) {
			return TTKErrEof;
		} else {
			return TTKErrNotReady;
		}
	}

	SegmentItem* pSeg = pParser->getSegmentItem(nIndex);

	if(sItem && pSeg) {
		memcpy(sItem, pSeg, sizeof(SegmentItem));
	}

	return TTKErrNone;
}
	
int  PlaylistManager::getSegmentItemBySeqNumFromItem(ListItem* pItem, int seqNum, SegmentItem* sItem)
{
	GKCAutoLock Lock(&mCritical);
	M3UParser* pParser = getM3UParser(pItem);
	if(pParser == NULL) {
		return TTKErrNotFound;
	}

	TTInt nIndex = seqNum - pParser->getSequenceNum();
	if(nIndex < 0) {
		if(isLive()) {
			nIndex = initIndex(pItem, 0);
			//LOGI("PlaylistManager::getSegmentItemBySeqNumFromItem nIndex %d, nStartSeq %d, nCount %d", nIndex, pParser->getSequenceNum(), pParser->getSegmentNum());
			return nIndex + pParser->getSequenceNum();
		}
		return TTKErrNotFound;
	}

	if(nIndex >= pParser->getSegmentNum()) {
		//LOGI("PlaylistManager::getSegmentItemBySeqNumFromItem nIndex %d, nStartSeq %d, nCount %d", nIndex, pParser->getSequenceNum(), pParser->getSegmentNum());
		if(pParser->isComplete()) {
			return TTKErrEof;
		} else {
			return TTKErrNotReady;
		}
	}

	SegmentItem* pSeg = pParser->getSegmentItem(nIndex);

	if(sItem && pSeg) {
		memcpy(sItem, pSeg, sizeof(SegmentItem));
	}

	return TTKErrNone;
}
	
int  PlaylistManager::getSeqNumberWithAnchorTimeFromItem(ListItem* pItem, TTInt64 anchorTimeUs)
{
	return 0;
}
    
int  PlaylistManager::getSeqNumberForDiscontinuityFromItem(ListItem* pItem, unsigned int discontinuitySeq)
{
	return 0;
}
    
int  PlaylistManager::getSeqNumberForTimeFromItem(ListItem* pItem, TTInt64* timeUs)
{
	GKCAutoLock Lock(&mCritical);
	int nSeq = -1;

	M3UParser* pParser = getM3UParser(pItem);
	if(pParser == NULL) {
		return TTKErrNotFound;
	}

	int nNum = pParser->getSegmentNum();
	TTInt64 nDuration = 0;
	SegmentItem* seg;
	for(int n = 0; n < nNum; n++) {
		seg = pParser->getSegmentItem(n);
		if(*timeUs < nDuration + seg->nDuration) {
			*timeUs = nDuration;
			nSeq = seg->nSeqNum;
			break;
		}
		nDuration += seg->nDuration;
	}

	if(nSeq == -1) {
		nSeq = seg->nSeqNum;
        *timeUs = nDuration - seg->nDuration;
	}

	return nSeq;
}

int  PlaylistManager::getIndexForTimeFromItem(ListItem* pItem, TTInt64 *timeUs)
{
	GKCAutoLock Lock(&mCritical);
	int nIndex = -1;

	M3UParser* pParser = getM3UParser(pItem);
	if(pParser == NULL) {
		return TTKErrNotFound;
	}

	int nNum = pParser->getSegmentNum();
	TTInt64 nDuration = 0;
	SegmentItem* seg;
	for(int n = 0; n < nNum; n++) {
		seg = pParser->getSegmentItem(n);
		if(*timeUs < nDuration + seg->nDuration) {
			*timeUs = nDuration;
			nIndex = n;
			break;
		}
		nDuration += seg->nDuration;					
	}

	if(nIndex == -1) {
		nIndex = nNum - 1;
	}

	return nIndex;
}

int  PlaylistManager::getPercentFromSeqNum(ListItem* pItem, int seqNum)
{
	GKCAutoLock Lock(&mCritical);
	M3UParser* pParser = getM3UParser(pItem);
	if(pParser == NULL) {
		return 0;
	}

	int nNum = seqNum - pParser->getSequenceNum();
	if(nNum < 0 || pParser->getSegmentNum() == 0) {
		return 0;
	}

	return  nNum*100/pParser->getSegmentNum();
}

M3UParser*  PlaylistManager::getM3UParser(ListItem* pItem)
{
	GKCAutoLock Lock(&mCritical);
	ListItem *pList = NULL;
	bool bFoundList = false;

	if(pItem == NULL) {
		return NULL;
	}

	List<ListItem * >::iterator it = mListUrlItem.begin();
	while (it != mListUrlItem.end()) {
		pList = *it;
		if(pList->nType == pItem->nType && pList->nBitrateIndex == pItem->nBitrateIndex && pList->nXMediaIndex == pItem->nXMediaIndex) {
			bFoundList = true;
			break;
		}
		++it;
	}

	if(bFoundList) {
		return pList->pM3UParser;
	}

	return NULL;
}

