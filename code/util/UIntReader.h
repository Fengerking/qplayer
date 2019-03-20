/*******************************************************************************
	File:		UIntReader.h

	Contains:	int reader header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-08		Bangfei			Create file

*******************************************************************************/
#ifndef __UIntReader_H__
#define __UIntReader_H__

unsigned short		qcIntReadUint16 (const unsigned char* aReadPtr);		// �����ֽ��ں����
unsigned short		qcIntReadUint16BE (const unsigned char* aReadPtr);		// �����ֽ���ǰ���
unsigned int		qcIntReadUint32 (const unsigned char* aReadPtr);
unsigned int		qcIntReadUint32BE (const unsigned char* aReadPtr);
unsigned long long	qcIntReadUint64 (const unsigned char* aReadPtr);
double				qcIntReadDouble64 (const unsigned char* aReadPtr);
unsigned long long	qcIntReadUint64BE (const unsigned char* aReadPtr);

unsigned short		qcIntReadWord (const unsigned char* aReadPtr);
unsigned int		qcIntReadDWord (const unsigned char* aReadPtr);			//��˳���

unsigned int		qcIntReadBytesNBE (const unsigned char* aReadPtr, int n);
unsigned int		qcIntReadBytesN (const unsigned char* aReadPtr, int n);

#endif // __UIntReader_H__
