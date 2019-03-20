/*******************************************************************************
	File:		CTestTask.h

	Contains:	the buffer trace class header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#ifndef __CTestTask_H__
#define __CTestTask_H__
#include "qcData.h"
#include "qcPlayer.h"
#include "CBaseObject.h"

#include "CTestItem.h"
#include "CNodeList.h"

#include "CTestPlayer.h"
#include "CExtSource.h"

class CTestTask : public CTestBase
{
public:
	CTestTask(CTestInst * pInst);
    virtual ~CTestTask(void);

	virtual int		FillTask(char * pTask);
	virtual int		Start(CTestPlayer * pPlayer);
	virtual int		Stop(void);
	virtual int		ExcuteItem(int nItem);
	virtual int		CheckStatus(void);

public:
	static void		NotifyEvent(void * pUserData, int nID, void * pValue);
	virtual void	HandleEvent(int nID, void * pValue);

	static int		ReceiveData(void * pUserData, QC_DATA_BUFF * pBuffer);
	virtual int		HandleData(QC_DATA_BUFF * pBuffer);
    
    int				OpenURL();

protected:
	virtual void	OnOpenDone(int nMsgID, void * pValue);
	virtual void	OnPlayComplete(void);
    virtual void    OnSeekDone(void);

	virtual void	ShowStatus(int nMsgID, void * pValue);

protected:
	CTestPlayer *			m_pPlayer;
	char *					m_pName;
	char *					m_pURL;

	long long				m_llStartPos;
	int						m_nPlayComplete;
	int						m_nExitClose;

	//#define	QCPLAY_OPEN_SAME_SOURCE				0X02000000
	int						m_nOpenFlag;
	//#define	QCPLAY_PID_AspectRatio				QC_PLAY_BASE + 0X01  xxxx:xxxx
	QCPLAY_ARInfo			m_sRatio;
	//#define	QCPLAY_PID_Speed					QC_PLAY_BASE + 0X02
	double					m_dSpeed;
	// #define    QCPLAY_PID_Clock_OffTime			QC_PLAY_BASE + 0X20
	int						m_nOffsetTime;
	// #define    QCPLAY_PID_Seek_Mode				QC_PLAY_BASE + 0X21
	int						m_nSeekMode;
	// #define	QCPLAY_PID_Prefer_Protocol	QC_PLAY_BASE + 0X60
	int						m_nPreProtocol;
	// #define    QCPLAY_PID_Prefer_Format			QC_PLAY_BASE + 0X50
	int						m_nPreferFormat;
	// #define    QCPLAY_PID_PD_Save_Path			QC_PLAY_BASE + 0X61
	char *					m_pSavePath;
	// #define    QCPLAY_PID_PD_Save_ExtName		QC_PLAY_BASE + 0X62
	char *					m_pExtName;
	// #define	  QCPLAY_PID_RTSP_UDPTCP_MODE		QC_PLAY_BASE + 0X81
	int						m_nRTSPMode;
	// #define    QCPLAY_PID_Socket_ConnectTimeout	QC_PLAY_BASE + 0X0200
	int						m_nConnectTimeOut;
	// #define    QCPLAY_PID_Socket_ReadTimeout		QC_PLAY_BASE + 0X0201
	int						m_nReadTimeOut;
	// #define    QCPLAY_PID_HTTP_HeadReferer		QC_PLAY_BASE + 0X0205
	char *					m_pHeadText;
	// #define    QCPLAY_PID_DNS_SERVER				QC_PLAY_BASE + 0X0208
	char *					m_pDNSServer;
	// #define    QCPLAY_PID_DNS_DETECT				QC_PLAY_BASE + 0X0209
	char *					m_pDNSDetect;
	// #define    QCPLAY_PID_PlayBuff_MaxTime		QC_PLAY_BASE + 0X0211
	int						m_nMaxBuffTime;
	// #define    QCPLAY_PID_PlayBuff_MinTime		QC_PLAY_BASE + 0X0212
	int						m_nMinBuffTime;
	// #define    QCPLAY_PID_DRM_KeyText			QC_PLAY_BASE + 0X0301
	char *					m_pDrmKeyText;
	// #define    QCPLAY_PID_Log_Level				QC_PLAY_BASE + 0X0320
	int						m_nLogLevel;
	// #define    QCPLAY_PID_Playback_Loop			QC_PLAY_BASE + 0X0340
	int						m_nPlayLoop;
	// #define    QCPLAY_PID_MP4_PRELOAD			QC_PLAY_BASE + 0X0341
	int						m_nPreloadTime;
    
    int						m_nExtSrcType;

	CObjectList<CTestItem>	m_lstItem;

	int						m_nStartTime;
	int						m_nIndex;
	char *					m_pTxtMsg;
	char *					m_pTxtErr;
	char *					m_pTxtTmp;

	CMutexLock				m_mtData;
	int						m_nVideoCount;
	int						m_nVideoError;
	long long				m_llVideoTime;
	int						m_nLastSysVideo;

	int						m_nAudioCount;
	long long				m_llAudioTime;
	int						m_nLastSysAudio;

	QCPLAY_STATUS			m_stsPlay;
	bool					m_bHaveSeek;
    bool					m_bPlaybackFromPos;
    
    CExtSource*				m_pExtSrc;
};

#endif //__CTestTask_H__
