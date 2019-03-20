/*******************************************************************************
	File:		CAudioUnitRnd.mm
 
	Contains:	Audio Unit render on iOS implement file.
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-03-01		Jun Lin			Create file
 
 ******************************************************************************/

#include "qcErr.h"
#include "ULogFunc.h"
#include "USystemFunc.h"
#include "CAudioUnitRnd.h"
#include <AVFoundation/AVFoundation.h>

#define ASC m_pASC


void AudioSessionEvent(void* userData, int ID, void *param1, void *param2)
{
    if(userData)
    	((CAudioUnitRnd*)userData)->onAudioSessionEvent(ID, param1, param2);
}

//AudioSessionControl* CAudioUnitRnd::m_pASC = nil;
CAudioUnitRnd::CAudioUnitRnd (CBaseInst* pInst, void* hInst)
:CBaseAudioRnd(pInst, hInst)
, m_hAU(NULL)
, m_fVolume(1.0)
, m_bIsRunning (false)
, m_nBufSize(0)
, m_llBuffTime(0)
, m_llRendTime(0)
, m_bInterrupted(false)
, m_pASC(nil)
, m_bDestroy(false)
, m_bWorking(false)
{
    SetObjectName ("CAudioUnitRnd");
    
	memset(&m_AudioFormat, 0, sizeof(AudioStreamBasicDescription));
    
    ASL asl;
    asl.userData = this;
    asl.listener = AudioSessionEvent;
    
    if(!m_pASC)
        m_pASC = [[AudioSessionControl alloc] init];

    [ASC setEventCB:&asl];
}

CAudioUnitRnd::~CAudioUnitRnd (void)
{
    QCLOGI("[AU]+CAudioUnitRnd destroy");
    m_bDestroy = true;
    Uninit();
    
    //if(m_pASC && [ASC getRefCount]<=0)
    {
        QCLOGI("[AU]Release audio session controller");
        [ASC uninit];
        [ASC release];
        m_pASC = nil;
        QCLOGI("[AU]-Release audio session controller");
    }
    QCLOGI("[AU]-CAudioUnitRnd destroy");
}

int CAudioUnitRnd::Init (QC_AUDIO_FORMAT* pFmt, bool bAudioOnly)
{
    CAutoLock lock (&m_mtAU);
    
    CBaseAudioRnd::Init(pFmt, bAudioOnly);
    
    if(!pFmt || pFmt->nSampleRate<=0 || pFmt->nChannels<=0)
        return QC_ERR_ARG;
    
    Uninit();
 
    if (pFmt->nBits == 0)
        pFmt->nBits = 16;

    m_fmtAudio.nBits = pFmt->nBits;
    m_fmtAudio.nChannels = pFmt->nChannels;
    m_fmtAudio.nSampleRate = pFmt->nSampleRate;

    int nRet = UpdateFormat(pFmt);
    
    //CBaseAudioRnd::Init(pFmt, bAudioOnly);
    if(m_bWorking)
        Start();
    
    return nRet;
}

int CAudioUnitRnd::Uninit (void)
{
    CAutoLock lock (&m_mtAU);
    
    StopInternal();
    
    UninitDevice();
    
    CBaseAudioRnd::Uninit();
    
    return QC_ERR_NONE;
}

int CAudioUnitRnd::Start (void)
{
    CBaseAudioRnd::Start();
    
    m_bWorking = true;
    if(m_bIsRunning)
    {
        if(m_hAU)
            AudioOutputUnitStart(m_hAU);
        return QC_ERR_NONE;
    }
    
    int nRet = QC_ERR_NONE;
    int nTime = qcGetSysTime();
    CAutoLock lock (&m_mtAU);
    
    //EnableAudioSession(true);
    
    if(!m_hAU)
        nRet = InitDevice();
    
    if(!m_hAU)
    {
        QCLOGE("[AU]Audio unit not init");
        return QC_ERR_FAILED;
    }
    
    if(nRet == QC_ERR_FAILED)
    {
        QCLOGE("[AU]Init device failed");
        return QC_ERR_FAILED;
    }
    
    // Start playback
    OSErr nErrCode = AudioOutputUnitStart(m_hAU);
    
    if (noErr != nErrCode)
    {
        QCLOGE("[AU]Error Render starting unit: %d", nErrCode);
        return QC_ERR_FAILED;
    }
    
    m_nRndCount     = 0;
    m_bIsRunning    = true;
    m_bInterrupted 	= false;

    QCLOGI("[AU]Audio unit start use time: %d", qcGetSysTime() - nTime);
    
    return QC_ERR_NONE;
}

