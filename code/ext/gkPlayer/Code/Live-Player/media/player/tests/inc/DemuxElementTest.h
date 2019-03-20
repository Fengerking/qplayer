#pragma once

#ifndef __TT_FILESRCELEMENT_TEST_H__
#define __TT_FILESRCELEMENT_TEST_H__

#include "TThread.h"
#include "TTDemuxElement.h"
#include <cppunit/extensions/HelperMacros.h>

class DemuxElementtest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( DemuxElementtest );
	CPPUNIT_TEST( testDemux );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();
protected:
	void testDemux();

private:
	static void* fun(void *p);

private:
	RTThread		iPlayThread;
};

#endif
