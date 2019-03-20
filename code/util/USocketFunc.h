/*******************************************************************************
	File:		USocketFunc.h

	Contains:	The base utility for socket header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __USocketFunc_H__
#define __USocketFunc_H__

#define	QC_DNS_FROM_HTTP		0X00
#define	QC_DNS_FROM_UDP			0XFFFFFFFF
#define	QC_DNS_FROM_SYS			0XFFFFFFFE
#define	QC_DNS_FROM_NONE		0XFFFFFFFD

bool	qcSocketInit (void);
void	qcSocketUninit (void);
bool	qcHostIsIPAddr(char * pHostName);

int		qcGetDNSType(char * pDNSServer);

unsigned long	qcGetIPAddrFromString(char * pIPAddr);
int				qcGetIPAddrFromValue(unsigned long uIPAddr, void ** ppAddr);
int				qcFreeIPAddr(void * pIPAddr);

/*
inet_ntop(
__in                                INT             Family,
__in                                PVOID           pAddr,
__out_ecount(StringBufSize)         PSTR            pStringBuf,
__in                                size_t          StringBufSize
);
*/
#ifdef __QC_OS_WIN32__

typedef char * (*QCINETNTOP) (int nFamily, void * pAddr, char * pStringBuf, int nStringBufSize);

class CQCInetNtop
{
public:
	CQCInetNtop(void);
	virtual ~CQCInetNtop();

	char *	qcInetNtop(int nFamily, void * pAddr, char * pStringBuf, int nStringBufSize);

protected:
	void *			m_hWSDll;
	QCINETNTOP		m_fInetNtop;
};
#endif // __QC_OS_WIN32__
#endif // __USocketFunc_H__