int CAudioUnitRnd::Pause (void)
{
    if(m_hAU)
        AudioOutputUnitStop(m_hAU);
    return CBaseAudioRnd::Pause();
}

int CAudioUnitRnd::StopInternal (void)
{
    CBaseAudioRnd::Stop();
    
    if(!m_hAU || !m_bIsRunning)
        return QC_ERR_NONE;
    
    QCLOGI("[AU]+Stop");
    int nTime = qcGetSysTime();
    
    //WaitAllBufferDone (1200);
    
    CAutoLock lock (&m_mtAU);
    
    m_bIsRunning = false;
    OSErr nErrCode = AudioOutputUnitStop(m_hAU);
    
    QCLOGI("[AU]-Stop use time %d, RC %d", qcGetSysTime()-nTime, nErrCode);
    
    m_llRendTime = 0;
    
    UninitDevice();
    
    //EnableAudioSession(false);
    
    return QC_ERR_NONE;
}

int CAudioUnitRnd::Stop(void)
{
    m_bWorking = false;
    return StopInternal();
}

int CAudioUnitRnd::Flush (void)
{
    QCLOGI("[AU]Flush audio unit, left count %d", (int)m_lstFull.size());
    
    m_nPCMLen = 0;
    m_nRndCount = 0;
    m_llPrevTime = 0;
    
    CAutoLock lockList (&m_mtList);
    
    // Full list
    std::list<PCMBuffer*>::iterator itr = m_lstFull.begin();
    
    while(itr != m_lstFull.end())
    {
        m_lstFree.push_back(*itr);
        ++itr;
    }
    m_lstFull.clear();
    
    // Free list
    itr = m_lstFree.begin();
    
    while(itr != m_lstFree.end())
    {
        PCMBuffer* buf = *itr;
        
        if(buf)
        {
            buf->mDataByteSize = 0;
            buf->mAvailableByteSize = 0;
        }
        
        ++itr;
    }

    return QC_ERR_NONE;
}

int CAudioUnitRnd::SetVolume (int nVolume)
{
    m_fVolume = nVolume/100.0;
    
    return QC_ERR_NONE;
}

int CAudioUnitRnd::GetVolume (void)
{
    return (int)m_fVolume;
}

bool CAudioUnitRnd::IsFormatChange(QC_AUDIO_FORMAT * pFormat)
{
    if (!pFormat)
        return false;

    if(m_fmtAudio.nChannels != pFormat->nChannels
       || m_fmtAudio.nBits != pFormat->nBits
       || m_fmtAudio.nSampleRate != pFormat->nSampleRate)
        return true;
    
    return false;
}

bool CAudioUnitRnd::CheckFormatChange(QC_AUDIO_FORMAT * pFormat)
{
    CAutoLock lock(&m_mtAU);
    
	if (!pFormat)
		return false;
	
    AudioStreamBasicDescription formatTemp;
    memset(&formatTemp, 0, sizeof(AudioStreamBasicDescription));
    
    formatTemp.mSampleRate       = pFormat->nSampleRate;
    formatTemp.mFormatID         = kAudioFormatLinearPCM;
    formatTemp.mFormatFlags      = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    formatTemp.mBytesPerPacket   = (pFormat->nBits / 8) * pFormat->nChannels;
    formatTemp.mFramesPerPacket  = 1;
    formatTemp.mBytesPerFrame    = (pFormat->nBits / 8) * pFormat->nChannels;
    formatTemp.mChannelsPerFrame = pFormat->nChannels;
    formatTemp.mBitsPerChannel   = pFormat->nBits;
    formatTemp.mReserved         = 0;
    
    if (formatTemp.mSampleRate <= 8000)
		formatTemp.mSampleRate = 8000;
	else if (formatTemp.mSampleRate <= 11025)
		formatTemp.mSampleRate = 11025;
	else if (formatTemp.mSampleRate <= 12000)
		formatTemp.mSampleRate = 12000;
	else if (formatTemp.mSampleRate <= 16000)
		formatTemp.mSampleRate = 16000;
	else if (formatTemp.mSampleRate <= 22050)
		formatTemp.mSampleRate = 22050;
	else if (formatTemp.mSampleRate <= 24000)
		formatTemp.mSampleRate = 24000;
	else if (formatTemp.mSampleRate <= 32000)
		formatTemp.mSampleRate = 32000;
	else if (formatTemp.mSampleRate <= 44100)
		formatTemp.mSampleRate = 44100;
	else if (formatTemp.mSampleRate <= 48000)
		formatTemp.mSampleRate = 48000;
	
	if (formatTemp.mChannelsPerFrame > 2)
		formatTemp.mChannelsPerFrame = 2;
	else if ( formatTemp.mChannelsPerFrame <= 0)
		formatTemp.mChannelsPerFrame = 1;
    
    if (IsEqual(formatTemp.mSampleRate, m_AudioFormat.mSampleRate)
        && (formatTemp.mFormatID == m_AudioFormat.mFormatID)
        && (formatTemp.mBytesPerPacket == m_AudioFormat.mBytesPerPacket)
        && (formatTemp.mBytesPerFrame == m_AudioFormat.mBytesPerFrame)
        && (formatTemp.mChannelsPerFrame == m_AudioFormat.mChannelsPerFrame)
        && (formatTemp.mBitsPerChannel == m_AudioFormat.mBitsPerChannel)
        )
    {
        return false;
    }

    memcpy(&m_AudioFormat, &formatTemp, sizeof(AudioStreamBasicDescription));
    
    return true;
}

