/*******************************************************************************
	File:		UFileFunc.cpp

	Contains:	The utility for file operation implement file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#ifdef _OS_LINUX
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif // _OS_LINUX

#include "UFileFunc.h"

#ifdef __QC_OS_WIN32__
#pragma warning (disable : 4996)
#endif // __QC_OS_WIN32__

qcFile qcFileOpen (char * pFile, int nFlag)
{
	qcFile	hFile = NULL;
#ifdef __QC_OS_WIN32__
	if (nFlag & QCFILE_READ)
		hFile = CreateFile(pFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, (DWORD) 0, NULL);
	else if (nFlag & QCFILE_WRITE) 
		hFile = CreateFile(pFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, (DWORD) 0, NULL);
	else
		hFile = CreateFile(pFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, (DWORD) 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		hFile = NULL;
#elif defined _OS_LINUX
	int nOpenFlag = O_RDONLY;
	int nMode = 0640;
	if (nFlag & QCFILE_READ)
		nOpenFlag = O_RDONLY;
	else if (nFlag & QCFILE_WRITE) 
		nOpenFlag = O_RDWR | O_CREAT;
	else
		nOpenFlag = O_RDWR | O_CREAT;
	hFile = open (pFile, nOpenFlag, nMode);
#endif// __QC_OS_WIN32__
	return hFile;
}

int qcFileRead (qcFile hFile, unsigned char * pBuff, int nSize)
{
	int nRead = 0;
#ifdef __QC_OS_WIN32__
	ReadFile (hFile, pBuff, nSize, (DWORD *)&nRead, NULL);
#elif defined _OS_LINUX
	nRead = read (hFile, pBuff, nSize);
#endif// __QC_OS_WIN32__
	return nRead;
}

int qcFileWrite (qcFile hFile, unsigned char * pBuff, int nSize)
{
	int nWrite = 0;
#ifdef __QC_OS_WIN32__
	WriteFile (hFile, pBuff, nSize, (DWORD *)&nWrite, NULL);
#elif defined _OS_LINUX
	nWrite = write (hFile, pBuff, nSize);
#endif// __QC_OS_WIN32__
	return nWrite;
}

long long qcFileSeek (qcFile hFile, long long llPos, int nFlag)
{
	long long llSeek = 0;
#ifdef __QC_OS_WIN32__
	long	lPos = (long)llPos;
	long	lHigh = (long)(llPos >> 32);
	int		sMove = FILE_BEGIN;
	if (nFlag == QCFILE_BEGIN)
		sMove = FILE_BEGIN;
	else if (nFlag == QCFILE_CUR)
		sMove = FILE_CURRENT;
	else if (nFlag == QCFILE_END)
		sMove = FILE_END;
	DWORD dwRC = SetFilePointer (hFile, lPos, &lHigh, sMove);
	if(dwRC == INVALID_SET_FILE_POINTER)
		llSeek = -1;
#elif defined _OS_LINUX
	int sMove = SEEK_SET;
	if (nFlag == QCFILE_BEGIN)
		sMove = SEEK_SET;
	else if (nFlag == QCFILE_CUR)
		sMove = SEEK_CUR;
	else if (nFlag == QCFILE_END)
		sMove = SEEK_END;
	llSeek = lseek64 (hFile, llPos, sMove);
#endif// __QC_OS_WIN32__
	return llSeek;
}

long long qcFileSize (qcFile hFile)
{
	long long llSize = 0;
#ifdef __QC_OS_WIN32__
	DWORD dwHigh = 0;
	DWORD dwSize = GetFileSize (hFile, &dwHigh);
	llSize = dwHigh;
	llSize = llSize << 32;
	llSize += dwSize;
#elif defined _OS_LINUX
	struct stat st;
	memset(&st, 0, sizeof(struct stat));		
    fstat(hFile, &st); 
	llSize = st.st_size;
#endif// __QC_OS_WIN32__
	return llSize;
}

int qcFileClose (qcFile hFile)
{
	if (hFile == NULL)
		return -1;

#ifdef __QC_OS_WIN32__
	CloseHandle (hFile);
#elif defined _OS_LINUX
	close (hFile);
#endif// __QC_OS_WIN32__
	return 0;
}

