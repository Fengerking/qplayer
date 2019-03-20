/*******************************************************************************
	File:		COpenGLRnd.h

	Contains:	The Video OpenGL render header file.

	Written by:	Jun Lin

	Change History (most recent first):
	2017-03-01		Jun Lin			Create file

*******************************************************************************/
#ifndef __COpenGLRndES1_H__
#define __COpenGLRndES1_H__

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/glext.h>

#include "COpenGLRnd.h"

class COpenGLRndES1 : public CBaseVideoRnd
{
public:
	COpenGLRndES1(CBaseInst * pBaseInst, void * hInst);
	virtual ~COpenGLRndES1(void);

	virtual int		SetView (void * hView, RECT * pRect);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Render (QC_DATA_BUFF * pBuff);
    virtual int		Start (void);
    virtual int		Stop (void);
    
protected:
	virtual bool	UpdateRenderSize (void);
    
private:
    void    UpdateOpenGLView(bool bCreate);
    int     SetRotation(COpenGLRnd::QC_ROTATIONMODE eType);
    int     SetTexture(int nWidth, int nHeight);
    int     OnTextureSizeChange();
    int     UpdateDecOutputType(int nCurrType);
    int     SetOutputRect(int nLeft, int nTop, int nRight, int nBottom);
    int     ClearGL();
        
    int     SetupGlViewport();
    int     SetupRenderBuffer();
    int     SetupFrameBuffer();
    int     SetupTexture();

    int     DeleteRenderBuffer();
    int     DeleteFrameBuffer();
    int     DeleteFrameData();
    int     DeleteTexture();
    
    int     Redraw();
    int		RefreshView();
    int     RenderToScreen(QC_VIDEO_BUFF* pVideoBuffer);
    int     Prepare(QC_VIDEO_BUFF * pVideoBuffer);
    int     UploadTexture(QC_VIDEO_BUFF* pVideoBuffer);
    int     GLTexImage2D(GLuint nTexture, unsigned char* pData, int nWidth, int nHeight);
    int     RenderCommit();
    
    void    UpdateVertices();
    bool    IsGLRenderReady();
    int     InitGLOnMainThread();
    bool    IsFormatChanged(QC_VIDEO_FORMAT* pFormat);
    void	InitOpenGLInternal();
    int		InitInternal(QC_VIDEO_FORMAT * pFmt);
    int		SetViewInternal(void * hView, RECT * pRect);
    
    int		ViewContentMode2ZoomMode(int nViewContentMode);
    bool	IsZoomModeChanged(UIView* hView);
    bool	IsDrawRectChanged(RECT* pNewRect);
    void	DrawBackgroundColor();
    bool	UpdateSelfRotate();
private:
    // View to draw
    COpenGLView*  	m_pVieoView;
    UIView*			m_pParent;
    
    // Basic
    EAGLContext*    m_pContext;

//    int             m_nPositionSlot;
//    int             m_nTexCoordSlot;
    
    GLuint          m_nFrameBuffer;
//    GLuint          m_nProgramHandle;
//    GLuint          m_nColorRenderBuffer;
    
    // Area and position
    /* Dimensions of the backing buffer, it's full area of video view */
    int     m_nBackingWidth;
    int     m_nBackingHeight;
    
    /* Dimensions of the texture in pixels, it's video's width and height */
    int     m_nTextureWidth;
    int     m_nTextureHeight;
    
    /* Drawing area according to backing and texture above */
    CGRect  m_rcGLFrameDrawArea;
    
//    GLfloat m_fTextureVertices[8];
//    GLfloat m_fSquareVertices[8];
    
    // Settings
    int     m_nRatio;
    int     m_nRotation;
    int     m_nZoomMode;
    
    // Status
    bool    m_bStop;
    bool    m_bInitializing;
    
    // Zoom in
//    float m_fOutLeftPer;
//    float m_fOutTopPer;
//    float m_fOutRightPer;
//    float m_fOutBottomPer;
    
    // YUV buffer or RGB data, cloned
    unsigned char*  m_pFrameData;
    
    //
    int                         m_nDecOutputType;
    int							m_nGlInitTime;
    
    // RGB render
private:
    
    void	InitGL2D();
    void 	CreateFramebuffer();
    void	SetupView();
    void	CreateTexture();
    void	UpdatePosition();
    int		GetTextureSize(int nSize);
    
    GLuint 	m_nRenderBuffer;
    unsigned int m_nFrameTexture;
    
    int m_nInputWidth;
    int m_nInputHeight;
    int m_nOutputWidth;
    int m_nOutputHeight;
    CGPoint m_cAdjustPoint;
    
    GLfloat m_fCoordinates[8];
    GLfloat m_fVertices[8];
    
    int m_nRGBType;
};

#endif // __COpenGLRndES1_H__