int CAudioUnitRnd::UpdateFormat (QC_AUDIO_FORMAT * pFormat)
{
    CAutoLock lock(&m_mtAU);
    
    if(CheckFormatChange(pFormat))
    {
        //
        int nSizePerSample = m_AudioFormat.mChannelsPerFrame * m_AudioFormat.mBitsPerChannel / 8;
        m_nSizeBySec = m_AudioFormat.mSampleRate * nSizePerSample;
        
        m_nBufSize = m_nSizeBySec / 10;
        
        //m_nBufSize = 4096;
        if (m_nBufSize < 4096)
            m_nBufSize = 4096;
        
        // render buffer size must be 16 bytes align
        //m_nBufSize = (m_nBufSize + 16 - 1) / 16 * 16;

        QCLOGI("[AU]Audio render chunk size %d, step is about %d ms, size of second %d", m_nBufSize, m_nBufSize*1000/m_nSizeBySec, m_nSizeBySec);

        return InitDevice();
    }
    
    return QC_ERR_NONE;
}

int CAudioUnitRnd::InitDevice(void)
{
    CAutoLock lock (&m_mtAU);
    
    if(m_hAU)
        return QC_ERR_STATUS;
    
    OSErr nErrCode = noErr;
    AudioComponentDescription acd;
    acd.componentType = kAudioUnitType_Output;

#if 1
    // for iOS
    acd.componentSubType = kAudioUnitSubType_RemoteIO;
#else
    //for Mac OS
    acd.componentSubType = kAudioUnitSubType_DefaultOutput;
#endif

    acd.componentManufacturer = kAudioUnitManufacturer_Apple;
    acd.componentFlags = 0;
    acd.componentFlagsMask = 0;
    
    // get output
    AudioComponent ac = AudioComponentFindNext(NULL, &acd);

    if(!ac)
    {
        QCLOGE("[AU]Can't find default output");
        return QC_ERR_FAILED;
    }
    
    // Create a new unit based on this that we'll use for output
    nErrCode = AudioComponentInstanceNew(ac, &m_hAU);
    
    if (!m_hAU)
    {
        QCLOGE("[AU]Error creating unit: %d", nErrCode);
        return QC_ERR_FAILED;
    }
    
    // disable IO for recording
    UInt32 flag = 0;
    AudioUnitElement kInputBus = 1;
    nErrCode = AudioUnitSetProperty(m_hAU, 
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Input, 
                                  kInputBus,
                                  &flag, 
                                  sizeof(flag));
    if (noErr != nErrCode)
    {
        QCLOGE("[AU]Error setting kAudioOutputUnitProperty_EnableIO: %d", nErrCode);
        return QC_ERR_FAILED;
    }
    
    // Affect fill buffer callback's size : PCMBufferList *ioData
//    NSTimeInterval preferredBufferSize = 0.0232;
//    [[AVAudioSession sharedInstance] setPreferredIOBufferDuration:preferredBufferSize error:nil];
    
    // Set our tone rendering function on the unit
    AURenderCallbackStruct input;
    input.inputProc = FillBuffer;
    input.inputProcRefCon = this;
    nErrCode = AudioUnitSetProperty(m_hAU, 
                                   kAudioUnitProperty_SetRenderCallback,
                                   kAudioUnitScope_Input,
                                   0, 
                                   &input, 
                                   sizeof(input));
    if (noErr != nErrCode)
    {
        QCLOGE("[AU]Error setting callback: %d", nErrCode);
        return QC_ERR_FAILED;
    }
    
    nErrCode = AudioUnitInitialize(m_hAU);
    
    if (noErr != nErrCode)
    {
        QCLOGE("[AU]Error initializing unit: %d", nErrCode);
        AudioUnitUninitialize(m_hAU);
        AudioComponentInstanceDispose(m_hAU);
        m_hAU = NULL;
        return QC_ERR_FAILED;
    }

    nErrCode = AudioUnitSetProperty (m_hAU,
                                     kAudioUnitProperty_StreamFormat,
                                     kAudioUnitScope_Input,
                                     0,
                                     &m_AudioFormat,
                                     sizeof(AudioStreamBasicDescription));
    
    QCLOGI("[AU]SetFormat mSampleRate:%e, mFormatID:%u, mBytesPerPacket:%u, mBytesPerFrame:%u, mChannelsPerFrame:%u, mBitsPerChannel:%u",
           m_AudioFormat.mSampleRate, (unsigned int)m_AudioFormat.mFormatID, (unsigned int)m_AudioFormat.mBytesPerPacket,
           (unsigned int)m_AudioFormat.mBytesPerFrame, (unsigned int)m_AudioFormat.mChannelsPerFrame, (unsigned int)m_AudioFormat.mBitsPerChannel);
    
    if (noErr != nErrCode)
    {
        QCLOGE("[AU]Error setting stream format: %d \n", nErrCode);
        AudioUnitUninitialize(m_hAU);
        AudioComponentInstanceDispose(m_hAU);
        m_hAU = NULL;
        return QC_ERR_FAILED;
    }
    
    AllocBuffer();
    
    m_llBuffTime = 0;
    
	return QC_ERR_NONE;
}


