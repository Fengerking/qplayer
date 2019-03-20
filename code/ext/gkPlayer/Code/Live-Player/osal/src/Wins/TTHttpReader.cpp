﻿/**
* File : TTHttpReader.cpp
* Created on : 2011-8-31
* Author : Kevin
* Copyright : Copyright (c) 2016 GoKu Software Ltd. All rights reserved.
* Description : CTTHttpReader 实现文件
*/

// INCLUDES
#include <stdio.h>
#include <string.h>
#include "GKMacrodef.h"
#include "TTHttpReader.h"
#include "GKOSALConfig.h"

CTTHttpReader::CTTHttpReader()
{
}

CTTHttpReader::~CTTHttpReader()
{
}

TTInt CTTHttpReader::Open( const TTChar* aUrl )
{
	return TTKErrNotSupported;
}

TTInt CTTHttpReader::Close()
{
	return TTKErrNotSupported;
}

TTInt CTTHttpReader::ReadSync( TTUint8* aReadBuffer, TTInt64 aReadPos, TTInt aReadSize )
{
	return TTKErrNotSupported;
}

TTInt64 CTTHttpReader::Size() const
{
	return TTKErrNotSupported;
}

//void CTTHttpReader::ReadAsync( TTUint8* aReadBuffer, TTInt64 aReadPos, TTInt aReadSize )
//{
//}

//void CTTHttpReader::CancelAsyncRead()
//{
//}

TTUint16 CTTHttpReader::ReadUint16(TTInt64 /*aReadPos*/)
{
	GKASSERT(ETTFalse);
	return 0;
}

TTUint16 CTTHttpReader::ReadUint16BE(TTInt64 /*aReadPos*/)
{
	GKASSERT(ETTFalse);
	return 0;
}

TTUint32 CTTHttpReader::ReadUint32(TTInt64 /*aReadPos*/)
{
	GKASSERT(ETTFalse);
	return 0;
}

TTUint32 CTTHttpReader::ReadUint32BE(TTInt64 /*aReadPos*/)
{
	GKASSERT(ETTFalse);
	return 0;
}

TTUint64 CTTHttpReader::ReadUint64(TTInt64 /*aReadPos*/)
{
	GKASSERT(ETTFalse);
	return 0;
}

TTUint64 CTTHttpReader::ReadUint64BE(TTInt64 /*aReadPos*/)
{
	GKASSERT(ETTFalse);
	return 0;
}

ITTDataReader::TTDataReaderId CTTHttpReader::Id()
{
	return ITTDataReader::ETTDataReaderIdHttp;
}

//void CTTHttpReader::RunL()
//{
//}

//TTUint CTTHttpReader::BufferedSize()
//{
//	return 0;
//}

void CTTHttpReader::SetStreamBufferingObserver(ITTStreamBufferingObserver *aObserver)
{
}

void CTTHttpReader::CloseConnection()
{
}
