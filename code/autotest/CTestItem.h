/*******************************************************************************
	File:		CTestItem.h

	Contains:	the buffer trace class header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#ifndef __CTestItem_H__
#define __CTestItem_H__
#include "qcData.h"
#include "CTestBase.h"

#include "CTestPlayer.h"

// the test item command
typedef enum {
	QCTEST_CMD_OPEN,
	QCTEST_CMD_CLOSE,
	QCTEST_CMD_RUN,
	QCTEST_CMD_PAUSE,
	QCTEST_CMD_STOP,
	QCTEST_CMD_SEEK,
	QCTEST_CMD_SETVIEW,
	QCTEST_CMD_SETVOLUME,
	QCTEST_CMD_SETPARAM,
	QCTEST_CMD_GETPARAM,
	QCTEST_CMD_EXIT,
	QCTEST_CMD_MAX = 0X7FFFFFFF
}QCTEST_Command;

// the test item command name
static const char* QCTEST_Command_Name[] = {"OPEN",
                                            "CLOSE",
                                            "RUN",
                                            "PAUSE",
                                            "STOP",
                                            "SEEK",
                                            "SETVIEW",
                                            "SETVOLUME",
                                            "SETPARAM",
                                            "GETPARAM",
                                            "EXIT"};

class CTestTask;

class CTestItem : public CTestBase
{
public:
	CTestItem(CTestTask * pTask, CTestInst * pInst);
    virtual ~CTestItem(void);

	virtual int		AddItem(char * pItem);
	virtual void	SetPlayer(CTestPlayer * pPlayer) { m_pPlayer = pPlayer; }

	virtual int		ScheduleTask(void);
	virtual int		ExecuteCmd(void);

public:
	int				m_nItemIndex;

protected:
	CTestTask *		m_pTestTask;
	CTestPlayer *	m_pPlayer;
	char *			m_pTxtItem;

	int				m_nTime;

	QCTEST_Command	m_nCmd;
	int				m_nParamID;
	int				m_nValue;
    char			m_szValue[1024];
	long long		m_llValue;
	void *			m_pValue;
	RECT			m_rtZoom;

protected:
	// #define    QCPLAY_PID_Disable_Video			QC_PLAY_BASE + 0X03
	// #define    QCPLAY_PID_StreamPlay				QC_PLAY_BASE + 0X06
	// #define    QCPLAY_PID_AudioTrackPlay			QC_PLAY_BASE + 0X08
	// #define    QCPLAY_PID_Zoom_Video				QC_PLAY_BASE + 0X11
	// #define    QCPLAY_PID_Download_Pause			QC_PLAY_BASE + 0X31
};

#endif //__CTestItem_H__
