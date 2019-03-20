#include "TTAvUtils.h"
#include "AVCDecoderTypes.h"
#include "TTStreamQueue.h"
#include "TTLog.h"

TTStreamQueue::TTStreamQueue(Mode mode, unsigned int aPID, unsigned int flags)
    : mMode(mode),
	  mPID(aPID), 
      mFlags(flags),
	  mAudioInfo(NULL),
	  mVideoInfo(NULL),
	  mSize(0),
	  mBuffer(NULL),
	  mCapacity(0),
	  mNumFrameSample(0),
	  mSampleRate(0),
	  mWidth(0),
	  mHeight(0),
	  mNumRef(0)
{
}


TTStreamQueue::~TTStreamQueue()
{
	clear(true);
	if(mBuffer) {
		free(mBuffer);
		mBuffer = NULL;
	}

	if(mAudioInfo) {
		delete mAudioInfo;
		mAudioInfo = NULL;
	}

	if(mVideoInfo) {
		if(mMode == H264 || mMode == HEVC) {
			if(mVideoInfo->iDecInfo) {
				TTAVCDecoderSpecificInfo *pAVC = (TTAVCDecoderSpecificInfo *)mVideoInfo->iDecInfo;
				if(pAVC->iSpsData) {
					free(pAVC->iSpsData);
					pAVC->iSpsData = NULL;
				}

				if(pAVC->iPpsData) {
					free(pAVC->iPpsData);
					pAVC->iPpsData = NULL;
				}

				if(pAVC->iConfigData) {
					free(pAVC->iConfigData);
					pAVC->iConfigData = NULL;
				}

				if(pAVC->iData) {
					free(pAVC->iData);
					pAVC->iData = NULL;
				}

				free(pAVC);
				mVideoInfo->iDecInfo = NULL;
			}
		}
		delete mVideoInfo;
		mVideoInfo = NULL;
	}		
}

void TTStreamQueue::clear(bool clearFormat) 
{
    if (mSize != 0 ) {
        mSize = 0;
    }

    mRangeInfos.clear();

	if(clearFormat) {
		if(mAudioInfo) {
			delete mAudioInfo;
			mAudioInfo = NULL;
		}

		if(mVideoInfo) {
			if(mMode == H264 || mMode == HEVC) {
				if(mVideoInfo->iDecInfo) {
					TTAVCDecoderSpecificInfo *pAVC = (TTAVCDecoderSpecificInfo *)mVideoInfo->iDecInfo;
					if(pAVC->iSpsData) {
						free(pAVC->iSpsData);
						pAVC->iSpsData = NULL;
					}

					if(pAVC->iPpsData) {
						free(pAVC->iPpsData);
						pAVC->iPpsData = NULL;
					}

					if(pAVC->iConfigData) {
						free(pAVC->iConfigData);
						pAVC->iConfigData = NULL;
					}

					if(pAVC->iData) {
						free(pAVC->iData);
						pAVC->iData = NULL;
					}

					free(pAVC);
					mVideoInfo->iDecInfo = NULL;
				}
			}
			delete mVideoInfo;
			mVideoInfo = NULL;
		}
	}
}

int TTStreamQueue::appendData(unsigned char* data, unsigned int size, TTInt64 timeUs, unsigned int flags) 
{
    unsigned int neededSize = (mBuffer == NULL ? 0 : mSize) + size;
    if (mBuffer == NULL || neededSize > mCapacity) {
        neededSize = (neededSize + 65535) & ~65535;
		unsigned char* buffer = (unsigned char *)malloc(neededSize);
		mCapacity = neededSize;

        if (mBuffer != NULL) {
            memcpy(buffer, mBuffer, mSize);
			free(mBuffer);
			mBuffer = NULL;
        } else {
            mSize = 0;
        }
        mBuffer = buffer;
    }

    memcpy(mBuffer + mSize, data, size);
	mSize += size;

	RangeInfo info;
	info.mFlags = flags;
	info.mLength = size;
	info.mTimestampUs = timeUs;
	mRangeInfos.push_back(info);

    return 0;
}

