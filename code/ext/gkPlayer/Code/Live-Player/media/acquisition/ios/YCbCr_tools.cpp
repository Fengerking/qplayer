
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "YCbCr_tools.h"

#define Clip(v)  ( (v)<0? 0 : ((v)>255?255:(v)) )
#define SHIFT_LEN 20

//RGB to YUV
float RC[] = {0.299, 0.587, 0.114,
			  0.499813, 0.418531, 0.081282,
			  0.168636, 0.331068, 0.499704};

// YUV to RGB
float YC[] = {1.403, 0.714, 0.344, 1.773, 1.0};

typedef struct _ytContext
{
	int h, w;
	int *RCTable[9];
	int *YCTable[4];
	int weight[5];
}YTContext;

void *yt_init(int h, int w)
{
	YTContext *pYTC = (YTContext *)malloc(sizeof(YTContext));
	pYTC->h = h;
	pYTC->w = w;

	const int M = 256;
	int i, j;
	double tmp;
	for(i = 0; i < 9; ++i)
	{
		pYTC->RCTable[i] = new int[M];
		for(j = 0; j < M; ++j)
		{
			tmp = j;
			pYTC->RCTable[i][j] = tmp * RC[i] * 1024.0 * 1024.0;
		}
	}
	for(i = 0; i < 4; ++i)
	{
		pYTC->YCTable[i] = new int[M];
		for(j = 0; j < M; ++j)
		{
			tmp = j;
			pYTC->YCTable[i][j] = tmp * YC[i] * 1024.0 * 1024.0;
		}
	}

	for(i = 0; i < 5; ++i)
		pYTC->weight[i] = YC[i] * 128 * 1024.0 * 1024.0;


	return (void *)pYTC;
}

void yt_uninit(void *ytHandle)
{
	if(NULL == ytHandle)
		return;

	YTContext *pYTC = (YTContext *)ytHandle;

	int i;
	for(i = 0; i < 9; ++i)
		delete [] pYTC->RCTable[i];
	for(i = 0; i < 4; ++i)
		delete [] pYTC->YCTable[i];

	free(pYTC);
}


void rgb2yuv(uint8_t *src, uint8_t *dst, void *ytHandle)
{
	YTContext *pYTC = (YTContext *)ytHandle;
	int h = pYTC->h;
	int w = pYTC->w;
	
	uint8_t *pin, *py, *pu, *pv;
	pin = src;
	py = dst;
	pu = dst + h * w;
	pv = pu + h * w;

	int length = h * w;

	int i;
	int tmp;
	int r, g, b;
	for(i = 0; i < length; ++i)
	{
		r = *pin++;
		g = *pin++;
		b = *pin++;

		tmp = pYTC->RCTable[0][r] + pYTC->RCTable[1][g] + pYTC->RCTable[2][b];
		tmp >>= 20;
		*py++ = Clip(tmp);

		tmp = pYTC->RCTable[3][r] - pYTC->RCTable[4][g] - pYTC->RCTable[5][b] + pYTC->weight[4];
		tmp >>= 20;
		*pu++ = Clip(tmp);

		tmp = -pYTC->RCTable[6][r] - pYTC->RCTable[7][g] + pYTC->RCTable[8][b] + pYTC->weight[4];
		tmp >>= 20;
		*pv++ = Clip(tmp);
	}
}

void yuv2rgb(uint8_t *src, uint8_t *dst, void *ytHandle)
{
	YTContext *pYTC = (YTContext *)ytHandle;
	int h = pYTC->h;
	int w = pYTC->w;

	uint8_t *pout, *py, *pu, *pv;
	py = src;
	pu = src + h * w;
	pv = pu + h * w;
	pout = dst;

	int length = h * w;

	int i;
	int tmp;
	int y, u, v;
	for(i = 0; i < length; ++i)
	{
		y = *py++;
		y <<= 20;
		u = *pu++;
		v = *pv++;

		tmp = y + pYTC->YCTable[0][u] - pYTC->weight[0];
		tmp >>= 20;
		*pout++ = Clip(tmp);

		tmp = y - pYTC->YCTable[1][u] + pYTC->weight[1] - pYTC->YCTable[2][v] + pYTC->weight[2];
		tmp >>= 20;
		*pout++ = Clip(tmp);

		tmp = y + pYTC->YCTable[3][v] - pYTC->weight[3];
		tmp >>= 20;
		*pout++ = Clip(tmp);
	}
}

