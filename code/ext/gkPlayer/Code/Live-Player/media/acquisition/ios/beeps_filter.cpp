
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "beeps_filter.h"
#include "mem_tools.h"
#include "SSE_header.h"

#ifdef USE_NEON_OPT
#include "beeps_neon.h"
#endif /* USE_NEON_OPT */

#define Clip(v) ( (v)<0? 0 : ((v)>255?255:(v)) )

int *mu_exp_table_15;

#define eps 0.000005
//typedef void (*BEEPS_FILTER_PROGRESSIVE)(uchar *src, int *dst, int h, int w, float sigma);
//typedef void (*BEEPS_FILTER_REGRESSIVE)(uchar *src, int *dst, int h, int w, float sigma);
typedef void (*BEEPS_FILTER_PROCESS)(uchar *n_src, uchar *t_src, int *dst_hv_p, int *dst_hv_r, int *dst_vh_p, int *dst_vh_r, int h, int w, int *pTable);
typedef void (*BEEPS_FILTER_CI)(int  *data1, int *data2, uchar *dst, int h, int w);
typedef void (*BEEPS_FILTER_IT)(uchar *src, uchar *dst, int h, int w);

typedef struct _BeepsContext
{
	int h;
	int w;
	int beeps_mode;
	int dataLen;

#if defined(USE_SSE2)
	_MM_ALIGN16 uchar *normal_src, *transform_src;
	_MM_ALIGN16 int *hv_data_g;
	_MM_ALIGN16 int *hv_data_p;
	_MM_ALIGN16 int *hv_data_r;
	_MM_ALIGN16 int *vh_data_g;
	_MM_ALIGN16 int *vh_data_p;
	_MM_ALIGN16 int *vh_data_r;
#else
	uchar *normal_src, *transform_src;
	int *hv_data_g;
	int *hv_data_p;
	int *hv_data_r;
	int *vh_data_g;
	int *vh_data_p;
	int *vh_data_r;
#endif /* USE_SSE2 */

	// function pointer
	BEEPS_FILTER_PROCESS pFunPR;
	/*BEEPS_FILTER_GAIN pFunGain;
	BEEPS_FILTER_CR pFunCR;*/
	BEEPS_FILTER_CI pFunCI;
	BEEPS_FILTER_IT pFunIT;

	int *gainTable[15];
	int *muTable[15];
}BeepsContext;


inline float gainBeeps(uchar *src, int *data,  int length, int *gainTable)
{
	int k;
	for(k = 0; k < length; ++k)
	{
		*data++ = gainTable[*src++];
	}

	return 1.0;
}

inline void calcuteResult(int *r, int * p, int * g, int length)
{
	int k;

#ifdef USE_SSE2
	for(k = 0; k < length; k += 4, r += 4, p += 4, g += 4)
	{
		__m128i vr = _mm_load_si128((__m128i *)r);
		__m128i vp = _mm_load_si128((__m128i *)p);
		__m128i vg = _mm_load_si128((__m128i *)g);

		vr = _mm_add_epi32(vr, vp);
		vr = _mm_sub_epi32(vr, vg);

		_mm_store_si128((__m128i *)r, vr);

	}
#else
	for(k = 0; k < length; ++k)
		r[k] += p[k] - g[k];
#endif /* instruct optimize */

}


