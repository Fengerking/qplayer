#ifndef _TT_STREAM_QUEUE_H_
#define _TT_STREAM_QUEUE_H_

#include "TTList.h"
#include "TTMediadef.h"
#include "TTMediainfoDef.h"

class TTStreamQueue {
public:
    enum Mode {
        H264,
        AAC,
        AC3,
        MPEG_AUDIO,
        MPEG_VIDEO,
        MPEG4_VIDEO,
        PCM_AUDIO,
		HEVC,
    };

    TTStreamQueue(Mode mode, unsigned int aPID, unsigned int flags = 0);
	virtual ~TTStreamQueue();

    int appendData(unsigned char *data, unsigned int size, TTInt64 timeUs, unsigned int flags);
    void clear(bool clearFormat = false);

    TTBuffer* dequeueAccessUnit();

private:
    struct RangeInfo {
        TTInt64 mTimestampUs;
        unsigned int mLength;
		unsigned int mFlags;
    };

    Mode mMode;
    unsigned int mFlags;
	unsigned int mPID;

	unsigned int    mSize;       
    unsigned char*	mBuffer;  
	unsigned int    mCapacity;   
	unsigned int	mSampleRate;
	unsigned int	mNumFrameSample;
	unsigned int	mWidth;
	unsigned int	mHeight;
	unsigned int	mNumRef;
	List<RangeInfo> mRangeInfos;

    TTAudioInfo* mAudioInfo;
	TTVideoInfo* mVideoInfo;
	
    TTBuffer* dequeueAccessUnitH264();
	TTBuffer* dequeueAccessUnitHEVC();
    TTBuffer* dequeueAccessUnitAAC();
    TTBuffer* dequeueAccessUnitMPEGAudio();

    // consume a logical (compressed) access unit of size "size",
    // returns its timestamp in us (or -1 if no time information).
    TTInt64 fetchTimestamp(unsigned int size);
    TTInt64 fetchTimestampAudio(unsigned int size);
	TTInt64 fetchTimestampMeta(unsigned int size);
};

#endif  // _TT_STREAM_QUEUE_H_
