#ifndef __TTPOD_TT_PTHREAD_INTERNAL_H
#define __TTPOD_TT_PTHREAD_INTERNAL_H

#include "ttAvcodec.h"

/* H264 slice threading seems to be buggy with more than 16 threads,
 * limit the number of threads to 16 for automatic detection */
#define MAX_AUTO_THREADS 16

int tt_slice_thread_init(TTCodecContext *avctx);
void tt_slice_thread_free(TTCodecContext *avctx);

int tt_frame_thread_init(TTCodecContext *avctx);
void tt_frame_thread_free(TTCodecContext *avctx, int thread_count);

#endif // __TTPOD_TT_PTHREAD_INTERNAL_H
