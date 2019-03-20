/*******************************************************************************
	File:		CAnalDataSender.h
 
	Contains:	Analysis collection header code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-03-21		Jun			Create file
 
 *******************************************************************************/

#ifndef CAnalDataSender_h
#define CAnalDataSender_h

#include "CHTTPClient.h"
#include "CThreadWork.h"

typedef struct _tagAnlRawData
{
    char*    	server;
    char*    	data;
    int			dataLen;
}AnlRawData;


class CAnalDataSender : public CHTTPClient, public CThreadFunc
{
public:
	CAnalDataSender(CBaseInst * pBaseInst, CDNSCache* pCache, char* pszDstServer);
    virtual ~CAnalDataSender();
    
public:
    int PostData();
    int PostData(const char* pData, int nLen, bool bWait=true);
    int ReadResponse(char* pData, int& nLen);
    int Stop();
    void UpdateServer(const char* pszDstServer);
    bool IsDNSParsed();
    void GetReportParam(int& nSampleInterval, int& nReportInterval);
    void Save(const char* pServerURL, int nURLLen, char* pData, int nDataLen);
    void Save(const char* pServerURL, char* pHeader, int nHeaderLen, char* pBody, int nBodyLen);
    void UpdateDNSServer(const char* pszServer);
    
protected:
    virtual int OnWorkItem (void);
    virtual int WaitSocketWriteBuffer(int& aSocketHandle, timeval& aTimeOut);

    void ReleaseAnlRawData(AnlRawData* pData);
    void ReleaseAnlRawList();
    
private:
    int Prepare(char* szServer);
    void UpdateTrackParam(char* pData, int nLen);
    bool GetLine(char ** pBuffer, int* nLen, char** pLine, int* nLineSize);
    int ReadRawData();
    int WriteRawData();
    void DeleteDumpFile(char* pszFileName);
    int GetRawDataSize();
    
private:
    char    m_szDstServer[1024];
    bool	m_bDNSReady;
    
    CMutexLock m_mtAnlRawList;
    CObjectList<AnlRawData> m_listAnlRaw;
    
    int m_nPostStartTime;
    bool m_bUpdateSampleTime;
    CThreadWork* m_pThreadWork;
    
    int m_nReportInterval;
    int m_nSampleInterval;
};

#endif /* CAnalDataSender_h */