bool CAudioUnitRnd::UninitDevice(void)
{
	if(!m_hAU)
        return false;
    
    bool bRet = true;
    
    OSErr nErrCode = noErr;
    m_bIsRunning = false;
    
    //WaitAllBufferDone(2000);
    ReleaseBuffer();
    
    //
    CAutoLock lock (&m_mtAU);

    // Must call stop before AudioUnitUninitialize since the stop in RenderProc not break AURemoteIO immediate
    nErrCode = AudioOutputUnitStop(m_hAU);
        
    if (noErr != nErrCode)
    {
        QCLOGE("[AU]AudioOutputUnitStop failed: %d", nErrCode);
        return QC_ERR_FAILED;
    }

    nErrCode = AudioUnitUninitialize(m_hAU);
    
    if (noErr != nErrCode)
    {
        QCLOGE("[AU]AudioUnitUninitialize failed: %d", nErrCode);
        bRet = false;
    }

    nErrCode = AudioComponentInstanceDispose(m_hAU);
    
    if (noErr != nErrCode)
    {
        QCLOGE("[AU]AudioComponentInstanceDispose failed: %d", nErrCode);
        bRet = false;
    }

    m_hAU = NULL;
    
    QCLOGI("[AU]Audio unit uninit success");

	return bRet;
}


