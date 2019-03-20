#include "ttAvassert.h"
#include "ttCommon.h"
#include "ttImgutils.h"
#include "ttInternal.h"
#include "ttIntreadwrite.h"
#include "ttLog.h"
#include "ttMathematics.h"
#include "ttPixdesc.h"
#include "ttRational.h"

void ttv_image_fill_max_pixsteps(int max_pixsteps[4], int max_pixstep_comps[4],
                                const AVPixFmtDescriptor *pixdesc)
{
    int i;
    memset(max_pixsteps, 0, 4*sizeof(max_pixsteps[0]));
    if (max_pixstep_comps)
        memset(max_pixstep_comps, 0, 4*sizeof(max_pixstep_comps[0]));

    for (i = 0; i < 4; i++) {
        const AVComponentDescriptor *comp = &(pixdesc->comp[i]);
        if ((comp->step_minus1+1) > max_pixsteps[comp->plane]) {
            max_pixsteps[comp->plane] = comp->step_minus1+1;
            if (max_pixstep_comps)
                max_pixstep_comps[comp->plane] = i;
        }
    }
}

static inline
int image_get_linesize(int width, int plane,
                       int max_step, int max_step_comp,
                       const AVPixFmtDescriptor *desc)
{
    int s, shifted_w, linesize;

    if (!desc)
        return AVERROR(EINVAL);

    if (width < 0)
        return AVERROR(EINVAL);
    s = (max_step_comp == 1 || max_step_comp == 2) ? desc->log2_chroma_w : 0;
    shifted_w = ((width + (1 << s) - 1)) >> s;
    if (shifted_w && max_step > INT_MAX / shifted_w)
        return AVERROR(EINVAL);
    linesize = max_step * shifted_w;

    if (desc->flags & TTV_PIX_FMT_FLAG_BITSTREAM)
        linesize = (linesize + 7) >> 3;
    return linesize;
}

int ttv_image_get_linesize(enum TTPixelFormat pix_fmt, int width, int plane)
{
    const AVPixFmtDescriptor *desc = ttv_pix_fmt_desc_get(pix_fmt);
    int max_step     [4];       /* max pixel step for each plane */
    int max_step_comp[4];       /* the component for each plane which has the max pixel step */

    if ((unsigned)pix_fmt >= TTV_PIX_FMT_NB || desc->flags & TTV_PIX_FMT_FLAG_HWACCEL)
        return AVERROR(EINVAL);

    ttv_image_fill_max_pixsteps(max_step, max_step_comp, desc);
    return image_get_linesize(width, plane, max_step[plane], max_step_comp[plane], desc);
}

int ttv_image_fill_linesizes(int linesizes[4], enum TTPixelFormat pix_fmt, int width)
{
    int i, ret;
    const AVPixFmtDescriptor *desc = ttv_pix_fmt_desc_get(pix_fmt);
    int max_step     [4];       /* max pixel step for each plane */
    int max_step_comp[4];       /* the component for each plane which has the max pixel step */

    memset(linesizes, 0, 4*sizeof(linesizes[0]));

    if (!desc || desc->flags & TTV_PIX_FMT_FLAG_HWACCEL)
        return AVERROR(EINVAL);

    ttv_image_fill_max_pixsteps(max_step, max_step_comp, desc);
    for (i = 0; i < 4; i++) {
        if ((ret = image_get_linesize(width, i, max_step[i], max_step_comp[i], desc)) < 0)
            return ret;
        linesizes[i] = ret;
    }

    return 0;
}

int ttv_image_fill_pointers(uint8_t *data[4], enum TTPixelFormat pix_fmt, int height,
                           uint8_t *ptr, const int linesizes[4])
{
    int i, total_size, size[4] = { 0 }, has_plane[4] = { 0 };

    const AVPixFmtDescriptor *desc = ttv_pix_fmt_desc_get(pix_fmt);
    memset(data     , 0, sizeof(data[0])*4);

    if (!desc || desc->flags & TTV_PIX_FMT_FLAG_HWACCEL)
        return AVERROR(EINVAL);

    data[0] = ptr;
    if (linesizes[0] > (INT_MAX - 1024) / height)
        return AVERROR(EINVAL);
    size[0] = linesizes[0] * height;

    if (desc->flags & TTV_PIX_FMT_FLAG_PAL ||
        desc->flags & TTV_PIX_FMT_FLAG_PSEUDOPAL) {
        size[0] = (size[0] + 3) & ~3;
        data[1] = ptr + size[0]; /* palette is stored here as 256 32 bits words */
        return size[0] + 256 * 4;
    }

    for (i = 0; i < 4; i++)
        has_plane[desc->comp[i].plane] = 1;

    total_size = size[0];
    for (i = 1; i < 4 && has_plane[i]; i++) {
        int h, s = (i == 1 || i == 2) ? desc->log2_chroma_h : 0;
        data[i] = data[i-1] + size[i-1];
        h = (height + (1 << s) - 1) >> s;
        if (linesizes[i] > INT_MAX / h)
            return AVERROR(EINVAL);
        size[i] = h * linesizes[i];
        if (total_size > INT_MAX - size[i])
            return AVERROR(EINVAL);
        total_size += size[i];
    }

    return total_size;
}

