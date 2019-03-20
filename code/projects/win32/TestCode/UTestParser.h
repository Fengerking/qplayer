/*******************************************************************************
	File:		UTestParser.h

	Contains:	The message manager header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-29		Bangfei			Create file

*******************************************************************************/
#ifndef __UTestParser_H__
#define __UTestParser_H__

#include "qcType.h"

#include "qcData.h"
#include "qcParser.h"

#include "CBuffMng.h"
#include "CThreadWork.h"

#include "CQCSource.h"
#include "CBoxSource.h"
#include "COMBoxMng.h"

class CTestParser : public CBaseObject, public CThreadFunc
{
public:
	CTestParser(void);
    virtual ~CTestParser(void);

	virtual int	Open (const char * pURL);
	virtual int	Close (void);

protected:
	virtual int			OnWorkItem (void);

public:
	QC_Parser_Func		m_fParser;
	QC_IO_Func			m_fIO;
	CBuffMng *			m_pBuffMng;
	QC_DATA_BUFF		m_buffInfo;

	CThreadWork *		m_pReadThread;

	CQCSource *			m_pSource;

	CBoxSource *		m_pBoxSource;

	COMBoxMng *			m_pBoxMng;

};

int	qcTestParserOpen (void);

int	qcTestParserClose (void);



#endif // __UTestParser_H__
