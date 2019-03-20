/**
* File : ClConv.c
* Created on : 2014-11-1
* Author : yongping.lin
* Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
* Description : ClConv实现文件
*/

#include "ClConv.h"

const unsigned char	*ccClip255 = &(ccClip255Base[384]);
TTUint32 alpha_value = 0;

#define NEWLOAD_DATA(a3, a7, c, y){\
	a0 = pSrc_u[c] - 128;\
	a1 = pSrc_v[c] - 128;\
	a3 = (pSrc_y[y] - 16)*ConstY;	\
	a7 = (pSrc_y[inStride+y] - 16)*ConstY;\
}

// input: a0 = u, a1 = v
// output: a2, a0, a1
#define NEWCAL_UV(){\
	a2 = (a0 * ConstU1);\
	a0 = (a1 * ConstV2 + a0 *ConstU2);\
	a1 = (a1 * ConstV1);\
}

#define NEWPIXARGB32_0(a3){\
	a4  = ((ccClip255[(a3 + a1)>>20]));\
	a5  = ((ccClip255[(a3 - a0)>>20]));\
	a3  = ((ccClip255[(a3 + a2)>>20]));\
	a4 = ((((a3<<8)|a5 )<<8))|(a4);\
	a4 |= (alpha_value<< 24) ;\
}

#define NEWLOAD_YPIX(a3, x){\
	a3 = (pSrc_y[(x)] - 16)*ConstY;\
}

#define WRITERGB32_PIX(a4,a5, x){\
	*((TTUint32*)(pOutBuf+(x)))   = (TTUint32)a4;\
	*((TTUint32*)(pOutBuf+(x)+4)) = (TTUint32)a5;\
}

#define NEWPIXARGB32_1(a3){\
	a5  = ((ccClip255[(a3 + a2)>>20]));\
	a8  = ((ccClip255[(a3 + a1 )>>20]));\
	a3  = (ccClip255[(a3 - a0)>>20]);\
	a5  = ((a5<<16)|(a3<<8)|a8);\
	a5 |= (alpha_value<< 24) ;\
}

#define NEWPIXARGB32_2(a3){\
	a4  = ((ccClip255[(a3 + a1)>>20]));\
	a5  = (ccClip255[(a3 - a0)>>20]);\
	a3  = ((ccClip255[(a3 + a2)>>20]));\
	a4 = ((((a3<<8)|a5 )<<8))|(a4);\
	a4 |= (alpha_value<< 24) ;\
}

#define NEWPIXARGB32_3(a3){\
	a5  = ((ccClip255[(a3 + a2)>>20]));\
	a8  = ((ccClip255[(a3 + a1)>>20]));\
	a3  = (ccClip255[(a3 - a0)>>20]);\
	a5  = ((a5<<16)|(a3<<8)|a8);\
	a5 |= (alpha_value<< 24) ;\
}


