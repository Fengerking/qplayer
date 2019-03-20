
#ifndef MEM_TOOLS_H
#define MEM_TOOLS_H

//#ifdef __cplusplus
//extern "C"
//{
//#endif /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "SSE_header.h"

#define MEM_PADDING 32

typedef unsigned char uint8_t;

/*memory*/
inline float ***mem_allocf_3(int n, int r,
	int c, int padding = MEM_PADDING)
{
	float **p, ***pp;
    int rc = r * c;
    int i, j;

#if defined(USE_SSE2)
	_MM_ALIGN16 float *a = (_MM_ALIGN16 float*) _mm_malloc(sizeof(float) * n * rc + padding, 16);
#else
	float *a = (float *)malloc(sizeof(float) * n * rc + padding);
#endif /* USE_SSE2 */

	if(a == NULL)
	{
		printf("mem_allocf_3 failed\n");
		return NULL;
	}

	memset(a, 0, sizeof(float) * n * rc + padding);

    p = (float **) malloc(sizeof(float *) * n * r);
    pp = (float ***) malloc(sizeof(float **) * n);
    for(i = 0; i < n; ++i) 
        for(j = 0; j < r; ++j) 
            p[i * r + j] = &a[i * rc + j * c];
    for(i = 0; i < n; ++i) 
        pp[i] = &p[i * r];

    return(pp);
}

inline void mem_freef_3(float ***p)
{
	if(p!=NULL)
	{

#if defined(USE_SSE2)
		_mm_free(p[0][0]);
#else
		free(p[0][0]);
#endif /* USE_SSE2 */

		free(p[0]);
		free(p);
		p=NULL;
	}
}

inline float** mem_allocf_2(int r,int c,int padding = MEM_PADDING)
{
	float **p;
	
#if defined(USE_SSE2)
	_MM_ALIGN16 float *a = (_MM_ALIGN16 float*) _mm_malloc(sizeof(float) * r * c + padding, 16);
#else
	float *a = (float*)malloc(sizeof(float) * r * c + padding);
#endif /* USE_SSE2 */

	if(a == NULL)
	{
		printf("mem_allocf_2 failed\n");
		return NULL;
	}

	p = (float **)malloc(sizeof(float *) * r);
	for(int i = 0; i < r; ++i)
		p[i] = &a[i * c];

	return(p);
}

inline void mem_freef_2(float **p)
{
	if(p!=NULL)
	{

#if defined(USE_SSE2)
		_mm_free(p[0]);
#else
		free(p[0]);
#endif /* USE_SSE2 */

		free(p);
		p = NULL;
	}
}

inline float *mem_allocf(int len, int padding = MEM_PADDING)
{
#if defined(USE_SSE2)
	_MM_ALIGN16 float *p = (_MM_ALIGN16 float*) _mm_malloc(sizeof(float) * len + padding, 16);
#else
	float *p = (float*)malloc(sizeof(float) * len + padding);
#endif /* USE_SSE2 */

	if(p == NULL)
	{
		printf("mem_allocf failed\n");
		return NULL;
	}

	return(p);
}

inline void mem_freef(float *p)
{
	if(p != NULL)
	{

#if defined(USE_SSE2)
		_mm_free(p);
#else
		free(p);
#endif /* USE_SSE2 */

		p = NULL;
	}
}

inline int *mem_alloci(int len, int padding = MEM_PADDING)
{
#if defined(USE_SSE2)
	_MM_ALIGN16 int *p = (_MM_ALIGN16 int*) _mm_malloc(sizeof(int) * len + padding, 16);
#else
	int *p = (int *)malloc(sizeof(int) * len + padding);
#endif /* USE_SSE2 */

	if(p == NULL)
	{
		printf("mem_allocf failed\n");
		return NULL;
	}

	return(p);
}

inline void mem_freei(int *p)
{
	if(p != NULL)
	{

#if defined(USE_SSE2)
		_mm_free(p);
#else
		free(p);
#endif /* USE_SSE2 */

		p = NULL;
	}
}

inline uint8_t *mem_allocuc(int len, int padding = MEM_PADDING)
{
#if defined(USE_SSE2)
	_MM_ALIGN16 uint8_t *p = (_MM_ALIGN16 uint8_t *) _mm_malloc(sizeof(uint8_t) * len + padding, 16);
#else
	uint8_t *p = (uint8_t*)malloc(sizeof(uint8_t) * len + padding);
#endif /* USE_SSE2 */

	if(p == NULL)
	{
		printf("mem_allocuc failed\n");
		return NULL;
	}

	return(p);
}

inline void mem_freeuc(uint8_t *p)
{
	if(p != NULL)
	{

#if defined(USE_SSE2)
		_mm_free(p);
#else
		free(p);
#endif /* USE_SSE2 */

		p = NULL;
	}
}

//#ifdef __cplusplus
//}
//#endif /* __cplusplus */

#endif /* MEM_TOOLS_H */