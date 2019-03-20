#pragma once

#ifndef __TT_BUFFER_TEST_H__
#define __TT_BUFFER_TEST_H__

#include "TTMediaBufferAlloc.h"
#include <cppunit/extensions/HelperMacros.h>

class buffertest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( buffertest );
	CPPUNIT_TEST( testbuffer );
	CPPUNIT_TEST( testBufferRequest );	
	CPPUNIT_TEST_SUITE_END();
public:
	buffertest(void);
	~buffertest(void);
public:
	void setUp();
	void tearDown();
protected:
	void testbuffer();
	void testBufferRequest();

private:
	CTTMediaBufferAlloc* iBuffer;
};

#endif