OSStatus CAudioUnitRnd::FillBuffer(void *inRefCon,
                                  AudioUnitRenderActionFlags 	*ioActionFlags,
                                  const AudioTimeStamp			*inTimeStamp,
                                  UInt32 						inBusNumber,
                                  UInt32 						inNumberFrames,
                                  AudioBufferList				*ioData)
{
    CAudioUnitRnd* pAudioUnit = (CAudioUnitRnd *)inRefCon;
    
    if(!pAudioUnit)
        return -1;
    
    bool ret = pAudioUnit->onFillBuffer(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
    
    if (!ret)
        return -1;
    
    pAudioUnit->UpdateAudioVolume((unsigned char *)ioData->mBuffers[0].mData, ioData->mBuffers[0].mDataByteSize);
    
    return noErr;
}

int CAudioUnitRnd::Render (QC_DATA_BUFF * pBuff)
{
    if (pBuff == NULL || pBuff->pBuff == NULL)
        return QC_ERR_ARG;
    
    {
        // Maybe init audio unit in Start/Stop due to interruption, so wait it
        CAutoLock lock (&m_mtAU);
        if(m_bInterrupted) // drop the frame for live stream
            return QC_ERR_NONE;
        if(!m_bIsRunning && m_hAU)
            return QC_ERR_RETRY;
    }
    
    //QCLOGI("[AU]Audio data comes, %lld, %d, type %d, size %d", pBuff->llTime, m_nRndCount, pBuff->uBuffType, pBuff->uSize);
    
    if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT && IsFormatChange((QC_AUDIO_FORMAT *)pBuff->pFormat))
    {
        QCLOGI("[AU]Audio format changed");
        Init ((QC_AUDIO_FORMAT *)pBuff->pFormat, m_bAudioOnly);
        Start ();
        m_nRndCount++;
        return QC_ERR_NONE;
    }
    
    if (m_lstFree.size() <= 0)
    {
        qcSleep (5000);
        //QCLOGW("[AU]%p Full buffer count %d, player is %s", this, (int)m_lstFull.size(), m_bIsRunning?"running":"stopped");
        return QC_ERR_RETRY;
    }
    
    m_nRndCount++;
    
    //ReadPCM(&(pBuff->pBuff), &pBuff->uSize);
    //DumpPCM(pBuff->pBuff, pBuff->uSize);

    {
        CAutoLock lockList (&m_mtList);
        m_llPrevTime = pBuff->llTime;
    }
    
    PCMBuffer*    pHead = m_lstFree.front();
    unsigned char *	pDst = (unsigned char*)pHead->mData;
    unsigned char *	pData = NULL;
    int				nSize = 0;
    
    // Copy the left PCM data first.
    if (m_pPCMBuff != NULL && m_nPCMLen > 0)
    {
        if (pHead->mDataByteSize + m_nPCMLen > m_nBufSize)
        {
            int nCopySize = m_nBufSize - pHead->mDataByteSize;
            memcpy (pDst + pHead->mDataByteSize, m_pPCMBuff, nCopySize);
            pHead->mDataByteSize += nCopySize;
            pHead->mAvailableByteSize += nCopySize;
            
            m_pPCMBuff += nCopySize;
            m_nPCMLen -= nCopySize;
            
            m_llBuffTime += (nCopySize * 1000) / m_nSizeBySec;
        }
        else
        {
            memcpy (pDst + pHead->mDataByteSize, m_pPCMBuff, m_nPCMLen);
            pHead->mDataByteSize += m_nPCMLen;
            pHead->mAvailableByteSize += m_nPCMLen;
            m_pPCMBuff = m_pPCMData;
            m_nPCMLen = 0;
        }
    }
    
    if (pHead->mDataByteSize < m_nBufSize)
    {
        m_llBuffTime = pBuff->llTime;
        
        pData = pBuff->pBuff;
        nSize = pBuff->uSize;
        
        if (pHead->mDataByteSize + nSize > m_nBufSize)
        {
            int nCopySize = m_nBufSize - pHead->mDataByteSize;
            memcpy (pDst + pHead->mDataByteSize, pData, nCopySize);
            
            if (m_pPCMData == NULL)
            {
                m_nPCMSize = m_AudioFormat.mSampleRate;
                if (m_nPCMSize < nSize)
                    m_nPCMSize = nSize;
                m_pPCMData = new unsigned char[m_nPCMSize];
                m_pPCMBuff = m_pPCMData;
                m_nPCMLen = 0;
            }
            
            if (m_nPCMLen == 0)
            {
                m_nPCMLen = pHead->mDataByteSize + nSize - m_nBufSize;
                memcpy (m_pPCMBuff, pData + nCopySize, m_nPCMLen);
            }
            else
            {
                m_pPCMBuff += nCopySize;
                m_nPCMLen  -= nCopySize;
            }
            
            pHead->mDataByteSize        = m_nBufSize;
            pHead->mAvailableByteSize   = m_nBufSize;
        }
        else
        {
            memcpy (pDst + pHead->mDataByteSize, pData, nSize);
            pHead->mDataByteSize        += nSize;
            pHead->mAvailableByteSize   += nSize;
            if (m_nPCMLen > 0)
            {
                m_pPCMBuff += nSize;
                m_nPCMLen  -= nSize;
            }
        }
    }
    
    if (pHead->mDataByteSize == m_nBufSize)
    {
        // lock in short time
        if (pHead != NULL)
        {
            CAutoLock lockList (&m_mtList);
            m_lstFree.pop_front();
        }
        
        if (pHead != NULL)
        {
            CAutoLock lockList (&m_mtList);
            m_lstFull.push_back(pHead);
            //DumpPCM(pHead->mData, pHead->mDataByteSize);
            
//            QCLOGI("[AU]Input PCM data, time %lld, size %d, full count %d, free count %d",
//                               pBuff->llTime, (unsigned int)pHead->mDataByteSize, (int)m_lstFull.size(), (int)m_lstFree.size());
        }
    }
    
    if (m_nPCMLen >= (int)m_nBufSize)
    {
        //QCLOGI("[AU]Buffer is full");
        return QC_ERR_RETRY;
    }
    
    return QC_ERR_NONE;
}

