/*
============================================================================
 Name        : TTAACParser.h
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : CTTAACParser declaration
============================================================================
*/

#ifndef TTAACPARSER_H
#define TTAACPARSER_H

// INCLUDES
#include "TTAACHeader.h"
#include "TTMediaParser.h"

/**
*  CTTAACParser
* 
*/
class CTTAACParser : public CTTMediaParser
{
public: // Constructors and destructor

	/**
    * Destructor.
    */
	~CTTAACParser();

    /**
    * Constructor.
    */
	CTTAACParser(ITTDataReader& aDataReader, ITTMediaParserObserver& aObserver);


public://functions from MTTMediaParser
	
	/**
    * \fn                               TTInt Parse(TTMediaInfo& aMediaInfo);
    * \brief                            解析媒体源
    * \return                           成功返回KErrNone，否则返回错误值。
    */
    virtual TTInt                       Parse(TTMediaInfo& aMediaInfo);

    /**
    * \fn                               TTUint MediaDuration()
    * \brief                            获取媒体时长(毫秒)
    * \return                           媒体时长
    */
    virtual TTUint						MediaDuration(TTInt aStreamId);

	/**
	* \fn								TTInt GetFrameLocation(TTInt aStreamId, TTInt& aFrmIdx, TTUint aTime);
	* \brief							根据时间获取帧索引位置
	* \param[in]	aStreamId			媒体流ID
	* \param[out]	aFrmIdx				帧索引
	* \param[in]	aTime				时间
	* \return							成功返回KErrNone，否则返回错误值。
	*/
	virtual TTInt					    GetFrameLocation(TTInt aStreamId, TTInt& aFrmIdx, TTUint64 aTime);


	/**
	* \fn								void StartFrmPosScan();
	* \brief							开始扫描帧位置，建立帧索引表
	*/
	virtual void						StartFrmPosScan();


private://functions from CTTMediaParser

	/**
	* \fn								ParseFrmPos(const TTUint8 *aData, TTInt aParserSize);
	* \param[in]	aData				数据指针
	* \param[in]	aParserSize			数据大小
	*/	
	virtual	void						ParseFrmPos(const TTUint8 *aData, TTInt aParserSize);

	/**
	* \fn                               TTInt SeekWithinFrmPosTab(TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo)
	* \brief                            在帧索引表中查找帧位置
	* \param[in]    aFrmIdx             帧序号
	* \param[out]   aFrameInfo         帧信息
	* \return                           操作是否成功
	*/
	virtual TTInt                       SeekWithinFrmPosTab(TTInt aStreamId, TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo);

	/**
	* \fn                               TTInt SeekWithoutFrmPosTab(TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo); 
	* \brief                            在帧索引表外查找帧位置
	* \param[in]    aFrameIdx           帧号
	* \param[out]   aFrameInfo          帧信息
	* \return                           操作是否成功
	*/
	virtual TTInt                       SeekWithoutFrmPosTab(TTInt aStreamId, TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo);

	/**
	* \fn                               TTInt SeekWithIdx(TTInt aFrmIdx, TTInt &aFrmPos, TTInt &aFrmSize)
	* \brief                            根据帧索引查找帧位置
	* \param[in]    aFrmIdx             帧序号
	* \param[out]   aFrmPos             帧位置
	* \param[out]   aFrmSize		    帧长度
	* \return                           操作是否成功
	*/
	TTInt                               SeekWithIdx(TTInt aStreamId, TTInt aFrmIdx, TTInt64 &aFrmPos, TTInt &aFrmSize);

	/**
	* \fn                               TTInt SeekWithPos(TTInt aPos, TTInt &aFrmPos, TTInt &aFrmSize)
	* \brief                            根据位置查找帧位置
	* \param[in]    aPos                起始查找位置
	* \param[out]   aFrmPos             帧位置
	* \param[out]   aFrmSize            帧长度
	* \return                           操作是否成功
	*/
	TTInt                               SeekWithPos(TTInt aStreamId, TTInt64 aPos, TTInt64 &aFrmPos, TTInt &aFrmSize);


protected:
	
	/**
	* \fn								RawDataEnd();
	* \brief							解析数据尾部位置
	* \return							数据尾部在文件中的偏移。
	*/
	virtual TTInt						RawDataEnd();


private:

	/**
	* \fn                               Ns_MediaParser::TTFrmSyncResult FrameSyncWithPos(TTInt aReadPos, TTInt& aOffSet, TTInt& aProcessedSize, AAC_FRAME_INFO& aFrameInfo, TBool aSyncFirstFrm)
	* \brief                            同步帧头
	* \param[in]    aReadPos            读取数据的位置。
	* \param[out]   aOffSet             帧头相对aReadPos的偏移。
	* \param[out]	aProcessedSize	    处理的字节数
	* \param[out]	aFrameInfo		    帧信息 
	* \param[in]    aCheckNextFrameHeader验证帧尾是否是下一帧帧头
	* \return                           处理状态
	*/
	TTFrmSyncResult						FrameSyncWithPos(TTInt aReadPos, TTInt& aOffSet, TTInt& aProcessedSize, 
		AAC_FRAME_INFO& aFrameInfo, TTBool aCheckNextFrameHeader = ETTFalse);

	/**
	* \fn                               void UpdateFrameInfo(TTMediaFrameInfo& aFrameInfo,  TTInt aFrameIdx);
	* \param[out]   aFrameInfo         帧信息
	* \param[in]    aFrameIdx          帧序号
	*/
	void								UpdateFrameInfo(TTMediaFrameInfo& aFrameInfo,  TTInt aFrameIdx);

private:

	AAC_HEADER							iAACHeader;
	AAC_FRAME_INFO						iFirstFrmInfo;
	TTInt								iTotalFrameSize;//平均帧长
	TTInt								iFrameNum;//平均帧长
	TTInt								iAVEFrameSize;//平均帧长
	TTUint								iFrameTime;
};

#endif // TTAACPARSER_H
