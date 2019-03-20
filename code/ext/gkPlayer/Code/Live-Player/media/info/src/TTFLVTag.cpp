/*
* File : TTFLVTag.cpp
* Created on : 2015-9-8
* Author : yongping.lin
* Description : TTFLVTag
*/
#include "TTIntReader.h"
#include "GKOsalConfig.h"
#include "TTSysTime.h"
#include "TTLog.h"
#include "TTFLVTag.h"
#include "TTAvUtils.h"


CTTFlvTagStream::CTTFlvTagStream(unsigned int streamType)
	: mStreamType(streamType)
	, mBuffer(NULL)
	, mMaxSize(0)
	, mNalLength(0)
	, mAudioCodec(0)
	, mSampleRate(44100)
	, mChannel(2)
	, mSampleBits(0)
	, mBitrate(0)
	, mTotalSize(0)
	, mStartTime(0)
	, mEndTime(0)
	, mVideoCodec(0)
	, mWidth(0)
	, mHeight(0)
	, mQueue(NULL)
	, mSource(NULL)
{
}

CTTFlvTagStream::~CTTFlvTagStream() 
{
	if(mQueue) {
		delete mQueue;
		mQueue = NULL;
	}

	if(mSource) {
		delete mSource;
		mSource = NULL;
	}

	if(mBuffer) {
		free(mBuffer);
		mBuffer = NULL;
	}
}

int  CTTFlvTagStream::addTag(unsigned char *data, unsigned int size, TTInt64 timeUs) 
{
	int nErr = 0;
    
    if(size == 0 || data == NULL) {
        return TTKErrUnderflow;
    }

	if(mStreamType == FLV_STREAM_TYPE_VIDEO) {
		nErr = addVideoTag(data, size, timeUs);
	} else if(mStreamType == FLV_STREAM_TYPE_AUDIO) {
		nErr = addAudioTag(data, size, timeUs);
	}

	return nErr;
}

int CTTFlvTagStream::tagHeader(unsigned char *data, unsigned int size, int &streamType, int &dataSize, int &timeUs)
{
	if(data == NULL || size < 11) {
		return -1;
	}

	streamType = data[0];
	dataSize = CTTIntReader::ReadBytesNBE(data + 1, 3);
	timeUs = CTTIntReader::ReadBytesNBE(data + 4, 3);
	timeUs |= (data[7] << 24);

	//int streamID = CTTIntReader::ReadUint32BE(data + 8);

	return 0;
}

int  CTTFlvTagStream::addAudioTag(unsigned char *data, unsigned int size, TTInt64 timeUs)
{
	TTInt nFlag = data[0];

	int nChannel = (nFlag & FLV_AUDIO_CHANNEL_MASK) == FLV_STEREO ? 2 : 1;
    int nSampleRate = 44100 << ((nFlag & FLV_AUDIO_SAMPLERATE_MASK) >> FLV_AUDIO_SAMPLERATE_OFFSET) >> 3;
    int nSampleBits = (nFlag & FLV_AUDIO_SAMPLESIZE_MASK) ? 16 : 8;

	TTInt nAudioCodec = nFlag & FLV_AUDIO_CODECID_MASK;

	if(mAudioCodec == 0) {
		mAudioCodec = nAudioCodec;
		if(mAudioCodec == FLV_CODECID_AAC) {
			if(mQueue == NULL) {
				mQueue = new TTStreamQueue(TTStreamQueue::AAC, 0);
			}

			if(mSource == NULL) {
				mSource = new TTBufferManager(TTBufferManager::MODE_MPEG2_AUDIO_ADTS, 0);
			}			
		} else if(mAudioCodec == FLV_CODECID_MP3) {
			if(mQueue == NULL) {
				mQueue = new TTStreamQueue(TTStreamQueue::MPEG_AUDIO, 0);
			}

			if(mSource == NULL) {
				mSource = new TTBufferManager(TTBufferManager::MODE_MPEG2_AUDIO, 0);
			}
		} else {
			return -1;
		}
	} else if(mAudioCodec != nAudioCodec) {
		return -1;
	}

	if(mAudioCodec == FLV_CODECID_AAC) {
		TTInt nType = data[1];
		if(nType == 0) {
			int nChannel = mChannel;
			int nSampleRate = mSampleRate;
			if(ParseAACConfig(data + 2, size - 2, &nSampleRate, &nChannel) == 0) {
				mChannel = nChannel;
				mSampleRate = nSampleRate;
			}
		} else {
			if(mMaxSize < size - 2 + 128) {
				free(mBuffer);
				mBuffer = (unsigned char *)malloc(size - 2 + 128);
				mMaxSize = size - 2 + 128;
			}

			if(ConstructAACHeader(mBuffer, mMaxSize, mSampleRate, mChannel, size - 2) != 7) {
				return -1;
			}

			memcpy(mBuffer + 7, data + 2, size - 2);

			onPayloadData(mBuffer, size + 5, timeUs);
		}
	} else if(mAudioCodec == FLV_CODECID_MP3) {
		onPayloadData(data + 1, size - 1, timeUs);
	}

	return 0;
}

