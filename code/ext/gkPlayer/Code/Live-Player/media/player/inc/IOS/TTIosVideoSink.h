/**
* File : TTAndroidVideoSink.h  
* Created on : 2014-11-5
* Author : yongping.lin
* Description : CTTAndroidVideoSinkþ
*/

#ifndef __TT_IOS_VIDEO_SINK_H__
#define __TT_IOS_VIDEO_SINK_H__

#include <stdio.h>
#include <unistd.h>

#include "TTBaseVideoSink.h"
#include "GKCritical.h"
//#include "TTGLRenderBase.h"

class TTGLRenderBase;
// CLASSES DECLEARATION
class CTTIosVideoSink : public TTCBaseVideoSink
{
public:
	CTTIosVideoSink(CTTSrcDemux* aSrcMux, TTCBaseAudioSink* aAudioSink, GKDecoderType aDecoderType);
	virtual ~CTTIosVideoSink();

public:
	virtual TTInt				render();
    
    virtual TTInt				redraw();

	virtual	TTInt				setView(void * pView);

	virtual TTInt				newVideoView();

	virtual TTInt				closeVideoView();

	virtual void				checkCPUFeature();
    
    virtual	TTInt				stop();
    
    virtual void                setRendType(TTInt aRenderType);
    
    virtual void                setMotionEnable(bool aEnable);
    
    virtual void                setTouchEnable(bool aEnable);
protected:
	virtual TTInt				renderYUV();

protected:

	RGKCritical					mCriView;
    
    TTGLRenderBase*             m_pRender;
    
    void*                       m_Context;

};

#endif // __TT_WIN_AUDIO_SINK_H__
