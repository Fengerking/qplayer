/*******************************************************************************
	File:		CTrackMng.h

	Contains:	The ai tracking manager header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2018-03-02		Bangfei			Create file

*******************************************************************************/
#ifndef __CTrackMng_H__
#define __CTrackMng_H__
#include "qcData.h"

#include "CBaseObject.h"
#include "CMutexLock.h"

#include "UMsgMng.h"
#include "YUVDrawBox.h"


#define FACE_TRACK_MAX_DELAY_TIME 500
#define AI_NORMAL_TYPE_VALUE    0
#define AI_WARNING_TYPE_VALUE    1

class CTrackMng : public CBaseObject, public CMsgReceiver
{
public:
	CTrackMng(CBaseInst * pBaseInst);
	virtual ~CTrackMng(void);

	virtual int		SendVideoBuff(QC_DATA_BUFF * pVideoBuff);

	virtual int		ReceiveMsg(CMsgItem * pItem);


private:
	int  InitSEICtx();
	int  UnInitSEICtx();
	int  VideoProc(void* pBuffer, char*   pPrivate, long long ullTimeStamp);
	int  TransFrameVideoBuf(QC_VIDEO_BUFF* pVideoBuff, long long ullTimeStamp);
	int  ParsePosInfo(char*   pPrivate);
	int  AddSEIInfo(QC_DATA_BUFF* pBuf);
	int  ExtractSeiPrivate(unsigned char*  pFrameData, int iFrameSize, unsigned char**  ppOutput);
	S_SEI_Info*   GetSEIInfo(long long ullTimeStamp);
protected:
	CMutexLock			m_mtFunc;
	int                 GetDetectInfoType(char*   pDetectInfo);
private:
	int                 ParseDetectJsonItem(void*  pDetectJson);



	S_DrawBoxContext   m_sDrawBox[DEFAULT_MAX_FACE_COUNT];
	int                m_iCurFrameIdx;
	int                m_iCurCachedFrameIdx;
	S_Rect            m_bbox[DEFAULT_MAX_FACE_COUNT];
	int               m_iBoxCount;
	int               m_iCurSeiCount;
	int               m_iCurInputIndex;
	int               m_iCurOutputIndex;
	QC_VIDEO_BUFF*    m_pVideoRndBuff;
	long long         m_illLastDrawTime;
	S_SEI_Info*       m_pSEIArray;
};

#endif // __CTrackMng_H__
