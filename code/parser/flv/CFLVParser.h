/*******************************************************************************
	File:		CFLVParser.h

	Contains:	the base parser header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-08		Bangfei			Create file

*******************************************************************************/
#ifndef __CFLVParser_H__
#define __CFLVParser_H__

#include "../CBaseParser.h"
#include "CFLVTag.h"

class CFLVParser : public CBaseParser
{
public:
    CFLVParser(CBaseInst * pBaseInst);
    virtual ~CFLVParser(void);

	virtual int			Open (QC_IO_Func * pIO, const char * pURL, int nFlag);
	virtual int 		Close (void);

	virtual int			Read (QC_DATA_BUFF * pBuff);
	virtual int			Send (QC_DATA_BUFF * pBuff);

	virtual int		 	CanSeek (void);
	virtual long long 	GetPos (void);
	virtual long long 	SetPos (long long llPos);

	virtual int 		GetParam (int nID, void * pParam);
	virtual int 		SetParam (int nID, void * pParam);

protected:
	virtual int		ReadNextTag (void);
	virtual	int		ReadMetaData (unsigned char * pBuffer, unsigned int nLength);
	virtual bool	CheckHaveBuff (int nFlag);

	virtual int		AmfGetString(unsigned char*	pSrcBuffer, int nLength, char* pDesString);
	virtual int		AmfReadObject(unsigned char* pBuffer, int nLength, const char* key);
	virtual int		KeyFrameIndex(unsigned char* pBuffer, int nLength);

	virtual int		FillKeyFrameList (void);
    
    int				EncodeMetaData(const char* pKey, int nValueType, void* pValue);

protected:
	long long		m_llOffset;

	CFLVTag *		m_pTagAudio;
	CFLVTag *		m_pTagVideo;
	CFLVTag *		m_pTagSubtt;

	unsigned char*	m_pTagBuffer;
	unsigned int	m_nMaxSize;
    char*			m_pMetaData;
    unsigned int	m_nCurrMetaDataSize;
    unsigned int	m_nMaxMetaDataSize;

	long long		m_llAudioLastTime;
	int				m_nAudioLoopTimes;
	long long		m_llVideoLastTime;
	int				m_nVideoLoopTimes;
	long long		m_llLoopOffsetTime;

	// for debug loop
	int				m_nStartTime;
	bool			m_bDebugDisconnect;
	char			m_szURL[2048];


};

#endif // __CBaseParser_H__
