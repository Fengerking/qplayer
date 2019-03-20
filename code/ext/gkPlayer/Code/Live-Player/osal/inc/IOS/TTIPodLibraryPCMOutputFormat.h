/**
* File : TTPureDataOutputFormat.h  
* Description : TTPureDataOutputFormat定义文件
*/
#ifndef __TT_PURE_DATA_OUTPUT_FORMAT_H__
#define __TT_PURE_DATA_OUTPUT_FORMAT_H__
class TTPureDataOutputFormat
{
public:
    TTChar*     iDataPtr;
    TTInt       iCurOffset;
    TTInt       iTotalLen;
    TTUint      iStartTime;
    TTUint      iStopTime;
    
    TTPureDataOutputFormat() : iDataPtr(NULL), iCurOffset(0), iTotalLen(0), iStartTime(0), iStopTime(0) {}
};
#endif
