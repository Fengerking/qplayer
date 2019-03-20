/*******************************************************************************
	File:		CBaseObject.cpp

	Contains:	base object implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CBaseObject.h"

#include "CMutexLock.h"
#include "CDNSCache.h"
#include "CDNSLookUp.h"
#include "CMsgMng.h"
#include "CQCMuxer.h"

#ifdef __QC_OS_WIN32__
#include "CTrackMng.h"
#endif // __QC_OS_WIN32__

#include "USystemFunc.h"

int CBaseObject::g_ObjectNum = 0;
CBaseObject * CBaseObject::g_lstObj[2048];

CBaseInst::CBaseInst(void) 
{
	m_pSetting = new CBaseSetting();
	m_pDNSCache = new CDNSCache(this);
	m_pDNSLookup = new CDNSLookup(this);
	m_pMuxer = NULL;
	m_pTrackMng = NULL;
	m_pBuffMng = NULL;
	m_hLibCodec = NULL;

	m_pMsgMng = NULL;
	m_bForceClose = false;
	m_bCheckReopn = false;
	m_bStartBuffing = false;
	m_bHadOpened = true;
	m_bAudioDecErr = false;
	m_bVideoDecErr = false;
	m_nDownloadPause = 0;

	strcpy (m_szAppVer, "");
	strcpy (m_szAppName, "");
	strcpy (m_szSDK_ID, "");
	strcpy (m_szSDK_Ver, "1.1.0.31");

	strcpy (m_szNetType, "WIFI");
	strcpy (m_szWifiName, "");	
	strcpy (m_szISPName, "Local");
	m_nNetSignalDB = 0;
	m_nSignalLevel = 0;

	m_llFVideoTime = 0;
	m_llFAudioTime = 0;

	m_nVideoCodecID = QC_CODEC_ID_NONE;
	m_nAudioCodecID = QC_CODEC_ID_NONE;

	m_nVideoRndCount = 0;
	m_nAudioRndCount = 0;

	m_nOpenSysTime = 0;
    
    m_bBeginMux = false;

	m_pLockCheckFunc = new CMutexLock();
	ResetLogFunc();
}

CBaseInst::~CBaseInst(void)
{
	delete m_pDNSCache;
	delete m_pDNSLookup;
	delete m_pSetting;
    QC_DEL_P(m_pMuxer);
	m_lstNotify.RemoveAll();

	delete m_pLockCheckFunc;

	QC_DEL_P(m_pMsgMng);
}

void CBaseInst::AddListener(CBaseObject * pListener)
{
	m_lstNotify.AddTail(pListener);
}

void CBaseInst::RemListener(CBaseObject * pListener)
{
	m_lstNotify.Remove(pListener);
}

int	CBaseInst::SetForceClose(bool bForceClose)
{
	if (m_bForceClose == bForceClose)
		return QC_ERR_NONE;

	m_bForceClose = bForceClose;
	NODEPOS pos = m_lstNotify.GetHeadPosition();
	CBaseObject * pObject = NULL;
	while (pos != NULL)
	{
		pObject = m_lstNotify.GetNext(pos);
		pObject->RecvEvent(QC_BASEINST_EVENT_FORCECLOE);
	}
	return QC_ERR_NONE;
}

int	CBaseInst::NotifyNetChanged (void)
{
	NODEPOS pos = m_lstNotify.GetHeadPosition();
	CBaseObject * pObject = NULL;
	while (pos != NULL)
	{
		pObject = m_lstNotify.GetNext(pos);
		pObject->RecvEvent(QC_BASEINST_EVENT_NETCHANGE);
	}
	return QC_ERR_NONE;
}

int CBaseInst::NotifyReopen (void)
{
    NODEPOS pos = m_lstNotify.GetHeadPosition();
    CBaseObject * pObject = NULL;
    while (pos != NULL)
    {
        pObject = m_lstNotify.GetNext(pos);
        pObject->RecvEvent(QC_BASEINST_EVENT_REOPEN);
    }
    return QC_ERR_NONE;
}

int	CBaseInst::SetSettingParam(int nID, int nParam, void * pParam)
{
	switch (nID)
	{
	case QC_BASEINST_EVENT_VIDEOZOOM:
	{
		RECT * pRect = (RECT *)pParam;
		if (pRect == NULL)
			return QC_ERR_ARG;
		m_pSetting->g_qcs_nVideoZoomTop = pRect->top & ~3;
		m_pSetting->g_qcs_nVideoZoomLeft = pRect->left & ~3;
		m_pSetting->g_qcs_nVideoZoomWidth = (pRect->right - pRect->left) & ~3;
		m_pSetting->g_qcs_nVideoZoomHeight = (pRect->bottom - pRect->top) & ~3;
		break;
	}

	case QC_BASEINST_EVENT_NEWFORMAT_V:
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pParam;
		if (pFmt != NULL)
		{
			m_nVideoWidth = pFmt->nWidth;
			m_nVideoHeight = pFmt->nHeight;
		}
		break;
	}

	default:
		break;
	}

	NODEPOS pos = m_lstNotify.GetHeadPosition();
	CBaseObject * pObject = NULL;
	while (pos != NULL)
	{
		pObject = m_lstNotify.GetNext(pos);
		pObject->RecvEvent(nID);
	}

	return QC_ERR_NONE;
}

int CBaseInst::StartLogFunc(void)
{
	CAutoLock lock(m_pLockCheckFunc);
	int		i = 0;
	int		nThreadID = qcThreadGetCurrentID();
	bool	bFound = false;
	for (i = 0; i < 256; i++)
	{
		if (m_aThreadID[i] == nThreadID)
		{
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		for (i = 0; i < 256; i++)
		{
			if (m_aThreadID[i] == 0)
				break;
		}
	}

	if (m_aThreadID[i] == 0)
		m_aThreadID[i] = qcThreadGetCurrentID();

	m_aFuncDeepCount[i]++;

	return m_aFuncDeepCount[i];
}
int	CBaseInst::LeaveLogFunc(void)
{
	CAutoLock lock(m_pLockCheckFunc);
	int i = 0;
	int nThreadID = qcThreadGetCurrentID();
	for (i = 0; i < 256; i++)
	{
		if (m_aThreadID[i] == nThreadID)
			break;
	}
	if (m_aThreadID[i] == 0)
		return -1;

	m_aFuncDeepCount[i]--;
	if (m_aFuncDeepCount[i] == 0)
		m_aThreadID[i] = 0;

	return m_aFuncDeepCount[i];
}

int	CBaseInst::ResetLogFunc(void)
{
	CAutoLock lock(m_pLockCheckFunc);
	for (int i = 0; i < 256; i++)
	{
		m_aThreadID[i] = 0;
		m_aFuncDeepCount[i] = 0;
	}
	return 0;
}


CBaseObject::CBaseObject(CBaseInst * pBaseInst)
	: m_pBaseInst(pBaseInst)
{
#if __QC_DEBUG__
	if (g_ObjectNum == 0)
	{
		for (int i = 0; i < 2048; i++)
			g_lstObj[i] = NULL;
	}
	g_lstObj[g_ObjectNum] = this;
#endif // __QC_DEBUG__
	SetObjectName ("CBaseObject");
	g_ObjectNum++;

	m_llDbgTime = 0;
}

CBaseObject::~CBaseObject(void)
{
	g_ObjectNum--;

#ifdef __QC_DEBUG__
	for (int i = 0; i < 2048; i++)
	{
		if (g_lstObj[i] == this)
		{
			g_lstObj[i] = NULL;
			break;
		}
	}
#endif // __QC_DEBUG__
}

void CBaseObject::SetObjectName (const char * pObjName)
{
	m_pClassFileName = strrchr (pObjName, '\\');
	if (m_pClassFileName == NULL)
		m_pClassFileName = strrchr (pObjName, '/');
	if (m_pClassFileName != NULL)
	{
		m_pClassFileName += 1;
		if (strlen (m_pClassFileName) >= sizeof (m_szObjName))
			strncpy (m_szObjName, m_pClassFileName, sizeof (m_szObjName) - 2);
		else
			strcpy (m_szObjName, m_pClassFileName);
	}
	else
	{
		strcpy (m_szObjName, pObjName);
	}
}

CLogOutFunc::CLogOutFunc(const char * pFile, const char * pFuncName, int * pRC, CBaseInst * pInst, int nValue)
{
	m_nStartTime = qcGetSysTime();

	char * pName = (char *)strrchr (pFile, '/');
	if (pName == NULL)
		pName = (char *)strrchr (pFile, '\\');
	if (pName == NULL)
		pName = (char *)pFile;
	strcpy(m_szFuncName, pName + 1);
	char * pDot = strchr (m_szFuncName, '.');
	if (pDot != NULL)
		*pDot = 0;
	strcat(m_szFuncName, "::");
#ifdef __QC_OS_WIN32__	
	strcpy(m_szFuncName, "");
#endif // __QC_OS_WIN32__
	strcat(m_szFuncName, pFuncName);
	m_pRC = pRC;
	m_pBaseInst = pInst;
	m_nValue = nValue;
	int nDeep = 0;
	if (m_pBaseInst != NULL)
		nDeep = m_pBaseInst->StartLogFunc();
	if (nDeep <= 0)
		nDeep = 1;
	char * pSpaceText = new char[nDeep * 4 + 1];
	memset(pSpaceText, '-', nDeep * 4);
	pSpaceText[nDeep * 4] = 0;

	QCLOGT("QCFuncLog", "%s%s Value is %08X start.", pSpaceText, m_szFuncName, m_nValue);

	delete []pSpaceText;
};

CLogOutFunc::~CLogOutFunc(void)
{
	int nDeep = 0;
	if (m_pBaseInst != NULL)
		nDeep = m_pBaseInst->LeaveLogFunc() + 1;
	if (nDeep <= 0)
		nDeep = 1;		
	char * pSpaceText = new char[nDeep * 4 + 1];
	memset(pSpaceText, '-', nDeep * 4);
	pSpaceText[nDeep * 4] = 0;

	if (m_pRC != NULL)
	{
		QCLOGT("QCFuncLog", "%s%s leave! rc = % 8d. Used Time: %d", pSpaceText, m_szFuncName, *m_pRC, qcGetSysTime () - m_nStartTime);
	}
	else
	{
		QCLOGT("QCFuncLog", "%s%s leave! no return. Ussed Time: %d", pSpaceText, m_szFuncName, qcGetSysTime() - m_nStartTime);
	}
    
    delete []pSpaceText;
};