inline void prBeeps(uchar *n_src, uchar *t_src, int *dst_hv_p, int *dst_hv_r, int *dst_vh_p, int *dst_vh_r, int h, int w, int *pTable)
{
	uchar *p_n_src, *r_n_src;
	uchar *p_t_src, *r_t_src;
	int *p_dst_hv_p, *p_dst_hv_r, *p_dst_vh_p, *p_dst_vh_r;

	int k;

	p_n_src = n_src;
	p_dst_hv_p = dst_hv_p;
	*p_dst_hv_p = *p_n_src;
	++p_n_src;
	++p_dst_hv_p;

	p_t_src = t_src;
	p_dst_vh_p = dst_vh_p;
	*p_dst_vh_p = *p_t_src;
	++p_t_src;
	++p_dst_vh_p;

	r_n_src = n_src + h * w - 1;
	p_dst_hv_r = dst_hv_r + h * w - 1;
	*p_dst_hv_r = *r_n_src;
	--r_n_src;
	--p_dst_hv_r;

	r_t_src = t_src + h * w - 1;
	p_dst_vh_r = dst_vh_r + h * w - 1;
	*p_dst_vh_r = *r_t_src;
	--r_t_src;
	--p_dst_vh_r;

	for(k = 1; k < h * w; ++k)
	{
		*p_dst_hv_p++ = pTable[((*p_n_src++) << 10) + ((*(p_dst_hv_p - 1)) >> 8)];
		*p_dst_vh_p++ = pTable[((*p_t_src++) << 10) + ((*(p_dst_vh_p - 1)) >> 8)];
		*p_dst_hv_r-- = pTable[((*r_n_src--) << 10) + ((*(p_dst_hv_r + 1)) >> 8)];
		*p_dst_vh_r-- = pTable[((*r_t_src--) << 10) + ((*(p_dst_vh_r + 1)) >> 8)];
	}
}

inline void prBeeps_color(uchar *n_src, uchar *t_src, int *dst_hv_p, int *dst_hv_r, int *dst_vh_p, int *dst_vh_r, int h, int w, int *pTable)
{
	uchar *p_n_src, *r_n_src;
	uchar *p_t_src, *r_t_src;
	int *p_dst_hv_p, *p_dst_hv_r, *p_dst_vh_p, *p_dst_vh_r;

	int k;

	p_n_src = n_src;
	p_dst_hv_p = dst_hv_p;
	*p_dst_hv_p++ = *p_n_src++;
	*p_dst_hv_p++ = *p_n_src++;
	*p_dst_hv_p++ = *p_n_src++;

	p_t_src = t_src;
	p_dst_vh_p = dst_vh_p;
	*p_dst_vh_p++ = *p_t_src++;
	*p_dst_vh_p++ = *p_t_src++;
	*p_dst_vh_p++ = *p_t_src++;

	r_n_src = n_src + h * w * 3 - 1;
	p_dst_hv_r = dst_hv_r + h * w * 3 - 1;
	*p_dst_hv_r-- = *r_n_src--;
	*p_dst_hv_r-- = *r_n_src--;
	*p_dst_hv_r-- = *r_n_src--;

	r_t_src = t_src + h * w * 3 - 1;
	p_dst_vh_r = dst_vh_r + h * w * 3 - 1;
	*p_dst_vh_r-- = *r_t_src--;

	for(k = 1; k < h * w; ++k)
	{
		*p_dst_hv_p++ = pTable[((*p_n_src++) << 10) + ((*(p_dst_hv_p - 3)) >> 8)];
		*p_dst_hv_p++ = pTable[((*p_n_src++) << 10) + ((*(p_dst_hv_p - 3)) >> 8)];
		*p_dst_hv_p++ = pTable[((*p_n_src++) << 10) + ((*(p_dst_hv_p - 3)) >> 8)];

		*p_dst_vh_p++ = pTable[((*p_t_src++) << 10) + ((*(p_dst_vh_p - 3)) >> 8)];
		*p_dst_vh_p++ = pTable[((*p_t_src++) << 10) + ((*(p_dst_vh_p - 3)) >> 8)];
		*p_dst_vh_p++ = pTable[((*p_t_src++) << 10) + ((*(p_dst_vh_p - 3)) >> 8)];

		*p_dst_hv_r-- = pTable[((*r_n_src--) << 10) + ((*(p_dst_hv_r + 3)) >> 8)];
		*p_dst_hv_r-- = pTable[((*r_n_src--) << 10) + ((*(p_dst_hv_r + 3)) >> 8)];
		*p_dst_hv_r-- = pTable[((*r_n_src--) << 10) + ((*(p_dst_hv_r + 3)) >> 8)];

		*p_dst_vh_r-- = pTable[((*r_t_src--) << 10) + ((*(p_dst_vh_r + 3)) >> 8)];
		*p_dst_vh_r-- = pTable[((*r_t_src--) << 10) + ((*(p_dst_vh_r + 3)) >> 8)];
		*p_dst_vh_r-- = pTable[((*r_t_src--) << 10) + ((*(p_dst_vh_r + 3)) >> 8)];
	}
}

