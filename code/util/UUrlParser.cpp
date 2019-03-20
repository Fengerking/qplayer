/*******************************************************************************
	File:		UUrlParser.cpp

	Contains:	url parser implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-06		Bangfei			Create file

*******************************************************************************/
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef __QC_OS_WIN32__
#include "windows.h"
#endif // __QC_OS_WIN32__

#include "qcErr.h"

#include "UUrlParser.h"
#include "USocketFunc.h"

#define HTTP_PREFIX_LENGTH	7
#define HTTPS_PREFIX_LENGTH	8
#define RTMP_PREFIX_LENGTH 7

static bool isSplashCharacter(char character) 
{
	return character == '/' || character == '\\';
}

static bool isDotCharacter(char character) 
{
	return character == '.';
}

static void upperCaseString(const char* head, const char* tail, char* upperCasedString) 
{
	if (head != NULL && tail != NULL)
	{
		do
		{
			*upperCasedString++ = toupper(*head++);
		}while (head < tail);
	}

	*upperCasedString = '\0';
}

static void copyString(const char* head, const char* tail, char* copiedString) 
{
	if (head != NULL && tail != NULL)
	{
		do
		{
			*copiedString++ = *head++;
		}while (head < tail);
	}

	*copiedString = '\0';
}

static const char* ptrToNearestDotFromStringTail(const char* head, const char* tail) 
{
	const char* ptr = tail;
	char character;

	do 
	{
		character = *(--ptr);
	} while (ptr >= head && !isSplashCharacter(character) && !isDotCharacter(character));
	ptr++;

	return character == '.' ? ptr : NULL;
}

static const char* ptrToNearestSplashFromStringTail(const char* head, const char* tail) 
{
	const char* ptr = tail;
	char character;

	do 
	{
		character = *(--ptr);
	} while (ptr >= head && !isSplashCharacter(character));
	ptr++;

	return ptr;
}

static const char* ptrToFirstQuestionMarkOrStringTail(const char* string) 
{
	const char* ptr = strchr(string, '?');
	if (ptr == NULL)
	{
		ptr = string + strlen(string);
	}

	return ptr;
}

static const char* ptrToExtHeadOrStringTail(const char* url) 
{
	const char* extTail = ptrToFirstQuestionMarkOrStringTail(url);

	const char* extHead = ptrToNearestDotFromStringTail(url, extTail);

	return (extHead != NULL) ? extHead - 1 : url + strlen(url);
}


void qcUrlParseProtocal(const char* aUrl, char* aProtocal)
{
	const char* protocalTail = strstr(aUrl, "://");
	int protocalLength = 0;
	if (protocalTail != NULL)
	{
		protocalLength = protocalTail - aUrl;
		memcpy(aProtocal, aUrl, protocalLength);
	}
	aProtocal[protocalLength] = '\0';
}

void qcUrlParseExtension(const char* aUrl, char* aExtension, int nExtensionLen)
{
	const char* tail = ptrToFirstQuestionMarkOrStringTail(aUrl);

	const char* head = ptrToNearestDotFromStringTail(aUrl, tail);
    
    if((tail - head) > nExtensionLen)
        tail = head + nExtensionLen;

	upperCaseString(head, tail, aExtension);

	// work around to remove "," in extension. For example,  MP3,1 -> MP3
	char* ptr = strchr(aExtension, ',');
	if (ptr != NULL) 
	{
		*ptr = '\0';
	}
}

void qcUrlParseShortName(const char* aUrl, char* aShortName)
{
	const char* shortNameTail = ptrToExtHeadOrStringTail(aUrl);

	const char* shortNameHead = ptrToNearestSplashFromStringTail(aUrl, shortNameTail);

	copyString(shortNameHead, shortNameTail, aShortName);
}

