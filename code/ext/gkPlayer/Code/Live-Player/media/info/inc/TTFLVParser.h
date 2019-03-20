/**
* File : TTFLVParser.h 
* Created on : 2015-9-8
* Author : yongping.lin
* Description : TTFLVParser定义文件
*/

#ifndef __TT_FLV_PARSER_H__
#define __TT_FLV_PARSER_H__

#include "TTMediaParser.h"
#include "TTFLVTag.h"
#include "TTEventThread.h"
#include "GKCritical.h"

enum TTFLVnfoMsg
{
	ECheckBufferStatus = 1
};

class CTTFLVParser : public CTTMediaParser
{
public: // Constructors and destructor

	/**
	* \fn                               ~CTTMP4Parser.
	* \brief                            C++ destructor.
	*/
	~CTTFLVParser();

	/**
	* \fn                               CTTAPEParser.
	* \brief                            C++ constructor.
	*/
	CTTFLVParser(ITTDataReader& aDataReader, ITTMediaParserObserver& aObserver);


public: // Functions from ITTMediaParser

	/**
	* \fn								TTInt Parse(TTMediaInfo& aMediaInfo);
	* \brief							解析媒体源
	* \param[out] 	aMediaInfo			解析好的媒体信息
	* \return							成功返回KErrNone，否则返回错误值。
	*/
	virtual TTInt						Parse(TTMediaInfo& aMediaInfo);

	/**
	* \fn                               TTimeIntervalMicroSeconds32 MediaDuration()
	* \brief                            获取媒体时长(毫秒)
	* \param[in]						流Id
	* \return                           媒体时长
	*/
	virtual TTUint						MediaDuration(TTInt aStreamId);

	/**
	* \fn								void Seek(TTUint aPosMS);
	* \brief							Seek操作
	* \param[in]	aPosMS				位置，单位：毫秒
	* \return							返回真正seek的位置，如果是负数，那是seek 失败。
	*/
	virtual TTInt64						Seek(TTUint64 aPosMS, TTInt aOption);

	/**
	* \fn								TInt GetFrameLocation(TInt aStreamId, TInt& aFrmIdx, TTUint aTime)
	* \brief							根据时间获取帧索引位置
	* \param[in]	aStreamId			媒体流ID
	* \param[out]	aFrmIdx				帧索引
	* \param[in]	aTime				时间
	* \return							成功返回KErrNone，否则返回错误值。
	*/
	virtual TTInt						GetFrameLocation(TTInt aStreamId, TTInt& aFrmIdx, TTUint64 aTime);

	/**
	* \fn								TTMediaInfo GetMediaSample();
	* \brief							获得audio，video，等sample信息
	* \param[in]	aUrl				文件路径
	*/
	virtual TTInt			 			GetMediaSample(TTMediaType aStreamType, TTBuffer* pMediaBuffer);

protected:
	TTInt								onInfoHandle(TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3);

	virtual TTInt						postInfoMsgEvent(TTInt  nDelayTime, TTInt32 nMsg = 0, int nParam1 = 0, int nParam2 = 0, void * pParam3 = NULL);

private:

	/**
	* \fn								TTInt SeekWithinFrmPosTab(TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo)
	* \brief							在帧索引表中查找帧位置
	* \param[in] 	aFrmIdx				帧序号
	* \param[out] 	aFrameInfo			帧位置
	* \return							操作是否成功
	*/
	virtual TTInt						SeekWithinFrmPosTab(TTInt aStreamId, TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo);

	virtual	TTInt						ReadMetaData(unsigned char*	pBuffer, unsigned int nLength);
	
	virtual	TTInt						ParserTag(TTInt64 nOffset);

	virtual	TTInt						AmfReadObject(unsigned char* pBuffer, int nLength, const char* key);

	virtual TTInt						FillBuffer();

	virtual TTInt						GetBufferTime(int nStreamType);

	virtual void						SendBufferStartEvent();

	virtual void						CheckBufferStatus();

	virtual	bool						CheckHead(int nFlag);

	virtual TTInt						CheckEOS(int nOffset, int nRead);

	virtual	TTInt						AmfGetString(unsigned char*	pSrcBuffer, int nLength, char* pDesString);

	virtual	TTInt						KeyFrameIndex(unsigned char*	pSrcBuffer, int nLength);

private:
	TTInt64								iDuration;
	TTUint								iTotalBitrate;
	TTInt64								iOffset;

	TTInt64								iLastBufferTime;
	TTInt64								iNowBufferTime;
	TTInt								iReBufferCount;
	TTInt								iCountAMAX;
	TTInt								iCountVMAX;
	TTInt								iFirstTime;

	TTEventThread*						mMsgThread;
	RGKCritical							iCritical;
	TTInt								iBufferStatus;
	
	CTTFlvTagStream*					iAudioStream;
	CTTFlvTagStream*					iVideoStream;

	unsigned char*						iTagBuffer;
	unsigned int						iMaxSize;
};

class CTTFLVParserEvent : public TTBaseEventItem
{
public:
    CTTFLVParserEvent(CTTFLVParser * pParser, TTInt (CTTFLVParser::* method)(TTInt, TTInt, TTInt, void*),
					    TTInt nType, TTInt nMsg = 0, TTInt nVar1 = 0, TTInt nVar2 = 0, void* nVar3 = 0)
		: TTBaseEventItem (nType, nMsg, nVar1, nVar2, nVar3)
	{
		mParser = pParser;
		mMethod = method;
    }

    virtual ~CTTFLVParserEvent()
	{
	}

    virtual void fire (void) 
	{
        (mParser->*mMethod)(mMsg, mVar1, mVar2, mVar3);
    }

protected:
    CTTFLVParser *		mParser;
    int (CTTFLVParser::* mMethod) (TTInt, TTInt, TTInt, void*);
};

#endif // __TT_MP4_PARSER_H__
