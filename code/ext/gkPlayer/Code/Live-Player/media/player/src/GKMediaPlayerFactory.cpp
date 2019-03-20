/**
* File : GKMediaPlayerFactory.cpp
* Created on : 2014-03-03
* Author : linyongping
* Description : CTTMediaPlayerFactory实现文件
*/

// INCLUDES
#include "GKMediaPlayer.h"

IGKMediaPlayer* CGKMediaPlayerFactory::NewL(IGKMediaPlayerObserver* aPlayerObserver)
{
	IGKMediaPlayer* pMediaPlayer = new CGKMediaPlayer(aPlayerObserver, NULL);

	return pMediaPlayer;
}
