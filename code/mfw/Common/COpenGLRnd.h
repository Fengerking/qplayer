/*******************************************************************************
	File:		COpenGLRnd.h

	Contains:	The Video OpenGL render header file.

	Written by:	Jun Lin

	Change History (most recent first):
	2017-03-01		Jun Lin			Create file

*******************************************************************************/
#ifndef __COpenGLRnd_H__
#define __COpenGLRnd_H__

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/glext.h>
//#import <GLKit/GLKit.h>

#include "CBaseVideoRnd.h"

const int UIView_EVT_SIZE_CHANGED	= 0;
const int UIView_EVT_APP_INACTIVE 	= 1;
const int UIView_EVT_APP_ACTIVE 	= 2;
typedef void (QC_API * UIViewNotifyEvent) (void * pUserData, int nID, void * pValue1);

@interface COpenGLView : UIView
{
    UIViewNotifyEvent	_callback;
    void*				_userData;
}
- (void)onDestroy;
- (void)setDelegate:(UIViewNotifyEvent)pCallback userData:(void*)userData;
@end

class COpenGLRnd : public CBaseVideoRnd
{
    friend class COpenGLRndES1;
    
    typedef enum
    {
        QC_ORIGINAL,    // just video original width and height, just like 480x320
        QC_FILLWINDOW,   // stretch to full size of view
        QC_ZOOMIN,      // scale
        QC_PANSCAN,     // stretch
        QC_FIXED_RATIO, // it depneds on QC_RATIOMODE
    }QC_ZOOMMODE;
    
    typedef enum
    {
        QC_RATIO_00, // video original width and height
        QC_RATIO_11,
        QC_RATIO_43,
        QC_RATIO_169,
        QC_RATIO_21,
        QC_RATIO_2331
    }QC_RATIOMODE;
    
    typedef enum
    {
        QC_ROTATION_0,
        QC_ROTATION_0FLIP,		// 左右翻转
        QC_ROTATION_90,
        QC_ROTATION_180,
        QC_ROTATION_180FLIP,	// 上下翻转
        QC_ROTATION_270,
    }QC_ROTATIONMODE;

public:
	COpenGLRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~COpenGLRnd(void);

	virtual int		SetView (void * hView, RECT * pRect);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Render (QC_DATA_BUFF * pBuff);
    virtual int		Start (void);
    virtual int		Stop (void);
    virtual int     SetParam(int nID, void * pParam);
    
public:
    int     		OnViewEvent(int nID, void * pValue1);
    
protected:
	virtual bool	UpdateRenderSize (void);
    
private:
    void    UpdateOpenGLView(bool bCreate);
    int     SetRotation(QC_ROTATIONMODE eType);
    int     SetTexture(int nWidth, int nHeight);
    int     OnTextureSizeChange();
    int     UpdateDecOutputType(int nCurrType);
    int     SetOutputRect(int nLeft, int nTop, int nRight, int nBottom);
    int     ClearGL();
    
    GLuint  CompileShader(const GLchar *pBuffer, GLenum shaderType);
    int     CompileAllShaders();
    
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
    int     RenderToScreen();
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
    
    void	UninitInternal();
    bool	UpdateSelfRotate();
    bool	IsDisableRender();
    bool	SetContext(EAGLContext* pCtx);
private:
    // View to draw
    COpenGLView*  	m_pVieoView;
    UIView*			m_pParent;
    
    // Basic
    EAGLContext*    m_pContext;

    int             m_nPositionSlot;
    int             m_nTexCoordSlot;
    
    GLuint          m_nFrameBuffer;
    GLuint          m_nProgramHandle;
    GLuint          m_nColorRenderBuffer;
    
    GLuint          m_nTexturePlanarY;
    GLuint          m_nTexturePlanarU;
    GLuint          m_nTexturePlanarV;
    
    GLint           m_nTextureUniformY;
    GLint           m_nTextureUniformU;
    GLint           m_nTextureUniformV;
    GLint           m_nTextureUniformUV;
    
    // Area and position
    /* Dimensions of the backing buffer, it's full area of video view */
    int     m_nBackingWidth;
    int     m_nBackingHeight;
    
    /* Dimensions of the texture in pixels, it's video's width and height */
    int     m_nTextureWidth;
    int     m_nTextureHeight;
    
    /* Drawing area according to backing and texture above */
    CGRect  m_rcGLFrameDrawArea;
    
    GLfloat m_fTextureVertices[8];
    GLfloat m_fSquareVertices[8];
    
    // Settings
    int     m_nRotation;
    int     m_nZoomMode;
    
    // Status
    bool    m_bStop;
    bool    m_bInitializing;
    
    // Zoom in
    float m_fOutLeftPer;
    float m_fOutTopPer;
    float m_fOutRightPer;
    float m_fOutBottomPer;
    
    // YUV buffer, cloned
    unsigned char*  m_pFrameData;
    
    // NV12
    CVOpenGLESTextureRef        m_pLumaTexture;
    CVOpenGLESTextureRef        m_pChromaTexture;
    CVOpenGLESTextureCacheRef   m_pVideoTextureCache;
    CVImageBufferRef            m_pPixelBuffer;
    
    //
    int                         m_nDecOutputType;
    int							m_nGlInitTime;
    
    int							m_nDisplayCtrl;
};

#endif // __COpenGLRnd_H__
