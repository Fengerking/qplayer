/**
* File : TTPlaylistManager.h 
* Created on : 2015-5-12
* Author : yongping.lin
* Description : PlaylistManagerþ
*/

#ifndef _TT_PLAYLIST_MANAGER_H_
#define _TT_PLAYLIST_MANAGER_H_

#include "GKTypedef.h"
#include "TTMediainfoDef.h"
#include "TTM3UParser.h"
#include "TTList.h"
#include "TTIOClient.h"
#include "GKCritical.h"

enum TTPlayListType
{
	EMasterlist = 0
	, EMediaList = 1
	, EXMediaRenditionList = 2
	, EXMediaAudioList = 3
	, EXMediaSubtitleList = 4
};

typedef  struct ListItem {
	char	cUrl[HLS_MAX_URL_SIZE];
	int		nType;		
	int		nBitrateIndex;
	int		nXMediaIndex;
	int		nFlags;	
	M3UParser* pM3UParser;
	int		nReserved1;
	int		nReserved2;
}ListItem;

class PlaylistManager {
public:
    PlaylistManager();
	virtual ~PlaylistManager();
	int open(const char* aUrl, unsigned char* aBuffer, unsigned int aSize);
	int stop();
	ListItem* getListItem(int nIndex, int nType, int nXMediaIndex);
	int addPlayList(ListItem *baseURI, unsigned char* aBuffer, unsigned int aSize);	
	int initIndex(ListItem *pItem, TTInt64 utime);
	int initSeqNum(ListItem *pItem, TTInt64 utime);
	int getCurBitrateIndex();
	int setCurBitrateIndex(int nIndex);
	int getPercentFromSeqNum(ListItem* pItem, int seqNum);
	int estimateSeqNumFromSeqNum(ListItem* pNewItem, ListItem* pOldItem, TTInt seqLastNum, int nPercent);
	int getTargetDuration() const;
	int getTotalDuration() const;
	int getCurMediaAudioItemIndex(int nBitrate);
	int getCurMediaVideoItemIndex(int nBitrate);
	int getCurMediaSubtitleItemIndex(int nBitrate);
	int setCurMediaAudioItemIndex(int nBitrate, int nIndex);
	int setCurMediaVideoItemIndex(int nBitrate, int nIndex);
	int setCurMediaSubtitleItemIndex(int nBitrate, int nIndex);

	

	int switchUp(int nBitrate);
	int switchDown(int nBitrate);
	int checkBitrate(int nBitrate);

	bool isVariantPlaylist() const;
    bool isComplete(ListItem *pItem);
    bool isEvent() const;
	bool isLive() const;
	
	int  getVariantNum() const;
	VariantItem*  getVariantItem(int nIndex) const;
	MediaItem*  getMediaAudioItem(int nStream, int nIndex);
	MediaItem*  getMediaVideoItem(int nStream, int nIndex);
	MediaItem*  getMediaSubtitleItem(int nStream, int nIndex);

	unsigned int  getSegmentNum(int nStream);
	int  getSegmentItemByIndex(int nStream, int nIndex, SegmentItem* sItem);
	int  getSegmentItemBySeqNum(int nStream, int seqNum, SegmentItem* sItem);	
	int  getSeqNumberWithAnchorTime(int nStream, TTInt64 anchorTimeUs);
    int  getSeqNumberForDiscontinuity(int nStream, unsigned int discontinuitySeq);
    int  getSeqNumberForTime(int nStream, TTInt64 timeUs);

	unsigned int  getSegmentNumFromItem(ListItem* pItem);
	int  getSegmentItemByIndexFromItem(ListItem* pItem, int nIndex, SegmentItem* sItem);
	int  getSegmentItemBySeqNumFromItem(ListItem* pItem, int seqNum, SegmentItem* sItem);	
	int  getSeqNumberWithAnchorTimeFromItem(ListItem* pItem, TTInt64 anchorTimeUs);
    int  getSeqNumberForDiscontinuityFromItem(ListItem* pItem, unsigned int discontinuitySeq);
    int  getSeqNumberForTimeFromItem(ListItem* pItem, TTInt64* timeUs);
	int  getIndexForTimeFromItem(ListItem* pItem, TTInt64* timeUs);

protected:
	M3UParser*  getM3UParser(ListItem* pItem);

private:
	M3UParser*	mMasterM3UParser;
	M3UParser*	mMediaM3UParser;

	TTInt		mTotalDuraiton;
	TTInt       mTagetDuration;
	TTInt		mCurBitrateIndex;

	bool mEvent;
    bool mLive;

	List<ListItem *>	mListUrlItem;
	RGKCritical			mCritical;
};


#endif  // _TT_PLAYLIST_MANAGER_H_