void combineImage(int *data1, int *data2, uchar *dst, int h, int w)
{
	int i, j, k;
	int index1, index2;
	unsigned char *p;
	
	p = dst;
	int *pData1, *pData2;
	for(i = 0; i < h; ++i)
	{
		pData1 = data1 + i * w;
		pData2 = data2 + i;
		for(j = 0; j < w; ++j, pData2 += h)
		{
			*pData1= (((*pData1) + (*pData2)) >> 11);
			*p++ = (uchar)(*pData1++);
		}
	}
}

void combineImage_padding(int *data1, int *data2, uchar *dst, int h, int w, int padding)
{
	int i, j, k;
	int index1, index2;
	unsigned char *p;
	
	p = dst;
	int *pData1, *pData2;
	for(i = 0; i < h; ++i)
	{
		pData1 = data1 + i * w;
		pData2 = data2 + i;
		for(j = 0; j < w; ++j, pData2 += h)
		{
			*pData1= (((*pData1) + (*pData2)) >> 11);
			*p++ = (uchar)(*pData1++);
		}
		p += padding;
	}
}

void imageTransposition(uchar *src, uchar *dst, int h, int w)
{
	int i, j, k;
	uchar *p;
	p = dst;
	for(j = 0; j < w; ++j)
	{
		for(i = 0; i < h; ++i)
			*p++ = src[i * w + j];
	}
}

void imageTransposition_color(uchar *src, uchar *dst, int h, int w)
{
	int i, j, k;
	for(i = 0; i < h; ++i)
	{
		for(j = 0; j < w; ++j, src += 3)
		{
			memcpy(dst + (j * h + i) * 3, src, 3);
		}
	}
}

void progressiveBeeps_color(int *data, int h, int w, float sigma)
{
	float photometricStandardDeviation = sigma;
	float spatialContraDecay = 1.0 -
		(sqrt(2 * pow(photometricStandardDeviation, 2) + 1.0) - 1.0) /
		pow(photometricStandardDeviation, 2);
	float rho = 1.0 + spatialContraDecay;
	float c = -0.5 /
		(photometricStandardDeviation * photometricStandardDeviation);

	int fixedRho = (int)(rho * 1024.0);
	float *p, mu;
	int i, j, k;
	int index;
	int a, b;
	for(i = 0; i < h; ++i, data += 3 * w)
	{
		data[0] = ((data[0] / fixedRho) << 10);
		data[1] = ((data[1] / fixedRho) << 10);
		data[2] = ((data[2] / fixedRho) << 10);
		
		k = 3;
		for(j = 1; j < w; ++j)
		{
			// B
			//k = j * 3;
			//mu = data[k] - rho * data[k - 3];
			//mu = spatialContraDecay * exp(c * mu * mu);
			//data[k] = data[k - 3] * mu + data[k] * (1.0 - mu) / rho;
			//++k;

			//// G
			//mu = data[k] - rho * data[k - 3];
			//mu = spatialContraDecay * exp(c * mu * mu);
			//data[k] = data[k - 3] * mu + data[k] * (1.0 - mu) / rho;
			//++k;

			//// R
			//mu = data[k] - rho * data[k - 3];
			//mu = spatialContraDecay * exp(c * mu * mu);
			//data[k] = data[k - 3] * mu + data[k] * (1.0 - mu) / rho;
			//++k;

			a = data[k];
			b = (data[k - 3] >> 8);
			data[k] = mu_exp_table_15[a + b];
			++k;

			// G
			a = data[k];
			b = (data[k - 3] >> 8);
			data[k] = mu_exp_table_15[a + b];
			++k;

			// R
			a = data[k];
			b = (data[k - 3] >> 8);
			data[k] = mu_exp_table_15[a + b];
			++k;
		}
	}

}

