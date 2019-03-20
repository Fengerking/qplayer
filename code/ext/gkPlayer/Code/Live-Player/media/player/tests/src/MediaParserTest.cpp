#include <cppunit/config/SourcePrefix.h>
#include <TTMacrodef.h>
#include "..\inc\MediaParserTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION( Mediatest );

Mediatest::Mediatest(void)
{
}

Mediatest::~Mediatest(void)
{
}

void Mediatest::setUp()
{
	iMediaParser = new CTTMediaParser();
}

void Mediatest::tearDown()
{
}


void Mediatest::testMediaParser()
{
// 	TTMediaInfo tMediaInfo;
// 	TTChar* p = "C:\\1.mp3";
// 	iMediaParser->Open(p);
// 	TTInt nErr = iMediaParser->Parser(tMediaInfo);
// 	CPPUNIT_ASSERT(tMediaInfo.iMediaType == EMediaTypeMP3);
}
