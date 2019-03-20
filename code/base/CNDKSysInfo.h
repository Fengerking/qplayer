/*******************************************************************************
	File:		CNDKSysInfo.h

	Contains:	The android ndk system info header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-15		Bangfei			Create file

*******************************************************************************/
#ifndef __CNDKSysInfo_H__
#define __CNDKSysInfo_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <android/log.h>
#include <unistd.h>

#include "qcType.h"
#include "qcData.h"

typedef unsigned long long DWORD;

struct systemCPUdata
{
    unsigned long utime,stime,cutime,cstime;
    unsigned long vmem, rmem;
    char pname[255];
};

class AndroidMemInfo
{
public:
    AndroidMemInfo();
    ~AndroidMemInfo();    

    void GetTotalPhys();
    void GetAvailPhys();

    DWORD ullTotalPhys;
    DWORD ullAvailPhys;

private:
    int m_memFile;
};

class AndroidCpuUsage
{
public:
    AndroidCpuUsage();

    struct timeval m_lasttime,m_nowtime;
    struct systemCPUdata m_lastproc, m_nowproc;

    void getProcCPUStat(struct systemCPUdata *proc, int pid);

    DWORD GetUsedCpu();
};

#endif // __CNDKSysInfo_H__