void regressiveBeeps_color(int *data, int h, int w, float sigma)
{
	float photometricStandardDeviation = sigma;
	float spatialContraDecay = 1.0 -
		(sqrt(2 * pow(photometricStandardDeviation, 2) + 1.0) - 1.0) /
		pow(photometricStandardDeviation, 2);
	float rho = 1.0 + spatialContraDecay;
	float c = -0.5 /
		(photometricStandardDeviation * photometricStandardDeviation);

	int fixedRho = (int)(rho * 1024.0);
	float mu;
	int i, j, k;
	int a, b;
	int index;
	for(i = 0; i < h; ++i, data += w * 3)
	{
		data[w * 3 - 1] = ((data[w * 3 - 1] / fixedRho) << 10);
		data[w * 3 - 2] = ((data[w * 3 - 2] / fixedRho) << 10);
		data[w * 3 - 3] = ((data[w * 3 - 3] / fixedRho) << 10);

		k = w * 3 - 4 ;
		for(j = w - 2; j >= 0; --j)
		{
			//// B
			//mu = data[k] - rho * data[k + 3];
			//mu = spatialContraDecay * exp(c * mu * mu);
			//data[k] = data[k + 3] * mu + data[k] * (1.0 - mu) / rho;
			//--k;

			//// G
			//mu = data[k] - rho * data[k + 3];
			//mu = spatialContraDecay * exp(c * mu * mu);
			//data[k] = data[k + 3] * mu + data[k] * (1.0 - mu) / rho;
			//--k;

			//// R
			//mu = data[k] - rho * data[k + 3];
			//mu = spatialContraDecay * exp(c * mu * mu);
			//data[k] = data[k + 3] * mu + data[k] * (1.0 - mu) / rho;
			//--k;

			// B
			a = data[k];
			b = (data[k + 3] >> 8);
			data[k] = mu_exp_table_15[a + b];
			--k;

			// G
			a = data[k];
			b = (data[k + 3] >> 8);
			data[k] = mu_exp_table_15[a + b];
			--k;

			// R
			a = data[k];
			b = (data[k + 3] >> 8);
			data[k] = mu_exp_table_15[a + b];
			--k;
		}
	}
}

void combineImage_color(int *data1, int *data2, unsigned char *dst, int h, int w)
{
	int i, j, k;
	int index1, index2;
	unsigned char *p;

	int b, g, r;

	p = dst;
	index1 = 0;
	for(i = 0; i < h; ++i)
	{
		for(j = 0; j < w; ++j, index1 += 3)
		{
			//index1 = w * 3 * i + j * 3;
			index2 = h * 3 * j + i * 3;

			b = ((data1[index1] + data2[index2]) >> 11);
			g = ((data1[index1 + 1] + data2[index2 + 1]) >> 11);
			r = ((data1[index1 + 2] + data2[index2 + 2]) >> 11);

			*p++ = Clip(b);
			*p++ = Clip(g);
			*p++ = Clip(r);
		}
	}
}

void calculateMuTable(int *data, float sigma)
{
	float photometricStandardDeviation = sigma;
	float spatialContraDecay = 1.0 -
		(sqrt(2 * pow(photometricStandardDeviation, 2) + 1.0) - 1.0) /
		pow(photometricStandardDeviation, 2);
	float rho = 1.0 + spatialContraDecay;
	float c = -0.5 /
		(photometricStandardDeviation * photometricStandardDeviation);

	int M = 256;
	float mu = 0.0;
	float ans;
	int i, j, k;
	float a, b;
	int n = 4;
	float step = 1.0 / (float)(n);
	for(i = 0; i < M; ++i)
	{
		a = i;
		b = 0;
		for(j = 0; j < M * n; ++j, b += step)
		{
			mu = a - rho * b;
			mu = spatialContraDecay * exp(c * mu * mu);
			ans = b * mu + a * (1.0 - mu) / rho;
			ans *= 1024.0;
			*data++ = (int)(ans);
		}
	}
}

