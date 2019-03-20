#include <cppunit/config/SourcePrefix.h>
#include "TTMacrodef.h"
#include "..\inc\buffertest.h"
#include "TTMediaBuffer.h"

CPPUNIT_TEST_SUITE_REGISTRATION( buffertest );

buffertest::buffertest(void)
{
}

buffertest::~buffertest(void)
{
}

void buffertest::setUp()
{
	iBuffer = new CTTMediaBufferAlloc(4 * KILO);
}

void buffertest::tearDown()
{
	 SAFE_DELETE(iBuffer);
}

void buffertest::testbuffer()
{
	CPPUNIT_ASSERT( 1 == 1 );
}

void buffertest::testBufferRequest()
{
	TTInt nRequestSize1 = 1668;
	TTInt nRequestSize2 = 1008;
	TTInt nRequestSize3 = 1158;

	CTTMediaBuffer* pMediaBuffer1 = NULL;
	CTTMediaBuffer* pMediaBuffer2 = NULL;
	CTTMediaBuffer* pMediaBuffer3 = NULL;

	CPPUNIT_ASSERT(NULL != (pMediaBuffer1 = iBuffer->RequestBuffer(NULL, nRequestSize1)));
	CPPUNIT_ASSERT_EQUAL(4 * KILO - nRequestSize1, iBuffer->BufferEmptySize());

	CPPUNIT_ASSERT(NULL != (pMediaBuffer2 = iBuffer->RequestBuffer(NULL, nRequestSize2)));
	CPPUNIT_ASSERT_EQUAL(4 * KILO - nRequestSize1 - nRequestSize2, iBuffer->BufferEmptySize());

	pMediaBuffer2->UnRef();
	CPPUNIT_ASSERT_EQUAL(4 * KILO - nRequestSize1, iBuffer->BufferEmptySize());


	CPPUNIT_ASSERT(NULL != (pMediaBuffer3 = iBuffer->RequestBuffer(NULL, nRequestSize3)));
	CPPUNIT_ASSERT_EQUAL(4 * KILO - nRequestSize1 - nRequestSize3, iBuffer->BufferEmptySize());

	pMediaBuffer1->UnRef();

	CPPUNIT_ASSERT_EQUAL(4 * KILO - nRequestSize3, iBuffer->BufferEmptySize());
}
