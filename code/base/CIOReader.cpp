/*******************************************************************************
	File:		CIOReader.cpp

	Contains:	the bit reader class implement file

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-01-04		Bangfei			Create file

*******************************************************************************/
#include "CIOReader.h"

CIOReader::CIOReader(CBaseInst * pBaseInst, CMP4IOReader * pIO)
	: CBaseObject (pBaseInst)
	, m_pIO (pIO)
{
	SetObjectName ("CIOReader");
}

CIOReader::~CIOReader() 
{
}

unsigned short CIOReader::ReadUint16(long long aReadPos)
{
	unsigned char	buf[sizeof(unsigned short)];
	int				nSize = sizeof(unsigned short);
	if (m_pIO->MP4ReadAt (aReadPos, buf, nSize, QCIO_READ_HEAD) == QC_ERR_NONE)
	{
		return (unsigned short)((buf[1] << 8) | buf[0]);
	}
	return 0;
}

unsigned short CIOReader::ReadUint16BE(long long aReadPos)
{
	unsigned char	buf[sizeof(unsigned short)];
	int				nSize = sizeof(unsigned short);
	if (m_pIO->MP4ReadAt (aReadPos, buf, nSize, QCIO_READ_HEAD) == QC_ERR_NONE)
	{
		return (unsigned short)((buf[0] << 8) | buf[1]);
	}
	return 0;
}

unsigned int CIOReader::ReadUint32(long long aReadPos)
{
	unsigned char	buf[sizeof(unsigned int)];
	int				nSize = sizeof(unsigned int);
	if (m_pIO->MP4ReadAt (aReadPos, buf, nSize, QCIO_READ_HEAD) == QC_ERR_NONE)
	{
		return (unsigned int)((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
	}
	return 0;
}

unsigned int CIOReader::ReadUint32BE(long long aReadPos)
{
	unsigned char	buf[sizeof(unsigned int)];
	int				nSize = sizeof(unsigned int);
	if (m_pIO->MP4ReadAt (aReadPos, buf, nSize, QCIO_READ_HEAD) == QC_ERR_NONE)
	{
		return (unsigned int)((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
	}
	return 0;
}

long long CIOReader::ReadUint64(long long aReadPos)
{
	unsigned int nMsb = ReadUint32(aReadPos);
	unsigned int nLsb = ReadUint32(aReadPos + 4);

	long long nRetVal = nMsb;
	return (nRetVal << 32) | nLsb;
}

long long CIOReader::ReadUint64BE(long long aReadPos)
{
	unsigned int nMsb = ReadUint32BE(aReadPos);
	unsigned int nLsb = ReadUint32BE(aReadPos + 4);

	long long nRetVal = nMsb;
	return (nRetVal << 32) | nLsb;
}
