
#include "AVTimeStamp.h"

TTUint64 CAVTimeStamp::m_startPts = 0;
long CAVTimeStamp::m_delta = 0;
long CAVTimeStamp::m_increase = 0;

long CAVTimeStamp::getCurrentTime()
{
    if (m_startPts == 0) {
        m_startPts = GetTimeOfDay();
    }
    
    long dtime = GetTimeOfDay() - m_startPts + m_delta;
	return dtime;
}

long CAVTimeStamp::getPeriodTime()
{
	return m_increase;
}

void CAVTimeStamp::pause()
{
    m_increase = GetTimeOfDay() - m_startPts;
    m_delta += m_increase;
}

void CAVTimeStamp::resume()
{	
    m_startPts = 0;
}

void CAVTimeStamp::reset()
{
    m_startPts = 0;
    m_delta = 0;
    m_increase = 0;
}


//end of file
