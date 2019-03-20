#pragma once

#ifndef __TT_BUFFER_TEST_H__
#define __TT_BUFFER_TEST_H__

#include <TTMediaParser.h>
#include <cppunit/extensions/HelperMacros.h>

class Mediatest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( Mediatest );
	CPPUNIT_TEST( testMediaParser );
	CPPUNIT_TEST_SUITE_END();
public:
	Mediatest(void);
	~Mediatest(void);
public:
	void setUp();
	void tearDown();
protected:
	void testMediaParser();

private:
	CTTMediaParser*		 iMediaParser;
};

#endif
