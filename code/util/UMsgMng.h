/*******************************************************************************
	File:		UMsgMng.h

	Contains:	The message manager header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#ifndef __UMsgMng_H__
#define __UMsgMng_H__
#include "stdio.h"
#include "string.h"
#include "qcErr.h"
#include "qcDef.h"
#include "qcType.h"
#include "qcMsg.h"

#include "CBaseObject.h"

#include "USystemFunc.h"

#define QCMSG_LOG(fmt, ...) \
{ \
	char		szQCMSG_LOGText[1024]; \
	sprintf (szQCMSG_LOGText, fmt, __VA_ARGS__); \
	QCMSG_Notify (m_pBaseInst, QC_MSG_LOG_TEXT, 0, 0, szQCMSG_LOGText); \
}
// convert the id to name
int		QCMSG_ConvertName (int nMsg, char * pName, int nSize);


// it will handle by module itself.
void *	QCMSG_InfoClone(int nMsg, void * pInfo);
int		QCMSG_InfoRelase(int nMsg, void * pInfo);

// message item class
class CMsgItem
{
public:
	CMsgItem (void)
	{
		m_nMsgID = 0;
		m_nValue = 0;
		m_llValue = 0;
		m_szValue = NULL;
		m_pInfo = NULL;
		QCMSG_ConvertName (m_nMsgID, m_szIDName, sizeof (m_szIDName));
	}
	CMsgItem (CMsgItem * pItem)
	{
		m_nMsgID = pItem->m_nMsgID;
		m_nValue = pItem->m_nValue;
		m_llValue = pItem->m_llValue;
		m_szValue = NULL;
		if (pItem->m_szValue != NULL)
		{
			m_szValue = new char[strlen (pItem->m_szValue)+1];
			strcpy (m_szValue, pItem->m_szValue);
		}
		if (pItem->m_pInfo != NULL)
			m_pInfo = QCMSG_InfoClone(pItem->m_nMsgID, pItem->m_pInfo);
		else
			m_pInfo = NULL;
		QCMSG_ConvertName (m_nMsgID, m_szIDName, sizeof (m_szIDName));
		m_nTime = pItem->m_nTime;
	}
	CMsgItem (int nMsgID, int nValue, long long llValue)
	{
		m_nMsgID = nMsgID;
		m_nValue = nValue;
		m_llValue = llValue;
		m_szValue = NULL;
		m_pInfo = NULL;
		QCMSG_ConvertName (m_nMsgID, m_szIDName, sizeof (m_szIDName));
		m_nTime = qcGetSysTime ();
	}
	CMsgItem (int nMsgID, int nValue, long long llValue, const char * pValue)
	{
		m_nMsgID = nMsgID;
		m_nValue = nValue;
		m_llValue = llValue;
		m_szValue = NULL;
		if (pValue != NULL)
		{
			m_szValue = new char[strlen (pValue)+1];
			strcpy (m_szValue, pValue);
		}
		m_pInfo = NULL;
		QCMSG_ConvertName (m_nMsgID, m_szIDName, sizeof (m_szIDName));
		m_nTime = qcGetSysTime ();
	}
	CMsgItem (int nMsgID, int nValue, long long llValue, const char * pValue, void * pInfo)
	{
		m_nMsgID = nMsgID;
		m_nValue = nValue;
		m_llValue = llValue;
		m_szValue = NULL;
		if (pValue != NULL)
		{
			m_szValue = new char[strlen (pValue)+1];
			strcpy (m_szValue, pValue);
		}
		if (pInfo != NULL)
			m_pInfo = QCMSG_InfoClone(nMsgID, pInfo);
		else
			m_pInfo = NULL;
		QCMSG_ConvertName (m_nMsgID, m_szIDName, sizeof (m_szIDName));
		m_nTime = qcGetSysTime ();
	}
	virtual ~CMsgItem (void)
	{
		if (m_szValue != NULL)
			delete []m_szValue;

		if (m_pInfo != NULL)
			QCMSG_InfoRelase(m_nMsgID, m_pInfo);
	}

public:
	virtual int SetValue (int nMsgID, int nValue, long long llValue)
	{
		m_nMsgID = nMsgID;
		m_nValue = nValue;
		m_llValue = llValue;
		QC_DEL_A (m_szValue);
		QCMSG_ConvertName (m_nMsgID, m_szIDName, sizeof (m_szIDName));
		m_nTime = qcGetSysTime ();
		return QC_ERR_NONE;
	}
	virtual int SetValue (int nMsgID, int nValue, long long llValue, const char * pValue)
	{
		m_nMsgID = nMsgID;
		m_nValue = nValue;
		m_llValue = llValue;
		QC_DEL_A (m_szValue);
		if (pValue != NULL)
		{
			m_szValue = new char[strlen (pValue)+1];
			strcpy (m_szValue, pValue);
		}
		m_pInfo = NULL;
		QCMSG_ConvertName (m_nMsgID, m_szIDName, sizeof (m_szIDName));
		m_nTime = qcGetSysTime ();
		return QC_ERR_NONE;
	}
	virtual int SetValue (int nMsgID, int nValue, long long llValue, const char * pValue, void * pInfo)
	{
		m_nMsgID = nMsgID;
		m_nValue = nValue;
		m_llValue = llValue;
		QC_DEL_A (m_szValue);
		if (pValue != NULL)
		{
			m_szValue = new char[strlen (pValue)+1];
			strcpy (m_szValue, pValue);
		}
		if (m_pInfo != NULL)
		{
			QCMSG_InfoRelase(m_nMsgID, m_pInfo);
			m_pInfo = NULL;
		}
		if (pInfo != NULL)
			m_pInfo = QCMSG_InfoClone(nMsgID, pInfo);
		QCMSG_ConvertName (m_nMsgID, m_szIDName, sizeof (m_szIDName));
		m_nTime = qcGetSysTime ();
		return QC_ERR_NONE;
	}

public:
	int			m_nMsgID;
	int			m_nValue;
	long long	m_llValue;
	char *		m_szValue;
	void *		m_pInfo;
	char		m_szIDName[64];
	int			m_nTime;
};

class CMsgReceiver 
{
public:
	CMsgReceiver (void){}
	virtual ~CMsgReceiver (void){}
public:
	virtual int		ReceiveMsg (CMsgItem * pItem) = 0;
};

/*
// those function should be called in top level
int		QCMSG_Init (CBaseInst * pBaseInst);
int		QCMSG_Close(CBaseInst * pBaseInst);

int		QCMSG_RegNotify(CBaseInst * pBaseInst, CMsgReceiver * pReceiver);
int		QCMSG_RemNotify(CBaseInst * pBaseInst, CMsgReceiver * pReceiver);


// this function is async, it will return immedidately
int		QCMSG_Notify(CBaseInst * pBaseInst, int nMsg, int nValue, long long llValue);
int		QCMSG_Notify(CBaseInst * pBaseInst, int nMsg, int nValue, long long llValue, const char * pValue);
int		QCMSG_Notify(CBaseInst * pBaseInst, int nMsg, int nValue, long long llValue, const char * pValue, void * pInfo);

// this function is async, it will return immedidately
int		QCMSG_Send(CBaseInst * pBaseInst, int nMsg, int nValue, long long llValue);
int		QCMSG_Send(CBaseInst * pBaseInst, int nMsg, int nValue, long long llValue, const char * pValue);
int		QCMSG_Send(CBaseInst * pBaseInst, int nMsg, int nValue, long long llValue, const char * pValue, void * pInfo);
*/

#endif // __UMsgMng_H__