TTBuffer*  TTStreamQueue::dequeueAccessUnit() 
{
    switch (mMode) {
        case H264:
            return dequeueAccessUnitH264();
		case HEVC:
            return dequeueAccessUnitHEVC();
        case AAC:
            return dequeueAccessUnitAAC();
        case MPEG_AUDIO:
            return dequeueAccessUnitMPEGAudio();
        default:
            return NULL;
    }
}

TTBuffer* TTStreamQueue::dequeueAccessUnitAAC() 
{
    if (mSize == 0) {
        return NULL;
    }

	if(mRangeInfos.empty()) {
		return NULL;
	}

	if(mSize < 7) {
		return NULL;
	}

	int offset = 0;
	int sampleRate;
	int channel;

	int err = GetAACFrameSize(mBuffer, mSize, &offset,&sampleRate, &channel);
	if(err < 0) {
		return NULL;
	}

	mNumFrameSample = 1024;
	mSampleRate = sampleRate;
	if(mAudioInfo == NULL) {
		mAudioInfo = new TTAudioInfo();
		mAudioInfo->iChannel = channel;
		mAudioInfo->iSampleRate = sampleRate;
		mAudioInfo->iMediaTypeAudioCode = TTAudioInfo::KTTMediaTypeAudioCodeAAC;
		mAudioInfo->iFourCC = MAKEFOURCC('A','D','T','S');
		mAudioInfo->iBitRate = offset*sampleRate/1024;
		mAudioInfo->iStreamId = mPID;

		TTAudioInfo* pAudioInfo = new TTAudioInfo();
		pAudioInfo->iChannel = channel;
		pAudioInfo->iSampleRate = sampleRate;
		pAudioInfo->iMediaTypeAudioCode = TTAudioInfo::KTTMediaTypeAudioCodeAAC;
		pAudioInfo->iFourCC = MAKEFOURCC('A','D','T','S');
		pAudioInfo->iBitRate = offset*sampleRate/1024;
		pAudioInfo->iStreamId = mPID;

		RangeInfo *info = &*mRangeInfos.begin();
		TTInt64 timeUs = info->mTimestampUs;

		TTBuffer* accessUnit = new TTBuffer;
		memset(accessUnit, 0, sizeof(TTBuffer));
		accessUnit->llTime = timeUs;
		accessUnit->pData = (void *)pAudioInfo;
		accessUnit->nFlag |= TT_FLAG_BUFFER_NEW_FORMAT;

		return accessUnit;
	}

    TTInt64 timeUs = fetchTimestampAudio(offset);

    TTBuffer* accessUnit = new TTBuffer;
	memset(accessUnit, 0, sizeof(TTBuffer));
	accessUnit->llTime = timeUs;
	accessUnit->nSize = offset;
	accessUnit->pBuffer = (unsigned char *)malloc(offset);

    memcpy(accessUnit->pBuffer, mBuffer, offset);
    memmove(mBuffer, mBuffer + offset, mSize - offset);
	mSize -= offset;
 
    return accessUnit;
}

TTInt64 TTStreamQueue::fetchTimestampMeta(unsigned int size)
{
    TTInt64 timeUs = -1;
	int nList = mRangeInfos.size();

	List<RangeInfo>::iterator it = mRangeInfos.begin();
	while (it != mRangeInfos.end()) {
		RangeInfo *info = &*it;
		
		if (info->mTimestampUs >= 0) {
            timeUs = info->mTimestampUs;
			break;			
        }
		
		++it;
	}
	
    return timeUs;
}

TTInt64 TTStreamQueue::fetchTimestamp(unsigned int size) 
{
    TTInt64 timeUs = -1;
	int nList = mRangeInfos.size();
	bool first = true;
	

    while (size > 0 && nList > 0) {
        RangeInfo *info = &*mRangeInfos.begin();

        if (first && info->mTimestampUs >= 0) {
            timeUs = info->mTimestampUs;
            first = false;
        }

        if (info->mLength > size) {
            info->mLength -= size;
            size = 0;
        } else {
            size -= info->mLength;

            mRangeInfos.erase(mRangeInfos.begin());
            info = NULL;
        }

		nList = mRangeInfos.size();
    }

    return timeUs;
}

