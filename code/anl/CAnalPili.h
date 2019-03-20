/*******************************************************************************
	File:		CAnalPili.h
 
	Contains:	Pandora analysis collection header code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-05-04		Jun			Create file
 
 *******************************************************************************/

#ifndef CAnalPili_H_
#define CAnalPili_H_

#include "qcAna.h"
#include "CAnalBase.h"
#include "CAnalysisMng.h"

#ifdef __QC_OS_NDK__
#include "CNDKSysInfo.h"
#endif // __QC_OS_NDK__

typedef struct
{
    long long 	llStartTime;
    long long 	llEndTime;
    bool		bBuffering;
    int			nBufferingCount;
    float		fVideoFPS;
    int			nVideoDrop;
    float		fAudioFPS;
    int			nAudioDrop;
    float		fVideoRenderFPS;
    float		fAudioRenderFPS;
    int			nVideoBuffSize;
    int			nAudioBuffSize;
    long long	llDownloadSize;
    int			nDownloadTime;
    int			nVideoBitrte;
    int			nAudioBitrte;
    
    long long   llReportTime;
}PILI_RECORD;

class CAnalPili : public CAnalBase
{
public:
	CAnalPili(CBaseInst * pBaseInst);
    virtual ~CAnalPili();
    
public:
    virtual int ReportEvent(QCANA_EVENT_INFO* pEvent, bool bDisconnect=true);
    virtual int onMsg(CMsgItem* pItem);
    virtual int onTimer();
    virtual int Open();
    virtual int Stop();
    virtual bool IsDNSParsed();
    virtual int Disconnect();
    virtual int PostData();
protected:
    virtual char*	GetEvtName(int nEvtID);
    
private:
    // specific event
    int			EncStartupEvent();
    int			EncPlayEvent();
    int			EncCloseEvent();
    int			EncConnectChangedEvent();
    int			EncOpenEvent();
    int         EncStopEvent();
    
    // basic
    int			EncBase(char* pData, char* pszEvtName=NULL);
    int			EncMediaBase(char* pData, QCANA_SOURCE_INFO* pSource=NULL);
    int			EncEndBase(char* pData);
    int			EncDeviceInfoBase(char* pData);
    int			EncNetworkInfoBase(char* pData);
    
    int			ReportPlayEvent();
    int			CreateHeader(bool bPersistent);
    void		UpdateTrackParam(char* pData, int nLen);
    bool 		GetLine (char ** pBuffer, int* nLen, char** pLine, int* nLineSize);
    
    float		GetCpuLoad();
    float		GetMemoryUsage(bool bApp);
    const char*	GetNetworkType();
    const char*	GetISP();
    int 		GetSignalStrength();
    void		GetWifiName(char* pszWifiName);
    int 		GetSignalLevel();
    const char*	GetReportURL(bool bPersistent);
    const char* GetCodecName(int nCodec);
    void		MeasureUsage();
    
private:
    //CAnalDataSender*    m_pSender;
    int					m_nReportInterval;
    int					m_nSampleInterval;
    int					m_nBufferingCount;
    int					m_nBufferingTime;
    long long			m_llDownloadBytes;
    int					m_nCpuLoadCount;
    float				m_fCpuLoadSys;
    float				m_fCpuLoadApp;
    int					m_nMemoryUsageCount;
    float				m_fMemoryUsageSys;
    float				m_fMemoryUsageApp;
    char				m_szResolveIP[64];
    PILI_RECORD			m_sRecord;
    bool                m_bUpdateSampleTime;

#ifdef __QC_OS_WIN32__
	int					m_nLastSysTime;
	FILETIME			m_ftKernelTime;
	FILETIME			m_ftUserTime;
#elif defined __QC_OS_NDK__
    AndroidMemInfo      m_ndkMemInfo;
    AndroidCpuUsage     m_ndkCPUInfo;
#endif // __QC_OS_WIN32__
};

#endif /* CAnalPili_H_ */
