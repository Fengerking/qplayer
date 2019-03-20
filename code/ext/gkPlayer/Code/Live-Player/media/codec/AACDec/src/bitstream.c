

#include "bitstream.h"
#include "struct.h"
#include <stdlib.h>
#include <string.h>
#define  ALIGNBIT 32
#define  CHECK_OVERFLOW (ALIGNBIT/2+4)
#define  OVERFLOW_FLAG 0xfd

void *RMAACDecAlignedMalloc(int size)
{
	int advance;
	char *realPt, *alignedPt;
	size += ALIGNBIT+1;//the last byte is used to store 0xfd to check the overflow

	realPt = (char *)malloc(size);
	if(realPt == NULL)
		return NULL;

	memset(realPt, 0, size);

	realPt[size-1]=0xfd;
	advance = (ALIGNBIT - ((int)realPt & (ALIGNBIT-1)));
	if(advance<=ALIGNBIT/2)//We just guarantee the ALIGNBIT/2 aligned
		advance +=ALIGNBIT/2;
	alignedPt = realPt + advance; // to aligned location;
	*(alignedPt - 1) = advance; // save real malloc pt at alignedPt[-1] location for free;

	*((int*)(alignedPt-CHECK_OVERFLOW))=size-advance-1;
	return alignedPt;
}

void  RMAACDecAlignedFree(void *alignedPt)
{
	char *realPt;
	int advance;
	if (!alignedPt)
		return;
	advance = *((char *)alignedPt - 1);

	{
		unsigned char* test   = alignedPt;
		int	  offset = *((int*)(test-CHECK_OVERFLOW));
		if(test[offset]!=0xfd)
		{
			//printf("RMAACDecAlignedFree overflow error!");
		}
	}
	realPt = (char *)alignedPt - advance ;
	
	free(realPt);

	return;
}