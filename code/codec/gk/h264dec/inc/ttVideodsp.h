#ifndef __TTPOD_TT_VIDEODSP_H_
#define __TTPOD_TT_VIDEODSP_H_

#include <stddef.h>
#include <stdint.h>

#define EMULATED_EDGE(depth) \
void tt_emulated_edge_mc_ ## depth(uint8_t *dst, const uint8_t *src, \
                                   ptrdiff_t dst_stride, ptrdiff_t src_stride, \
                                   int block_w, int block_h,\
                                   int src_x, int src_y, int w, int h);

EMULATED_EDGE(8)
EMULATED_EDGE(16)

typedef struct VideoDSPContext {
    /**
     * Copy a rectangular area of samples to a temporary buffer and replicate
     * the border samples.
     *
     * @param dst destination buffer
     * @param dst_stride number of bytes between 2 vertically adjacent samples
     *                   in destination buffer
     * @param src source buffer
     * @param dst_linesize number of bytes between 2 vertically adjacent
     *                     samples in the destination buffer
     * @param src_linesize number of bytes between 2 vertically adjacent
     *                     samples in both the source buffer
     * @param block_w width of block
     * @param block_h height of block
     * @param src_x x coordinate of the top left sample of the block in the
     *                source buffer
     * @param src_y y coordinate of the top left sample of the block in the
     *                source buffer
     * @param w width of the source buffer
     * @param h height of the source buffer
     */
    void (*emulated_edge_mc)(uint8_t *dst, const uint8_t *src,
                             ptrdiff_t dst_linesize,
                             ptrdiff_t src_linesize,
                             int block_w, int block_h,
                             int src_x, int src_y, int w, int h);

    /**
     * Prefetch memory into cache (if supported by hardware).
     *
     * @param buf    pointer to buffer to prefetch memory from
     * @param stride distance between two lines of buf (in bytes)
     * @param h      number of lines to prefetch
     */
    void (*prefetch)(uint8_t *buf, ptrdiff_t stride, int h);
} VideoDSPContext;

void tt_videodsp_init(VideoDSPContext *ctx, int bpc);

/* for internal use only (i.e. called by tt_videodsp_init() */
void tt_videodsp_init_aarch64(VideoDSPContext *ctx, int bpc);
void tt_videodsp_init_arm(VideoDSPContext *ctx, int bpc);
void tt_videodsp_init_ppc(VideoDSPContext *ctx, int bpc);
void tt_videodsp_init_x86(VideoDSPContext *ctx, int bpc);

#endif /* __TTPOD_TT_VIDEODSP_H_ */
