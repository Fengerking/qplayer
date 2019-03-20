#include <limits.h>

#include "ttInternal.h"
#include "ttAvcodec.h"
#include "ttErrorResilience.h"
#include "ttMpegutils.h"
#include "ttThread.h"
#include "ttVersion.h"

void tt_er_frame_start(ERContext *s)
{
    if (!s->avctx->error_concealment)
        return;

    memset(s->error_status_table, ER_MB_ERROR | VP_START | ER_MB_END,
           s->mb_stride * s->mb_height * sizeof(uint8_t));
    s->error_count    = 3 * s->mb_num;
    s->error_occurred = 0;
}

static int er_supported(ERContext *s)
{
    if(!s->cur_pic.f                                                  ||
       s->cur_pic.field_picture
    )
        return 0;
    return 1;
}

void tt_er_add_slice(ERContext *s, int startx, int starty,
                     int endx, int endy, int status)
{
    const int start_i  = ttv_clip(startx + starty * s->mb_width, 0, s->mb_num - 1);
    const int end_i    = ttv_clip(endx   + endy   * s->mb_width, 0, s->mb_num);
    const int start_xy = s->mb_index2xy[start_i];
    const int end_xy   = s->mb_index2xy[end_i];
    int mask           = -1;

//     if (s->avctx->hwaccel && s->avctx->hwaccel->decode_slice)
//         return;

    if (start_i > end_i || start_xy > end_xy) {
        ttv_log(s->avctx, TTV_LOG_ERROR,
               "internal error, slice end before start\n");
        return;
    }

    if (!s->avctx->error_concealment)
        return;

    mask &= ~VP_START;
    if (status & (ER_AC_ERROR | ER_AC_END)) {
        mask           &= ~(ER_AC_ERROR | ER_AC_END);
        s->error_count -= end_i - start_i + 1;
    }
    if (status & (ER_DC_ERROR | ER_DC_END)) {
        mask           &= ~(ER_DC_ERROR | ER_DC_END);
        s->error_count -= end_i - start_i + 1;
    }
    if (status & (ER_MV_ERROR | ER_MV_END)) {
        mask           &= ~(ER_MV_ERROR | ER_MV_END);
        s->error_count -= end_i - start_i + 1;
    }

    if (status & ER_MB_ERROR) {
        s->error_occurred = 1;
        s->error_count    = INT_MAX;
    }

    if (mask == ~0x7F) {
        memset(&s->error_status_table[start_xy], 0,
               (end_xy - start_xy) * sizeof(uint8_t));
    } else {
        int i;
        for (i = start_xy; i < end_xy; i++)
            s->error_status_table[i] &= mask;
    }

    if (end_i == s->mb_num)
        s->error_count = INT_MAX;
    else {
        s->error_status_table[end_xy] &= mask;
        s->error_status_table[end_xy] |= status;
    }

    s->error_status_table[start_xy] |= VP_START;

    if (start_xy > 0 && !(s->avctx->active_thread_type & TT_THREAD_SLICE) &&
        er_supported(s) && s->avctx->skip_top * s->mb_width < start_i) {
        int prev_status = s->error_status_table[s->mb_index2xy[start_i - 1]];

        prev_status &= ~ VP_START;
        if (prev_status != (ER_MV_END | ER_DC_END | ER_AC_END)) {
            s->error_occurred = 1;
            s->error_count = INT_MAX;
        }
    }
}

void tt_er_frame_end(ERContext *s)
{
    int *linesize = NULL;
    int mb_x;
   // int distance;
    //int threshold_part[4] = { 100, 100, 100 };
    //int threshold = 50;
    //int is_intra_likely;
    //int size = s->b8_stride * 2 * s->mb_height;

    /* We do not support ER of field pictures yet,
     * though it should not crash if enabled. */
    if (!s->avctx->error_concealment || s->error_count == 0            ||
        s->avctx->lowres                                               ||
        !er_supported(s)                                               ||
        s->error_count == 3 * s->mb_width *
                          (s->avctx->skip_top + s->avctx->skip_bottom)) {
        return;
    }
    linesize = s->cur_pic.f->linesize;
    for (mb_x = 0; mb_x < s->mb_width; mb_x++) {
        int status = s->error_status_table[mb_x + (s->mb_height - 1) * s->mb_stride];
        if (status != 0x7F)
            break;
    }
}
