/*******************************************************************************
	File:		USystemFunc.h

	Contains:	The base utility for system header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#ifndef __USystemFunc_H__
#define __USystemFunc_H__

#include "qcType.h"
#include "ULibFunc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int					qcGetSysTime (void);
void				qcSleep (int nTime);
void 				qcSleepEx(int nTime, bool* pExit);
int					qcGetThreadTime (void * hThread);
int					qcGetCPUNum (void);
signed long long	qcGetUTC(void);

int					qcGetProcessID(void);

bool				qcIsIPv6();

int					qcGetAppPath (void * hInst, char * pPath, int nSize);
int                 qcGetCachePath (void * hInst, char * pPath, int nSize);
bool				qcDeleteFolder (char * pFolder);
bool				qcCreateFolder(char * pFolder);
long long			qcGetFreeSpace(const char * pPath);
bool				qcPathExist (char* pPath); // pPath can be folder or file
bool				qcIsEnableAnalysis();
int                 qcGetAppID(char* pID, int nSize);


int					qcReadTextLine(char * pData, int nSize, char * pLine, int nLine);
int 				qcStrComp(const char * pString1, const char * pString2, int nCompareLen/* = -1*/, bool bCaseSenstive/* = false*/);
    
int					qcDumpFile(char* pBuff, int nBuffSize, char* pszExtName);

extern	char		g_szWorkPath[1024];
extern	int			gqc_android_devces_ver;
extern	int			g_nDebugTime01;
extern	int			g_nDebugTime02;
extern	int			g_nDebugTime03;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __USystemFunc_H__