TTInt64 TTStreamQueue::fetchTimestampAudio(unsigned int size) 
{
    TTInt64 timeUs = -1;
	int nList = mRangeInfos.size();
    bool first = true;

    size_t samplesize = size;
    while (size > 0 && nList > 0) {
        RangeInfo *info = &*mRangeInfos.begin();

        if (first) {
            timeUs = info->mTimestampUs;
            first = false;
        }

        if (info->mLength > size) {
            info->mLength -= size;
            unsigned int numSamples = mNumFrameSample * size / samplesize;
            info->mTimestampUs += numSamples * 1000 / mSampleRate;
            size = 0;
        } else {
            size -= info->mLength;
            mRangeInfos.erase(mRangeInfos.begin());
            info = NULL;
        }

		nList = mRangeInfos.size();
    }

    return timeUs;
}

#define XRAW_IS_ANNEXB(p) ( !(*((p)+0)) && !(*((p)+1)) && (*((p)+2)==1))
#define XRAW_IS_ANNEXB2(p) ( !(*((p)+0)) && !(*((p)+1)) && !(*((p)+2))&& (*((p)+3)==1))

TTBuffer* TTStreamQueue::dequeueAccessUnitH264() 
{
	int naluType = 0;
	int naluLast = 0;
	int keyFrame = -1;
	int firstmb = -1;
	unsigned char* p = mBuffer;
	unsigned char* startPos = NULL;
	unsigned char* nalStart = NULL;
	unsigned char* endPos = mBuffer + mSize;

	unsigned char *spsBuffer = (unsigned char*)malloc(256);
	unsigned char *ppsBuffer = (unsigned char*)malloc(256);
	
	unsigned int nSpsSize = 0;
	unsigned int nPpsSize = 0;

	bool foundSlice = false;
	bool foundFrame = false;

	for (; p < endPos - 3; p++)	{
		if (XRAW_IS_ANNEXB(p) || XRAW_IS_ANNEXB2(p))	{
			if(startPos == NULL) {
				startPos = p;
			}

			unsigned int step = 3;
			if(XRAW_IS_ANNEXB(p)) {
				naluType = p[3]&0x0f;
				firstmb = p[4] & 0x80;
			} else if(XRAW_IS_ANNEXB2(p)) {
				naluType = p[4]&0x0f;
				firstmb = p[5] & 0x80;
				step = 4;
			}

			if(naluLast == 9 && foundSlice) {
				foundFrame = true;
				break;
			}

			if(naluLast == 7) {
				if(foundSlice) {
					foundFrame = true;
					break;
				}
				unsigned int nSize = p - nalStart;
				unsigned char *pBuffer = spsBuffer;
				if(nSize> 256) {
					pBuffer = (unsigned char *)malloc(nSize + 256);
					free(spsBuffer);
					spsBuffer = pBuffer;
				}

				memcpy(pBuffer, nalStart, nSize);
				nSpsSize = nSize;
			}

			if(naluLast == 8) {
				unsigned int nSize = p - nalStart;
				unsigned char *pBuffer = ppsBuffer;
				if(nSize> 256) {
					pBuffer = (unsigned char *)malloc(nSize + 256);
					free(ppsBuffer);
					ppsBuffer = pBuffer;
				}

				memcpy(pBuffer, nalStart, nSize);
				nPpsSize = nSize;
			}

			if (naluType == 1 || naluType == 5) {
				if (foundSlice) {
					if (firstmb) {
						foundFrame = true;
						break;
					}
				} else {
					keyFrame = (naluType == 5) ? 1 : 0;
				}

				foundSlice = true;
			} 
			
			if((naluType == 9 || naluType == 7) && foundSlice) {
				foundFrame = true;
				break;
			}
			naluLast = naluType;
			nalStart = p;
			p += step;
		}
	}

	if((nSpsSize == 0 || nPpsSize == 0) && mVideoInfo == NULL) {
		if(foundFrame) {
			unsigned int offset = p - mBuffer;
			memmove(mBuffer, mBuffer + offset, mSize - offset);
			mSize -= offset;

			fetchTimestamp(offset);//TTInt64 timeUs =
		}

		free(spsBuffer);
		free(ppsBuffer);
		return NULL;
	}

	int newFormat = 0;
	if(nSpsSize > 0) {
		TTBuffer sps;
		sps.pBuffer = spsBuffer;
		sps.nSize = nSpsSize;

		int width = 0;
		int height = 0;
		int numref = 0;
		int sarwidth = 0;
		int sarheight = 0;

		FindAVCDimensions(&sps, &width, &height, &numref, &sarwidth, &sarheight);
		if(mVideoInfo == NULL || (mWidth != width) || (mHeight != height) || (mNumRef != numref)) {
			mWidth = width;
			mHeight = height;
			mNumRef = numref;
			newFormat = 1;
		}
	}

	if(newFormat) { 
		if(mVideoInfo) {
			if(mVideoInfo->iDecInfo) {
				TTAVCDecoderSpecificInfo *pAVC = (TTAVCDecoderSpecificInfo *)mVideoInfo->iDecInfo;				
				if(pAVC->iSpsData) {
					free(pAVC->iSpsData);
					pAVC->iSpsData = NULL;
				}

				if(pAVC->iPpsData) {
					free(pAVC->iPpsData);
					pAVC->iPpsData = NULL;
				}

				if(pAVC->iConfigData) {
					free(pAVC->iConfigData);
					pAVC->iConfigData = NULL;
				}

				if(pAVC->iData) {
					free(pAVC->iData);
					pAVC->iData = NULL;
				}

				free(pAVC);
				mVideoInfo->iDecInfo = NULL;
			}

			delete mVideoInfo;
			mVideoInfo = NULL;
		}

		mVideoInfo = new TTVideoInfo();
		mVideoInfo->iWidth = mWidth;
		mVideoInfo->iHeight = mHeight;
		mVideoInfo->iMediaTypeVideoCode = TTVideoInfo::KTTMediaTypeVideoCodeH264;
		TTAVCDecoderSpecificInfo *pAVC = (TTAVCDecoderSpecificInfo *)malloc(sizeof(TTAVCDecoderSpecificInfo));
		memset(pAVC, 0, sizeof(TTAVCDecoderSpecificInfo));
		pAVC->iSpsData = (unsigned char *)malloc(nSpsSize);
		memcpy(pAVC->iSpsData, spsBuffer, nSpsSize);
		pAVC->iSpsSize = nSpsSize;
		pAVC->iPpsData = (unsigned char *)malloc(nPpsSize);
		memcpy(pAVC->iPpsData, ppsBuffer, nPpsSize);
		pAVC->iPpsSize = nPpsSize;
		pAVC->iSize = nSpsSize + nPpsSize;
		pAVC->iData = (unsigned char *)malloc(pAVC->iSize);
		memcpy(pAVC->iData, spsBuffer, nSpsSize);
		memcpy(pAVC->iData + nSpsSize, ppsBuffer, nPpsSize);
		mVideoInfo->iDecInfo = pAVC;
		mVideoInfo->iStreamId = mPID;

		TTVideoInfo* pVideoInfo = new TTVideoInfo();
		pVideoInfo->iWidth = mWidth;
		pVideoInfo->iHeight = mHeight;
		pVideoInfo->iMediaTypeVideoCode = TTVideoInfo::KTTMediaTypeVideoCodeH264;
		TTAVCDecoderSpecificInfo *mAVC = (TTAVCDecoderSpecificInfo *)malloc(sizeof(TTAVCDecoderSpecificInfo)); 
		memset(mAVC, 0, sizeof(TTAVCDecoderSpecificInfo));
		mAVC->iSpsData = (unsigned char *)malloc(nSpsSize);
		memcpy(mAVC->iSpsData, spsBuffer, nSpsSize);
		mAVC->iSpsSize = nSpsSize;
		mAVC->iPpsData = (unsigned char *)malloc(nPpsSize);
		memcpy(mAVC->iPpsData, ppsBuffer, nPpsSize);
		mAVC->iPpsSize = nPpsSize;
		mAVC->iSize = nSpsSize + nPpsSize;
		mAVC->iData = (unsigned char *)malloc(mAVC->iSize);
		memcpy(mAVC->iData, spsBuffer, nSpsSize);
		memcpy(mAVC->iData + nSpsSize, ppsBuffer, nPpsSize);
		pVideoInfo->iDecInfo = mAVC;
		pVideoInfo->iStreamId = mPID;

		TTInt64 timeUs = fetchTimestampMeta(mAVC->iSize);

		TTBuffer* accessUnit = new TTBuffer;
		memset(accessUnit, 0, sizeof(TTBuffer));
		accessUnit->llTime = timeUs;
		accessUnit->pData = (void *)pVideoInfo;
		accessUnit->nFlag |= TT_FLAG_BUFFER_NEW_FORMAT;

		free(spsBuffer);
		free(ppsBuffer);

		return accessUnit;
	}

	free(spsBuffer);
	free(ppsBuffer);

	if(!foundFrame) {
		return NULL;
	}

	unsigned int offset = p - mBuffer;
	unsigned int frameSize = p - startPos;

    TTInt64 timeUs = fetchTimestamp(offset);
    TTBuffer* accessUnit = new TTBuffer;
	memset(accessUnit, 0, sizeof(TTBuffer));
	accessUnit->llTime = timeUs;
	accessUnit->nSize = frameSize;
	accessUnit->pBuffer = (unsigned char *)malloc(offset);
	
	if(keyFrame) {
		accessUnit->nFlag |= TT_FLAG_BUFFER_KEYFRAME;
	}

    memcpy(accessUnit->pBuffer, startPos, frameSize);
    memmove(mBuffer, mBuffer + offset, mSize - offset);
	mSize -= offset;
 
    return accessUnit;
}

