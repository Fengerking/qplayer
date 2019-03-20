#ifndef __TTPOD_TT_THREAD_H
#define __TTPOD_TT_THREAD_H

#include "ttBuffer.h"

#include "config.h"
#include "ttAvcodec.h"

typedef struct ThreadFrame {
    TTFrame *f;
    TTCodecContext *owner;
    // progress->data is an array of 2 ints holding progress for top/bottom
    // fields
    TTBufferRef *progress;
} ThreadFrame;

/**
 * Wait for decoding threads to finish and reset internal state.
 * Called by ttcodec_flush_buffers().
 *
 * @param avctx The context.
 */
void tt_thread_flush(TTCodecContext *avctx);

/**
 * Submit a new frame to a decoding thread.
 * Returns the next available frame in picture. *got_picture_ptr
 * will be 0 if none is available.
 * The return value on success is the size of the consumed packet for
 * compatibility with ttcodec_decode_video2(). This means the decoder
 * has to consume the full packet.
 *
 * Parameters are the same as ttcodec_decode_video2().
 */
int tt_thread_decode_frame(TTCodecContext *avctx, TTFrame *picture,
                           int *got_picture_ptr, TTPacket *avpkt);

/**
 * If the codec defines update_thread_context(), call this
 * when they are ready for the next thread to start decoding
 * the next frame. After calling it, do not change any variables
 * read by the update_thread_context() method, or call tt_thread_get_buffer().
 *
 * @param avctx The context.
 */
void tt_thread_finish_setup(TTCodecContext *avctx);

/**
 * Notify later decoding threads when part of their reference picture is ready.
 * Call this when some part of the picture is finished decoding.
 * Later calls with lower values of progress have no effect.
 *
 * @param f The picture being decoded.
 * @param progress Value, in arbitrary units, of how much of the picture has decoded.
 * @param field The field being decoded, for field-picture codecs.
 * 0 for top field or frame pictures, 1 for bottom field.
 */
void tt_thread_report_progress(ThreadFrame *f, int progress, int field);

/**
 * Wait for earlier decoding threads to finish reference pictures.
 * Call this before accessing some part of a picture, with a given
 * value for progress, and it will return after the responsible decoding
 * thread calls tt_thread_report_progress() with the same or
 * higher value for progress.
 *
 * @param f The picture being referenced.
 * @param progress Value, in arbitrary units, to wait for.
 * @param field The field being referenced, for field-picture codecs.
 * 0 for top field or frame pictures, 1 for bottom field.
 */
void tt_thread_await_progress(ThreadFrame *f, int progress, int field);

/**
 * Wrapper around get_format() for frame-multithreaded codecs.
 * Call this function instead of avctx->get_format().
 * Cannot be called after the codec has called tt_thread_finish_setup().
 *
 * @param avctx The current context.
 * @param fmt The list of available formats.
 */
enum TTPixelFormat tt_thread_get_format(TTCodecContext *avctx, const enum TTPixelFormat *fmt);

/**
 * Wrapper around get_buffer() for frame-multithreaded codecs.
 * Call this function instead of tt_get_buffer(f).
 * Cannot be called after the codec has called tt_thread_finish_setup().
 *
 * @param avctx The current context.
 * @param f The frame to write into.
 */
int tt_thread_get_buffer(TTCodecContext *avctx, ThreadFrame *f, int flags);

/**
 * Wrapper around release_buffer() frame-for multithreaded codecs.
 * Call this function instead of avctx->release_buffer(f).
 * The TTFrame will be copied and the actual release_buffer() call
 * will be performed later. The contents of data pointed to by the
 * TTFrame should not be changed until tt_thread_get_buffer() is called
 * on it.
 *
 * @param avctx The current context.
 * @param f The picture being released.
 */
void tt_thread_release_buffer(TTCodecContext *avctx, ThreadFrame *f);

int tt_thread_ref_frame(ThreadFrame *dst, ThreadFrame *src);

int tt_thread_init(TTCodecContext *s);
void tt_thread_free(TTCodecContext *s);

int tt_alloc_entries(TTCodecContext *avctx, int count);
void tt_reset_entries(TTCodecContext *avctx);
void tt_thread_report_progress2(TTCodecContext *avctx, int field, int thread, int n);
void tt_thread_await_progress2(TTCodecContext *avctx,  int field, int thread, int shift);

#endif /* __TTPOD_TT_THREAD_H */
