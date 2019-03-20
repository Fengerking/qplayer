#include "TTBufferManager.h"
#include "TTTSParserProxy.h"
#include "AVCDecoderTypes.h"
#include "TTLog.h"

const TTInt64 kNearEOSMarkUs = 2000; // 2 secs

TTBufferManager::TTBufferManager(int aMode, unsigned int aPID)
	: mMode(aMode),
	  mPID(aPID),
	  mEOSResult(0),
	  mLastQueuedTimeUs(-1),
	  mStartTimeUs(-1),
	  mOption(0),
	  mCurBuffers(NULL)
{
	mLock.Create();
}


TTBufferManager::~TTBufferManager() 
{
	if(mCurBuffers != NULL) {
		freeBuffer(mCurBuffers);
		mCurBuffers = NULL;
	}
	clear();
	mLock.Destroy();
}

int  TTBufferManager::dequeueAccessUnit(TTBuffer* buffer) 
{
	GKCAutoLock autoLock(&mLock);
	if(mCurBuffers != NULL) {
		freeBuffer(mCurBuffers);
		mCurBuffers = NULL;
	}

	if(buffer == NULL) {
		return TTKErrArgument;
	}

	if(mBuffers.empty()) {
		if(mEOSResult) {
			return TTKErrEof;
		} else {
			return TTKErrNotReady;
		}
    }

	List<TTBuffer *>::iterator it = mBuffers.begin();	
	if(isVideo()) {
		if( (*it)->llTime > 0 && (*it)->llTime + 300 < buffer->llTime && (*it)->llTime + 3000 > buffer->llTime) {
			it = getNearKeyFrame(buffer->llTime);
		}

		List<TTBuffer *>::iterator it_start = mBuffers.begin();
		while (it_start != it) {
			TTBuffer *buffer = *it_start;
			freeBuffer(buffer);
			it_start = mBuffers.erase(it_start);
		}
	}

	mCurBuffers = *it;
	mBuffers.erase(it);
	memcpy(buffer, mCurBuffers, sizeof(TTBuffer));
	return 0;
}


int TTBufferManager::queueAccessUnit(TTBuffer*  buffer)  
{
	if(buffer == NULL)
		return -1;

	GKCAutoLock autoLock(&mLock);
	if(isAudio() && buffer->nFlag == 0) {
		if(mOption) {
			if(buffer->llTime + 20 < mStartTimeUs) {
				freeBuffer(buffer);
				return 0;
			} else {
				mOption = 0;
			}
		}
	}


	if(mLastQueuedTimeUs > 0 && (mLastQueuedTimeUs > buffer->llTime + 20000 || mLastQueuedTimeUs + 20000 < buffer->llTime)) {
		buffer->nFlag |= TT_FLAG_BUFFER_TIMESTAMP_RESET;
	}
    mLastQueuedTimeUs = buffer->llTime;
    mBuffers.push_back(buffer);

	return 0;
}

bool TTBufferManager::isAudio()
{
    switch (mMode) {
        case MODE_MPEG1_AUDIO:
        case MODE_MPEG2_AUDIO:
        case MODE_MPEG2_AUDIO_ADTS:
        case MODE_LPCM_AC3:
        case MODE_AC3:
            return true;

        default:
            return false;
    }
}

bool TTBufferManager::isVideo()
{
    switch (mMode) {
        case MODE_H264:
        case MODE_MPEG1_VIDEO:
        case MODE_MPEG2_VIDEO:
        case MODE_MPEG4_VIDEO:
            return true;

        default:
            return false;
    }
}

