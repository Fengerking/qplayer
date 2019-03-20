
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "sw_lc.h"

typedef struct _swcontext
{
    uint8_t *swTable;
    int h, w;
}SWContext;

void *sw_lc_init(int h, int w, float beta)
{
    SWContext *pSWC = (SWContext *)malloc(sizeof(SWContext));
    pSWC->h = h;
    pSWC->w = w;
    
    int i;
    float betaLog = log(beta);
    float tmp;
    
    const int M = 256;
    pSWC->swTable = new uint8_t[M];
    for(i = 0; i < M; ++i)
    {
        tmp = i;
        tmp /= 255.0;
        tmp = log(tmp * (beta - 1.0) + 1.0) / betaLog;
        tmp *= 255.0;
        
        if(tmp < 0.0)
            pSWC->swTable[i] = 0;
        else if(tmp > 255.0)
            pSWC->swTable[i] = 255;
        else
            pSWC->swTable[i] = tmp;
    }
    
    return (void *)pSWC;
}

void sw_lc_uninit(void *swHandle)
{
    if(NULL == swHandle)
        return;
    
    SWContext *pSWC = (SWContext *)swHandle;
    delete [] pSWC->swTable;
    
    free(pSWC);
}

uint8_t max_3(uint8_t r, uint8_t g, uint8_t b)
{
    r = r > g ? r : g;
    r = r > b ? r : b;
    
    return r;
}

uint8_t min_3(uint8_t r, uint8_t g, uint8_t b)
{
    r = r < g ? r : g;
    r = r < b ? r : b;
    
    return r;
}

void sw_lc_process(uchar *srcBuf, uchar *dstBuf, void *swHandle)
{
    SWContext *pSWC = (SWContext *)swHandle;
    
    int i;
    float tmp;
    uint8_t *pin, *pout;
    
    pin = srcBuf;
    pout= dstBuf;
    
    float beta = 1.1;
    float betaLog = log(beta);
    uint8_t r, g, b;
    for(i = 0; i < pSWC->h * pSWC->w; ++i)
    {
        r = *pin++;
        g = *pin++;
        b = *pin++;
        
        *pout++ = pSWC->swTable[r];
        *pout++ = pSWC->swTable[g];
        *pout++ = pSWC->swTable[b];
        
    }
}

void sw_lc_process_single(uchar *srcBuf, uchar *dstBuf, void *swHandle)
{
    SWContext *pSWC = (SWContext *)swHandle;
    
    int i;
    float tmp;
    uint8_t *pin, *pout;
    
    pin = srcBuf;
    pout= dstBuf;
    
    uint8_t r, g, b;
    for(i = 0; i < pSWC->h * pSWC->w; ++i)
    {
        r = *pin++;
        *pout++ = pSWC->swTable[r];
    }
}

void rgb2rgbplan(uint8_t * srcBuf, uint8_t *dstBuf, int h, int w)
{
    uint8_t *pin, *pr, *pg, *pb;
    int i, j;
    
    pin = srcBuf;
    pr = dstBuf;
    pg = dstBuf + h * w;
    pb = pg + h * w;
    for(i = 0; i < h * w * 3; i += 3)
    {
        *pr++ = *pin++;
        *pg++ = *pin++;
        *pb++ = *pin++;
    }
}

void rgbplan2rgb(uint8_t * srcBuf, uint8_t *dstBuf, int h, int w)
{
    uint8_t *pin, *pr, *pg, *pb;
    int i, j;
    
    pr = srcBuf;
    pg = srcBuf + h * w;
    pb = pg + h * w;
    pin = dstBuf;
    for(i = 0; i < h * w; ++i)
    {
        *pin++ = *pr++;
        *pin++ = *pg++;
        *pin++ = *pb++;
    }
}

void blend_y(uint8_t *srcBuf, uint8_t *dstBuf, int h, int w, float alpha)
{
    int i;
    for(i = 0; i < h * w; ++i)
    {
        srcBuf[i] = srcBuf[i] * alpha + dstBuf[i] * (1 - alpha);
        //printf("i %d\n", i);
        
    }
}
