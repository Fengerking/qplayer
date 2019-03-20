/*******************************************************************************
	File:		CAudioUnitRnd.h
 
	Contains:	Audio Unit render on iOS header file.
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-03-01		Jun Lin			Create file
 
******************************************************************************/
#ifndef __CAUDIO_UNIT___
#define __CAUDIO_UNIT___

#include "CMutexLock.h"
#include "CBaseAudioRnd.h"

#include <list>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudio/CoreAudioTypes.h>
#import <CoreFoundation/CoreFoundation.h>
#import "AudioSessionControl.h"

#define AU_MAXINPUTBUFFERS		3

class CAudioUnitRnd : public CBaseAudioRnd
{
    struct PCMBuffer
    {
        UInt32              mAvailableByteSize;
        UInt32              mDataByteSize;
        unsigned char*      mData;
    };

public:
	CAudioUnitRnd (CBaseInst* pInst, void* hInst);
	virtual ~CAudioUnitRnd (void);
    
public:
    virtual int		Init (QC_AUDIO_FORMAT * pFmt, bool bAudioOnly);
    virtual int		Uninit (void);
    
    virtual int		Start (void);
    virtual int		Pause (void);
    virtual int		Stop (void);
    virtual int		Flush (void);
    
    virtual int		Render (QC_DATA_BUFF * pBuff);
    
    virtual int		SetVolume (int nVolume);
    virtual int		GetVolume (void);
    
public:
    void 			onAudioSessionEvent(int ID, void *param1, void *param2);
    
private:
    int     InitDevice (void);
    bool    UninitDevice();
    
    int     UpdateFormat (QC_AUDIO_FORMAT * pFormat);
    bool    CheckFormatChange(QC_AUDIO_FORMAT * pFormat);
    bool    IsFormatChange(QC_AUDIO_FORMAT * pFormat);
    bool    IsEqual(Float64 a, Float64 b);
    void    EnableAudioSession(bool bEnable);
    int     UpdateAudioVolume(unsigned char* pBuffer, int nSize);
    
    void    AllocBuffer();
    void    ReleaseBuffer();
    int     WaitAllBufferDone(int nWaitTime);

    int		StopInternal();
private:
    static OSStatus FillBuffer(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp,
                               UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList* ioData);
    bool            onFillBuffer(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp,
                                 UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList* ioData);
    int             FillBuffer(AudioBufferList* pData);
    int             GetAvailableBufferSize();
    
    long long       GetPlayingTime();
    
    void            DumpPCM(unsigned char* pBuffer, int nSize);
    void            ReadPCM(unsigned char** pBuffer, unsigned int* nSize);
    
    

private:
    AudioComponentInstance      m_hAU;
    AudioStreamBasicDescription m_AudioFormat;

    float   m_fVolume;
	bool	m_bIsRunning;
    
    CMutexLock                  m_mtList;
    CMutexLock                  m_mtAU;
    
    std::list<PCMBuffer*>       m_lstFull;
    std::list<PCMBuffer*>       m_lstFree;
    unsigned int                m_nBufSize;
    long long                   m_llBuffTime;
    long long                   m_llRendTime;
    
    AudioSessionControl*		m_pASC;
    bool						m_bInterrupted;
    bool						m_bDestroy;
    bool						m_bWorking;
};


#endif  // end of __CAUDIO_UNIT___
