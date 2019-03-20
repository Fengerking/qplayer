/*******************************************************************************
	File:		CTestItem.cpp

	Contains:	the buffer trace r class implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#include <stdlib.h>
#include "qcErr.h"

#include "CTestItem.h"
#include "CTestTask.h"
#include "USystemFunc.h"

CTestItem::CTestItem(CTestTask * pTask, CTestInst * pInst)
	: CTestBase(pInst)
	, m_pTestTask(pTask)
	, m_pPlayer(NULL)
	, m_pTxtItem(NULL)
	, m_nCmd(QCTEST_CMD_MAX)
	, m_nParamID(0)
	, m_nValue(0)
	, m_llValue(0)
	, m_pValue(NULL)
	, m_nTime(0)
{
	m_nItemIndex = m_pInst->m_nItemIndex++;
	memset(&m_rtZoom, 0, sizeof(m_rtZoom));
}

CTestItem::~CTestItem(void)
{
	switch (m_nCmd)
	{
	case QCTEST_CMD_SETPARAM:
		break;

	default:
		break;
	}
	QC_DEL_A(m_pTxtItem);
}

int	CTestItem::AddItem(char * pItem)
{
	QC_DEL_A(m_pTxtItem);
	m_pTxtItem = new char[strlen(pItem) + 1];
	strcpy(m_pTxtItem, pItem);
	char * pText = m_pTxtItem;

	if (strncmp(m_pTxtItem, "ACTION=", 7) == 0)
	{
		pText = m_pTxtItem + 7;
		if (strncmp(pText, "exit:", 5) == 0)
		{
			m_nCmd = QCTEST_CMD_EXIT;
			pText += 5;
			m_nTime = atoi(pText);
		}
		else if (strncmp(pText, "play:", 5) == 0)
		{
			m_nCmd = QCTEST_CMD_RUN;
			pText += 5;
			m_nTime = atoi(pText);
		}
		else if (strncmp(pText, "pause:", 6) == 0)
		{
			m_nCmd = QCTEST_CMD_PAUSE;
			pText += 6;
			m_nTime = atoi(pText);
		}
		else if (strncmp(pText, "seek:", 5) == 0)
		{
			m_nCmd = QCTEST_CMD_SEEK;
			pText += 5;
			sscanf(pText, "%d:%d", &m_nTime, &m_llValue);
		}
		else if (strncmp(pText, "setview:", 8) == 0)
		{
			m_nCmd = QCTEST_CMD_SETVIEW;
			pText += 8;
			sscanf(pText, "%d:%d", &m_nTime, &m_nValue);
		}
        else if (strncmp(pText, "stop:", 5) == 0)
        {
            m_nCmd = QCTEST_CMD_STOP;
            pText += 5;
            sscanf(pText, "%d:%d", &m_nTime, &m_nValue);
        }
        else if (strncmp(pText, "open:", 5) == 0)
        {
            m_nCmd = QCTEST_CMD_OPEN;
            pText += 5;
            sscanf(pText, "%d:%d", &m_nTime, &m_nValue);
        }
		else if (strncmp(pText, "disvideo:", 9) == 0)
		{
			m_nCmd = QCTEST_CMD_SETPARAM;
			m_nParamID = QCPLAY_PID_Disable_Video;
			pText += 9;
			sscanf(pText, "%d:%d", &m_nTime, &m_nValue);
		}
		else if (strncmp(pText, "playstream:", 11) == 0)
		{
			m_nCmd = QCTEST_CMD_SETPARAM;
			m_nParamID = QCPLAY_PID_StreamPlay;
			pText += 11;
			sscanf(pText, "%d:%d", &m_nTime, &m_nValue);
		}
		else if (strncmp(pText, "zoom:", 5) == 0)
		{
			m_nCmd = QCTEST_CMD_SETPARAM;
			m_nParamID = QCPLAY_PID_Zoom_Video;
			pText += 5;
			sscanf(pText, "%d:%d:%d:%d:%d", &m_nTime, &m_rtZoom.left, &m_rtZoom.top, &m_rtZoom.right, &m_rtZoom.bottom);
		}
		else if (strncmp(pText, "downpause:", 10) == 0)
		{
			m_nCmd = QCTEST_CMD_SETPARAM;
			m_nParamID = QCPLAY_PID_Download_Pause;
			pText += 10;
			sscanf(pText, "%d:%d", &m_nTime, &m_nValue);
		}
        else if (strncmp(pText, "muxstart:", 9) == 0)
        {
            m_nCmd = QCTEST_CMD_SETPARAM;
            m_nParamID = QCPLAY_PID_START_MUX_FILE;
            pText += 9;
            memset(m_szValue, 0, 1024);
            sscanf(pText, "%d:%s", &m_nTime, m_szValue);
#ifdef __QC_OS_IOS__
            if(strlen(m_szValue) > 0)
            {
                char szFilePath[1024];
                qcGetAppPath(NULL, szFilePath, 1024);
                strcat(szFilePath, m_szValue);
                strcpy(m_szValue, szFilePath);
            }
#endif
        }
        else if (strncmp(pText, "muxstop:", 8) == 0)
        {
            m_nCmd = QCTEST_CMD_SETPARAM;
            m_nParamID = QCPLAY_PID_STOP_MUX_FILE;
            pText += 8;
            sscanf(pText, "%d:%d", &m_nTime, &m_nValue);
        }
	}
	else if (strncmp(m_pTxtItem, "SETTING=", 8) == 0)
	{
		pItem = m_pTxtItem + 8;

	}

	return QC_ERR_NONE;
}

int CTestItem::ScheduleTask(void)
{
	m_pInst->m_pTestMng->PostTask(QCTEST_TASK_ITEM, m_nTime, m_nItemIndex, 0, NULL);
	return QC_ERR_NONE;
}

int CTestItem::ExecuteCmd(void)
{
    int nUseTime = qcGetSysTime();
    
	switch(m_nCmd)
	{
	case QCTEST_CMD_EXIT:
		m_pInst->m_pTestMng->PostTask(QCTEST_TASK_EXIT, 0, 0, 0, NULL);
		break;

	case QCTEST_CMD_RUN:
		m_pPlayer->Run();
		break;

	case QCTEST_CMD_PAUSE:
		m_pPlayer->Pause();
		break;

    case QCTEST_CMD_STOP:
    	m_pPlayer->Stop();
    	break;

    case QCTEST_CMD_OPEN:
        m_pTestTask->OpenURL();
        break;

	case QCTEST_CMD_SEEK:
		m_pPlayer->SetPos(m_llValue);
		break;

	case QCTEST_CMD_SETVIEW:
		if (m_nValue == 0)
			m_pPlayer->SetView(NULL, NULL);
		else
			m_pPlayer->SetView(m_pInst->m_hWndVideo, NULL);
		break;

	case QCTEST_CMD_SETPARAM:
	{
		switch (m_nParamID)
		{
		case QCPLAY_PID_Disable_Video:
		case QCPLAY_PID_StreamPlay:
		case QCPLAY_PID_Download_Pause:
			m_pPlayer->SetParam(m_nParamID, &m_nValue);
			break;
        case QCPLAY_PID_START_MUX_FILE:
            m_pPlayer->SetParam(m_nParamID, m_szValue);
            break;
        case QCPLAY_PID_STOP_MUX_FILE:
            m_pPlayer->SetParam(m_nParamID, &m_nValue);
            break;

		case QCPLAY_PID_Zoom_Video:
			m_pPlayer->SetParam(m_nParamID, &m_rtZoom);
			break;

		default:
			break;
		}
		break;
	}

	default:
		break;
	}
    
    if ((qcGetSysTime() - nUseTime) > 100)
    {
        char text[256];
        if(m_nCmd == QCTEST_CMD_SETPARAM || m_nCmd == QCTEST_CMD_GETPARAM)
        	sprintf(text, "API - %s use time %d, PID %d", QCTEST_Command_Name[m_nCmd], (qcGetSysTime() - nUseTime), m_nParamID);
        else
            sprintf(text, "API - %s use time %d", QCTEST_Command_Name[m_nCmd], (qcGetSysTime() - nUseTime));
        m_pInst->AddInfoItem(this, QCTEST_INFO_Err, text);
    }
    
	return QC_ERR_NONE;
}