void TTBufferManager::freeBuffer(TTBuffer* buffer)
{
	if(buffer == NULL) {
		return;
	}

	if(buffer->pBuffer) {
		free(buffer->pBuffer);
		buffer->pBuffer = NULL;
	}

	if(buffer->pData) {
		if(mMode == MODE_H264) {
			TTVideoInfo* pVideoInfo = (TTVideoInfo* )buffer->pData;
			if(pVideoInfo->iDecInfo) {
				TTAVCDecoderSpecificInfo *pAVC = (TTAVCDecoderSpecificInfo *)pVideoInfo->iDecInfo;
				if(pAVC->iSpsData) {
					free(pAVC->iSpsData);
					pAVC->iSpsData = NULL;
				}

				if(pAVC->iPpsData) {
					free(pAVC->iPpsData);
					pAVC->iPpsData = NULL;
				}

				if(pAVC->iData) {
					free(pAVC->iData);
					pAVC->iData = NULL;
				}

				if(pAVC->iConfigData) {
					free(pAVC->iConfigData);
					pAVC->iConfigData = NULL;
				}

				free(pAVC);
				pVideoInfo->iDecInfo = NULL;
			}
			delete pVideoInfo;
			pVideoInfo = NULL;
			buffer->pData = NULL;
		} else if(isAudio()) {
			TTAudioInfo* pAudioInfo = (TTAudioInfo* )buffer->pData;
			delete pAudioInfo;
			buffer->pData = NULL;
		}
	}

	delete buffer;
}

void TTBufferManager::clear(bool clearFormat) 
{
    GKCAutoLock autoLock(&mLock);
	if(clearFormat) {
		List<TTBuffer *>::iterator it = mBuffers.begin();
		while (it != mBuffers.end()) {
			TTBuffer *buffer = *it;
			freeBuffer(buffer);
			it = mBuffers.erase(it);
		}
	} else {
		int nNum = 0;

		List<TTBuffer *>::iterator it = mBuffers.begin();
		while (it != mBuffers.end()) {
			TTBuffer *buffer = *it;
			if(buffer->nFlag & (TT_FLAG_BUFFER_NEW_PROGRAM | TT_FLAG_BUFFER_NEW_FORMAT)) {
				++it;
				++nNum;
				continue;
			}
			freeBuffer(buffer);
			it = mBuffers.erase(it);
		}

		if(nNum > 1) {
			it = mBuffers.begin();
			while (it != mBuffers.end()) {
				TTBuffer *buffer = *it;
				if(nNum == 1) {
					break;
				}
				freeBuffer(buffer);
				it = mBuffers.erase(it);
				nNum--;
			}
		}
	}

	mEOSResult = 0;
    mLastQueuedTimeUs = -1;
}


void TTBufferManager::signalEOS(int result) 
{
    GKCAutoLock autoLock(&mLock);
    mEOSResult = result;
}

bool TTBufferManager::hasBufferAvailable(int *finalResult) 
{
   GKCAutoLock autoLock(&mLock);
    if (!mBuffers.empty()) {
        return true;
    }

    *finalResult = mEOSResult;
    return false;
}

int TTBufferManager::setStartTime(TTInt64 nTimeUs, int aOption)
{
	GKCAutoLock autoLock(&mLock);
	mOption = aOption;
	mStartTimeUs = nTimeUs;

	return 0;
}

TTInt64 TTBufferManager::seek(TTInt64 timeUs)
{
	GKCAutoLock autoLock(&mLock);
	TTInt64 nTime = timeUs;

	if(mBuffers.empty()) {
		return TTKErrNotReady;
	}

	if(isAudio()) {
		GKCAutoLock autoLock(&mLock);
		List<TTBuffer *>::iterator it = mBuffers.begin();
		while (it != mBuffers.end()) {
			TTBuffer *buffer = *it;
			if(buffer->nFlag & (TT_FLAG_BUFFER_NEW_PROGRAM | TT_FLAG_BUFFER_NEW_FORMAT)) {
				++it;
				continue;
			}
			if(buffer->llTime < timeUs) {
				nTime = buffer->llTime;
				freeBuffer(buffer);
				it = mBuffers.erase(it);				
			} else {
				break;
			}
		}
	} else if(isVideo()) {
		List<TTBuffer *>::iterator itKey = mBuffers.begin();
		List<TTBuffer *>::iterator it = mBuffers.begin();

		while (it != mBuffers.end()) {
			TTBuffer *buffer = *it;
			if(buffer->nFlag & TT_FLAG_BUFFER_TIMESTAMP_RESET) {				
				if(buffer->llTime > timeUs || buffer->llTime <= (*itKey)->llTime) {
					break;
				}
			}

			if(buffer->nFlag & TT_FLAG_BUFFER_KEYFRAME) {
				if(buffer->llTime > timeUs) {
					break;
				}
				itKey = it;
			}
			++it;
		}

		it = mBuffers.begin();
		while (it != itKey) {
			TTBuffer *buffer = *it;
			if(buffer->nFlag & (TT_FLAG_BUFFER_NEW_PROGRAM | TT_FLAG_BUFFER_NEW_FORMAT)) {
				++it;
				continue;
			}
			freeBuffer(buffer);
			it = mBuffers.erase(it);				
		}

		TTBuffer *buffer = *itKey;
		nTime = buffer->llTime;
	}

	return nTime;
}

