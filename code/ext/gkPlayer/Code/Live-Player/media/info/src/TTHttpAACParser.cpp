#include "TTHttpAACParser.h"

CTTHttpAACParser::CTTHttpAACParser(ITTDataReader& aDataReader, ITTMediaParserObserver& aObserver)
: CTTAACParser(aDataReader, aObserver)
{
}

void CTTHttpAACParser::StartFrmPosScan()
{
}

TTInt CTTHttpAACParser::RawDataEnd()
{
	return iDataReader.Size();
}