/*******************************************************************************
	File:		CHttpIO.h
 
	Contains:
 
	Written by:	liangliang
 
	Change History (most recent first):
	12/14/16		liangliang			Create file
 
 *******************************************************************************/

#ifndef __CHttpIO_H__
#define __CHttpIO_H__

#include <vector>
#include <string>

#include "CBaseIO.h"
#include "CUrl.h"

class CHttpIO : public CBaseIO
{
public:
    CHttpIO(void);
    virtual ~CHttpIO(void);
    
    virtual int 		Open (const char * pURL, int nFlag);
    virtual int 		Reconnect (const char * pNewURL);
    virtual int 		Close (void);
    
    virtual int			Read (unsigned char * pBuff, int nSize);
    virtual int			ReadAt (long long llPos, unsigned char * pBuff, int nSize);
    virtual int			Write (unsigned char * pBuff, int nSize);
    virtual int			GetSpeed (void);
    
    virtual int 		Run (void);
    virtual int 		Pause (void);
    virtual int 		Stop (void);
    
    virtual long long 	GetPos (void);
    virtual long long 	SetPos (long long llPos, int nFlag);
    virtual long long 	GetSize (void);
    virtual QCIOType	GetType (void);
    
    virtual int 		GetParam (int nID, void * pParam);
    virtual int 		SetParam (int nID, void * pParam);
    
private:
    bool                IsSpace (char c);
    int                 AddHeader (const char * fmt, ...);
    int                 ReadLine (std::string & line);
    void                ParseLine (std::string & line);
    void                ParseContentRange (const char * p);
    int                 CheckHttpCode ();
    
    static const int    MAX_REDIRECTS = 8;
    
    CBaseIO *           m_pIO;
    CUrl                m_url;
    std::vector<char>   m_vHeaders;
    long long           m_llPos;
    int                 m_nRedicrts;
    
    std::vector<char>   m_vBuff;
    int                 m_nBuffRead;
    int                 m_nBuffWrite;
    
    int                 m_nHttpCode;
    std::string         m_sLocation;
    bool                m_bNewLocation;
    long long           m_llOff;
    long long           m_llFileSize;
    std::string         m_sContentType;
    bool                m_bSeekable;
    long long           m_llChunkSize;
    std::string         m_sServer;
    
    
};

#endif /* __CHttpIO_H__ */
