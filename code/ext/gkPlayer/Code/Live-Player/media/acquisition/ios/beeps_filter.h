
#ifndef BEEPS_FILTER_H
#define BEEPS_FILTER_H

//#ifdef __cplusplus
//extern "C"
//{
//#endif /* __cplusplus */

#define BEEPS_GRAY_MODE 0
#define BEEPS_COLOR_MODE 1

typedef unsigned char uchar;
void *beeps_init(int h, int w, int mode);
void beeps_process(unsigned char *src, unsigned char *dst,
	float sigma, void *beepsHandle);
void beeps_process_padding(unsigned char *src, unsigned char *dst,
	float sigma, int padding, void *beepsHandle);
void beeps_uninit(void *beepsHandle);
//#ifdef __cplusplus
//}
//#endif /* __cplusplus */

#endif /* BEEPS_FILTER_H */