bool CAudioUnitRnd::onFillBuffer(AudioUnitRenderActionFlags *ioActionFlags,
                                const AudioTimeStamp 		*inTimeStamp, 
                                UInt32 						inBusNumber, 
                                UInt32 						inNumberFrames, 
                                AudioBufferList 			*ioData)
{
	if(!ioData)
		return false;
    
    //QCLOGI("[AU]%p Want fill buffer size %u", this, (unsigned int)ioData->mBuffers[0].mDataByteSize);
    
    if(m_lstFull.size() <= 0)
    {
        //QCLOGW("[AU]PCM data is empty");
        memset ((unsigned char*)ioData->mBuffers[0].mData, 0, ioData->mBuffers[0].mDataByteSize);
        return true;
    }

    int nSizeFilled = FillBuffer(ioData);
    
    if(nSizeFilled == 0)
        return true;
    
    if (m_pClock != NULL)
    {
        //m_llRendTime += nSizeFilled * 1000 / m_nSizeBySec;
        long long llPlayingTime = GetPlayingTime();
        m_pClock->SetTime(llPlayingTime);
        //QCLOGI("[AU]Playing time %lld, render count %d", llPlayingTime, m_nRndCount);
    }
    
    //m_nRndCount++;
    
#if 0
    static int lastFill = qcGetSysTime();
    static long long wallClock = qcGetSysTime() - lastFill;
    
    wallClock += (qcGetSysTime()-lastFill);
    QCLOGI("[AU]Output PCM data, playing time %lld, wall clock %lld, diff %lld, %d, full count %d, free count %d",
           m_llRendTime, wallClock, wallClock-m_llRendTime, (int)pReady->mDataByteSize, (int)m_lstFull.size(), (int)m_lstFree.size());
    lastFill = qcGetSysTime();
#endif
    
    return true;
}

int CAudioUnitRnd::FillBuffer(AudioBufferList* pData)
{
    CAutoLock lockList (&m_mtList);
    
    int nSizeFilled = 0;
    int nSizeWant = pData->mBuffers[0].mDataByteSize;
    unsigned char* pDst = (unsigned char*)pData->mBuffers[0].mData;
    
    if(GetAvailableBufferSize() < nSizeWant)
    {
        memset (pDst, 0, nSizeWant);
        //QCLOGW("[AU]No enough PCM data, want %d", (int)pData->mBuffers[0].mDataByteSize);
        return 0;
    }
    
    while(nSizeFilled < nSizeWant)
    {
        PCMBuffer* pReady   = m_lstFull.front();
        int nSizeLeft       = nSizeWant - nSizeFilled;
        
        if(pReady)
        {
            int nSizeCopied = pReady->mDataByteSize - pReady->mAvailableByteSize;
            
            if(pReady->mAvailableByteSize >= nSizeLeft)
            {
                memcpy(pDst+nSizeFilled, pReady->mData+nSizeCopied, nSizeLeft);
                pReady->mAvailableByteSize -= nSizeLeft;
                nSizeFilled += nSizeLeft;
            }
            else
            {
                memcpy(pDst+nSizeFilled, pReady->mData+nSizeCopied, pReady->mAvailableByteSize);
                nSizeFilled += pReady->mAvailableByteSize;
                pReady->mAvailableByteSize = 0;
            }
            
            if(pReady->mAvailableByteSize == 0)
            {
                // reuse the buffer
                //QCLOGI("Reuse buffer, size %d, full %d, free %d", (int)pReady->mDataByteSize, (int)m_lstFull.size(), (int)m_lstFree.size());
                pReady->mDataByteSize = 0;
                m_lstFull.pop_front();
                m_lstFree.push_back(pReady);
            }
        }
    }
    
    //DumpPCM(pDst, nSizeWant);
    
    return nSizeFilled;
}

