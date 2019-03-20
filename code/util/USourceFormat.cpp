/*******************************************************************************
	File:		USourceFormat.cpp

	Contains:	The utility for file operation implement file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-22		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcDef.h"
#include "qcErr.h"

#include "USourceFormat.h"
#include "UFileFunc.h"

bool qcChangeSourceName (char * pName, int nNum, bool bUP)
{
	for (int i = 0; i < nNum; i++)
	{
		if (bUP)
		{
			if (*(pName + i) >= 'a' && *(pName + i) <= 'z')
				*(pName + i) -= 'a' - 'A';
		}
		else
		{
			if (*(pName + i) >= 'A' && *(pName + i) <= 'Z')
				*(pName + i) += 'a' - 'A';
		}
	}

	return true;
}

QCIOProtocol qcGetSourceProtocol (const char * pSource)
{
	char szProt[8];
	memset (szProt, 0, 8);
	strncpy (szProt, pSource, 6);
	qcChangeSourceName (szProt, 5, false);
	if (!strncmp (szProt, "http:", 5))
		return QC_IOPROTOCOL_HTTP;
	else if (!strncmp (szProt, "https:", 6))
		return QC_IOPROTOCOL_HTTP;
	else if (!strncmp (szProt, "rtmp:", 5))
		return QC_IOPROTOCOL_RTMP;
	else if (!strncmp(szProt, "rtsp:", 5))
		return QC_IOPROTOCOL_RTSP;

	return QC_IOPROTOCOL_FILE;

}

bool qcIsExtNameEqual(const char* pExtName, const char* pEqual)
{
    int nLen = strlen(pEqual);
    
    if(!strncmp (pExtName, pEqual, nLen) && strlen(pExtName)==nLen)
        return true;
    
    return false;
}

QCParserFormat	qcGetSourceFormat (const char * pSource)
{
	//http://218.92.209.74/897756da-ec18-4c55-ae91-cd0ceda988a0.mp3?domain=7xsmlc.com1.z1.glb.clouddn.com

	QCParserFormat nFormat = QC_PARSER_NONE;
	char * pURL = new char[strlen(pSource) + 1];
	strcpy(pURL, pSource);

	char *	pExtChar = strstr(pURL, "?domain=");
	//char *	pExtChar = strstr(pURL, "?");
	if (pExtChar != NULL)
		*pExtChar = 0;
	else
	{
		pExtChar = strstr(pURL, "?");
		if (pExtChar != NULL)
		{
			char * pExtOrign = pExtChar;
			*pExtChar = 0;
			pExtChar = strrchr(pURL, '.');
			if (pExtChar != NULL)
			{
				if (qcIsExtNameEqual(pExtChar+1, "mp3"))
					nFormat = QC_PARSER_MP3;
				else if (qcIsExtNameEqual(pExtChar + 1, "m3u8"))
					nFormat = QC_PARSER_M3U8;
				else if (qcIsExtNameEqual(pExtChar + 1, "flv"))
					nFormat = QC_PARSER_FLV;
				else if (qcIsExtNameEqual(pExtChar + 1, "mp4") || qcIsExtNameEqual(pExtChar + 1, "m4v") ||
					qcIsExtNameEqual(pExtChar + 1, "mpa") || qcIsExtNameEqual(pExtChar + 1, "m4a"))
					nFormat = QC_PARSER_MP4;
				else if (qcIsExtNameEqual(pExtChar + 1, "ts"))
					nFormat = QC_PARSER_TS;
				else if (qcIsExtNameEqual(pExtChar + 1, "aac"))
					nFormat = QC_PARSER_AAC;
				else if (qcIsExtNameEqual(pExtChar + 1, "ffconcat") || qcIsExtNameEqual(pExtChar + 1, "concat") || qcIsExtNameEqual(pExtChar + 1, "ffcat"))
					nFormat = QC_PARSER_FFCAT;

				if (nFormat != QC_PARSER_NONE)
				{
					delete[] pURL;
					return nFormat;
				}
			}
			*pExtOrign = '?';
		}
	}
	pExtChar = strrchr(pURL, '.');
	if (pExtChar == NULL)
    {
        delete []pURL;
        return QC_PARSER_NONE;
    }

	int	nExtLen = strlen (pExtChar);

	char * pExtName = new char[nExtLen+1];
	memset (pExtName, 0, nExtLen + 1);
	strncpy (pExtName, pExtChar + 1, nExtLen-1);
	qcChangeSourceName (pExtName, nExtLen, false);
    
	if (qcIsExtNameEqual(pExtName, "flv"))
		nFormat = QC_PARSER_FLV;
	else if (qcIsExtNameEqual(pExtName, "mp4") || qcIsExtNameEqual(pExtName, "m4v") ||
		qcIsExtNameEqual(pExtName, "mpa") || qcIsExtNameEqual(pExtName, "m4a"))
		nFormat = QC_PARSER_MP4;
	else if (qcIsExtNameEqual(pExtName, "m3u8"))
		nFormat = QC_PARSER_M3U8;
	else if (qcIsExtNameEqual(pExtName, "ts"))
		nFormat = QC_PARSER_TS;
	else if (qcIsExtNameEqual(pExtName, "mp3"))
		nFormat = QC_PARSER_MP3;
	else if (qcIsExtNameEqual(pExtName, "aac"))
		nFormat = QC_PARSER_AAC;
	else if (qcIsExtNameEqual(pExtChar + 1, "ffconcat") || qcIsExtNameEqual(pExtChar + 1, "concat") || qcIsExtNameEqual(pExtChar + 1, "ffcat"))
		nFormat = QC_PARSER_FFCAT;

    delete []pExtName;
	delete []pURL;

	return nFormat;
}

QCParserFormat qcGetSourceFormat(const char * pSource, QC_IO_Func * pIO)
{
	if (pSource == NULL || pIO == NULL || pIO->hIO == NULL)
		return QC_PARSER_NONE;

	QCParserFormat	nFormat = QC_PARSER_NONE;
	int				nRC = QC_ERR_NONE;
	if (pIO->GetSize(pIO->hIO) <= 0)
	{
		nRC = pIO->Open(pIO->hIO, pSource, 0, QCIO_FLAG_READ);
		if (nRC != QC_ERR_NONE)
			return nFormat;
	}
	long long nSize = pIO->GetSize(pIO->hIO);
	if (nSize > 1024 || nSize == -1)
		nSize = 1024;
	unsigned char * pData = new unsigned char[nSize];
	nRC = pIO->ReadSync(pIO->hIO, 0, pData, nSize, QCIO_READ_HEAD);
	if (nRC <= 0)
	{
		delete[]pData;
		return nFormat;
	}
	pIO->SetPos(pIO->hIO, 0, QCIO_SEEK_BEGIN);

	if (!strncmp((char *)pData, "#EXTM3U", 7))
		nFormat = QC_PARSER_M3U8;
	else if (!strncmp((char *)pData, "FLV", 3))
		nFormat = QC_PARSER_FLV;
	else if (!strncmp((char *)pData, "ffconcat", 8))
		nFormat = QC_PARSER_FFCAT;
	else
	{
		char szMP4Ext[5];
		strcpy(szMP4Ext, "moov");
		char szMP4Ext1[16];
		strcpy(szMP4Ext1, "ftypmp42");
		char szMP4Ext2[16];
		strcpy(szMP4Ext2, "ftypisom");
		char szMP4Ext3[16];
		strcpy(szMP4Ext3, "ftypqt");
		unsigned char * pFind = pData;
		while ((pFind - pData) < nSize - 4)
		{
			if (!memcmp(pFind, szMP4Ext, 4))
			{
				nFormat = QC_PARSER_MP4;
				break;
			}
			else if (!memcmp(pFind, szMP4Ext1, 8))
			{
				nFormat = QC_PARSER_MP4;
				break;
			}
			else if (!memcmp(pFind, szMP4Ext2, 8))
			{
				nFormat = QC_PARSER_MP4;
				break;
			}
			else if (!memcmp(pFind, szMP4Ext3, 6))
			{
				nFormat = QC_PARSER_MP4;
				break;
			}
			pFind++;
		}
	}

	if (nFormat == QC_PARSER_NONE)
	{
		char * pType = NULL;
		if (pIO->GetParam(pIO->hIO, QCIO_PID_HTTP_CONTENT_TYPE, &pType) == QC_ERR_NONE)
		{
            if (pType)
            {
                if (!strcmp(pType, "audio/mpeg") || !strcmp(pType, "audio/mp3"))
                    nFormat = QC_PARSER_MP3;
                else if (!strcmp(pType, "audio/aac"))
                    nFormat = QC_PARSER_AAC;
                else if (!strcmp(pType, "video/mp4") || !strcmp(pType, "video/m4v") || !strcmp(pType, "audio/m4a"))
                    nFormat = QC_PARSER_MP4;
                else if (!strcmp(pType, "video/flv"))
                    nFormat = QC_PARSER_FLV;
                else if (!strcmp(pType, "video/hls"))
                    nFormat = QC_PARSER_M3U8;
                else if (!strcmp(pType, "video/m3u8"))
                    nFormat = QC_PARSER_M3U8;
            }
		}
	}
	delete[]pData;
	return nFormat;
}