int  CTTFlvTagStream::addVideoTag(unsigned char *data, unsigned int size, TTInt64 timeUs)
{
	TTInt nFlag = data[0];
	TTInt nVideoCodec = nFlag & FLV_VIDEO_CODECID_MASK;
	
	if(mVideoCodec == 0) {
		if(nVideoCodec == FLV_CODECID_H264) {
			if(mQueue == NULL) {
				mQueue = new TTStreamQueue(TTStreamQueue::H264, 0);
			}

			if(mSource == NULL) {
				mSource = new TTBufferManager(TTBufferManager::MODE_H264, 0);
			}
		}  else if(nVideoCodec == FLV_CODECID_HEVC) {
			if(mQueue == NULL) {
				mQueue = new TTStreamQueue(TTStreamQueue::HEVC, 0);
			}

			if(mSource == NULL) {
				mSource = new TTBufferManager(TTBufferManager::MODE_HEVC, 0);
			}
		}
		else {
			return -1;
		}

		if(size < 5) {
			return -1;
		}
		mVideoCodec = nVideoCodec;
	} else if(mVideoCodec != nVideoCodec) {
		return -1;
	}
	
	TTInt AVCPacketType = data[1];
	TTInt CTS = CTTIntReader::ReadBytesNBE(data + 2, 3);
	TTInt nErr = 0;
	TTInt nKey = ((nFlag & FLV_FRAME_KEY) && AVCPacketType)? 1 : 0;

	if(mMaxSize < size - 5 + 128) {
		free(mBuffer);
		mBuffer = (unsigned char *)malloc(size - 5 + 128);
		mMaxSize = size - 5 + 128;
	}

	int nSize = mMaxSize;
	if(AVCPacketType == 0) {
		if(nVideoCodec == FLV_CODECID_H264) {
			nErr = ConvertAVCNalHead(mBuffer, nSize, data + 5, size - 5, mNalLength);
		} else if(nVideoCodec == FLV_CODECID_HEVC) {
			nErr = ConvertHEVCNalHead(mBuffer, nSize, data + 5, size - 5, mNalLength);
		}

		if(nErr < 0) {
			return nErr;
		}

		onPayloadData(mBuffer, nSize, -1);
	} else if(AVCPacketType == 1) {
		timeUs += CTS;
		int isKeyFrame = 0;
		nErr = ConvertAVCNalFrame(mBuffer, nSize, data + 5, size - 5, mNalLength, isKeyFrame, nVideoCodec);		
		if(nErr < 0) {
			return nErr;
		}

		if(mNalLength < 3) {
			onPayloadData(mBuffer, nSize, timeUs);
		} else {
			onPayloadData(data + 5, size - 5, timeUs);
		}
	}

	return 0;
}

void   CTTFlvTagStream::onPayloadData(unsigned char *data, unsigned int size, TTInt64 timeUs)
{
	int err = mQueue->appendData(data, size, timeUs, 0);
    
	mTotalSize += size;
	
	if(mStartTime == 0 && timeUs > 0) {
		mStartTime = timeUs;
	}

	if(mEndTime < timeUs && timeUs > 0) {
		mEndTime = timeUs;
	}
	
	if (err != 0) {
        return;
    }

	TTBuffer*  accessUnit = NULL;
    while ((accessUnit = mQueue->dequeueAccessUnit()) != NULL) {        
		mSource->queueAccessUnit(accessUnit);
    }
}

void CTTFlvTagStream::signalEOS(int finalResult) 
{
	if(mSource) {
		mSource->signalEOS(finalResult);
	}
}

TTBufferManager* CTTFlvTagStream::getSource()
{
	return mSource;
}

int CTTFlvTagStream::flush()
{
	if(mSource) {
		mSource->clear(false);
	}
	return 0;
}

int CTTFlvTagStream::getBitrate()
{
	int nBitrate = 0;
	if(mEndTime - mStartTime > 0) {
		nBitrate = mTotalSize*1000/(mEndTime - mStartTime);
	}

	return nBitrate;
}