void qcUrlParseUrl(const char* aUrl, char* aHost, char* aPath, int& aPort, char * aDomain)
{
	// parse host
	bool bHTTPS = false;
	char* hostHead = const_cast<char*> (aUrl);
	if (!strncmp(hostHead, "http://", HTTP_PREFIX_LENGTH))
	{
		hostHead += HTTP_PREFIX_LENGTH;
	}
	else if (!strncmp(hostHead, "https://", HTTPS_PREFIX_LENGTH))
	{
		hostHead += HTTPS_PREFIX_LENGTH;
		bHTTPS = true;
	}
    else if (!strncmp(hostHead, "rtmp://", RTMP_PREFIX_LENGTH))
    {
        hostHead += RTMP_PREFIX_LENGTH;
    }

	char* urlTail = hostHead + strlen(hostHead);
	char* hostTail = strchr(hostHead, '/');
	if (hostTail == NULL)
		hostTail = urlTail;

	// Parse host
	int hostLength = hostTail - hostHead;
	memcpy(aHost, hostHead, hostLength);
	aHost[hostLength] = '\0';

	// Parse port
	char* pCharColon = strchr(aHost, ':');
	if (pCharColon != NULL)
	{
		*pCharColon++ = '\0';
		aPort = atoi(pCharColon);
	}
	else
	{
		aPort = 80;
		if (bHTTPS)
			aPort = 443;
	}

	// Parse domain
	char * pCharAT = NULL;
	char * pDomain = strstr((char *)aUrl, "?domain=");
	if (pDomain == NULL)
		pDomain = strstr((char *)aUrl, "&domain=");
	if (aDomain != NULL)
		aDomain[0] = 0;
	if (pDomain != NULL && aDomain != NULL)
	{
		pCharAT = strstr(pDomain + 2, "&");
		if (pCharAT == NULL)
			strcpy(aDomain, pDomain + 8);
		else
		{
			int nDomainLen = pCharAT - pDomain - 8;
			strncpy(aDomain, pDomain + 8, nDomainLen);
			aDomain[nDomainLen] = 0;
		}
	}
    
	// parse path
	aPath[0] = '\0';
	if (hostTail < urlTail) 
	{
		char* pathHead = hostTail + 1;
		int pathLength = urlTail - pathHead;
		if (pDomain == NULL)
		{
			memcpy(aPath, pathHead, pathLength);
		}
		else
		{
			pCharAT = strstr(pDomain + 2, "&");
			pathLength = pDomain - pathHead;
			memcpy(aPath, pathHead, pathLength);
			aPath[pathLength] = '\0';

			if (pCharAT != NULL)
			{
				//pathLength += strlen(pDomain);
				//strcat(aPath, pDomain);			
				if (pDomain[0] == '?')
				{
					pathLength += strlen(pCharAT);
					strcat(aPath, "?");
					strcat(aPath, pCharAT + 1);
				}
				else
				{
					pathLength += strlen(pCharAT);
					strcat(aPath, pCharAT);
				}
			}
		}
		aPath[pathLength] = '\0';
	}
}

int	qcUrlConvert (const char * pURL, char * pDest, int nSize)
{
	if (pURL == NULL)
		return -1;
	int		nIndex = 0;
#ifdef __QC_OS_WIN32__
	char	szUTF8[2048];
	wchar_t	wzURL[2048];
	memset (wzURL, 0, sizeof (wzURL));
	memset (szUTF8, 0, sizeof (szUTF8));

	MultiByteToWideChar (CP_ACP, 0, pURL, -1, wzURL, sizeof (wzURL));
	WideCharToMultiByte (CP_UTF8, 0, wzURL, -1, szUTF8, sizeof (szUTF8), NULL, NULL);
	pURL = szUTF8;

	char	szTemp[8];
	int		i = 0;
	int		nLen = strlen (pURL);
	for (i = 0; i < nLen; i++)  
	{  
		if (pURL[i] > 0)
		{
			pDest[nIndex] = pURL[i];
			nIndex++;
		}
		else
		{
			sprintf (szTemp,"%%%X%X",((unsigned char*)pURL)[i] >>4,((unsigned char*)pURL)[i] %16);  
			strcat (pDest, szTemp);
			nIndex += strlen (szTemp);
		}
		if (nIndex >= nSize)
			return -1;
	} 
#else
	strcpy (pDest, pURL);
	nIndex = strlen (pDest);
#endif // __QC_OS_WIN32__
	return nIndex;
}