static void colorCon16x16_c(TTPBYTE pSrc_y, TTPBYTE pSrc_u, TTPBYTE pSrc_v,const TTUint32 inStride,TTPBYTE pOutBuf, const TTUint32 outStride,
												TTInt32 width, TTInt32 height,const TTInt32 uinStride, const TTInt32 vinStride)
{
	TTUint32 a6 = 8;

	do{
		TTInt32 a0, a1, a2, a3, a4, a5,a7,a8;
		//0
		NEWLOAD_DATA(a3, a7, 0, 0)
		NEWCAL_UV()
		NEWPIXARGB32_0(a3)
		NEWLOAD_YPIX(a3, 1)
		NEWPIXARGB32_1(a3)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5,0)
		NEWPIXARGB32_2(a7)
		NEWLOAD_YPIX(a7, inStride+1)
		NEWPIXARGB32_3(a7)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5,outStride)

		//1	
		NEWLOAD_DATA(a3, a7, 1, 2)
		NEWCAL_UV()
		NEWPIXARGB32_0(a3)
		NEWLOAD_YPIX(a3, 3)	
		NEWPIXARGB32_1(a3)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, 8)

		NEWPIXARGB32_2(a7)
		NEWLOAD_YPIX(a7, inStride+3)
		NEWPIXARGB32_3(a7)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5,outStride+8)

		//2
		NEWLOAD_DATA(a3, a7, 2, 4)
		NEWCAL_UV()
		NEWPIXARGB32_0(a3)
		NEWLOAD_YPIX(a3, 5)
		NEWPIXARGB32_1(a3)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, 16)

		NEWPIXARGB32_2(a7)
		NEWLOAD_YPIX(a7, inStride+5)
		NEWPIXARGB32_3(a7)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, outStride+16)

		//3
		NEWLOAD_DATA(a3, a7, 3, 6)
		NEWCAL_UV()
		NEWPIXARGB32_0(a3)
		NEWLOAD_YPIX(a3, 7)	
		NEWPIXARGB32_1(a3)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, 24)

		NEWPIXARGB32_2(a7)
		NEWLOAD_YPIX(a7, inStride+7)
		NEWPIXARGB32_3(a7)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, outStride+24)

		//4
		NEWLOAD_DATA(a3, a7, 4, 8)
		NEWCAL_UV()
		NEWPIXARGB32_0(a3)
		NEWLOAD_YPIX(a3, 9)	
		NEWPIXARGB32_1(a3)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, 32)

		NEWPIXARGB32_2(a7)
		NEWLOAD_YPIX(a7, inStride+9)
		NEWPIXARGB32_3(a7)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, outStride+32)

		//5
		NEWLOAD_DATA(a3, a7, 5, 10)
		NEWCAL_UV()
		NEWPIXARGB32_0(a3)
		NEWLOAD_YPIX(a3, 11)	
		NEWPIXARGB32_1(a3)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, 40)

		NEWPIXARGB32_2(a7)
		NEWLOAD_YPIX(a7, inStride+11)
		NEWPIXARGB32_3(a7)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, outStride+40)

		//6
		NEWLOAD_DATA(a3, a7, 6, 12)
		NEWCAL_UV()
		NEWPIXARGB32_0(a3)
		NEWLOAD_YPIX(a3, 13)	
		NEWPIXARGB32_1(a3)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, 48)

		NEWPIXARGB32_2(a7)
		NEWLOAD_YPIX(a7, inStride+13)
		NEWPIXARGB32_3(a7)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, outStride+48)

		//7
		NEWLOAD_DATA(a3, a7, 7, 14)
		NEWCAL_UV()
		NEWPIXARGB32_0(a3)
		NEWLOAD_YPIX(a3, 15)	
		NEWPIXARGB32_1(a3)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, 56)

		NEWPIXARGB32_2(a7)
		NEWLOAD_YPIX(a7, inStride+15)
		NEWPIXARGB32_3(a7)

		a4 |= (255 << 24);
		a5 |= (255 << 24);
		WRITERGB32_PIX(a4,a5, outStride+56)

		pSrc_y += (inStride<<1);
		pSrc_u += uinStride;
		pSrc_v += vinStride;
		pOutBuf += (outStride<<1);

	}while(--a6 != 0);
}

