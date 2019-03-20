/*******************************************************************************
	File:		USystemFunc.cpp

	Contains:	The base utility for system implement file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#ifdef __QC_OS_WIN32__
#include "stdio.h"
#include "WinSock2.h"
#include "shlobj.h"
#include "shlwapi.h"
#else
#include <stdio.h>
#include <unistd.h>
#include <time.h>      
#include <pthread.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <string.h>
#import <arpa/inet.h>
#import <net/if.h>
#endif // __QC_OS_WIN32__

#if defined (__QC_OS_LINUX__) || defined (__QC_OS_NDK__)
#include <cctype> // for toupper  
#include <sys/vfs.h>
#define statvfs statfs
#endif

#if defined (__QC_OS_IOS__) || defined (__QC_OS_MASOS__)
#include <ifaddrs.h>
#import <sys/sysctl.h>
#import <Foundation/Foundation.h>
#import <mach/mach.h>
#endif

#if defined (__QC_OS_IOS__)
#import <UIKit/UIKit.h>
#endif

#include "qcDef.h"
#include "qcErr.h"
#include "USystemFunc.h"
#include "ULogFunc.h"

char	g_szWorkPath[1024];

int		gqc_android_devces_ver = 8;

int		g_nDebugTime01 = 0;
int		g_nDebugTime02 = 0;
int		g_nDebugTime03 = 0;

int	qcGetSysTime (void)
{
	int nTime = 0;
#ifdef __QC_OS_WIN32__
	nTime = GetTickCount ();
	if (nTime < 0)
		nTime = nTime & 0X7FFFFFFF;
#elif defined __QC_OS_LINUX__ || defined __QC_OS_NDK__
    timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);

    static timespec stv = {0, 0};
    if ((0 == stv.tv_sec) && (0 == stv.tv_nsec))
	{
		stv.tv_sec = tv.tv_sec;
		stv.tv_nsec = tv.tv_nsec;
	}
    
    nTime = (int)((tv.tv_sec - stv.tv_sec) * 1000 + (tv.tv_nsec - stv.tv_nsec) / 1000000);
#elif defined __QC_OS_IOS__
    static double uptime = [[NSProcessInfo processInfo] systemUptime];
    nTime = ((long long)(([[NSProcessInfo processInfo] systemUptime] - uptime) * 1000)) & 0x7FFFFFFF;
#endif // __QC_OS_WIN32__

	return nTime;
}

void qcSleep (int nTime)
{
#ifdef __QC_OS_WIN32__
	Sleep (nTime / 1000);
#else
	usleep (nTime);
#endif // __QC_OS_WIN32__
}

void qcSleepEx(int nTime, bool* pExit)
{
    int nMilliTime = nTime / 1000;
    int nStep = 5000;
    if(nTime <= nStep)
    {
        qcSleep(nTime);
        return;
    }
    
    int nStart = qcGetSysTime();
    while(qcGetSysTime() < (nStart + nMilliTime))
    {
        if(pExit && *pExit)
            return;
        qcSleep(nStep);
    }
}


int qcGetThreadTime (void * hThread)
{
	int nTime = -1;
#ifdef __QC_OS_WIN32__
	if(hThread == NULL)
		hThread = GetCurrentThread();

	FILETIME ftCreationTime;
	FILETIME ftExitTime;
	FILETIME ftKernelTime;
	FILETIME ftUserTime;

	BOOL bRC = GetThreadTimes(hThread, &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUserTime);
	if (!bRC)
		return nTime;

	LONGLONG llKernelTime = ftKernelTime.dwHighDateTime;
	llKernelTime = llKernelTime << 32;
	llKernelTime += ftKernelTime.dwLowDateTime;

	LONGLONG llUserTime = ftUserTime.dwHighDateTime;
	llUserTime = llUserTime << 32;
	llUserTime += ftUserTime.dwLowDateTime;

	nTime = int((llKernelTime + llUserTime) / 10000);

#elif defined __QC_OS_LINUX__
    timespec tv;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tv);

    static timespec stvThread = {0, 0};
    if ((0 == stvThread.tv_sec) && (0 == stvThread.tv_nsec))
	{
		stvThread.tv_sec = tv.tv_sec;
		stvThread.tv_nsec = tv.tv_nsec;
	}
    
    nTime = (int)((tv.tv_sec - stvThread.tv_sec) * 1000 + (tv.tv_nsec - stvThread.tv_nsec) / 1000000);
#elif defined __QC_OS_IOS__
    // TODO

#endif // __QC_OS_WIN32__

	return nTime;
}

int	qcGetCPUNum (void)
{
#ifdef __QC_OS_NDK__
	int nTemps[] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21};
	char cCpuName[512];
	memset(cCpuName, 0, sizeof(cCpuName));

	for(int i = (sizeof(nTemps)/sizeof(nTemps[0])) - 1 ; i >= 0; i--)
	{
		sprintf(cCpuName, "/sys/devices/system/cpu/cpu%d", nTemps[i]);
		int nOk = access(cCpuName, F_OK);
		if( nOk == 0)
		{
			return nTemps[i]+1;
		}
	}
	return 1;
#elif defined __QC_OS_WIN32__
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
#endif

	return 1;
}

int	qcGetAppPath (void * hInst, char * pPath, int nSize)
{
#ifdef __QC_OS_WIN32__
	GetModuleFileName ((HMODULE)hInst, pPath, nSize);
    char * pPos = _tcsrchr(pPath, _T('/'));
	if (pPos == NULL)
		pPos = _tcsrchr(pPath, _T('\\'));
    int nPos = pPos - pPath;
    pPath[nPos+1] = _T('\0');	
#elif defined __QC_OS_NDK__
	FILE *		hFile = NULL;
	char		szPkgName[256];
	memset (szPkgName, 0, sizeof (szPkgName));
	hFile = fopen("/proc/self/cmdline", "rb");
	if (hFile != NULL)
	{  
		fgets(szPkgName, 256, hFile);
		fclose(hFile);
		strcpy (pPath, "/data/data/");
		strcat (pPath, szPkgName);
		strcat (pPath, "/");
	}
#elif defined __QC_OS_IOS__
    //NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *filePath = [[paths objectAtIndex:0] stringByAppendingString:@"/"];
    strcpy(pPath, [filePath UTF8String]);
    //[pool release];
#endif // __QC_OS_WIN32__

	return 0;
}

bool qcDeleteFolder (char * pFolder)
{
#ifdef __QC_OS_WIN32__
	char	szFolder[1024];
	char	szFilter[1024];
	_tcscpy (szFilter, pFolder);
	_tcscat (szFilter, _T("\\*.*"));
	WIN32_FIND_DATA  data;
	HANDLE  hFind = FindFirstFile(szFilter,&data);
	if (hFind == INVALID_HANDLE_VALUE)
		return true;
	do
	{
		if (!_tcscmp (data.cFileName, _T(".")) || !_tcscmp (data.cFileName, _T("..")))
			continue;	
		
		_tcscpy (szFolder, pFolder);
		_tcscat (szFolder, _T("\\"));
		_tcscat (szFolder, data.cFileName);
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			qcDeleteFolder (szFolder);
		}

		DeleteFile (szFolder);
	}while(FindNextFile(hFind, &data));
	FindClose (hFind);

	BOOL bRC = RemoveDirectory (pFolder);

	return bRC == TRUE ? true : false;
#else
	return false;
#endif // __QC_OS_WIN32__
}

bool qcPathExist (char* pPath)
{
    bool bExist = false;
#ifdef __QC_OS_WIN32__
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
	char szFind[2048];
	strcpy(szFind, pPath);
	strcat(szFind, "\\*.*");
	hFind = FindFirstFile(szFind, &ffd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		bExist = true;
		FindClose(hFind);
	}
#else
    bExist = (-1 != access(pPath, 0));
#endif
    
    return bExist;
}

bool qcCreateFolder (char * pFolder)
{
	if (qcPathExist(pFolder))
		return true;

	char szNewPath[2048];
	memset(szNewPath, 0, sizeof(szNewPath));
	strcpy(szNewPath, pFolder);

	char * pPos = strchr(szNewPath, '\\');
	while (pPos != NULL)
	{
		*pPos = '/';
		pPos++;
		pPos = strchr(szNewPath, '\\');
	}

	pPos = strchr(szNewPath, '/');
	if (pPos == NULL)
		return false;
	pPos = strchr(pPos + 1, '/');
	while (pPos != NULL)
	{
#ifdef __QC_OS_WIN32__
		*pPos = 0;
		if (PathFileExists(szNewPath) == FALSE)
			CreateDirectory(szNewPath, NULL);
		*pPos = '/';
		pPos = strchr(pPos + 1, '/');
		if (pPos == NULL)
		{
			if (PathFileExists(szNewPath) == FALSE)
				CreateDirectory(szNewPath, NULL);
		}
#elif defined __QC_OS_NDK__ || defined __QC_OS_IOS__ || defined __QC_OS_MACOS__
        *pPos = 0;
        if(!qcPathExist(szNewPath))
            mkdir(szNewPath, 0777);
        *pPos = '/';
        pPos = strchr(pPos + 1, '/');
        if (pPos == NULL)
        {
            if(!qcPathExist(szNewPath))
                mkdir(szNewPath, 0777);
        }
#else

#endif // __QC_OS_WIN32__
	}
	return false;
}

signed long long qcGetUTC(void)
{
    signed long long utc = -1;
    
#ifdef __QC_OS_WIN32__
    static const unsigned __int64 epoch = ((unsigned __int64)116444736000000000ULL);
    SYSTEMTIME sysTime;
    FILETIME fileTime;
    ULARGE_INTEGER largeInt;
    
    GetSystemTime(&sysTime);
    SystemTimeToFileTime(&sysTime, &fileTime);
    largeInt.HighPart = fileTime.dwHighDateTime;
    largeInt.LowPart = fileTime.dwLowDateTime;
    utc = (largeInt.QuadPart - epoch) / 10000L;
#elif defined __QC_OS_LINUX__ || defined __QC_OS_NDK__ || defined __QC_OS_IOS__ || defined __QC_OS_MACOS__
    struct timeval tv;
    gettimeofday(&tv, NULL);
    utc = tv.tv_sec;
    utc = utc * 1000  + tv.tv_usec / 1000;
#endif // __QC_OS_WIN32__
    
    return utc;
}

int	qcGetProcessID(void)
{
	int nProcID = 0;
#ifdef __QC_OS_WIN32__
	nProcID = (int)GetCurrentProcessId();
#elif defined __QC_OS_NDK__
	nProcID = getpid ();
#elif defined __QC_OS_IOS__ || defined __QC_OS_MACOS__
    nProcID = getpid ();
#else
#endif // __QC_OS_WIN32__
	return nProcID;
}

bool qcIsIPv6()
{
    bool bIPv6 = false;
    
#if defined __QC_OS_IOS__ || defined __QC_OS_MACOS__
    struct ifaddrs *interfaces;
    if(!getifaddrs(&interfaces))
    {
        // Loop through linked list of interfaces
        struct ifaddrs *interface;
        for(interface=interfaces; interface; interface=interface->ifa_next)
        {
            if(!(interface->ifa_flags & IFF_UP) /* || (interface->ifa_flags & IFF_LOOPBACK) */ ) {
                continue; // deeply nested code harder to read
            }
            
            char addrBuf[INET6_ADDRSTRLEN];
            const struct sockaddr_in *addr = (const struct sockaddr_in*)interface->ifa_addr;
            
            if(addr && (addr->sin_family==AF_INET || addr->sin_family==AF_INET6))
            {
                if(addr->sin_family == AF_INET)
                {
                    if(inet_ntop(AF_INET, &addr->sin_addr, addrBuf, INET_ADDRSTRLEN))
                    {
                        //printf("[I]ipv4 %s, %s\n",interface->ifa_name, addrBuf);
                    }
                }
                else
                {
                    const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6*)interface->ifa_addr;
                    if(inet_ntop(AF_INET6, &addr6->sin6_addr, addrBuf, INET6_ADDRSTRLEN))
                    {
                        //printf("[I]ipv6 %s, %s\n", interface->ifa_name, addrBuf);
                        if(!strcmp(interface->ifa_name, "awdl0")		// Apple Wireless Direct Link
                           || !strcmp(interface->ifa_name, "en0")		// WIFI
                           || !strcmp(interface->ifa_name, "utun0")		// VPN
                           || !strcmp(interface->ifa_name, "pdp_ip0")	// CELLULAR
                           )
                        {
                            char* found = strstr(addrBuf, "fe80");
                            if(!found)
                            {
                                bIPv6 = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        // Free memory
        freeifaddrs(interfaces);
    }
#else
    char szHostName[65];
    if (gethostname(szHostName, sizeof(szHostName)) >= 0)
    {
        struct hostent* hp = gethostbyname(szHostName);
        if (hp != NULL && hp->h_addrtype == AF_INET)
            bIPv6 = false;
        else if (hp != NULL && hp->h_addrtype == AF_INET6)
            bIPv6 = true;
    }
#endif // __QC_OS_IOS__
    
    return bIPv6;
}


long long qcGetFreeSpace(const char * pPath)
{
	long long llSpaceSize = 0;
#ifdef __QC_OS_WIN32__
	ULARGE_INTEGER llFree;
	ULARGE_INTEGER	llTotal;
	ULARGE_INTEGER llTotalFree;
	memset(&llFree, 0, sizeof(llFree));
	memset(&llTotal, 0, sizeof(llFree));
	memset(&llTotalFree, 0, sizeof(llTotalFree));
	if (GetDiskFreeSpaceEx(pPath, &llFree, &llTotal, &llTotalFree) == TRUE)
	{
		llSpaceSize = llFree.QuadPart;
	}
#elif defined __QC_OS_LINUX__ || defined __QC_OS_NDK__
	struct statfs diskInfo;
	statfs(pPath, &diskInfo);
	unsigned long long blocksize = diskInfo.f_bsize;	
	unsigned long long totalsize = blocksize * diskInfo.f_blocks;	
	unsigned long long freeDisk = diskInfo.f_bfree * blocksize;		
	unsigned long long availableDisk = diskInfo.f_bavail * blocksize;   
	llSpaceSize = (long long)availableDisk;	
#elif defined __QC_OS_IOS__ || defined __QC_OS_MACOS__
    NSDictionary *fattributes = [[NSFileManager defaultManager] attributesOfFileSystemForPath:NSHomeDirectory() error:nil];
    llSpaceSize = [[fattributes objectForKey:NSFileSystemFreeSize] longLongValue];
#endif // __QC_OS_WIN32__

	return llSpaceSize;
}

int	qcReadTextLine(char * pData, int nSize, char * pLine, int nLine)
{
	if (pData == NULL)
		return 0;

	char * pBuff = pData;
	while (pBuff - pData < nSize)
	{
		if (*pBuff == '\r' || *pBuff == '\n')
		{
			pBuff++;
			if (*(pBuff) == '\r' || *(pBuff) == '\n')
				pBuff++;
			break;
		}
		pBuff++;
	}

	int nLineLen = pBuff - pData;
	if (nLine > nLineLen)
	{
		int nRNLen = 0;
		pBuff--;
		while (pBuff > pData && (*pBuff == '\r' || *pBuff == '\n'))
		{
			nRNLen++;
			pBuff--;
		}

		memset(pLine, 0, nLine);
		strncpy(pLine, pData, nLineLen - nRNLen);
	}
	return nLineLen;
}

bool qcIsEnableAnalysis()
{
    char szAppID[512];
    memset(szAppID, 0, 512);
#ifdef __QC_OS_NDK__
    qcGetAppPath (NULL, szAppID, sizeof (szAppID));
    szAppID[strlen (szAppID)-1] = 0;
#elif defined __QC_OS_IOS__
    NSString* appID = [[NSBundle mainBundle] bundleIdentifier];
    if(appID)
        strcpy(szAppID, [appID UTF8String]);
#endif
    
    if(strlen(szAppID) == 0)
        return true;
    
    if(strstr(szAppID, "com.yaoyao.live"))
        return false;
    else if(strstr(szAppID, "com.wf.custom"))
        return false;
//    else if(strstr(szAppID, "com.qiniu."))
//        return false;

    return true;
}

int qcGetAppID(char* pID, int nSize)
{
    if(!pID)
        return 0;
#ifdef __QC_OS_NDK__
    qcGetAppPath (NULL, pID, sizeof (pID));
    pID[strlen (pID)-1] = 0;
#elif defined __QC_OS_IOS__
    NSString* appID = [[NSBundle mainBundle] bundleIdentifier];
    if(appID)
        strcpy(pID, [appID UTF8String]);
#endif
    
    return 0;
}

int qcStrComp(const char * pString1, const char * pString2, int nCompareLen/*=-1*/, bool bCaseSenstive/*=false*/)
{
    if(!pString1 || !pString2)
        return 1;
    
    if(bCaseSenstive)
    {
        if(nCompareLen == -1)
            return strcmp(pString1, pString2);
        else
            return strncmp(pString1, pString2, nCompareLen);
    }
    else
    {
        char * tmp1 = new char[strlen(pString1) + 1];
        char * tmp2 = new char[strlen(pString2) + 1];
        
        char * tmp = tmp1;
        int len = strlen(pString1);
        for (int n=0; n<len; n++)
            *tmp++ = toupper(*pString1++);
        *tmp = '\0';
        
        tmp = tmp2;
        len = strlen(pString2);
        for (int n=0; n<len; n++)
            *tmp++ = toupper(*pString2++);
        *tmp = '\0';

		int nRC = 1;
        if(nCompareLen == -1)
            nRC = strcmp(tmp1, tmp2);
        else
            nRC = strncmp(tmp1, tmp2, nCompareLen);
		delete []tmp1;
		delete []tmp2;
		return nRC;
    }
    
    return 1;
}

int qcGetCachePath (void * hInst, char * pPath, int nSize)
{
#ifdef __QC_OS_WIN32__
    //
	BOOL bRC = SHGetSpecialFolderPath (NULL, pPath, CSIDL_COMMON_APPDATA, TRUE);
	if (!bRC)
		return QC_ERR_FAILED;
	TCHAR szApp[1024];
	memset (szApp, 0, sizeof (szApp));
	GetModuleFileName (NULL, szApp, sizeof (szApp));
	TCHAR * pDir = _tcsrchr (szApp, _T('\\'));
	if (pDir == NULL)
		return QC_ERR_FAILED;
	TCHAR * pExt = _tcsrchr(szApp, _T('.'));
	if (pExt != NULL)
		*pExt = 0;
	_tcscat(pPath, pDir);
	DWORD dwAttr = GetFileAttributes(pPath);
	if (dwAttr == -1)
		bRC = CreateDirectory(pPath, NULL);
	_tcscat(pPath, _T("\\"));
	return 0;
#elif defined __QC_OS_NDK__
    //
	return qcGetAppPath(hInst, pPath, nSize);
#elif defined __QC_OS_IOS__
    NSString *cachePath = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES).firstObject;
    strcpy(pPath, [cachePath UTF8String]);
    strcat (pPath, "/");
#endif // __QC_OS_WIN32__
    
    return 0;
}

int qcDumpFile(char* pBuff, int nBuffSize, char* pszExtName)
{
#ifdef __QC_OS_IOS__
    static FILE* hFile = NULL;
    if (NULL == hFile)
    {
        char szTmp[256];
        qcGetAppPath(NULL, szTmp, 256);
        strcat(szTmp, "dump.");
        if(pszExtName)
        	strcat(szTmp, pszExtName);
        else
            strcat(szTmp, "flv");
        hFile = fopen(szTmp, "wb");
    }
    
    if ((NULL != hFile) && (nBuffSize > 0) && pBuff)
    {
        return (int)fwrite(pBuff, 1, nBuffSize, hFile);
    }
#endif
    
    return 0;
}
