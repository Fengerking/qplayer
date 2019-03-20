/**
* File : TTBufferManager.h 
* Created on : 2015-5-12
* Author : yongping.lin
* Description : TTBufferManager定义文件
*/

#ifndef __TTBUFFER_MANAGER_H_
#define __TTBUFFER_MANAGER_H_

#include "TTList.h"
#include "TTMediadef.h"
#include "TTMediainfoDef.h"
#include "GKCritical.h"

class TTBufferManager {
public:
	enum {
        MODE_RESERVED             = 0x00,
        MODE_MPEG1_VIDEO          = 0x01,
        MODE_MPEG2_VIDEO          = 0x02,
        MODE_MPEG1_AUDIO          = 0x03,
        MODE_MPEG2_AUDIO          = 0x04,
        MODE_MPEG2_AUDIO_ADTS     = 0x0f,
        MODE_MPEG4_VIDEO          = 0x10,
        MODE_H264                 = 0x1b,
		MODE_HEVC                 = 0x24,

        // From ATSC A/53 Part 3:2009, 6.7.1
        MODE_AC3                  = 0x81,

        // Stream type 0x83 is non-standard,
        // it could be LPCM or TrueHD AC3
        MODE_LPCM_AC3             = 0x83,
    };

    TTBufferManager(int aMode, unsigned int aPID);
	virtual ~TTBufferManager();

	int queueAccessUnit(TTBuffer*  buffer);

    int  dequeueAccessUnit(TTBuffer* buffer);

	void clear(bool clearFormat = true);

    bool hasBufferAvailable(int *finalResult);

    TTInt64 getBufferedDurationUs(int *finalResult);

    TTInt64 getEstimatedDurationUs();

	TTInt64 seek(TTInt64 timeUs);

	List<TTBuffer *>::iterator getNearKeyFrame(TTInt64 timeUs);

    int nextBufferTime(TTInt64 *timeUs);
	int getBufferCount();

	int setStartTime(TTInt64 nTimeUs, int aOption);

	int getPID() {return mPID; };

	void signalEOS(int result);

    bool isFinished(TTInt64 duration) const;

	void freeBuffer(TTBuffer* buffer);

	bool isAudio();
	bool isVideo();

private:
    int  mMode;
	unsigned int mPID;
	int	 mEOSResult;
    RGKCritical mLock;
	TTInt64 mLastQueuedTimeUs;

	TTInt64 mStartTimeUs;
	TTInt	mOption;

	TTBuffer*  mCurBuffers;
	List<TTBuffer*> mBuffers;

	TTInt64 getBufferedDurationUs_l(int *finalResult);
	
};


#endif  // __TTBUFFER_MANAGER_H_