int ttpriv_set_systematic_pal2(uint32_t pal[256], enum TTPixelFormat pix_fmt)
{
    int i;

    for (i = 0; i < 256; i++) {
        int r, g, b;

        switch (pix_fmt) {
        case TTV_PIX_FMT_RGB8:
            r = (i>>5    )*36;
            g = ((i>>2)&7)*36;
            b = (i&3     )*85;
            break;
        case TTV_PIX_FMT_BGR8:
            b = (i>>6    )*85;
            g = ((i>>3)&7)*36;
            r = (i&7     )*36;
            break;
        case TTV_PIX_FMT_RGB4_BYTE:
            r = (i>>3    )*255;
            g = ((i>>1)&3)*85;
            b = (i&1     )*255;
            break;
        case TTV_PIX_FMT_BGR4_BYTE:
            b = (i>>3    )*255;
            g = ((i>>1)&3)*85;
            r = (i&1     )*255;
            break;
        case TTV_PIX_FMT_GRAY8:
            r = b = g = i;
            break;
        default:
            return AVERROR(EINVAL);
        }
        pal[i] = b + (g << 8) + (r << 16) + (0xFFU << 24);
    }

    return 0;
}


typedef struct ImgUtils {
    const AVClass *class;
    int   log_offset;
    void *log_ctx;
} ImgUtils;

static const AVClass imgutils_class = { "IMGUTILS", ttv_default_item_name, NULL, LIB__TTPOD_TT_VERSION_INT, offsetof(ImgUtils, log_offset), offsetof(ImgUtils, log_ctx) };

int ttv_image_check_size(unsigned int w, unsigned int h, int log_offset, void *log_ctx)
{
    ImgUtils imgutils = { &imgutils_class, log_offset, log_ctx };

    if ((int)w>0 && (int)h>0 && (w+128)*(uint64_t)(h+128) < INT_MAX/8)
        return 0;

    ttv_log(&imgutils, TTV_LOG_ERROR, "Picture size %ux%u is invalid\n", w, h);
    return AVERROR(EINVAL);
}

int ttv_image_check_sar(unsigned int w, unsigned int h, TTRational sar)
{
    int64_t scaled_dim;

    if (!sar.den)
        return AVERROR(EINVAL);

    if (!sar.num || sar.num == sar.den)
        return 0;

    if (sar.num < sar.den)
        scaled_dim = ttv_rescale_rnd(w, sar.num, sar.den, TTV_ROUND_ZERO);
    else
        scaled_dim = ttv_rescale_rnd(h, sar.den, sar.num, TTV_ROUND_ZERO);

    if (scaled_dim > 0)
        return 0;

    return AVERROR(EINVAL);
}

void ttv_image_copy_plane(uint8_t       *dst, int dst_linesize,
                         const uint8_t *src, int src_linesize,
                         int bytewidth, int height)
{
    if (!dst || !src)
        return;
    ttv_assert0(abs(src_linesize) >= bytewidth);
    ttv_assert0(abs(dst_linesize) >= bytewidth);
    for (;height > 0; height--) {
        memcpy(dst, src, bytewidth);
        dst += dst_linesize;
        src += src_linesize;
    }
}

void ttv_image_copy(uint8_t *dst_data[4], int dst_linesizes[4],
                   const uint8_t *src_data[4], const int src_linesizes[4],
                   enum TTPixelFormat pix_fmt, int width, int height)
{
    const AVPixFmtDescriptor *desc = ttv_pix_fmt_desc_get(pix_fmt);

    if (!desc || desc->flags & TTV_PIX_FMT_FLAG_HWACCEL)
        return;

    if (desc->flags & TTV_PIX_FMT_FLAG_PAL ||
        desc->flags & TTV_PIX_FMT_FLAG_PSEUDOPAL) {
        ttv_image_copy_plane(dst_data[0], dst_linesizes[0],
                            src_data[0], src_linesizes[0],
                            width, height);
        /* copy the palette */
        memcpy(dst_data[1], src_data[1], 4*256);
    } else {
        int i, planes_nb = 0;

        for (i = 0; i < desc->nb_components; i++)
            planes_nb = FFMAX(planes_nb, desc->comp[i].plane + 1);

        for (i = 0; i < planes_nb; i++) {
            int h = height;
            int bwidth = ttv_image_get_linesize(pix_fmt, width, i);
            if (bwidth < 0) {
                ttv_log(NULL, TTV_LOG_ERROR, "ttv_image_get_linesize failed\n");
                return;
            }
            if (i == 1 || i == 2) {
                h = TT_CEIL_RSHIFT(height, desc->log2_chroma_h);
            }
            ttv_image_copy_plane(dst_data[i], dst_linesizes[i],
                                src_data[i], src_linesizes[i],
                                bwidth, h);
        }
    }
}
