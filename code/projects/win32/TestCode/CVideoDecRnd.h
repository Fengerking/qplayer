/*******************************************************************************
	File:		CVideoDecRnd.h

	Contains:	the video decoder and render header file

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-04		Bangfei			Create file

*******************************************************************************/
#ifndef __CVideoDecRnd_H__
#define __CVideoDecRnd_H__
#include "qcPlayer.h"

#include "CQCVideoDec.h"
#include "CDDrawRnd.h"
#include "CNDKVDecRnd.h"

class CVideoDecRnd
{
public:
	CVideoDecRnd(void * hInst);
	virtual ~CVideoDecRnd(void);

	virtual void	SetView (HWND hView);
	virtual int		SetPlayer (QCM_Player * pPlay);

protected:
	virtual int		Render (unsigned char * pBuff, int nSize, long long llTime, int nFlag);	

protected:
	void *			m_hInst;

	QCM_Player *	m_pPlay;
	HWND 			m_hView;

	CQCVideoDec *	m_pDec;
	CDDrawRnd *		m_pRnd;

	CNDKVDecRnd *	m_pVDecRnd;

	QC_DATA_BUFF	m_buffData;
	QC_DATA_BUFF *	m_pBuffRnd;


public:
	static int	ReceiveData (void * pUserData, unsigned char * pBuff, int nSize, long long llTime, int nFlag);
};
#endif //__CVideoDecRnd_H__