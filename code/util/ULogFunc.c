/*******************************************************************************
	File:		ULogFunc.c

	Contains:	Log printf implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#include "qcType.h"
#include "ULogFunc.h"

#ifdef __QC_OS_IOS__
#include <sys/timeb.h>
#include "USystemFunc.h"
#endif // __QC_OS_IOS__

#ifdef __QC_OS_NDK__
pthread_mutex_t*        g_hMutex = NULL;
#endif

int		g_nInitLogTime = 0;
int		g_nLogOutLevel = QC_LOG_LEVEL_DEBUG;

#ifdef __QC_OS_WIN32__
HANDLE                g_hLogFile = NULL;
#else
FILE *                g_hLogFile = NULL;
#endif // _OS_WIN32

void qcCheckLogCache()
{
#ifdef __QC_OS_NDK__
    if(!g_hLogFile)
        return;
    if (g_nLogOutLevel < QC_LOG_LEVEL_DUMP)
        return;
    fflush(g_hLogFile);
#endif
}

void qcDumpLog(const char * pLogText)
{
    if (g_nLogOutLevel >= QC_LOG_LEVEL_DUMP)
    {
#ifdef __QC_OS_WIN32__
//        if(!g_hLogFile)
//        {
//            qcGetAppPath(NULL, szDir, 2048)
//            g_hLogFile = CreateFile(pNewURL, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, (DWORD) 0, NULL);
//
//            if (g_hLogFile == INVALID_HANDLE_VALUE)
//            {
//                g_hLogFile = NULL;
//                return;
//            }
//        }
#elif defined __QC_OS_NDK__
        if (!g_hMutex)
        {
            pthread_mutexattr_t attr = PTHREAD_MUTEX_RECURSIVE_NP;
            g_hMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
            pthread_mutex_init (g_hMutex, &attr);
        }
        pthread_mutex_lock (g_hMutex);
        
        struct timeval t;
        long long curr = 0;
        if (gettimeofday(&t, NULL) == 0)
        {
            curr = t.tv_sec ;
            curr = curr * 1000 + t.tv_usec / 1000;
        }
        int mmsec = curr % 1000;
        
        struct tm date;
        time_t totalTime = curr/1000;
        localtime_r(&totalTime, &date);
        
        char log[1024];
        memset( log, 0, 1024);
        int len = snprintf(log, 1024, "%04d-%02d-%02d %02d:%02d:%02d.%03d %s", date.tm_year + 1900, date.tm_mon + 1, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec, mmsec, pLogText);
        
        if(!g_hLogFile)
        {
            char szTmp[256];
            sprintf(szTmp, "%s", "/sdcard/");
            strcat(szTmp, "core.txt");
            g_hLogFile = fopen(szTmp, "wb");
            if (g_hLogFile == NULL)
            {
                pthread_mutex_unlock (g_hMutex);
                return;
            }
        }

        fwrite(log, 1, len, g_hLogFile);
        pthread_mutex_unlock (g_hMutex);
#elif defined __QC_OS_IOS__
        if(!g_hLogFile)
        {
            char szTmp[256];
            qcGetAppPath(NULL, szTmp, 256);
            strcat(szTmp, "core.txt");
            g_hLogFile = fopen(szTmp, "wb");
            if (g_hLogFile == NULL)
                return;
        }
        fwrite(pLogText, 1, strlen(pLogText), g_hLogFile);
#endif // __QC_OS_WIN32__
    }
}


void qcDisplayLog (const char * pLogText)
{
#ifdef __QC_OS_WIN32__
#ifdef _UNICODE
	char wzLogText[1024];
	int	nLogTime =  0;
	if (g_nInitLogTime == 0)
		g_nInitLogTime = GetTickCount ();
	nLogTime = GetTickCount () - g_nInitLogTime;
	_stprintf (wzLogText, _T("%02d:%02d:%03d   "), ((nLogTime / 1000) % 3600) / 60, (nLogTime / 1000) % 60, nLogTime % 1000);
	MultiByteToWideChar (CP_ACP, 0, pLogText, -1, wzLogText + 10, sizeof (wzLogText));
#ifdef _CPU_MSB2531
	RETAILMSG (1, (wzLogText));
#else
	OutputDebugString (wzLogText); 
#endif // _CPU_MSB2531
#else
	OutputDebugString (pLogText); 
#endif // _UNIDCOE
    
#elif defined __QC_OS_IOS__
    struct timeb curr;
    ftime(&curr);
    struct tm btm;
    localtime_r(&curr.time, &btm);

    char log[1024];
    snprintf(log, 1024, "%04d-%02d-%02d %02d:%02d:%02d.%03d %s", btm.tm_year + 1900, btm.tm_mon + 1, btm.tm_mday, btm.tm_hour, btm.tm_min, btm.tm_sec, curr.millitm, pLogText);
    printf("%s", log);
    qcDumpLog(log);
#endif // __QC_OS_WIN32__
}


void qcDisplayMsg (void * hWnd, const char * pLogText)
{
#ifdef __QC_OS_WIN32__
#ifdef _UNICODE
	char wzLogText[1024];
	MultiByteToWideChar (CP_ACP, 0, pLogText, -1, wzLogText, sizeof (wzLogText));
	MessageBox ((HWND)hWnd, wzLogText, _T("QCMSG"), MB_OK); 
#else
	MessageBox ((HWND)hWnd, pLogText, _T("QCMSG"), MB_OK); 
#endif // _UNIDCOE
#endif // __QC_OS_WIN32__
}
