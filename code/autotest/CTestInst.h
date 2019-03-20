/*******************************************************************************
	File:		CTestInst.h

	Contains:	the buffer trace class header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#ifndef __CTestInst_H__
#define __CTestInst_H__
#ifdef __QC_OS_NDK__
#include <jni.h>
#endif // __QC_OS_NDK__

#include "qcData.h"

#ifdef __QC_OS_WIN32__
#include "CWndSlider.h"
#endif // __QC_OS_WIN32__

#ifdef __QC_OS_NDK__
#include "CNDKVideoRnd.h"
#include "CNDKVDecRnd.h"
#include "COpenSLESRnd.h"
#include "CNDKAudioRnd.h"
#endif // __QC_OS_NDK__

#ifdef __QC_OS_IOS__
class CTestAdp;
#endif

#include "CMutexLock.h"
#include "CNodeList.h"
#include "CThreadWork.h"

typedef enum {
	QCTEST_INFO_Item	= 1,
	QCTEST_INFO_Func	= 2,
	QCTEST_INFO_Msg		= 3,
	QCTEST_INFO_Err		= 4,
	QCTEST_INFO_MAX = 0X7FFFFFFF
}QCTEST_InfoType;

typedef struct
{
	void *	pTask;
	int		nInfoType;
	char *	pInfoText;
} QCTEST_InfoItem;

class CTestBase;

class CTestInst
{
public:
	CTestInst(void);
    virtual ~CTestInst(void);

	virtual int		AddInfoItem(void * pTask, int nType, char * pText);
	virtual int		ShowInfoItem(void);

public:
	CMutexLock		m_mtMain;
	bool			m_bExitTest;
	CTestBase *		m_pTestMng;
	int				m_nItemIndex;

#ifdef __QC_OS_WIN32__
	HWND		m_hWndMain;
	HWND		m_hWndVideo;
	CWndSlider* m_pSlidePos;

	HWND		m_hLBItem;
	HWND		m_hLBFunc;
	HWND		m_hLBMsg;
	HWND		m_hLBErr;
#elif defined __QC_OS_NDK__
	JavaVM * 			m_pJVM;
	JNIEnv *			m_pENV;
	jobject				m_hWndVideo;

	CBaseAudioRnd *		m_pRndAudio;
	CNDKVideoRnd *		m_pRndVideo;
	CNDKVDecRnd *		m_pRndVidDec;
#elif __QC_OS_IOS__
    void*				m_hWndVideo;
    CTestAdp*			m_pAdp;
#endif //__QC_OS_WIN32__

	CMutexLock						m_mtInfo;
	CObjectList<QCTEST_InfoItem>	m_lstInfo;
	CObjectList<QCTEST_InfoItem>	m_lstHist;

};

#endif //__CTestInst_H__
