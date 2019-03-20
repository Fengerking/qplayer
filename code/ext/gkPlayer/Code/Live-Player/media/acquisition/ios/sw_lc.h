
#ifndef SW_LC_H
#define SW_LC_H

//#ifdef __cplusplus
//extern "C"
//{
//#endif /* __cplusplus */


typedef unsigned char uchar;
typedef unsigned char uint8_t;

void *sw_lc_init(int h, int w, float beta);
void sw_lc_process(uchar *srcBuf, uchar *dstBuf, void *swHandle);
void sw_lc_process_single(uchar *srcBuf, uchar *dstBuf, void *swHandle);
void sw_lc_uninit(void *swHandle);

void rgb2rgbplan(uint8_t * srcBuf, uint8_t *dstBuf, int h, int w);
void rgbplan2rgb(uint8_t * srcBuf, uint8_t *dstBuf, int h, int w);
void blend_y(uint8_t *srcBuf, uint8_t *dstBuf, int h, int w, float alpha);

//#ifdef __cplusplus
//}
//#endif /* __cplusplus */

#endif /* SW_LC_H */