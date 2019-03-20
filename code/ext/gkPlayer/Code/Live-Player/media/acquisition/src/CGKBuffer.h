#ifndef __GK_CTTBUFFER_H__
#define __GK_CTTBUFFER_H__

#include "GKTypedef.h"

typedef struct
{
    TTInt32         nFlag;          /*!< Flag of the buffer, refer to TT_BUFFER_FLAG
                                     TT_FLAG_BUFFER_KEYFRAME:    pBuffer is video key frame
                                     TT_FLAG_BUFFER_HEADDATA:    pBuffer is head data
                                     TT_FLAG_BUFFER_NEW_PROGRAM: pBuffer is NULL, pData is TTAudioInfo/TTVideoInfo
                                     TT_FLAG_BUFFER_NEW_FORMAT:  pBuffer is NULL, pData is TTAudioInfo/TTVideoInfo  */
    TTInt32         nSize;          /*!< pBuffer actual ocupied size*/
    TTInt32         nPreAllocSize;  /*!< pBuffer PreAlloced size */
    TTPBYTE			pBuffer;        /*!< Buffer pointer */
    TTInt64	        llTime;         /*!< [in/out] The time of the buffer */
    TTInt32         nDuration;      /*!< [In]AV offset, [out]Duration of buffer(MS) */
    TTInt32		    Tag ;		    /*!< auido or video*/
}CGKBuffer;


#endif
