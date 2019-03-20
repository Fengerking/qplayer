/*******************************************************************************
File:		YUVDrawBox.h

Contains:	YUV DrawBox implement file.

Written by:	Qichao Shen

Change History (most recent first):
2018-01-24		Qichao			Create file

*******************************************************************************/
#include "YUVDrawBox.h"
#include <string.h>

enum { Y, U, V, A };


int InitDrawBoxContext(S_DrawBoxContext*  pDrawBox)
{
	memset(pDrawBox, 0, sizeof(S_DrawBoxContext));
	unsigned char   uRR = 0;
	unsigned char   uGG = 0;
	unsigned char   uBB = 0x7F;

	pDrawBox->yuv_color[Y] = RGB_TO_Y_CCIR(uRR, uGG, uBB);
	pDrawBox->yuv_color[U] = RGB_TO_U_CCIR(uRR, uGG, uBB, 0);
	pDrawBox->yuv_color[V] = RGB_TO_V_CCIR(uRR, uGG, uBB, 0);
	pDrawBox->yuv_color[A] = 255;

	pDrawBox->hsub = 1;
	pDrawBox->vsub = 1;
	pDrawBox->have_alpha = 0;
	pDrawBox->thickness = 2;

	return 0;
}

void AdjustBackGroundColor(S_DrawBoxContext*  pDrawBox)
{
	unsigned char   uRR = 0;
	unsigned char   uGG = 0;
	unsigned char   uBB = 0x7F;

	switch (pDrawBox->iInfoType)
	{
		case AI_DRAW_INFO_WARNING_BLUE:
		{
			uRR = 0;
			uGG = 0;
			uBB = 0x7F;
			break;
		}

		case AI_DRAW_INFO_WARNING_RED:
		{
			uRR = 0x7F;
			uGG = 0;
			uBB = 0;
			break;
		}

		default:
		{
			break;
		}
	}

	pDrawBox->yuv_color[Y] = RGB_TO_Y_CCIR(uRR, uGG, uBB);
	pDrawBox->yuv_color[U] = RGB_TO_U_CCIR(uRR, uGG, uBB, 0);
	pDrawBox->yuv_color[V] = RGB_TO_V_CCIR(uRR, uGG, uBB, 0);
}


int SetPosInfoDrawBoxContext(S_DrawBoxContext*  pDrawBox, int ix, int iy, int iw, int ih)
{
	pDrawBox->x = ix;
	pDrawBox->y = iy;
	pDrawBox->w = iw;
	pDrawBox->h = ih;
	return 0;
}

int DoDrawBoxContext(S_DrawBoxContext*  pDrawBox, unsigned char* pYUVData[], int ilinesizes[], int iWidth, int iHeight)
{
	S_DrawBoxContext *s = pDrawBox;
	int plane, x, y, xb = s->x, yb = s->y;
	unsigned char *row[4] = {0};


	for (y = FFMAX(yb, 0); y < iHeight && y < (yb + s->h); y++) {
		row[0] = pYUVData[0] + y * ilinesizes[0];

		for (plane = 1; plane < 3; plane++)
			row[plane] = pYUVData[plane] +
			ilinesizes[plane] * (y >> s->vsub);

		if (s->invert_color) {
			for (x = FFMAX(xb, 0); x < xb + s->w && x < iWidth; x++)
				if ((y - yb < s->thickness) || (yb + s->h - 1 - y < s->thickness) ||
					(x - xb < s->thickness) || (xb + s->w - 1 - x < s->thickness))
					row[0][x] = 0xff - row[0][x];
		}
		else {
			for (x = FFMAX(xb, 0); x < xb + s->w && x < iWidth; x++) {
				double alpha = (double)s->yuv_color[A] / 255;

				if ((y - yb < s->thickness) || (yb + s->h - 1 - y < s->thickness) ||
					(x - xb < s->thickness) || (xb + s->w - 1 - x < s->thickness)) {
					row[0][x] = (1 - alpha) * row[0][x] + alpha * s->yuv_color[Y];
					row[1][x >> s->hsub] = (1 - alpha) * row[1][x >> s->hsub] + alpha * s->yuv_color[U];
					row[2][x >> s->hsub] = (1 - alpha) * row[2][x >> s->hsub] + alpha * s->yuv_color[V];
				}
			}
		}
	}

	return 0;
}