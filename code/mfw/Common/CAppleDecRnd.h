/*******************************************************************************
	File:		CAppleDecRnd.h

	Contains:	the video decoder and render header file

	Written by:	Jun Lin

	Change History (most recent first):
	2017-03-01		Jun			Create file

*******************************************************************************/
#ifndef __CAppleDecRnd_H__
#define __CAppleDecRnd_H__

#include "CBaseVideoRnd.h"

class CBaseVideoDec;
class CBaseVideoRnd;

class CAppleDecRnd : public CBaseVideoRnd
{
public:
	CAppleDecRnd(void * hInst);
	virtual ~CAppleDecRnd(void);

	virtual int SetView (void * hView, RECT * pRect);
    virtual int	Render (QC_DATA_BUFF * pBuff);
    
    virtual int Start (void);
    virtual int Pause (void);
    virtual int	Stop (void);
    
    
protected:
	CBaseVideoDec*	m_pDec;
	CBaseVideoRnd*	m_pRnd;

	QC_DATA_BUFF	m_buffData;
	QC_DATA_BUFF *	m_pBuffRnd;
};
#endif //__CAppleDecRnd_H__
