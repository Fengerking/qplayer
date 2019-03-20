#ifndef __AFP_TEST_H__
#define __AFP_TEST_H__
#include "GKTypedef.h"
#include "TThread.h"
#include "GKCritical.h"


typedef struct _TWavHeader
{  
	long rId; 
	long rLen; 
	long wId; 
	long fId; 
	long fLen; 
	unsigned short wFormatTag; 
	unsigned short nChannels; 
	long nSamplesPerSec; 
	long nAvgBytesPerSec; 
	unsigned short nBlockAlign; 
	unsigned short wBitsPerSample; 
	long dId; 
	long wSampleLength;
}TWavHead;


class AFPTest{
public:
	AFPTest();
	~AFPTest();
	TTInt Open(const TTChar* aUrl);
	void Close();

	static void* 	AFPThreadProc(void* aPtr);
	void 			AFPThreadProcL(void* aPtr);

private:
	RTThread		iThreadHandle;
	RGKCritical     iCritical;
	TTBool			iIsStopAFPTest;
	TTChar*			iUrl;
};

#endif