static void colorConNxN_c(TTPBYTE pSrc_y, TTPBYTE pSrc_u, TTPBYTE pSrc_v,const TTUint32 inStride,TTPBYTE pOutBuf, const TTUint32 outStride,
											  TTInt32 width, TTInt32 height,const TTInt32 uinStride, const TTInt32 vinStride)
{
	TTInt32 i;

	do{
		i = width;
		do{
			TTInt32 a0, a1, a2, a3, a4;

			a3 = *(pSrc_v++) - 128;	
			a2 = *(pSrc_u++) - 128;

			a0 = a3 * ConstV1;
			a1 = a3 * ConstV2 + a2 *ConstU2;
			a2 = a2 * ConstU1;
			a3 = ((*pSrc_y) - 16) * ConstY;
			a4 = (*(pSrc_y + 1) - 16) * ConstY;

			a3 = ((ccClip255[((a3 + a0)>>20)]))|((ccClip255[((a3 - a1)>>20)])<<8)|((ccClip255[((a3 + a2)>>20)])<<16)|(255 << 24);
			a4 = ((ccClip255[((a4 + a0)>>20)]))|((ccClip255[((a4 - a1)>>20)])<<8)|((ccClip255[((a4 + a2)>>20)])<<16)|(255 << 24);
			*((TTUint32*)pOutBuf)       = a3;
			*((TTUint32*)(pOutBuf + 4)) = a4;

			/////////////////////////////////////////////////////////////////////
			a3 = (*(pSrc_y + inStride) - 16) * ConstY;
			a4 = (*(pSrc_y + inStride + 1) - 16) * ConstY;
			pSrc_y += 2;

			a3 = ((ccClip255[((a3 + a0)>>20)]))|((ccClip255[((a3 - a1)>>20)])<<8)|((ccClip255[((a3 + a2)>>20)])<<16)|(255 << 24);
			a4 = ((ccClip255[((a4 + a0)>>20)]))|((ccClip255[((a4 - a1)>>20)])<<8)|((ccClip255[((a4 + a2)>>20)])<<16)|(255 << 24);
			*((TTUint32*)(pOutBuf + outStride))     = a3;
			*((TTUint32*)(pOutBuf + outStride + 4)) = a4;
			pOutBuf += 8;

		}while((i-=2) != 0);

		i = (width >> 1);
		pSrc_u -= i;
		pSrc_v -= i;
		pSrc_u += uinStride;
		pSrc_v += vinStride;
		pSrc_y -= width;
		pSrc_y += (inStride << 1);

		pOutBuf -= (width << 2 );
		pOutBuf += (outStride << 1);
	}while((height-=2) != 0);
}

static void colorConNxN_new(TTPBYTE pSrc_y, TTPBYTE pSrc_u, TTPBYTE pSrc_v,const TTUint32 inStride,TTPBYTE pOutBuf, const TTUint32 outStride,
											  TTInt32 width, TTInt32 height,const TTInt32 uinStride, const TTInt32 vinStride)
{
	TTInt32 i;
	TTInt32 b0, b1, b2;

	do{
		i = width;
		do{
			TTInt32 a0, a1, a2, a3, a4;

			a3 = *(pSrc_v++) - 128;	
			a2 = *(pSrc_u++) - 128;

			a0 = a3 * ARMV7ConstV1;
			a1 = a3 * ARMV7ConstV2 + a2 *ARMV7ConstU2;
			a2 = a2 * ARMV7ConstU1;
			a3 = ((*pSrc_y) - 16) * ARMV7ConstY;
			a4 = (*(pSrc_y + 1) - 16) * ARMV7ConstY;

			b0 = SAT((a3 + a0)>>12)
			b1 = SAT((a3 - a1)>>12)
			b2 = SAT((a3 + a2)>>12)
			a3 = (255 << 24)|(b2<<16)|(b1<<8)|b0;
			b0 = SAT((a4 + a0)>>12)
			b1 = SAT((a4 - a1)>>12)
			b2 = SAT((a4 + a2)>>12)
			a4 = (255 << 24)|(b2<<16)|(b1<<8)|b0;

			*((TTUint32*)pOutBuf)       = a3;
			*((TTUint32*)(pOutBuf + 4)) = a4;

			/////////////////////////////////////////////////////////////////////
			a3 = (*(pSrc_y + inStride) - 16) * ARMV7ConstY;
			a4 = (*(pSrc_y + inStride + 1) - 16) * ARMV7ConstY;
			pSrc_y  += 2;

			b0 = SAT((a3 + a0)>>12)
			b1 = SAT((a3 - a1)>>12)
			b2 = SAT((a3 + a2)>>12)
			a3 = (255 << 24)|(b2<<16)|(b1<<8)|b0;
			b0 = SAT((a4 + a0)>>12)
			b1 = SAT((a4 - a1)>>12)
			b2 = SAT((a4 + a2)>>12)
			a4 = (255 << 24)|(b2<<16)|(b1<<8)|b0;
			*((TTUint32*)(pOutBuf + outStride))     = a3;
			*((TTUint32*)(pOutBuf + outStride + 4)) = a4;
			pOutBuf += 8;

		}while((i-=2) != 0);

		i = (width >> 1);
		pSrc_u -= i;
		pSrc_v -= i;
		pSrc_u += uinStride;
		pSrc_v += vinStride;
		pSrc_y -= width;
		pSrc_y += (inStride << 1);

		pOutBuf -= (width << 2 );
		pOutBuf += (outStride << 1);
	}while((height-=2) != 0);
}


