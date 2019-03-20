/**
* File : TTBufferReader.cpp
* Created on : 2014-10-21
* Author : yongping.lin
* Description : CTTBufferReader实现文件
*/

// INCLUDES
#include <stdio.h>
#include <string.h>
#include "TTLog.h"
#include "GKMacrodef.h"
#include "GKOsalConfig.h"
#include "TTBufferReader.h"
#include "TTBufferReaderProxy.h"

//CTTBufferReaderProxy*	CTTBufferReader::iBufferReaderProxy = NULL;

CTTBufferReader::CTTBufferReader()
{
	iBufferReaderProxy = new CTTBufferReaderProxy();
}

CTTBufferReader::~CTTBufferReader()
{
	if (iBufferReaderProxy != NULL)
	{
		if (iBufferReaderProxy->Release() == 0)
		{
			iBufferReaderProxy = NULL;
		}
	}
}

TTInt CTTBufferReader::Open(const TTChar* aUrl)
{
	if (iBufferReaderProxy != NULL) {
		return iBufferReaderProxy->Open(aUrl);
	}

	return TTKErrNotReady;
}

TTInt CTTBufferReader::Close()
{	
	if (iBufferReaderProxy != NULL) {
		iBufferReaderProxy->Close();
	}
	return TTKErrNone;
}

TTInt CTTBufferReader::ReadSync(TTUint8* aReadBuffer, TTInt64 aReadPos, TTInt aReadSize)
{
	return iBufferReaderProxy->Read(aReadBuffer, aReadPos, aReadSize);
}

TTInt CTTBufferReader::ReadWait(TTUint8* aReadBuffer, TTInt64 aReadPos, TTInt aReadSize)
{
	return iBufferReaderProxy->ReadWait(aReadBuffer, aReadPos, aReadSize);
}

TTInt64 CTTBufferReader::Size() const
{
	return iBufferReaderProxy->Size();
}

void CTTBufferReader::SetDownSpeed(TTInt aFast)
{
	if (iBufferReaderProxy != NULL)
		iBufferReaderProxy->SetDownSpeed(aFast);
}

TTUint CTTBufferReader::BandWidth()
{
	return iBufferReaderProxy->BandWidth();
}

TTUint CTTBufferReader::BandPercent()
{
	return iBufferReaderProxy->BandPercent();
}

TTUint16 CTTBufferReader::ReadUint16(TTInt64 aReadPos)
{
	TTUint8 buf[sizeof(TTUint16)];
	if (sizeof(TTUint16) == ReadSync(buf, aReadPos, sizeof(TTUint16)))
	{
		return TTUint16((buf[1] << 8) | buf[0]);
	}
	return 0;
}

TTUint16 CTTBufferReader::ReadUint16BE(TTInt64 aReadPos)
{
	TTUint8 buf[sizeof(TTUint16)];
	if (sizeof(TTUint16) == ReadSync(buf, aReadPos, sizeof(TTUint16)))
	{
		return TTUint16((buf[0] << 8) | buf[1]);
	}
	return 0;
}

TTUint32 CTTBufferReader::ReadUint32(TTInt64 aReadPos)
{
	TTUint8 buf[sizeof(TTUint32)];
	if (sizeof(TTUint32) == ReadSync(buf, aReadPos, sizeof(TTUint32)))
	{
		return TTUint32((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
	}
	return 0;
}

TTUint32 CTTBufferReader::ReadUint32BE(TTInt64 aReadPos)
{
	TTUint8 buf[sizeof(TTUint32)];
	if (sizeof(TTUint32) == ReadSync(buf, aReadPos, sizeof(TTUint32)))
	{
		return TTUint32((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
	}
	return 0;
}

TTUint64 CTTBufferReader::ReadUint64(TTInt64 aReadPos)
{
	TTUint32 nMsb = ReadUint32(aReadPos);
	TTUint32 nLsb = ReadUint32(aReadPos + 4);

	TTUint64 nRetVal = nMsb;
	return (nRetVal << 32) | nLsb;
}

TTUint64 CTTBufferReader::ReadUint64BE(TTInt64 aReadPos)
{
	TTUint32 nLsb = ReadUint32BE(aReadPos);
	TTUint32 nMsb = ReadUint32BE(aReadPos + 4);

	TTUint64 nRetVal = nMsb;
	return (nRetVal << 32) | nLsb;
}

ITTDataReader::TTDataReaderId CTTBufferReader::Id()
{
	return ITTDataReader::ETTDataReaderIdBuffer;
}

TTUint64 CTTBufferReader::BufferedSize()
{
	GKASSERT(iBufferReaderProxy != NULL);
	return iBufferReaderProxy->BufferedSize();
}

TTUint CTTBufferReader::ProxySize()
{
	GKASSERT(iBufferReaderProxy != NULL);
	return iBufferReaderProxy->ProxySize();
}

void CTTBufferReader::SetStreamBufferingObserver(ITTStreamBufferingObserver* aObserver)
{
	GKASSERT(iBufferReaderProxy != NULL);
	//LOGI("CTTHttpReader::SetStreamBufferingObserver. iHttpReaderProxy = %p, aObserver = %p", iBufferReaderProxy, aObserver);
	iBufferReaderProxy->SetStreamBufferingObserver(aObserver);	
	//LOGI("CTTHttpReader::SetStreamBufferingObserver return");
}

void CTTBufferReader::CloseConnection()
{
	if (iBufferReaderProxy != NULL)
		iBufferReaderProxy->Cancel();

	iCancel = true;
}

void CTTBufferReader::CheckOnLineBuffering()
{
    if (iBufferReaderProxy != NULL)
		iBufferReaderProxy->CheckOnLineBuffering();
}

TTInt CTTBufferReader::PrepareCache(TTInt64 aReadPos, TTInt aReadSize, TTInt aFlag)
{
	if (iBufferReaderProxy != NULL)
		return iBufferReaderProxy->PrepareCache(aReadPos, aReadSize, aFlag);
		
	return TTKErrNotReady;
}

void CTTBufferReader::SetBitrate(TTInt aMediaType, TTInt aBitrate)
{
    if (iBufferReaderProxy != NULL)
		iBufferReaderProxy->SetBitrate(aMediaType, aBitrate);
}

void CTTBufferReader::SetNetWorkProxy(TTBool aNetWorkProxy)
{
    if (iBufferReaderProxy != NULL)
		iBufferReaderProxy->SetNetWorkProxy(aNetWorkProxy);
}

TTInt CTTBufferReader::GetStatusCode()
{
	if (iBufferReaderProxy != NULL)
		return iBufferReaderProxy->GetStatusCode();

	return 0;
}

TTUint CTTBufferReader::GetHostIP()
{
	if (iBufferReaderProxy != NULL)
		return iBufferReaderProxy->GetHostIP();

	return 0;
}