TTBuffer* TTStreamQueue::dequeueAccessUnitHEVC()
{
	int naluType = 0;
	int naluLast = 0;
	int keyFrame = -1;
	int firstmb = -1;
	unsigned char* p = mBuffer;
	unsigned char* startPos = NULL;
	unsigned char* nalStart = NULL;
	unsigned char* endPos = mBuffer + mSize;

	unsigned char *spsBuffer = (unsigned char*)malloc(256);
	unsigned char *ppsBuffer = (unsigned char*)malloc(256);
	unsigned char *vpsBuffer = (unsigned char*)malloc(256);
	
	unsigned int nSpsSize = 0;
	unsigned int nPpsSize = 0;
	unsigned int nVpsSize = 0;

	bool foundSlice = false;
	bool foundFrame = false;

	for (; p < endPos - 3; p++)	{
		if (XRAW_IS_ANNEXB(p) || XRAW_IS_ANNEXB2(p))	{
			if(startPos == NULL) {
				startPos = p;
			}

			unsigned int step = 3;
			if(XRAW_IS_ANNEXB(p)) {
				naluType = (p[3] >> 1)&0x3f;
				firstmb = p[5] & 0x80;
			} else if(XRAW_IS_ANNEXB2(p)) {
				naluType = (p[4] >> 1)&0x3f;
				firstmb = p[6] & 0x80;
				step = 4;
			}

			if(naluLast == 35 && foundSlice) {
				foundFrame = true;
				break;
			}

			if(naluLast == 32) {
				if(foundSlice) {
					foundFrame = true;
					break;
				}
				unsigned int nSize = p - nalStart - step;
				unsigned char *pBuffer = vpsBuffer;
				if(nSize> 256) {
					pBuffer = (unsigned char *)malloc(nSize + 256);
					free(vpsBuffer);
					vpsBuffer = pBuffer;
				}

				memcpy(pBuffer, nalStart + step, nSize);
				nVpsSize = nSize;
			}

			if(naluLast == 33) {
				if(foundSlice) {
					foundFrame = true;
					break;
				}
				unsigned int nSize = p - nalStart- step;
				unsigned char *pBuffer = spsBuffer;
				if(nSize> 256) {
					pBuffer = (unsigned char *)malloc(nSize + 256);
					free(spsBuffer);
					spsBuffer = pBuffer;
				}

				memcpy(pBuffer, nalStart + step, nSize);
				nSpsSize = nSize;
			}

			if(naluLast == 34) {
				unsigned int nSize = p - nalStart- step;
				unsigned char *pBuffer = ppsBuffer;
				if(nSize> 256) {
					pBuffer = (unsigned char *)malloc(nSize + 256);
					free(ppsBuffer);
					ppsBuffer = pBuffer;
				}

				memcpy(pBuffer, nalStart + step, nSize);
				nPpsSize = nSize;
			}

			if (naluType < 21) {
				if (foundSlice) {
					if (firstmb) {
						foundFrame = true;
						break;
					}
				} else {
					keyFrame = (naluType >= 16 && naluType <= 23) ? 1 : 0;
				}

				foundSlice = true;
			} 
			
			if((naluType == 35 || naluType == 32 || naluType == 33) && foundSlice) {
				foundFrame = true;
				break;
			}
			naluLast = naluType;
			nalStart = p;
			p += step;
		}
	}

	if((nSpsSize == 0 || nPpsSize == 0 || nVpsSize == 0) && mVideoInfo == NULL) {
		if(foundFrame) {
			unsigned int offset = p - mBuffer;
			memmove(mBuffer, mBuffer + offset, mSize - offset);
			mSize -= offset;

			fetchTimestamp(offset);//TTInt64 timeUs =
		}

		free(spsBuffer);
		free(ppsBuffer);
		free(vpsBuffer);
		return NULL;
	}

	int newFormat = 0;
	if(nSpsSize > 0) {
		int width = 0;
		int height = 0;
		int numref = 0;
		int sarwidth = 0;
		int sarheight = 0;

		FindHEVCDimensions(spsBuffer, nSpsSize, &width, &height);
		if(mVideoInfo == NULL || (mWidth != width) || (mHeight != height)) {
			mWidth = width;
			mHeight = height;
			newFormat = 1;
		}
	}

	if(newFormat) { 
		if(mVideoInfo) {
			if(mVideoInfo->iDecInfo) {
				TTAVCDecoderSpecificInfo *pAVC = (TTAVCDecoderSpecificInfo *)mVideoInfo->iDecInfo;				
				if(pAVC->iSpsData) {
					free(pAVC->iSpsData);
					pAVC->iSpsData = NULL;
				}

				if(pAVC->iPpsData) {
					free(pAVC->iPpsData);
					pAVC->iPpsData = NULL;
				}

				if(pAVC->iConfigData) {
					free(pAVC->iConfigData);
					pAVC->iConfigData = NULL;
				}

				if(pAVC->iData) {
					free(pAVC->iData);
					pAVC->iData = NULL;
				}

				free(pAVC);
				mVideoInfo->iDecInfo = NULL;
			}

			delete mVideoInfo;
			mVideoInfo = NULL;
		}

		mVideoInfo = new TTVideoInfo();
		mVideoInfo->iWidth = mWidth;
		mVideoInfo->iHeight = mHeight;
		mVideoInfo->iMediaTypeVideoCode = TTVideoInfo::KTTMediaTypeVideoCodeHEVC;
		TTAVCDecoderSpecificInfo *pAVC = (TTAVCDecoderSpecificInfo *)malloc(sizeof(TTAVCDecoderSpecificInfo));
		memset(pAVC, 0, sizeof(TTAVCDecoderSpecificInfo));


		//int paramSetSize = 0;
		//int numArr = 0;
	
		//if(nVpsSize > 0){
  //          paramSetSize += 1 + 2 + 2 + nVpsSize;
		//	numArr++;
  //      }
		//if(nSpsSize > 0){
  //          paramSetSize += 1 + 2 + 2 + nSpsSize;
		//	numArr++;
  //      }
		//if(nPpsSize > 0){
  //          paramSetSize += 1 + 2 + 2 + nPpsSize;
		//	numArr++;
  //      }

		//int csdSize =
  //              1 + 1 + 4 + 6 + 1 + 2 + 1 + 1 + 1 + 1 + 2 + 1 
  //              + 1 + paramSetSize;

		//pAVC->iSize = csdSize;
		//pAVC->iData = (unsigned char *)malloc(pAVC->iSize + 32);
		//unsigned char* body = pAVC->iData;
		//int i = 0;

		///*HEVCDecoderConfigurationRecord*/
		///* configurationVersion */
		//body[i++] = 0x01;		
		///*
		//* unsigned int(2) general_profile_space;
		//* unsigned int(1) general_tier_flag;
		//* unsigned int(5) general_profile_idc;
		//*/
		///* unsigned int(32) general_profile_compatibility_flags; */
		///* unsigned int(48) general_constraint_indicator_flags; */
		///* unsigned int(8) general_level_idc; */
		//memcpy(body,spsBuffer + 3, 1 + 4 + 6 + 1);
		//body += 1 + 4 + 6 + 1;

		///*
		//* bit(4) reserved = ??1111??£¤b;
		//* unsigned int(12) min_spatial_segmentation_idc;
		//*/
		//body[i++] = 0xf0;
		//body[i++] = 0;

		///*
		//* bit(6) reserved = ??111111??£¤b;
		//* unsigned int(2) parallelismType;
		//*/
		//body[i++] = 0xfe;

		///*
		//* bit(6) reserved = ??111111??£¤b;
		//* unsigned int(2) chromaFormat;
		//*/
		//body[i++] = 0;

		///*
		//* bit(5) reserved = ??11111??£¤b;
		//* unsigned int(3) bitDepthLumaMinus8;
		//*/
		//body[i++] = 0xf8;

		///*
		//* bit(5) reserved = ??11111??£¤b;
		//* unsigned int(3) bitDepthChromaMinus8;
		//*/
		//body[i++] = 0xf8;

		///* bit(16) avgFrameRate; */
		//body[i++] = 0x0;
		//body[i++] = 0x0;

		///*
		//* bit(2) constantFrameRate;
		//* bit(3) numTemporalLayers;
		//* bit(1) temporalIdNested;
		//* unsigned int(2) lengthSizeMinusOne;
		//*/
		//body[i++] = 0x03;

		////* unsigned int(8) numOfArrays; */
		//body[i++] = numArr;

		///*vps*/
		//if(nVpsSize > 0) {
		//	body[i++]   = 32;
		//	body[i++]   = 0;
		//	body[i++]   = 1;
		//	body[i++] = (nVpsSize >> 8) & 0xff;
		//	body[i++] = (nVpsSize) & 0xff;
		//	memcpy(&body[i], vpsBuffer, nVpsSize);
		//	i += nVpsSize;
		//}

		///*sps*/
		//if(nSpsSize > 0) {
		//	body[i++]   = 33;
		//	body[i++]   = 0;
		//	body[i++]   = 1;
		//	body[i++] = (nSpsSize >> 8) & 0xff;
		//	body[i++] = nSpsSize & 0xff;
		//	memcpy(&body[i],spsBuffer,nSpsSize);
		//	i +=  nSpsSize;
		//}

		///*pps*/
		//if(nPpsSize > 0) {
		//	body[i++]   = 34;
		//	body[i++]   = 0;
		//	body[i++]   = 1;
		//	body[i++] = (nPpsSize >> 8) & 0xff;
		//	body[i++] = (nPpsSize) & 0xff;
		//	memcpy(&body[i], ppsBuffer, nPpsSize);
		//	i +=  nPpsSize;
		//}
		mVideoInfo->iDecInfo = pAVC;
		mVideoInfo->iStreamId = mPID;

		TTVideoInfo* pVideoInfo = new TTVideoInfo();
		pVideoInfo->iWidth = mWidth;
		pVideoInfo->iHeight = mHeight;
		pVideoInfo->iMediaTypeVideoCode = TTVideoInfo::KTTMediaTypeVideoCodeHEVC;
		TTAVCDecoderSpecificInfo *mAVC = (TTAVCDecoderSpecificInfo *)malloc(sizeof(TTAVCDecoderSpecificInfo));
		memset(mAVC, 0, sizeof(TTAVCDecoderSpecificInfo));
		//mAVC->iSize = pAVC->iSize;
		//mAVC->iData = (unsigned char *)malloc(pAVC->iSize + 32);	
		//memcpy(mAVC->iData, pAVC->iData, pAVC->iSize);
		pVideoInfo->iDecInfo = mAVC;
		pVideoInfo->iStreamId = mPID;

		TTInt64 timeUs = fetchTimestampMeta(mAVC->iSize);

		TTBuffer* accessUnit = new TTBuffer;
		memset(accessUnit, 0, sizeof(TTBuffer));
		accessUnit->llTime = timeUs;
		accessUnit->pData = (void *)pVideoInfo;
		accessUnit->nFlag |= TT_FLAG_BUFFER_NEW_FORMAT;

		free(spsBuffer);
		free(ppsBuffer);
		free(vpsBuffer);

		return accessUnit;
	}

	free(spsBuffer);
	free(ppsBuffer);
	free(vpsBuffer);

	if(!foundFrame) {
		return NULL;
	}

	unsigned int offset = p - mBuffer;
	unsigned int frameSize = p - startPos;

    TTInt64 timeUs = fetchTimestamp(offset);
    TTBuffer* accessUnit = new TTBuffer;
	memset(accessUnit, 0, sizeof(TTBuffer));
	accessUnit->llTime = timeUs;
	accessUnit->nSize = frameSize;
	accessUnit->pBuffer = (unsigned char *)malloc(offset);
	
	if(keyFrame) {
		accessUnit->nFlag |= TT_FLAG_BUFFER_KEYFRAME;
	}

    memcpy(accessUnit->pBuffer, startPos, frameSize);
    memmove(mBuffer, mBuffer + offset, mSize - offset);
	mSize -= offset;
 
    return accessUnit;
}

