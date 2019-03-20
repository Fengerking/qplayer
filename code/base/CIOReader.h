/*******************************************************************************
	File:		CIOReader.h

	Contains:	the bit reader class header file

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-01-04		Bangfei			Create file

*******************************************************************************/
#ifndef __CIOReader_H__
#define __CIOReader_H__
#include "qcIO.h"
#include "CBaseObject.h"

class CMP4IOReader
{
public:
	virtual int MP4ReadAt (long long llPos, unsigned char * pBuff, int nSize, int nFlag) = 0;
};

class CIOReader : public CBaseObject
{
public:
	CIOReader(CBaseInst * pBaseInst, CMP4IOReader * pIO);
    virtual ~CIOReader();

	/**
	* \fn						unsigned short ReadUint16(TTInt aReadPos);
	* \brief                    ��ȡһ��16Ϊ���޷������� �����ֽ��ں����?
	* \param[in]	aReadPos	�ļ���ƫ��λ��
	* \return					��������ֵ��������Χ��Assert
	*/
	virtual unsigned short		ReadUint16(long long aReadPos);// �����ֽ��ں����?

	/**
	* \fn						unsigned short ReadUint16BE(TTInt aReadPos);
	* \brief                    ��ȡһ��16Ϊ���޷�������,�����ֽ���ǰ���?
	* \param[in]	aReadPos	�ļ���ƫ��λ��
	* \return					��������ֵ��������Χ��Assert
	*/
	virtual unsigned short		ReadUint16BE(long long aReadPos);// �����ֽ���ǰ���?

	/**
	* \fn						unsigned int ReadUint32(TTInt aReadPos);
	* \brief                    ��ȡһ��32Ϊ���޷�������,�����ֽ��ں����?
	* \param[in]	aReadPos	�ļ���ƫ��λ��
	* \return					��������ֵ��������Χ��Assert
	*/
	virtual unsigned int		ReadUint32(long long aReadPos);

	/**
	* \fn						unsigned int ReadUint32BE(TTInt aReadPos);
	* \brief                    ��ȡһ��32Ϊ���޷�������,�����ֽ���ǰ���?
	* \param[in]	aReadPos	�ļ���ƫ��λ��
	* \return					��������ֵ��������Χ��Assert
	*/
	virtual unsigned int		ReadUint32BE(long long aReadPos);

	/**
	* \fn						long long ReadUint64(TTInt aReadPos);
	* \brief                    ��ȡһ��64Ϊ���޷�������,�����ֽ��ں����?
	* \param[in]	aReadPos	�ļ���ƫ��λ��
	* \return					��������ֵ��������Χ��Assert
	*/
	virtual long long			ReadUint64(long long aReadPos);

	/**
	* \fn						long long ReadUint64BE(TTInt aReadPos);
	* \brief                    ��ȡһ��64Ϊ���޷�������,�����ֽ���ǰ���?
	* \param[in]	aReadPos	�ļ���ƫ��λ��
	* \return					��������ֵ��������Χ��Assert
	*/
	virtual long long			ReadUint64BE(long long aReadPos);

protected:
	CMP4IOReader *				m_pIO;
};

#endif  // __CIOReader_H__
