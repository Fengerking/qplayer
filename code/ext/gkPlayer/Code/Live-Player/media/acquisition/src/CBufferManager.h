/**
* File : CBufferManager.h 
* Created on : 2015-5-12
* Author : yongping.lin
* Copyright : Copyright (c) 2016 GoKu Software Ltd. All rights reserved.
* Description : CBufferManager定义文件
*/

#ifndef __TTBUFFER_MANAGER_H_
#define __TTBUFFER_MANAGER_H_

#include "TTList.h"
#include "TTMediadef.h"
#include "GKCritical.h"
#include "CGKBuffer.h"
#include "CBufferQueue.h"

class CBufferManager {
public:

    CBufferManager();
	virtual ~CBufferManager();

	CGKBuffer*  dequeue(int mode);
    int  queue(CGKBuffer* buffer, int mode);

	void clear();
	void release();

	CGKBuffer* getBuffer();
	void eraseBuffer(TTInt64 pts, int type);
    
    int  getBufferCount();

    int nextBufferTime(TTInt64 *timeUs);

private:

	int	 mEOSResult;
    RGKCritical mLock;
	CBufferQueue* m_AudioQ;
	CBufferQueue* m_VideoQ;
	CBufferQueue* m_MainQ;

	CGKBuffer*  mCurBuffers;
	List<CGKBuffer*> mBuffers;
	
};


#endif  // __TTBUFFER_MANAGER_H_
