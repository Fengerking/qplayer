#include "CBufferQueue.h"
#include "TTLog.h"
#include "GKMacrodef.h"
#include "GKCollectCommon.h"
#include <string.h> 

#define AUDIO_PREALLOC_SIZE  512
#define VIDEO_PREALLOC_SIZE  (1024*20)
#define MAX_BUFFERING_SIZE   50

CBufferQueue::CBufferQueue(int mode, int size)
:mBuffCnt(0) 
,mMode(mode)
,mPreAllocSize(size)
,mvideoQ(NULL)
,maudioQ(NULL)
,mKeyset(ETTFalse)
,mStartTimeUs(-1)
{
	mLock.Create();
	CGKBuffer*  buffer;
	if (size > 0)
	{
		for(int i =0; i<size ;i++){
			buffer = (CGKBuffer*)malloc(sizeof(CGKBuffer));
			memset(buffer,0,sizeof(CGKBuffer));
			mBuffers.push_back(buffer);
			if(mMode == MODE_AUDIO){
				buffer->pBuffer = (TTPBYTE)malloc(AUDIO_PREALLOC_SIZE);
				buffer->nPreAllocSize = AUDIO_PREALLOC_SIZE;
				buffer->Tag = MODE_AUDIO;
			}
			else if(mMode == MODE_VIDEO){
				buffer->pBuffer = (TTPBYTE)malloc(VIDEO_PREALLOC_SIZE);
				buffer->nPreAllocSize = VIDEO_PREALLOC_SIZE;
				buffer->Tag = MODE_VIDEO;
			}
		}

		if(mMode != MODE_MIX)
			mBuffCnt = size;
	}
}


CBufferQueue::~CBufferQueue() 
{
	clear();
	mLock.Destroy();
}

CGKBuffer* CBufferQueue::dequeue()
{
	GKCAutoLock autoLock(&mLock);

	CGKBuffer* buffer;

	if (mMode == MODE_MIX)
		return NULL;

	if(mBuffers.empty()) {
		buffer = (CGKBuffer*)malloc(sizeof(CGKBuffer));
		if (buffer == NULL)
			return NULL;
		mBuffCnt++;
		memset(buffer,0,sizeof(CGKBuffer));
		if(mMode == MODE_AUDIO){
			buffer->pBuffer = (TTPBYTE)malloc(AUDIO_PREALLOC_SIZE);
			buffer->nPreAllocSize = AUDIO_PREALLOC_SIZE;
			buffer->Tag = MODE_AUDIO;
		}
		else if(mMode == MODE_VIDEO){
			buffer->pBuffer = (TTPBYTE)malloc(VIDEO_PREALLOC_SIZE);
			buffer->nPreAllocSize = VIDEO_PREALLOC_SIZE;
			buffer->Tag = MODE_VIDEO;
		}
		mBuffers.push_back(buffer);
	}

	mBuffCnt--;

	List<CGKBuffer *>::iterator it = mBuffers.begin();
	buffer = *it;
	mBuffers.erase(it);

	/*if(mMode == MODE_AUDIO)
		LOGE(" ---.Q  = %d",mBuffCnt)
	if(mMode == MODE_VIDEO)
		LOGE(" +++.Q  = %d",mBuffCnt)*/

	return buffer;
}

int CBufferQueue::queue(CGKBuffer*  buffer)
{
    int ret = 0;
	if(buffer == NULL)
		return -1;

	GKCAutoLock autoLock(&mLock);

	//±ê¼Ç¹Ø¼üÖ¡
	if ((mMode == MODE_MIX) && (buffer->Tag == MODE_VIDEO))
	{
		if (buffer->nFlag == TT_FLAG_BUFFER_KEYFRAME){
			mKeyset = ETTTrue;
		}
	}

	if (mMode == MODE_MIX){
		if (mKeyset)
			mBuffers.push_back(buffer);
		else
			return -1;

		mBuffCnt++;

		if (mBuffCnt >MAX_BUFFERING_SIZE){
			clearForMix();
			LOGE(" remove frame data end! m.Q  = %d",mBuffCnt)
			mKeyset = ETTFalse;
            ret = 1;
		}

		//LOGE(" m.Q  = %d",mBuffCnt)
	}
	else{
		mBuffCnt++;
		mBuffers.push_back(buffer);
	}

	return ret;
}


//only for MODE_MIX
CGKBuffer* CBufferQueue::getBuffer()
{
	GKCAutoLock autoLock(&mLock);

	if (mMode != MODE_MIX)
		return NULL;

	if(mBuffers.empty())
		return NULL;
	
	List<CGKBuffer *>::iterator it = mBuffers.begin();
	CGKBuffer* buffer = *it;

	return buffer;
}

CGKBuffer* CBufferQueue::eraseBuffer(TTInt64 pts, int type)
{
	GKCAutoLock autoLock(&mLock);

	if (mMode != MODE_MIX)
		return NULL;

	if(mBuffers.empty())
		return NULL;

	List<CGKBuffer *>::iterator it = mBuffers.begin();
	CGKBuffer* buffer = *it;

	if (buffer->llTime == pts && buffer->Tag == type){
		mBuffers.erase(it);
		mBuffCnt--;
		return buffer;
	}
	else
		return NULL;
}

void CBufferQueue::freeBuffer(CGKBuffer* buffer)
{
	if(buffer == NULL) {
		return;
	}

	if(buffer->pBuffer) {
		free(buffer->pBuffer);
		buffer->pBuffer = NULL;
	}

	delete buffer;
}

void CBufferQueue::clear() 
{
	GKCAutoLock autoLock(&mLock);

	List<CGKBuffer *>::iterator it = mBuffers.begin();
	while (it != mBuffers.end()) {
		CGKBuffer *buffer = *it;
		SAFE_FREE(buffer->pBuffer);
		freeBuffer(buffer);
		it = mBuffers.erase(it);
		mBuffCnt--;
	}
}

void CBufferQueue::setAVQ(CBufferQueue* videoQ,CBufferQueue* audioQ)
{
	maudioQ = audioQ;
	mvideoQ = videoQ;
}

void CBufferQueue::clearForMix()
{	
	if (mMode != MODE_MIX)
		return ;

    GKCAutoLock autoLock(&mLock);

	List<CGKBuffer *>::iterator it = mBuffers.begin();
	while (it != mBuffers.end()) {
		CGKBuffer *buffer = *it;
		it = mBuffers.erase(it);
		mBuffCnt--;
		if (buffer->Tag == MODE_AUDIO)
			maudioQ->queue(buffer);
		else if(buffer->Tag == MODE_VIDEO)
			mvideoQ->queue(buffer);
	}

	mKeyset = ETTFalse;
    LOGE("-- clearForMix ")
}

int CBufferQueue::getBufferCount()
{
	GKCAutoLock autoLock(&mLock);
    return mBuffers.size();
}

