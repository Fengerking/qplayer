/**
* File : GKMediaPlayerItf.h
* Created on : 2011-3-11
* Description : IGKMsgEnumDef.h
*/

#ifndef __GK_MSG_ENUM_DEF_H__
#define __GK_MSG_ENUM_DEF_H__

enum GKNotifyMsg
{
    ENotifyNone = 0
    , ENotifyPrepare = 1
    , ENotifyPlay = 2
    , ENotifyComplete = 3
    , ENotifyPause = 4
    , ENotifyClose = 5
    , ENotifyException = 6
    , ENotifyUpdateDuration = 7
//USE ONLY FOR  IOS
    , ENotifyAssetReaderFail = 8
    , ENotifyStop = 9
//USE END
    , ENotifyTimeReset = 10
    , ENotifySeekComplete = 11
    , ENotifyAudioFormatChanged = 12
    , ENotifyVideoFormatChanged = 13
    , ENotifyAudioFormatUpsupport = 14
    , ENotifyVideoFormatUpsupport = 15
    , ENotifyBufferingStart = 16
    , ENotifyBufferingDone = 17
    , ENotifyDNSDone = 18
    , ENotifyConnectDone = 19
    , ENotifyHttpHeaderReceived = 20
    , ENotifyPrefetchStart = 21
    , ENotifyPrefetchCompleted = 22
    , ENotifyCacheCompleted = 23
    , ENotifyMediaStartToOpen = 24
    , ENotifyMediaFirstFrame = 25
    , ENotifyMediaPreOpenStart = 26
    , ENotifyMediaPreOpenSucess = 27
    , ENotifyMediaPreOpenFailed = 28
    
    , ENotifyMediaChangedStart = 50
    , ENotifyMediaChangedSucess = 51
    , ENotifyMediaChangedFailed = 52
    
    , ENotifyMediaPreOpen = 80
    , ENotifyMediaPreRelease = 81
    , ENotifyMediaOpenLoaded = 82
    
    , ENotifySCMediaStartToOpen = 100
    , ENotifySCMediaStartToPlay = 101
    , ENotifySCMediaOpenSeek = 102
    , ENotifySCMediaPlaySeek = 103
    
    , ENotifyPlayerInfo = 200
};

enum GKPlayStatus
{
    EStatusStarting = 1
    , EStatusPlaying = 2
    , EStatusPaused = 3
    , EStatusStoped = 4
    , EStatusPrepared = 5
};

enum GKDecoderType
{
    EDecoderDefault = 0
    , EDecoderSoft = 1
};

enum GKRenderType
{
    ERenderDefault = 0
    , ERenderLR3D = 0x1
    , ERenderUD3D = 0x2
    , ERenderGlobelView = 0x4
    , ERenderSplitView = 0x8
    , ERender180View = 0x10
    , ERenderMeshDistortion = 0x20
};

enum GKSourceFlag
{
    ESourceDefault = 0x0
    , ESourceBuffer = 0x1
    , ESourcePreOpen = 0x2
    , ESourceOpenLoaded = 0x4
    , ESourceChanged = 0x8
};

#endif