long long CAudioUnitRnd::GetPlayingTime()
{
    CAutoLock lockList (&m_mtList);
    
    return m_llPrevTime - (GetAvailableBufferSize()*1000 / m_nSizeBySec);
}

int CAudioUnitRnd::GetAvailableBufferSize()
{
    CAutoLock lockList (&m_mtList);
    
    int nTotalSize = 0;

    std::list<PCMBuffer*>::iterator itr = m_lstFull.begin();
    
    while(itr != m_lstFull.end())
    {
        PCMBuffer* buf = *itr;
        
        if(buf)
        {
            nTotalSize += buf->mAvailableByteSize;
        }
        
        ++itr;
    }
    
    return nTotalSize;
}

int CAudioUnitRnd::UpdateAudioVolume(unsigned char* pBuffer, int nSize)
{
    int nAudioValume = m_fVolume * 100;
    
    if ((nAudioValume >= 0 && nAudioValume < 100) || (nAudioValume > 100 && nAudioValume <= 200))
	{
		if (nAudioValume == 0)
		{
			memset (pBuffer, 0, nSize);
		}
		else
		{
			if (m_AudioFormat.mBitsPerChannel == 16)
			{
				int nTmp;
				short * pSData;
				pSData = (short *)pBuffer;
				for (int i = 0; i < nSize; i+=2)
				{
					nTmp = ((*pSData) * nAudioValume / 100);
					
					if(nTmp >= -32768 && nTmp <= 32767)
					{
						*pSData = (short)nTmp;
					}
					else if(nTmp < -32768)
					{
						*pSData = -32768;
					}
					else if(nTmp > 32767)
					{
						*pSData = 32767;
					}
                    
					pSData++;
				}
			}
			else if (m_AudioFormat.mBitsPerChannel == 8)
			{
				int nTmp;
				char * pCData;
				pCData = (char *)pBuffer;
				for (int i = 0; i < nSize; i++)
				{
					nTmp = (*pCData) * nAudioValume / 100;
                    
					if(nTmp >= -256 && nTmp <= 255)
					{
						*pCData = (char)nTmp;
					}
					else if(nTmp < -256)
					{
						*pCData = (char)-256;
					}
					else if(nTmp > 255)
					{
						*pCData = (char)255;
					}
                    
					pCData++;
				}
			}
		}
	}
    
    return QC_ERR_NONE;
}

bool CAudioUnitRnd::IsEqual(Float64 a, Float64 b)
{
    const static Float64 V_RANGE = 0.000001;
    if (((a - b) > -V_RANGE)
        && ((a - b) < V_RANGE) ) {
        return true;
    }
    return false;
}


void CAudioUnitRnd::AllocBuffer()
{
    ReleaseBuffer();
    
    CAutoLock lock(&m_mtList);
    
    for(int i=0; i < AU_MAXINPUTBUFFERS; i++)
    {
        PCMBuffer *pAudioUbuffer = new PCMBuffer();
        pAudioUbuffer->mData = (unsigned char*)malloc (m_nBufSize);
        pAudioUbuffer->mDataByteSize = 0; // size of real PCM data size
        pAudioUbuffer->mAvailableByteSize = 0;
        memset (pAudioUbuffer->mData, 0, m_nBufSize);
        m_lstFree.push_back(pAudioUbuffer);
    }
}

