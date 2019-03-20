/**
 * File : AVTimeStamp.H
 * Created on : 2011-3-1
 * Copyright : Copyright (c) 2016 All rights reserved.
 */

#ifndef __AVTIMESTAMP_H__
#define __AVTIMESTAMP_H__

// INCLUDES
#include "GKTypedef.h"
#include "TTSysTime.h"

// CLASSES DECLEARATION
class CAVTimeStamp
{
public:
    CAVTimeStamp(){}
    
    virtual ~CAVTimeStamp(){}
    
    static long				getCurrentTime();
    
    static long			    getPeriodTime();
    
    static void             pause();
    static void             resume();
    static void             reset();
    
private:
    
    static TTUint64         m_startPts;
    static long             m_delta;
    static long             m_increase;
    
};


#endif
