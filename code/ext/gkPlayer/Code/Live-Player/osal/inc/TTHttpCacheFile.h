/**
* File : TTHttpCacheFile.h  
* Description : CTTHttpCacheFile
*/
#ifndef __TT_HTTP_FILE_CACHE_H__
#define __TT_HTTP_FILE_CACHE_H__
#include <stdio.h>
#include <string.h>
#include "GKOsalConfig.h"
#include "GKMacrodef.h"
#include "GKTypedef.h"
#include "GKCritical.h"

#define BUFFER_UNIT_SIZE		(80*1024)
#define BUFFER_UNIT_MAXSIZE		(16*32*BUFFER_UNIT_SIZE)

typedef struct
{
	TTInt64		iPosition;
	TTInt		iTotalSize;
	TTInt		iSize;
	TTUint8*	iBuffer;
	TTInt		iFlag;
} TTBufferUnit;

class CTTHttpCacheFile
{
public:

	/**
	* \fn							CTTHttpCacheFile();
	* \brief						���캯��
	*/
	CTTHttpCacheFile();

	/**
	* \fn							~CTTHttpCacheFile();
	* \brief						��������
	*/
	~CTTHttpCacheFile();

	/**
	* \fn							TTInt Read(void* aBuffer, TTInt aReadPos, TTInt aReadSize);
	* \brief						��ȡ�ļ�
	* \param[in]	aBuffer			��Ŷ������ݵĻ�����
	* \param[in]	aReadPos		��ȡ�������ļ��е�ƫ��λ��
	* \param[in]	aReadSize		��ȡ���ֽ���
	* \return						��ȡ��ȷʱ����ʵ�ʶ�ȡ���ֽ���������Ϊ������
	*/
	TTInt							Read(void* aBuffer, TTInt aReadPos, TTInt aReadSize);
	
	/**
	* \fn							TTInt Write(void* aBuffer, TTInt aWriteSize);
	* \brief						д�ļ�
	* \param[in]	aBuffer			���ڴ��Ҫд���ļ������ݵ�Buffer
	* \param[in]	aWriteSize		�ֽ���
	* \return						����״̬
	*/
	TTInt							Write(void* aBuffer, TTInt aWriteSize);


	/**
	* \fn							TTInt ReadBuffer(void* aBuffer, TTInt aReadPos, TTInt aReadSize);
	* \brief						��ȡ�ļ�
	* \param[in]	aBuffer			��Ŷ������ݵĻ�����
	* \param[in]	aReadPos		��ȡ�������ļ��е�ƫ��λ��
	* \param[in]	aReadSize		��ȡ���ֽ���
	* \return						��ȡ��ȷʱ����ʵ�ʶ�ȡ���ֽ���������Ϊ������
	*/
	TTInt							ReadBuffer(TTUint8* aBuffer, TTInt aReadPos, TTInt aReadSize);
	
	/**
	* \fn							TTInt WriteBuffer(void* aBuffer, TTInt aWriteSize);
	* \brief						д�ļ�
	* \param[in]	aBuffer			���ڴ��Ҫд���ļ������ݵ�Buffer
	* \param[in]	aWriteSize		�ֽ���
	* \return						����״̬
	*/
	TTInt							WriteBuffer(TTUint8* aBuffer, TTInt aWriteSize);
	
	/**
	* \fn							TTInt TotalSize();
	* \brief						��ȡ�ļ��ܴ�С
	* \return						�ļ��ܴ�С
	*/
	INLINE_C TTInt64				TotalSize(){return iTotalSize;};

	/**
	* \fn							TTInt CachedSize();
	* \brief						��ȡ�ѻ������ݵĴ�С
	* \return						�ѻ������ݵĴ�С
	*/
	TTInt64							CachedSize();

	/**
	* \fn							void SetTotalSize(TTInt aTotalSize);
	* \brief						�����ļ��ܴ�С
	* \param[in]  aTotalSize		�ļ��ܴ�С
	*/
	INLINE_C	void				SetTotalSize(TTInt64 aTotalSize){iTotalSize = aTotalSize;};
	
	/**
	* \fn							TTInt Open(const TTChar* aUrl);
	* \brief						���ļ������ڼ���Ƿ��Ѿ�����������ļ�
	* \param[in]  aUrl				·��
	* \return						������
	*/
	TTInt							Open(const TTChar* aUrl);


	/**
	* \fn							TTInt Create(const TTChar* aUrl, TTInt aTotalSize);
	* \brief						�����ļ�
	* \param[in]  aUrl				Url·��
	* \param[in]  aTotalSize		�ļ��ܴ�С
	* \return						������
	*/
	TTInt							Create(const TTChar* aUrl, TTInt64 aTotalSize);

	/**
	* \fn							TTInt WriteFile();
	* \brief						д�ļ�
	* \return						������
	*/
	TTInt							WriteFile(TTInt aCount);

	/**
	* \fn							void Close();
	* \brief						�ر��ļ�
	*/
	void							Close();
    
    void                            ResetBufferUnit();

private:
	TTInt							InitBufferUnit(int nSize);
	TTInt							UnInitBufferUnit();
		
private:
	FILE*				iFileHandle;
	TTInt64				iTotalSize;
	TTInt				iCachedSize;
	TTInt				iIndexEnd;
	TTInt				iCurIndex;
	TTInt				iBufferMode;
	TTBufferUnit**		iBufferUnit;
	TTUint8*			iWriteBuffer;
	TTInt				iBufferCount;
	TTInt				iWriteCount;
	TTInt				iWriteIndex;
    TTBufferUnit*		iBufferPool;
	RGKCritical			iCritical;
};
#endif