void rgb2yuv_float(uint8_t *src, uint8_t *dst, int h, int w)
{
	uint8_t *pin, *py, *pu, *pv;
	pin = src;
	py = dst;
	pu = dst + h * w;
	pv = pu + h * w;

	int length = h * w;

	int i;
	float tmp;
	float r, g, b;
	for(i = 0; i < length; ++i)
	{
		r = *pin++;
		g = *pin++;
		b = *pin++;

		tmp = RC[0] * r + RC[1] * g + RC[2] * b;
		tmp = Clip(tmp);
		*py++ = tmp;

		tmp = RC[3] * r - RC[4] * g - RC[5] * b + 128;
		tmp = Clip(tmp);
		*pu++ = tmp;

		tmp = -RC[6] * r - RC[7] * g + RC[8] * b + 128;
		tmp = Clip(tmp);
		*pv++ = tmp;
	}
}

void yuv2rgb_float(uint8_t *src, uint8_t *dst, int h, int w)
{
	uint8_t *pout, *py, *pu, *pv;
	py = src;
	pu = src + h * w;
	pv = pu + h * w;
	pout = dst;

	int length = h * w;

	int i;
	float tmp;
	float y, u, v;
	for(i = 0; i < length; ++i)
	{
		y = *py++;
		u = *pu++;
		v = *pv++;


		tmp = y + YC[0] * (u - 128);
		tmp = Clip(tmp);
		*pout++ = tmp;

		tmp = y - YC[1] * (u - 128) - YC[2] * (v - 128);
		tmp = Clip(tmp);
		*pout++ = tmp;

		tmp = y + YC[3] * (v - 128);
		tmp = Clip(tmp);
		*pout++ = tmp;
	}
}

void bgra2bgr(uint8_t *src, uint8_t *dst, int h, int w)
{
	uint8_t *pin, *pout;

	pin = src;
	pout = dst;
	int i, j;
	for(i = 0; i < h * w; ++i)
	{
		memcpy(pout, pin, 3);

		pout += 3;
		pin += 4;
	}
}

void bgr2bgra(uint8_t *src, uint8_t *dst, int h, int w)
{
	uint8_t *pin, *pout;

	pin = src;
	pout = dst;
	int i, j;
	for(i = 0; i < h * w; ++i)
	{
		memcpy(pout, pin, 3);

		pout += 4;
		pin += 3;
	}
}


void bgr2yuv(uint8_t *src, uint8_t *dst, void *ytHandle)
{
	YTContext *pYTC = (YTContext *)ytHandle;
	int h = pYTC->h;
	int w = pYTC->w;
	
	uint8_t *pin, *py, *pu, *pv;
	pin = src;
	py = dst;
	pu = dst + h * w;
	pv = pu + h * w;

	int length = h * w;

	int i;
	int tmp;
	int r, g, b;
	for(i = 0; i < length; ++i)
	{
		b = *pin++;
		g = *pin++;
		r = *pin++;

		tmp = pYTC->RCTable[0][r] + pYTC->RCTable[1][g] + pYTC->RCTable[2][b];
		tmp >>= 20;
		*py++ = Clip(tmp);

		tmp = pYTC->RCTable[3][r] - pYTC->RCTable[4][g] - pYTC->RCTable[5][b] + pYTC->weight[4];
		tmp >>= 20;
		*pu++ = Clip(tmp);

		tmp = -pYTC->RCTable[6][r] - pYTC->RCTable[7][g] + pYTC->RCTable[8][b] + pYTC->weight[4];
		tmp >>= 20;
		*pv++ = Clip(tmp);
	}
}

void yuv2bgr(uint8_t *src, uint8_t *dst, void *ytHandle)
{
	YTContext *pYTC = (YTContext *)ytHandle;
	int h = pYTC->h;
	int w = pYTC->w;

	uint8_t *pout, *py, *pu, *pv;
	py = src;
	pu = src + h * w;
	pv = pu + h * w;
	pout = dst;

	int length = h * w;

	int i;
	int tmp;
	int y, u, v;
	for(i = 0; i < length; ++i)
	{
		y = *py++;
		y <<= 20;
		u = *pu++;
		v = *pv++;
		
		tmp = y + pYTC->YCTable[3][v] - pYTC->weight[3];
		tmp >>= 20;
		*pout++ = Clip(tmp);
		
		tmp = y - pYTC->YCTable[1][u] + pYTC->weight[1] - pYTC->YCTable[2][v] + pYTC->weight[2];
		tmp >>= 20;
		*pout++ = Clip(tmp);

		tmp = y + pYTC->YCTable[0][u] - pYTC->weight[0];
		tmp >>= 20;
		*pout++ = Clip(tmp);
	}
}