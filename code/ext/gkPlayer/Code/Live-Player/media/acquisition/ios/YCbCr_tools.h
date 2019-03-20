
#ifndef YCBCR_TOOLS_H
#define YCBCR_TOOLS_H

typedef unsigned char uint8_t;

void *yt_init(int h, int w);
void yt_uninit(void *ytHandle);

void rgb2yuv(uint8_t *src, uint8_t *dst, void *ytHandle);
void yuv2rgb(uint8_t *src, uint8_t *dst, void *ytHandle);

void bgr2yuv(uint8_t *src, uint8_t *dst, void *ytHandle);
void yuv2bgr(uint8_t *src, uint8_t *dst, void *ytHandle);

void rgb2yuv_float(uint8_t *src, uint8_t *dst, int h, int w);
void yuv2rgb_float(uint8_t *src, uint8_t *dst, int h, int w);

void bgra2bgr(uint8_t *src, uint8_t *dst, int h, int w);
void bgr2bgra(uint8_t *src, uint8_t *dst, int h, int w);

#endif /* YCBCR_TOOLS_H */