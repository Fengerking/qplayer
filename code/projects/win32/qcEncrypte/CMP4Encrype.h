#pragma once
class CMP4Encrype
{
public:
	CMP4Encrype();
	virtual ~CMP4Encrype();

	virtual bool		EncrypeFile (const char * pSource, const char * pDest, const char * pKeyComp, const char * pKeyFile);
	virtual bool		UnencrypeFile(const char * pSource, const char * pDest);

protected:
	virtual bool		ParserMP4 (HANDLE hFileSrc, long long llFileSize);

public:
	char				m_szCompKey[32];
	char				m_szFileKey[32];

protected:
	int					m_nHeadSize;
	int					m_nTypeSize;
	int					m_nQKeySize;
	long long			m_llMoovBeg;
	long long			m_llMoovEnd;
	long long			m_llDataBeg;
	long long			m_llDataEnd;


protected:
	unsigned int		qcIntReadUint32BE(const unsigned char* aReadPtr);
	unsigned long long	qcIntReadUint64BE(const unsigned char* aReadPtr);
};