void calculateGainTable(int *data, float sigma)
{
	float photometricStandardDeviation, spatialContraDecay, mu;
	
	photometricStandardDeviation = sigma;
	spatialContraDecay = 1.0 -
		(sqrt(2 * pow(photometricStandardDeviation, 2) + 1.0) - 1.0) /
			pow(photometricStandardDeviation, 2);
	mu = (1.0 - spatialContraDecay) / (1.0 + spatialContraDecay);
	int fixedMu = (int)(mu * 1024.0);

	int k;
	for(k = 0; k < 256; ++k)
	{
		data[k] = k << 10;
		data[k] = ((data[k] * fixedMu) >> 10);

	}
}

void *beeps_init(int h, int w, int mode)
{
	BeepsContext *pBC;
	if(mode != BEEPS_GRAY_MODE && mode != BEEPS_COLOR_MODE)
		return NULL;

	pBC = (BeepsContext *)malloc(sizeof(BeepsContext));

	pBC->h = h;
	pBC->w = w;
	pBC->beeps_mode = mode;

	switch(mode)
	{
	case BEEPS_GRAY_MODE:
		pBC->dataLen = h * w;

		pBC->transform_src = mem_allocuc(h * w);
		pBC->hv_data_g = mem_alloci(h * w);
		pBC->hv_data_p = mem_alloci(h * w);
		pBC->hv_data_r = mem_alloci(h * w);
		pBC->vh_data_g = mem_alloci(h * w);
		pBC->vh_data_p = mem_alloci(h * w);
		pBC->vh_data_r = mem_alloci(h * w);

		break;
	case BEEPS_COLOR_MODE:
		pBC->dataLen = h * w * 3;

		pBC->transform_src = mem_allocuc(h * w * 3);
		pBC->hv_data_g = mem_alloci(h * w * 3);
		pBC->hv_data_p = mem_alloci(h * w * 3);
		pBC->hv_data_r = mem_alloci(h * w * 3);
		pBC->vh_data_g = mem_alloci(h * w * 3);
		pBC->vh_data_p = mem_alloci(h * w * 3);
		pBC->vh_data_r = mem_alloci(h * w * 3);

		break;
	default:
		break;
	}

	//TESTADD
	int i;
	float sigma = 13.0;
	for(i = 0; i < 1; ++i, sigma += 1.5)
	{
		pBC->muTable[i] = (int *)malloc(256 * 256 * 4 * sizeof(int));
		pBC->gainTable[i] = (int *)malloc(256 * sizeof(int));

		calculateMuTable(pBC->muTable[i], sigma);
		calculateGainTable(pBC->gainTable[i], sigma);
	}
	
	return (void *)pBC;
}

void beeps_process(unsigned char *src, unsigned char *dst,
	float sigma, void *beepsHandle)
{
	BeepsContext *pBC;

	pBC = (BeepsContext *)beepsHandle;
	
	int *pMuTable, *pGainTable;
	pMuTable = pGainTable = NULL;
	int index = (int)sigma - 1;
	//if(index < 0 || index > 14)
	//	index = 0;
	index = 0;
	
	pMuTable = pBC->muTable[index];
	pGainTable = pBC->gainTable[index];

	if(pBC->beeps_mode == BEEPS_GRAY_MODE)
	{
		imageTransposition(src, pBC->transform_src, pBC->h, pBC->w);
		prBeeps(src, pBC->transform_src, pBC->hv_data_p, pBC->hv_data_r, pBC->vh_data_p, pBC->vh_data_r, pBC->h, pBC->w, pMuTable);
		
		gainBeeps(src, pBC->hv_data_g, pBC->dataLen, pGainTable);
		gainBeeps(pBC->transform_src, pBC->vh_data_g, pBC->dataLen, pGainTable);

		calcuteResult(pBC->hv_data_r, pBC->hv_data_p, pBC->hv_data_g, pBC->dataLen);
		calcuteResult(pBC->vh_data_r, pBC->vh_data_p, pBC->vh_data_g, pBC->dataLen);
		
		// combine image
		combineImage(pBC->hv_data_r, pBC->vh_data_r, dst, pBC->h, pBC->w);
	}
	else
	{
		imageTransposition_color(src, pBC->transform_src, pBC->h, pBC->w);
		prBeeps_color(src, pBC->transform_src, pBC->hv_data_p, pBC->hv_data_r, pBC->vh_data_p, pBC->vh_data_r, pBC->h, pBC->w, pMuTable);
		
		gainBeeps(src, pBC->hv_data_g, pBC->dataLen, pGainTable);
		gainBeeps(pBC->transform_src, pBC->vh_data_g, pBC->dataLen, pGainTable);

		calcuteResult(pBC->hv_data_r, pBC->hv_data_p, pBC->hv_data_g, pBC->dataLen);
		calcuteResult(pBC->vh_data_r, pBC->vh_data_p, pBC->vh_data_g, pBC->dataLen);

		combineImage_color(pBC->hv_data_r, pBC->vh_data_r, dst, pBC->h, pBC->w);
	}
}

