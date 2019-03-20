/**
* File : TTWinVideoSink.h  
* Created on : 2014-9-1
* Author : yongping.lin
* Copyright : Copyright (c) 2016 GoKu Software Ltd. All rights reserved.
* Description : TTWinVideoSink定义文件
*/

#ifndef __TT_WIN_VIDEO_SINK_H__
#define __TT_WIN_VIDEO_SINK_H__

#include "stdio.h"

// INCLUDES
#include "TTBaseVideoSink.h"
#include "GKCritical.h"
#include <ddraw.h>
#include <mmsystem.h>

// CLASSES DECLEARATION
class CTTWinVideoSink : public TTCBaseVideoSink
{
public:
	/**
	* \fn							CTTAudioSink(ITTSinkDataProvider* aDataProvider, ITTPlayRangeObserver& aObserver);
	* \brief						构造函数
	* \param[in] aDataProvider		数据提供者接口引用
	* \param[in] aObserver			回调
	*/
	CTTWinVideoSink(CTTSrcDemux* aSrcMux, TTCBaseAudioSink* aAudioSink, GKDecoderType aDecoderType);

	/**
	* \fn							~CTTAudioSink();
	* \brief						析构函数
	*/
	virtual ~CTTWinVideoSink();


public://from ITTDataSink

	virtual TTInt				render();

	virtual	TTInt				stop();

	virtual TTInt				newVideoView();

	virtual TTInt				closeVideoView();

protected:
	virtual TTInt				checkSurfaceLost();
	virtual void				updateRect();
	virtual void				FlushWnd();

	LPDIRECTDRAW7				mDD;				
	LPDIRECTDRAWSURFACE7		mDDSPrimary;		

	LPDIRECTDRAWSURFACE7		mDDSOffScr;		
	DDSURFACEDESC2				mOffScrSurfDesc;	

	TTBool						mSurfaceLost;

	TTInt						mLeftOffset;
	TTInt						mTopOffset;
	TTInt						mDrawWidth;
	TTInt						mDrawHeight;
};

#endif // __TT_WIN_AUDIO_SINK_H__