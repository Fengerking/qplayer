#ifndef _TT_M3U_PARSER_H_
#define _TT_M3U_PARSER_H_

#include"TTString.h"
#include"GKArray.h"
#define HLS_MAX_URL_SIZE 4096
#define HLS_MAX_NAME_SIZE 64

typedef struct SegmentItem {
	char url[HLS_MAX_URL_SIZE];
	int  nDuration;
	int	 nSeqNum;
	int	 nDisseqNum;
	int  nDisflag;
	int	 nFlags;
	void*	pData;
	int  nReserved1;
	void*  nReserved2;
}SegmentItem;

typedef  struct VariantItem {
	char url[HLS_MAX_URL_SIZE];
	int  nBandwidth;
	int	 nMediaAudioNum;
	int  nMediaVideoNum;
	int  nMediaSubtileNum;
	int	 nFlags;
	void*pData;
	int  nReserved1;
	void*  nReserved2;
}VariantItem;

typedef  struct MediaItem {
	char url[HLS_MAX_URL_SIZE];	
	char mName[HLS_MAX_NAME_SIZE];
	char mLanguage[HLS_MAX_NAME_SIZE];	
	int nStream;
	int nType;
	int nFlags;
	void* pData;
	int  nReserved1;
	void*  nReserved2;
}MediaItem;

class M3UParser
{
public:
	M3UParser(const char *baseURI, const void *data, unsigned int size);
	virtual ~M3UParser();

	int initCheck() const;
	bool isExtM3U() const;
	bool isVariantPlaylist() const;
	bool isComplete() const;
	bool isEvent() const;

	int parse(const void *_data, TTUint32 size);
	unsigned int getDiscontinuitySeq() const;
	unsigned int getTargetDuration() const;
	unsigned int getTotalDuration() const;

	unsigned int  getVariantNum() const;
	VariantItem*  getVariantItem(int nIndex) const;

	int  getSequenceNum() const;			  
	unsigned int  getSegmentNum() const;
	SegmentItem*  getSegmentItem(int nIndex) const;

	MediaItem*  getMediaAudioItem(int nStream, int nIndex);
	MediaItem*  getMediaVideoItem(int nStream, int nIndex);
	MediaItem*  getMediaSubtitleItem(int nStream, int nIndex);

protected:
	void	addVariantItem(VariantItem* pItem);

private:
	bool mIsExtM3U;
	bool mIsVariantPlayList;
	bool mIsSegmentPlayList;
	bool mIsComplete;
	bool mIsEvent;
	int  mTargetDuration;
	int	 mDiscontinuitySeq;
	int	 mSequenceNum;
	int  mVariantNum;
	int  mSegenmtNum;
	TTInt64 mDuration;
	TTString mBaseURI;
	int mInitCheck;

	RGKPointerArray<VariantItem>		mVec;
	RGKPointerArray<SegmentItem>		mSec;


	static	int	parseMetaData(const TTString &line, TTInt32 *meta);
	static  unsigned int parseStreamInf(const TTString &line, unsigned long *bandwith);
	static  int parseMetaDataDuration(const TTString &line, TTInt64 *duration);
	static  int ParseDouble(const char *s, double *x);
	static  int ParseInt32(const char *s, TTInt32 *x);
};

#endif  // _TT_M3U_PARSER_H_
