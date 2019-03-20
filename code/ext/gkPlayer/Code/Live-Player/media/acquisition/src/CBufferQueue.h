#ifndef __TTBUFFER_QUEUE_H_
#define __TTBUFFER_QUEUE_H_

#include "TTList.h"
#include "TTMediadef.h"
#include "GKCritical.h"
#include "CGKBuffer.h"

class CBufferQueue {
public:

    CBufferQueue(int mode, int size);
	virtual ~CBufferQueue();

	int queue(CGKBuffer*  buffer);

    CGKBuffer* dequeue();

	void clear();

	void clearForMix();

	int getBufferCount();

	void setAVQ(CBufferQueue* videoQ,CBufferQueue* audioQ);

	void freeBuffer(CGKBuffer* buffer);

	CGKBuffer* getBuffer();
	CGKBuffer* eraseBuffer(TTInt64 pts, int type);

private:
	int	 mEOSResult;
	int  mMode;
	int  mPreAllocSize;
	TTInt mBuffCnt; 

	TTBool mKeyset;
	CBufferQueue* mvideoQ;
	CBufferQueue* maudioQ;

    RGKCritical mLock;

	TTInt64 mStartTimeUs;

	List<CGKBuffer*> mBuffers;
 
};


#endif  // __TTBUFFER_MANAGER_H_
