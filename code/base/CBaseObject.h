/*******************************************************************************
	File:		CBaseObject.h

	Contains:	the base class of all objects.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#ifndef __CBaseObject_H__
#define __CBaseObject_H__

#include "stdio.h"
#include "string.h"

#include "qcType.h"
#include "qcDef.h"

#include "CBaseSetting.h"
#include "CNodeList.h"

#include "ULogFunc.h"
#include "ULibFunc.h"

#define		QC_BASEINST_EVENT_FORCECLOE		0X1001
#define		QC_BASEINST_EVENT_NETCHANGE		0X1002
#define		QC_BASEINST_EVENT_VIDEOZOOM		0X1003
#define		QC_BASEINST_EVENT_VIDEOROTATE	0X1004
#define		QC_BASEINST_EVENT_NEWFORMAT_V	0X1005
#define		QC_BASEINST_EVENT_NEWFORMAT_A	0X1006
#define     QC_BASEINST_EVENT_REOPEN    	0X1007

class CBaseObject;
class CMutexLock;
class CDNSCache;
class CDNSLookup;
class CTrackMng;
class CMsgMng;
class CQCMuxer;

class CBaseInst
{
public:
	CBaseInst(void);
	virtual ~CBaseInst(void);

	virtual	void	AddListener(CBaseObject * pListener);
	virtual	void	RemListener(CBaseObject * pListener);
	virtual int		SetForceClose(bool bForceClose);
	virtual int		NotifyNetChanged (void);
    virtual int     NotifyReopen (void);

	virtual int		SetSettingParam(int nID, int nParam, void * pParam);

	virtual int		StartLogFunc(void);
	virtual int		LeaveLogFunc(void);
	virtual int		ResetLogFunc(void);

public:
	CBaseSetting *				m_pSetting;
	CMsgMng *					m_pMsgMng;
	CDNSCache *					m_pDNSCache;
	CDNSLookup *				m_pDNSLookup;
	CTrackMng *					m_pTrackMng;
    CQCMuxer *					m_pMuxer;
	void *						m_pBuffMng;
	qcLibHandle					m_hLibCodec;

	bool						m_bForceClose;
	bool						m_bCheckReopn;
	bool						m_bStartBuffing;
	bool						m_bHadOpened;
	bool						m_bAudioDecErr;
	bool						m_bVideoDecErr;
	int							m_nDownloadPause;

	char						m_szAppVer[32];
	char						m_szAppName[64];
	char						m_szSDK_ID[64];
	char						m_szSDK_Ver[32];

	char						m_szNetType[32];
	char						m_szISPName[32];
	char						m_szWifiName[64];
	int							m_nNetSignalDB;
	int							m_nSignalLevel;

	int							m_nVideoWidth;
	int							m_nVideoHeight;

	long long					m_llFVideoTime;
	long long					m_llFAudioTime;

	QCCodecID					m_nVideoCodecID;
	QCCodecID					m_nAudioCodecID;

	int							m_nVideoRndCount;
	int							m_nAudioRndCount;

	int							m_nOpenSysTime;

	CObjectList<CBaseObject>	m_lstNotify;
    
    bool						m_bBeginMux;

protected:
	CMutexLock	*				m_pLockCheckFunc;
	int							m_aThreadID[256];
	int							m_aFuncDeepCount[256];
};

class CBaseObject
{
public:
	CBaseObject(CBaseInst * pBaseInst);
	virtual ~CBaseObject(void);

	CBaseInst *		GetBaseInst(void) { return m_pBaseInst; }
	void 			SetBaseInst (CBaseInst * pBaseInst) {m_pBaseInst = pBaseInst;}

	virtual int		RecvEvent(int nEventID) { return 0; }

protected:
	virtual void	SetObjectName (const char * pObjName);

protected:
	const char *	m_pClassFileName;

	CBaseInst *		m_pBaseInst;

	long long		m_llDbgTime;

public:
	char			m_szObjName[64];
	static int		g_ObjectNum;

	static CBaseObject *	g_lstObj[2048];
};

#define QCLOG_CHECK_FUNC(p,q,n) \
	 CLogOutFunc qcLogCheckFunc(__FILE__, __FUNCTION__, p, q, n)

class CLogOutFunc
{
public:
	CLogOutFunc(const char * pFile, const char * pFuncName, int * pRC, CBaseInst * pInst, int nValue);
	virtual ~CLogOutFunc(void);

protected:
	char		m_szFuncName[128];
	int	*		m_pRC;
	CBaseInst *	m_pBaseInst;
	int			m_nValue;
	int			m_nStartTime;
};

#endif // __CBaseObject_H__
