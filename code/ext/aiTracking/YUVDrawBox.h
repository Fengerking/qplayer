/*******************************************************************************
File:		YUVDrawBox.h

Contains:	YUV DrawBox header file.

Written by:	Qichao Shen

Change History (most recent first):
2018-01-24		Qichao			Create file

*******************************************************************************/


#ifndef __YUV_DRAWBOX_H__
#define __YUV_DRAWBOX_H__


#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))


#define RGB_TO_Y_CCIR(r, g, b) \
((FIX(0.29900*219.0/255.0) * (r) + FIX(0.58700*219.0/255.0) * (g) + \
  FIX(0.11400*219.0/255.0) * (b) + (ONE_HALF + (16 << SCALEBITS))) >> SCALEBITS)

#define RGB_TO_U_CCIR(r1, g1, b1, shift)\
(((- FIX(0.16874*224.0/255.0) * r1 - FIX(0.33126*224.0/255.0) * g1 +         \
     FIX(0.50000*224.0/255.0) * b1 + (ONE_HALF << shift) - 1) >> (SCALEBITS + shift)) + 128)

#define RGB_TO_V_CCIR(r1, g1, b1, shift)\
(((FIX(0.50000*224.0/255.0) * r1 - FIX(0.41869*224.0/255.0) * g1 -           \
   FIX(0.08131*224.0/255.0) * b1 + (ONE_HALF << shift) - 1) >> (SCALEBITS + shift)) + 128)



#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define AIDRAW_MAX_SEI_ENTRY_COUNT 128
#define DEFAULT_MAX_FACE_COUNT    32
#define MAX_SEI_INFO_SIZE         768

#define AI_DRAW_INFO_WARNING_BLUE   0
#define AI_DRAW_INFO_WARNING_RED    1



typedef struct S_DrawBoxContext
{
	int x, y, w, h;
	int thickness;
	char *color_str;
	unsigned char yuv_color[4];
	int invert_color; ///< invert luma color
	int vsub, hsub;   ///< chroma subsampling
	char *x_expr, *y_expr; ///< expression for x and y
	char *w_expr, *h_expr; ///< expression for width and height
	char *t_expr;          ///< expression for thickness
	int have_alpha;
	int iInfoType;
} S_DrawBoxContext;


typedef struct S_SEI_Info
{
	long long   ullTimeStamp;
	unsigned    char        uSeiInfo[8196];
	int         iSeiInfoSize;
} S_SEI_Info;

typedef struct S_Rect
{
	int  iX;
	int  iY;
	int  iW;
	int  iH;
} S_Rect;


int InitDrawBoxContext(S_DrawBoxContext*  pDrawBox);
void AdjustBackGroundColor(S_DrawBoxContext*  pDrawBox);
int SetPosInfoDrawBoxContext(S_DrawBoxContext*  pDrawBox, int ix, int iy, int ih, int iw);
int DoDrawBoxContext(S_DrawBoxContext*  pDrawBox, unsigned char* pYUVData[], int ilinesize[], int iWidth, int iHeight);

#endif