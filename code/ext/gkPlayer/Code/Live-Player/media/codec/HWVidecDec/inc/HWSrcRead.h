/**
* File : TTHWSrcRead.h  
* Created on : 2014-11-5
* Author : yongping.lin
* Copyright : Copyright (c) 2014 GoKu Software Ltd. All rights reserved.
* Description : HWSource定义文件
*/

#ifndef __HWSRCREAD_H__
#define __HWSRCREAD_H__

#include "HWDecoder.h"

#define  LOG_TAG    "TTHWSRC"

class HWSource : public MediaSource {
public:
    HWSource(CTTHWDecoder* aDecoder, sp<MetaData> &meta) {
        mHwDecoder = aDecoder;
        source_meta = meta;
		mHwDecoder->getParam(TT_PID_VIDEO_FORMAT, &mVideoFormat);
        frame_size  = (mVideoFormat.Width * mVideoFormat.Height * 3) / 2;
        buf_group.add_buffer(new MediaBuffer(frame_size));
		mObserver = mHwDecoder->getObserver();
		mlastTime = 0;
		first_frame = 1;
    }

    virtual sp<MetaData> getFormat() {
        return source_meta;
    }

	void setFormat(const sp<MetaData> &meta) {
		source_meta = meta;
	}

    virtual status_t start(MetaData *params) {
        return OK;
    }

    virtual status_t stop() {
        return OK;
    }

    virtual status_t read(MediaBuffer **buffer,
                          const MediaSource::ReadOptions *options) {
        status_t nRet = OK;
		
		if(mObserver == NULL)
			return -1;

		TTBuffer SrcBuffer;
		memset(&SrcBuffer, 0, sizeof(TTBuffer));

		nRet = buf_group.acquire_buffer(buffer);
		if(nRet != OK) {
			LOGI("could not acquire_buffer");
			return nRet;
		}

		int64_t seekTimeUs;
		ReadOptions::SeekMode mode;
		uint32_t findFlags = 0;
		if (options && options->getSeekTo(&seekTimeUs, &mode)) {
			switch (mode) {
			case ReadOptions::SEEK_PREVIOUS_SYNC:
			case ReadOptions::SEEK_NEXT_SYNC:
			case ReadOptions::SEEK_CLOSEST_SYNC:
			case ReadOptions::SEEK_CLOSEST:
				findFlags = 1;
				first_frame = 1;
				break;
			default:
				break;
			}

			LOGI("seek options findFlags %d", findFlags);
		}

		nRet = mObserver->pObserver(mObserver->pUserData, TT_HWDECDEC_CALLBACK_PID_READBUFFER, findFlags, 0, &SrcBuffer);

		if(first_frame) {
			if(SrcBuffer.nFlag & TT_FLAG_BUFFER_KEYFRAME) {
				first_frame = 0;
			} else {
				nRet = TTKErrNotReady;
			}
		}

		TTInt nSize = 0;
		if (nRet == TTKErrNone) {
			mlastTime = SrcBuffer.llTime*1000;
			memcpy((*buffer)->data(), SrcBuffer.pBuffer, SrcBuffer.nSize);
			(*buffer)->set_range(0, SrcBuffer.nSize);
			(*buffer)->meta_data()->clear();
			(*buffer)->meta_data()->setInt32(kKeyIsSyncFrame, SrcBuffer.nFlag&TT_FLAG_BUFFER_KEYFRAME);
			(*buffer)->meta_data()->setInt64(kKeyTime, mlastTime);
			nSize = SrcBuffer.nSize;
		//} else if(nRet == TTKErrInUse) {
		//	mlastTime++;
		//	mHwDecoder->getConfigData(&SrcBuffer);
		//	memcpy((*buffer)->data(), SrcBuffer.pBuffer, SrcBuffer.nSize);
		//  (*buffer)->meta_data()->clear();
		//  (*buffer)->meta_data()->setInt32(kKeyIsSyncFrame, 0);
		//	(*buffer)->meta_data()->setInt64(kKeyTime, mlastTime);
		//	nSize = SrcBuffer.nSize;
		} else if(nRet == TTKErrEof) {
			(*buffer)->release();
			return -1;
		} else {
			mlastTime++;
			(*buffer)->set_range(0, 0);			
            (*buffer)->meta_data()->clear();
            (*buffer)->meta_data()->setInt32(kKeyIsSyncFrame, 0);
			(*buffer)->meta_data()->setInt64(kKeyTime, mlastTime);
			nSize = 0;
		}

		//LOGI("get video sample %d, szie %d", nRet, nSize);

		return OK;
    }

private:
    MediaBufferGroup buf_group;
    sp<MetaData> source_meta;
    TTVideoFormat mVideoFormat;
    CTTHWDecoder *mHwDecoder;
    TTObserver *mObserver;
    int frame_size;
	int	first_frame;
    int64_t mlastTime;
};

#endif
