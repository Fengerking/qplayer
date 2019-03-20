/*******************************************************************************
	File:		CiOSPlayer.h

	Contains:	The iOS player wrapper header file.

	Written by:	Jun Lin

	Change History (most recent first):
	2018-05-16		Jun Lin			Create file

*******************************************************************************/
#ifndef __CiOSPlayer_H__
#define __CiOSPlayer_H__

#include "UMsgMng.h"
#include "CNodeList.h"

#if TARGET_IPHONE_SIMULATOR
#define __QC_CPU_X86__
#else
#if __LP64__
#define __QC_CPU_ARMV8__
#else
#define __QC_CPU_ARMV7__
#endif // end of __LP64__
#endif // end of TARGET_IPHONE_SIMULATOR

class CBaseVideoRnd;

class CiOSPlayer : public CMsgReceiver, public CBaseObject
{
public:
    CiOSPlayer(CBaseInst * pBaseInst);
	virtual ~CiOSPlayer(void);
    
public:
    CBaseVideoRnd*	CreateRender(void* pView, RECT* pRect);
    void			DestroyRender(CBaseVideoRnd** pRnd);
    int 			Open(const char* pURL);
    
public:
    virtual int ReceiveMsg (CMsgItem* pItem);
    
private:
    
    
private:
    int                    	m_nStartTime;
    int                    	m_nIndex;
};


#endif // __CiOSPlayer_H__
