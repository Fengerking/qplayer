#ifndef __TT_HTTP_AAC_PARSER_H__
#define __TT_HTTP_AAC_PARSER_H__

#include "TTAACParser.h"

class CTTHttpAACParser : public CTTAACParser
{
public:
	CTTHttpAACParser(ITTDataReader& aDataReader, ITTMediaParserObserver& aObserver);

public: // Functions from ITTDataParser

	/**
	* \fn                           void StartFrmPosScan();
	* \brief                        开始扫描帧位置，建立帧索引表
	*/
	virtual void                    StartFrmPosScan();

	/**
	* \fn							RawDataEnd();
	* \brief						解析数据尾部位置
	* \return						数据尾部在文件中的偏移。
	*/
	virtual TTInt					RawDataEnd();
};

#endif