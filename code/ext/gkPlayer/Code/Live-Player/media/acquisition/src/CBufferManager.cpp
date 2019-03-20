#include "CBufferManager.h"
#include "TTLog.h"
#include "GKMacrodef.h"
#include "GKCollectCommon.h"

const TTInt64 kNearEOSMarkUs = 2000; // 2 secs
#define MAX_AUDIOPACKET  50
#define MAX_VIDEOPACKET  20

CBufferManager::CBufferManager()
{
	mLock.Create();
	m_AudioQ = new CBufferQueue(MODE_AUDIO,MAX_AUDIOPACKET);
	m_VideoQ = new CBufferQueue(MODE_VIDEO,MAX_VIDEOPACKET);
	m_MainQ  = new CBufferQueue(MODE_MIX,0);
	m_MainQ->setAVQ(m_VideoQ,m_AudioQ);
}

CBufferManager::~CBufferManager() 
{
	clear();
	mLock.Destroy();
	SAFE_DELETE(m_AudioQ);
	SAFE_DELETE(m_VideoQ);
	SAFE_DELETE(m_MainQ);
}

CGKBuffer* CBufferManager::dequeue(int mode)
{
	CGKBuffer* buffer;
	if (mode == MODE_AUDIO)
	{
		return m_AudioQ->dequeue();
	}
	else if(mode == MODE_VIDEO)
	{
		return m_VideoQ->dequeue();
	}
	else if(mode == MODE_MIX)
	{
		return m_MainQ->dequeue();
	}

	return NULL;
}

int CBufferManager::queue(CGKBuffer* buffer, int mode)
{
	if (mode == MODE_AUDIO)
	{
		return m_AudioQ->queue(buffer);
	}
	else if(mode == MODE_VIDEO)
	{
		return m_VideoQ->queue(buffer);
	}
	else if(mode == MODE_MIX)
	{
		return m_MainQ->queue(buffer);
	}

	return NULL;
}

CGKBuffer* CBufferManager::getBuffer()
{
	return m_MainQ->getBuffer();
}

int CBufferManager::getBufferCount()
{
    return m_MainQ->getBufferCount();
}

void CBufferManager::eraseBuffer(TTInt64 pts, int type)
{
	CGKBuffer* abuffer = m_MainQ->eraseBuffer(pts,type);
	if (abuffer)
	{
		abuffer->nFlag = 0;
		queue(abuffer,abuffer->Tag);
	}
}

void CBufferManager::clear() 
{
	m_MainQ->clearForMix();
}

void CBufferManager::release()
{
	m_MainQ->clearForMix();
	m_AudioQ->clear();
	m_VideoQ->clear();
}