void colorConvAlpha(TTUint alpha)
{
	alpha_value = alpha;
}

#if defined _ARM_ARCH_NEON_
TTInt colorConvYUV2RGB(TTVideoBuffer* aSrc, TTVideoBuffer *sDst, TTInt aWidth, TTInt aHeight)
{
    TTInt vx = 0, width, height;
	TTPBYTE psrc_y = aSrc->Buffer[0];
    TTPBYTE psrc_u = aSrc->Buffer[1];
    TTPBYTE psrc_v = aSrc->Buffer[2];
    TTPBYTE y, u, v;
    TTPBYTE	start_out_buf, out_buf;
    TTInt in_stride, uin_stride, vin_stride;
    TTInt out_width = aWidth;
    TTInt out_height = aHeight;
	const TTInt out_stride = sDst->Stride[0];

	in_stride  = aSrc->Stride[0];
    uin_stride = aSrc->Stride[1];
    vin_stride = aSrc->Stride[2];

	y = psrc_y;
    u = psrc_u;
    v = psrc_v;
    //add end

	start_out_buf = out_buf = sDst->Buffer[0];

	colorConNxN_neon(psrc_y, psrc_u, psrc_v, in_stride, out_buf, out_stride, out_width - (out_width & 0xf), out_height, uin_stride, vin_stride);

	if((out_width & 0xf))  { //width remain 
		int width_remain = (out_width & 0xf);
		int width_height = out_height;
		psrc_y += out_width - width_remain;
		psrc_u += (out_width - width_remain) >> 1;
		psrc_v += (out_width - width_remain) >> 1;
		out_buf += (out_width - width_remain) * 4;
		colorConNxN_new(psrc_y, psrc_u, psrc_v, in_stride, out_buf, out_stride, width_remain, out_height, uin_stride, vin_stride);
	}

	return 0;
}

#else

TTInt colorConvYUV2RGB(TTVideoBuffer* aSrc, TTVideoBuffer *sDst, TTInt aWidth, TTInt aHeight)
{
    TTInt vx = 0, width, height;
	TTPBYTE psrc_y = aSrc->Buffer[0];
    TTPBYTE psrc_u = aSrc->Buffer[1];
    TTPBYTE psrc_v = aSrc->Buffer[2];
    TTPBYTE y, u, v;
    TTPBYTE	start_out_buf, out_buf;
    TTInt in_stride, uin_stride, vin_stride;
    TTInt out_width = aWidth;
    TTInt out_height = aHeight;
	const TTInt out_stride = sDst->Stride[0];

	in_stride  = aSrc->Stride[0];
    uin_stride = aSrc->Stride[1];
    vin_stride = aSrc->Stride[2];

	y = psrc_y;
    u = psrc_u;
    v = psrc_v;
    //add end

	start_out_buf = out_buf = sDst->Buffer[0];

	do {
		vx = out_width;
		out_buf = start_out_buf;
		do {
			width = vx < 16 ? vx : 16;
			height = out_height < 16 ? out_height : 16;
			if((width==16)&&(height==16)) {
				colorCon16x16_c(psrc_y, psrc_u, psrc_v, in_stride, out_buf, out_stride, width, height, uin_stride, vin_stride);
			} else {
				colorConNxN_c(psrc_y, psrc_u, psrc_v, in_stride, out_buf, out_stride, width, height, uin_stride, vin_stride);
			}

			psrc_y += 16;
			psrc_u += 8;
			psrc_v += 8;
			out_buf += 64;
		}
		while((vx -= 16) > 0);

		psrc_y = y = y + (in_stride << 4);
		psrc_u = u = u + (uin_stride << 3);
		psrc_v = v = v + (vin_stride << 3);
		start_out_buf += (out_stride << 4);
	}
	while((out_height -= 16) > 0);

	return 0;
}

#endif