void beeps_process_padding(unsigned char *src, unsigned char *dst,
	float sigma, int padding, void *beepsHandle)
{
	BeepsContext *pBC;

	pBC = (BeepsContext *)beepsHandle;
	
	int *pMuTable, *pGainTable;
	pMuTable = pGainTable = NULL;
	int index = (int)sigma - 1;
	//if(index < 0 || index > 14)
	//	index = 0;
	index = 0;
	
	pMuTable = pBC->muTable[index];
	pGainTable = pBC->gainTable[index];

	if(pBC->beeps_mode == BEEPS_GRAY_MODE)
	{
		imageTransposition(src, pBC->transform_src, pBC->h, pBC->w);
		prBeeps(src, pBC->transform_src, pBC->hv_data_p, pBC->hv_data_r, pBC->vh_data_p, pBC->vh_data_r, pBC->h, pBC->w, pMuTable);
		
		gainBeeps(src, pBC->hv_data_g, pBC->dataLen, pGainTable);
		gainBeeps(pBC->transform_src, pBC->vh_data_g, pBC->dataLen, pGainTable);

		calcuteResult(pBC->hv_data_r, pBC->hv_data_p, pBC->hv_data_g, pBC->dataLen);
		calcuteResult(pBC->vh_data_r, pBC->vh_data_p, pBC->vh_data_g, pBC->dataLen);
		
		// combine image
		combineImage_padding(pBC->hv_data_r, pBC->vh_data_r, dst, pBC->h, pBC->w, padding);
	}
	else
	{
		imageTransposition_color(src, pBC->transform_src, pBC->h, pBC->w);
		prBeeps_color(src, pBC->transform_src, pBC->hv_data_p, pBC->hv_data_r, pBC->vh_data_p, pBC->vh_data_r, pBC->h, pBC->w, pMuTable);
		
		gainBeeps(src, pBC->hv_data_g, pBC->dataLen, pGainTable);
		gainBeeps(pBC->transform_src, pBC->vh_data_g, pBC->dataLen, pGainTable);

		calcuteResult(pBC->hv_data_r, pBC->hv_data_p, pBC->hv_data_g, pBC->dataLen);
		calcuteResult(pBC->vh_data_r, pBC->vh_data_p, pBC->vh_data_g, pBC->dataLen);

		combineImage_color(pBC->hv_data_r, pBC->vh_data_r, dst, pBC->h, pBC->w);
	}
}

void beeps_uninit(void *beepsHandle)
{
	if(beepsHandle == NULL)
		return;

	BeepsContext *pBC = (BeepsContext *)beepsHandle;
#ifdef USE_SSE2
	_mm_free(pBC->transform_src);
#else
	free(pBC->transform_src);
#endif /* USE_SSE2 */

	mem_freei(pBC->hv_data_g);
	mem_freei(pBC->hv_data_p);
	mem_freei(pBC->hv_data_r);

	mem_freei(pBC->vh_data_g);
	mem_freei(pBC->vh_data_p);
	mem_freei(pBC->vh_data_r);

	int i;
	for(i = 0; i < 1; ++i)
	{
		free(pBC->muTable[i]);
		free(pBC->gainTable[i]);
	}

	free(pBC);
}
