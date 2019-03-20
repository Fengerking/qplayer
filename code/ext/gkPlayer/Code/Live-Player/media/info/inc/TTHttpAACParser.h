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
	* \brief                        ��ʼɨ��֡λ�ã�����֡������
	*/
	virtual void                    StartFrmPosScan();

	/**
	* \fn							RawDataEnd();
	* \brief						��������β��λ��
	* \return						����β�����ļ��е�ƫ�ơ�
	*/
	virtual TTInt					RawDataEnd();
};

#endif