void CAudioUnitRnd::ReleaseBuffer()
{
//    if (m_lstFull.size() > 0 || m_lstFree.size())
//        WaitAllBufferDone (1000);
    
    CAutoLock lock(&m_mtList);
    
    while (0 < m_lstFree.size())
    {
        PCMBuffer *pAudioUbuffer = m_lstFree.front();
        
        if (NULL != pAudioUbuffer)
        {
            if(pAudioUbuffer->mData)
            {
                free(pAudioUbuffer->mData);
                pAudioUbuffer->mData = NULL;
            }
            
            delete pAudioUbuffer;
        }
        
        m_lstFree.pop_front();
    }
    
    while (0 < m_lstFull.size())
    {
        PCMBuffer *pAudioUbuffer = m_lstFull.front();
        
        if (NULL != pAudioUbuffer)
        {
            if(pAudioUbuffer->mData)
            {
                free(pAudioUbuffer->mData);
                pAudioUbuffer->mData = NULL;
            }

            delete pAudioUbuffer;
        }
        
        m_lstFull.pop_front();
    }
}

int CAudioUnitRnd::WaitAllBufferDone (int nWaitTime)
{
    int nStartTime = qcGetSysTime ();
    
    while (m_lstFree.size () < AU_MAXINPUTBUFFERS)
    {
        qcSleep (2000);
        
        if (qcGetSysTime () - nStartTime >= nWaitTime)
        {
            QCLOGW ("[AU]The count is %d", (int)m_lstFree.size ());
            return nWaitTime;
        }
    }
    
    return qcGetSysTime () - nStartTime;
}

void CAudioUnitRnd::EnableAudioSession(bool bEnable)
{
    CAutoLock lock (&m_mtAU);
    if(ASC)
       [ASC enableAudioSession:bEnable force:NO];
}

void CAudioUnitRnd::ReadPCM(unsigned char** pBuffer, unsigned int* nSize)
{
#if 0
    static int              nCurrPos = 0;
    static unsigned char*   pMockBuffer = NULL;
    static int              nMockSize = 0;
    static FILE*            hFile = NULL;
    
    if(nMockSize < *nSize)
    {
        nMockSize = *nSize;
        
        if(pMockBuffer)
        {
            delete []pMockBuffer;
        }
        
        pMockBuffer = new unsigned char[nMockSize];
    }
    
     if (NULL == hFile)
    {
        char szTmp[256];
        qcGetAppPath(NULL, szTmp, 256);
        strcat(szTmp, "PCM");
        hFile = fopen(szTmp, "rb");
    }
    
    if ((NULL != hFile) && (nMockSize > 0))
    {
        fseek(hFile, nCurrPos, SEEK_SET);
        int nCount = (int)fread(pMockBuffer, 1, nMockSize, hFile);
        
        *pBuffer = pMockBuffer;
        *nSize = nCount;
    }
    
#endif
}

void CAudioUnitRnd::DumpPCM(unsigned char* pBuffer, int nSize)
{
#if 0
    static FILE* hFile = NULL;
    static void* pSelf = NULL;
    
    if (pSelf != this)
    {
        pSelf = this;
    
        if (NULL != hFile)
        {
            fclose(hFile);
            hFile = NULL;
        }
    }
    
    if (NULL == hFile)
    {
        char szTmp[256];
        qcGetAppPath(NULL, szTmp, 256);
        strcat(szTmp, "PCM");
        hFile = fopen(szTmp, "wb");
    }
    
    if ((NULL != hFile) && (nSize > 0))
    {
        fwrite(pBuffer, nSize, 1, hFile);
        fflush(hFile);
    }
#endif
}

void CAudioUnitRnd::onAudioSessionEvent(int ID, void *param1, void *param2)
{
    int nRC = QC_ERR_NONE;
    QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, ID);

    if(m_bDestroy)
        return;
    
    CAutoLock lock (&m_mtAU);
    
    if(ID == ASE_INTERRUPTION_BEGIN)
    {
        if(m_bIsRunning)
        {
            QCLOGI("[AU]BEGIN INTERRUPTION");
            m_bInterrupted = true;
            if(ASC)
                [ASC enableAudioSession:NO force:YES];
            StopInternal();
        }
    }
    else if(ID == ASE_INTERRUPTION_END || ID == ASE_APP_RESUME)
    {
        if(m_bInterrupted)
        {
            QCLOGI("%s", (ID == ASE_INTERRUPTION_END)?"[AU]END INTERRUPTION":"[AU]APP RESUME");
            if(ASC)
            {
                bool bRet = [ASC enableAudioSession:YES force:YES];
                if(bRet)
                {
                    nRC = Start();
                    if(nRC == QC_ERR_NONE)
                    	m_bInterrupted = false;
                }
            }
        }
    }
}
