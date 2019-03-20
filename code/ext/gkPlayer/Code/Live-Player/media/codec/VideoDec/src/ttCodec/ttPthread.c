#include "ttAvcodec.h"
#include "ttInternal.h"
#include "ttPthreadInternal.h"
#include "ttThread.h"

static void validate_thread_parameters(TTCodecContext *avctx)
{
    int frame_threading_supported = (avctx->codec->capabilities & CODEC_CAP_FRAME_THREADS)
                                && !(avctx->flags & CODEC_FLAG_TRUNCATED)
                                && !(avctx->flags & CODEC_FLAG_LOW_DELAY)
                                && !(avctx->flags2 & CODEC_FLAG2_CHUNKS);
    if (avctx->thread_count == 1) {
        avctx->active_thread_type = 0;
    } else if (frame_threading_supported && (avctx->thread_type & TT_THREAD_FRAME)) {
        avctx->active_thread_type = TT_THREAD_FRAME;
    } else if (avctx->codec->capabilities & CODEC_CAP_SLICE_THREADS &&
               avctx->thread_type & TT_THREAD_SLICE) {
        avctx->active_thread_type = TT_THREAD_SLICE;
    } else if (!(avctx->codec->capabilities & CODEC_CAP_AUTO_THREADS)) {
        avctx->thread_count       = 1;
        avctx->active_thread_type = 0;
    }

    if (avctx->thread_count > MAX_AUTO_THREADS)
        ttv_log(avctx, TTV_LOG_WARNING,
               "Application has requested %d threads. Using a thread count greater than %d is not recommended.\n",
               avctx->thread_count, MAX_AUTO_THREADS);
}

int tt_thread_init(TTCodecContext *avctx)
{
    validate_thread_parameters(avctx);

    if (avctx->active_thread_type&TT_THREAD_SLICE)
        return tt_slice_thread_init(avctx);
    else if (avctx->active_thread_type&TT_THREAD_FRAME)
        return tt_frame_thread_init(avctx);

    return 0;
}

void tt_thread_free(TTCodecContext *avctx)
{
    if (avctx->active_thread_type&TT_THREAD_FRAME)
        tt_frame_thread_free(avctx, avctx->thread_count);
    else
        tt_slice_thread_free(avctx);
}
