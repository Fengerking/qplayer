#ifndef __TTPOD_TT_IMGUTILS_H_
#define __TTPOD_TT_IMGUTILS_H_

#include "ttAvutil.h"
#include "ttPixdesc.h"
#include "ttRational.h"

void ttv_image_fill_max_pixsteps(int max_pixsteps[4], int max_pixstep_comps[4],
                                const AVPixFmtDescriptor *pixdesc);

int ttv_image_get_linesize(enum TTPixelFormat pix_fmt, int width, int plane);


int ttv_image_fill_linesizes(int linesizes[4], enum TTPixelFormat pix_fmt, int width);

int ttv_image_fill_pointers(uint8_t *data[4], enum TTPixelFormat pix_fmt, int height,
                           uint8_t *ptr, const int linesizes[4]);

void ttv_image_copy_plane(uint8_t       *dst, int dst_linesize,
                         const uint8_t *src, int src_linesize,
                         int bytewidth, int height);

void ttv_image_copy(uint8_t *dst_data[4], int dst_linesizes[4],
                   const uint8_t *src_data[4], const int src_linesizes[4],
                   enum TTPixelFormat pix_fmt, int width, int height);

int ttv_image_check_size(unsigned int w, unsigned int h, int log_offset, void *log_ctx);

int ttv_image_check_sar(unsigned int w, unsigned int h, TTRational sar);

#endif /* __TTPOD_TT_IMGUTILS_H_ */
