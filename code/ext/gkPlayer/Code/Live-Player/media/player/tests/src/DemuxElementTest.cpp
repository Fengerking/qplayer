#include <cppunit/config/SourcePrefix.h>
#include "TTMacrodef.h"
#include "..\inc\DemuxElementTest.h"
#include "TTDemuxElement.h"
#include "TTPlayControl.h"

CPPUNIT_TEST_SUITE_REGISTRATION( DemuxElementtest );

void DemuxElementtest::setUp()
{
}

void DemuxElementtest::tearDown()
{
	iPlayThread.Close();
}

void DemuxElementtest::testDemux()
{
	const TTChar* pThreadName = "PlayThread";
	iPlayThread.Create(pThreadName, fun, this);
}

void* DemuxElementtest::fun(void *p)
{
// 	CTTActiveScheduler* pScheduler = new CTTActiveScheduler();
// 	CTTActiveScheduler::Install(pScheduler);
// 
// 	CTTPlayControl* pPlayControl = new CTTPlayControl();
// 	CPPUNIT_ASSERT(KErrNone == pPlayControl->Open("c:\\1.wav")); 
// 	CPPUNIT_ASSERT(KErrNone == pPlayControl->Play());
// 
// 	CTTActiveScheduler::Start();
// 
// 	SAFE_DELETE(pPlayControl);
	return NULL;
}