TTBuffer* TTStreamQueue::dequeueAccessUnitMPEGAudio() 
{
    unsigned int nbytes = mSize;
	unsigned char *pBuf = mBuffer;
	int len, hlen; 
	unsigned int frameSize;
    int samplingRate, numChannels, bitrate, numSamples;

    if (nbytes < 4) {
        return NULL;
    }
		
	hlen = 0;

	do{
		if(hlen < 0) pBuf++;

		len = -1;
		for (unsigned int i = 0; i < nbytes - 4; i++) {
			if ( pBuf[i+0] == 0xff && (pBuf[i+1] & 0xe0) == 0xe0) {
				len = i;
				break;
			}
		}
	
		if(len < 0)	{
			return NULL;
		}
		
		pBuf += len;
		nbytes -= len;
		hlen = GetMPEGAudioFrameSize(
			pBuf, &frameSize, &samplingRate, &numChannels,
                &bitrate, &numSamples);
	}while(hlen < 0);

	mNumFrameSample = numSamples;
	mSampleRate = samplingRate;

	if(mAudioInfo == NULL) {
		mAudioInfo = new TTAudioInfo();
		mAudioInfo->iMediaTypeAudioCode = TTAudioInfo::KTTMediaTypeAudioCodeMP3;
		mAudioInfo->iChannel = numChannels;
		mAudioInfo->iSampleRate = samplingRate;
		mAudioInfo->iBitRate = bitrate;
		mAudioInfo->iStreamId = mPID;

		TTAudioInfo* pAudioInfo = new TTAudioInfo();
		pAudioInfo->iMediaTypeAudioCode = TTAudioInfo::KTTMediaTypeAudioCodeMP3;
		pAudioInfo->iChannel = numChannels;
		pAudioInfo->iSampleRate = samplingRate;
		pAudioInfo->iBitRate = bitrate;
		pAudioInfo->iStreamId = mPID;

		RangeInfo *info = &*mRangeInfos.begin();
		TTInt64 timeUs = info->mTimestampUs;

		TTBuffer* accessUnit = new TTBuffer;
		memset(accessUnit, 0, sizeof(TTBuffer));
		accessUnit->llTime = timeUs;
		accessUnit->pData = (void *)pAudioInfo;
		accessUnit->nFlag |= TT_FLAG_BUFFER_NEW_FORMAT;

		return accessUnit;
	}

    if (mSize - (pBuf - mBuffer) < frameSize) {
        return NULL;
    }

	int offset = pBuf - mBuffer + frameSize;
    TTInt64 timeUs = fetchTimestampAudio(offset);

    TTBuffer* accessUnit = new TTBuffer;
	memset(accessUnit, 0, sizeof(TTBuffer));
	accessUnit->llTime = timeUs;
	accessUnit->nSize = offset;
	accessUnit->pBuffer = (unsigned char *)malloc(offset);

    memcpy(accessUnit->pBuffer, mBuffer, offset);
    memmove(mBuffer, mBuffer + offset, mSize - offset);
	mSize -= offset;
 
    return accessUnit;
}