List<TTBuffer *>::iterator TTBufferManager::getNearKeyFrame(TTInt64 timeUs)
{
	List<TTBuffer *>::iterator it = mBuffers.begin();
	List<TTBuffer *>::iterator it_key = it;
	while (it != mBuffers.end()) {
		TTBuffer *buffer = *it;
		if(buffer->nFlag & TT_FLAG_BUFFER_TIMESTAMP_RESET) {
			if(buffer->llTime > timeUs || buffer->llTime <= (*it_key)->llTime) {
				break;
			}
		}

		if(buffer->nFlag & (TT_FLAG_BUFFER_NEW_PROGRAM | TT_FLAG_BUFFER_NEW_FORMAT)) {
			it_key = it;
			break;
		}

		if(buffer->nFlag & TT_FLAG_BUFFER_KEYFRAME) {
			if(buffer->llTime > timeUs) {
				break;
			}
			it_key = it;
		}
		++it;
	}

	return it_key;
}

TTInt64 TTBufferManager::getBufferedDurationUs(int *finalResult) 
{
    GKCAutoLock autoLock(&mLock);
    return getBufferedDurationUs_l(finalResult);
}

TTInt64 TTBufferManager::getBufferedDurationUs_l(int *finalResult) 
{
    *finalResult = mEOSResult;

    if (mBuffers.empty()) {
        return 0;
    }

    TTInt64 time1 = -1;
    TTInt64 time2 = -1;
    TTInt64 durationUs = 0;

    List<TTBuffer *>::iterator it = mBuffers.begin();
    while (it != mBuffers.end()) {
        const TTBuffer *buffer = *it;

        TTInt64 timeUs = buffer->llTime;
        if (buffer->nFlag & TT_FLAG_BUFFER_TIMESTAMP_RESET) {
			durationUs += time2 - time1;
            time1 = time2 = -1;
        } else {
            if (time1 < 0 || timeUs < time1) {
                time1 = timeUs;
            }

            if (time2 < 0 || timeUs > time2) {
                time2 = timeUs;
            }
        }

        ++it;
    }

    return durationUs + (time2 - time1);
}

int TTBufferManager::getBufferCount()
{
	GKCAutoLock autoLock(&mLock);
    return mBuffers.size();
}

TTInt64 TTBufferManager::getEstimatedDurationUs() 
{
    GKCAutoLock autoLock(&mLock);
    if (mBuffers.empty()) {
        return 0;
    }

    List<TTBuffer *>::iterator it = mBuffers.begin();
    TTBuffer* buffer = *it;

    TTInt64 startTimeUs = buffer->llTime;
    if (startTimeUs < 0) {
        return 0;
    }

    it = mBuffers.end();
    --it;
    buffer = *it;

    TTInt64 endTimeUs = buffer->llTime;;
    if (endTimeUs < 0) {
        return 0;
    }

    TTInt64 diffUs;
    if (endTimeUs > startTimeUs) {
        diffUs = endTimeUs - startTimeUs;
    } else {
        diffUs = startTimeUs - endTimeUs;
    }
    return diffUs;
}

int TTBufferManager::nextBufferTime(TTInt64 *timeUs) 
{
    *timeUs = 0;

    GKCAutoLock autoLock(&mLock);

    if (mBuffers.empty()) {
        return mEOSResult != 0 ? mEOSResult : -1;
    }

	List<TTBuffer *>::iterator it = mBuffers.begin();
	while (it != mBuffers.end()) {
		TTBuffer *buffer = *it;
		*timeUs = buffer->llTime;
		if(buffer->nSize > 0) {
			break;
		}
		++it;
	}

    return 0;
}

bool TTBufferManager::isFinished(TTInt64 duration) const 
{
    if (duration > 0) {
        TTInt64 diff = duration - mLastQueuedTimeUs;
        if (diff < kNearEOSMarkUs && diff > -kNearEOSMarkUs) {
            return true;
        }
    }
    return (mEOSResult != 0);
}

