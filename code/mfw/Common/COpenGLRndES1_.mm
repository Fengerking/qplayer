/*******************************************************************************
	File:		COpenGLRndES1.mm

	Contains:	The OpenGL render implement on iOS code

	Written by:	Jun Lin

	Change History (most recent first):
	2017-03-01		Jun Lin			Create file

*******************************************************************************/
#include "qcErr.h"
#include "COpenGLRndES1.h"
#include "ULogFunc.h"
#include "USystemFunc.h"
#include "qcClrConv.h"
#import <OpenGLES/ES1/glext.h>

//#define VIDEO_BACK_COLOR 	1.0, 0.5, 0.0, 1.0
#define VIDEO_BACK_COLOR 	0.0, 0.0, 0.0, 1.0

COpenGLRndES1::COpenGLRndES1(CBaseInst * pBaseInst, void * hInst)
:CBaseVideoRnd (pBaseInst, hInst)
,m_pContext(NULL)
,m_nTextureWidth(0) 	// must be divisible by 64, see GetTextureSize
,m_nTextureHeight(0) 	// must be divisible by 64, see GetTextureSize
,m_nBackingWidth(0)
,m_nBackingHeight(0)
,m_pFrameData(NULL)
,m_nFrameBuffer(0)
,m_bInitializing(false)
,m_nZoomMode(COpenGLRnd::QC_FIXED_RATIO)
,m_nRotation(COpenGLRnd::QC_ROTATION_0)
,m_nRatio(COpenGLRnd::QC_RATIO_00)
,m_bStop(true)
,m_nDecOutputType(QC_VDT_YUV420_P)
,m_pVieoView(NULL)
,m_nGlInitTime(0)
,m_pParent(nil)
,m_nRGBType(32)
,m_nInputWidth(0)
,m_nInputHeight(0)
,m_nOutputWidth(0)
,m_nOutputHeight(0)
{
    QCLOGI("[GL]Render instance is created. %p, %s %s", this, __TIME__,  __DATE__);
    
    //
    SetObjectName ("COpenGLRndES1");
    memset (&m_rcGLFrameDrawArea, 0, sizeof (m_rcGLFrameDrawArea));
    
    m_cAdjustPoint.x = 0;
    m_cAdjustPoint.y = 0;
    
    memset(m_fCoordinates, 0, sizeof(m_fCoordinates));
    memset(m_fVertices, 0, sizeof(m_fVertices));
}

COpenGLRndES1::~COpenGLRndES1(void)
{
    m_pBaseInst = NULL;
    QCLOGI("[GL]GL begin destroy, %p", this);
    CAutoLock lock (&m_mtDraw);
    
    QCLOGI("[GL]+Render instance is destroy. %p, parent %p, base %p", this, m_pParent, m_pBaseInst);
	Uninit ();
    
    //glFinish();
    QCLOGI("[GL]-Render instance is destroy. %p, ctx %p, view %p, parent %p, base %p", this, m_pContext, m_pVieoView, m_pParent, m_pBaseInst);
}

int COpenGLRndES1::SetViewInternal(void * hView, RECT * pRect)
{
    CAutoLock lock (&m_mtDraw);
    
    bool bDrawRectChanged = IsDrawRectChanged(pRect);
    bool bZoomModeChanged = false;
    if([NSRunLoop mainRunLoop] == [NSRunLoop currentRunLoop])
        bZoomModeChanged = IsZoomModeChanged((__bridge UIView*)hView);
    
    if (pRect != NULL && (pRect->right-pRect->left)>0 && (pRect->bottom-pRect->top)>0)
        memcpy (&m_rcView, pRect, sizeof (RECT));
    else
    {
        // if user not set draw rect, default is view's size
        if(hView)
        {
            UIView* view = (__bridge UIView*)hView;
            m_rcView = {(int)view.bounds.origin.x, (int)view.bounds.origin.y, (int)view.bounds.size.width, (int)view.bounds.size.height};
        }
    }
    
    if(!m_pParent)
        m_pParent = (__bridge UIView*)hView;
    else if(m_pParent != hView)
    {
        Uninit ();
        m_pParent = (__bridge UIView*)hView;
    }
    
    if(pRect && m_pVieoView && bDrawRectChanged)
    {
        //if([NSRunLoop mainRunLoop] == [NSRunLoop currentRunLoop])
        
        m_pVieoView.frame = CGRectMake(m_rcView.left, m_rcView.top, m_rcView.right-m_rcView.left, m_rcView.bottom-m_rcView.top);
    }
    
    if(hView && bZoomModeChanged)
    {
        m_nZoomMode = ViewContentMode2ZoomMode(m_pParent.contentMode);
    }
    
    if(bDrawRectChanged || bZoomModeChanged)
    {
        if(IsGLRenderReady())
        {
            RefreshView();
        }
    }

	return QC_ERR_NONE;
}

int COpenGLRndES1::SetView (void* hView, RECT * pRect)
{
    SetViewInternal(hView, pRect);
    return QC_ERR_NONE;
}

bool COpenGLRndES1::IsZoomModeChanged(UIView* hView)
{
    if(!hView)
        return false;
    
    int nNewMode = ViewContentMode2ZoomMode(((UIView*)hView).contentMode);
    return nNewMode != m_nZoomMode;
}

bool COpenGLRndES1::IsDrawRectChanged(RECT* pNewRect)
{
	if(pNewRect == NULL || (pNewRect->right-pNewRect->left)<=0 || (pNewRect->bottom-pNewRect->top)<=0)
        return false;
    
    if(pNewRect->left!=m_rcView.left || pNewRect->top!=m_rcView.top
       || pNewRect->right!=m_rcView.right || pNewRect->bottom!=m_rcView.bottom)
        return true;
    
    return false;
}


int COpenGLRndES1::Start (void)
{
    m_bStop = false;
    return CBaseVideoRnd::Start();
}

int COpenGLRndES1::Stop (void)
{
    if(m_bStop)
        return QC_ERR_NONE;
    
    //QCLOGI("[GL]Stop");
    
    m_bStop = true;
    CBaseVideoRnd::Stop();
    
    CAutoLock lock (&m_mtDraw);
    
    //ClearGL();
    
    return QC_ERR_NONE;
}

int COpenGLRndES1::Init (QC_VIDEO_FORMAT * pFmt)
{
    if(!IsFormatChanged((QC_VIDEO_FORMAT *)pFmt) && IsGLRenderReady())
        return QC_ERR_NONE;
    else if(IsFormatChanged((QC_VIDEO_FORMAT *)pFmt) && IsGLRenderReady())
    {
        CBaseVideoRnd::Init(pFmt);
        SetTexture(pFmt->nWidth, pFmt->nHeight);
        return QC_ERR_NONE;
    }
    
    if (CBaseVideoRnd::Init (pFmt) != QC_ERR_NONE)
        return QC_ERR_STATUS;

    //if(pFmt->nCodecID == QC_CODEC_ID_NONE)
        //return QC_ERR_NONE;
    if(pFmt && pFmt->nWidth>0 && pFmt->nHeight>0)
		return InitInternal(pFmt);
    return QC_ERR_NONE;
}

int COpenGLRndES1::InitInternal (QC_VIDEO_FORMAT * pFmt)
{
    m_nGlInitTime		= qcGetSysTime();
    m_nTextureWidth     = GetTextureSize(pFmt->nWidth);//(pFmt->nWidth+3) & ~3;
    m_nTextureHeight    = GetTextureSize(pFmt->nWidth);//pFmt->nHeight;
    m_nInputWidth = pFmt->nWidth;
    m_nInputHeight = pFmt->nHeight;

    
    InitGLOnMainThread();
    return QC_ERR_NONE;
}

int COpenGLRndES1::InitGLOnMainThread()
{
    if(m_bInitializing || !m_pBaseInst)
    {
        QCLOGI("[GL]GL is initializing..., %d, %p, %p", qcGetSysTime()-m_nGlInitTime, this, m_pBaseInst);
        return QC_ERR_NONE;
    }
    
    m_bInitializing = true;
    
    if([NSRunLoop mainRunLoop] == [NSRunLoop currentRunLoop])
    {
        QCLOGI("[GL]GL init on main thread, %p", this);
        InitOpenGLInternal();
    }
    else
    {
        QCLOGW("[GL]GL init on sub thread, dangerous. %p", this);
        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            InitOpenGLInternal();
        }];
    }
    
    return QC_ERR_NONE;
}


void COpenGLRndES1::InitOpenGLInternal()
{
    QCLOGI("[GL]GL +InitOpenGLInternal: %p, parent %p, base %p, %d", this, m_pParent, m_pBaseInst, qcGetSysTime()-m_nGlInitTime);
    if(!m_pParent || !m_pBaseInst)
    {
        QCLOGI("[GL]GL -InitOpenGLInternal: %p, parent %p, base %p", this, m_pParent, m_pBaseInst);
        m_bInitializing = false;
        return;
    }
    
    CAutoLock lock (&m_mtDraw);
    
    if(!m_pContext)
        m_pContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
    
    UpdateOpenGLView(true);
    InitGL2D();
    RefreshView();
    UpdatePosition();
    m_bInitializing = false;
    QCLOGI("[GL]GL -InitOpenGLInternal: %p, %d", this, qcGetSysTime()-m_nGlInitTime);
}


int COpenGLRndES1::Uninit (void)
{
    CAutoLock lock (&m_mtDraw);
    
    if(m_pContext)
        [EAGLContext setCurrentContext: m_pContext];
    
    DeleteRenderBuffer();
    DeleteFrameBuffer();
    DeleteTexture();
    
    DeleteFrameData();
    
    //
    if(m_pContext)
    {
        [EAGLContext setCurrentContext: nil];
        [m_pContext release];
        m_pContext = nil;
    }
    
    UpdateOpenGLView(false);
    m_pParent = nil;
    
	return QC_ERR_NONE;
}

int COpenGLRndES1::Prepare (QC_VIDEO_BUFF* pVideoBuffer)
{
	
    return QC_ERR_NONE;
}

int COpenGLRndES1::UploadTexture(QC_VIDEO_BUFF* pVideoBuffer)
{
    //QCLOGI("[GL]Texture width %d, height %d", m_nTextureWidth, m_nTextureHeight);
    //    QCLOGI("YUV comes, time %lld, %p(%d), %p(%d), %p(%d)", pBuff->llTime, pVideoBuffer->pBuff[0], pVideoBuffer->nStride[0],
    //                                    pVideoBuffer->pBuff[1], pVideoBuffer->nStride[1], pVideoBuffer->pBuff[2], pVideoBuffer->nStride[2]);
    
    if(pVideoBuffer->nType == QC_VDT_YUV420_P)
    {
        int nBytesOfPixcel = 4;
        if (565 == m_nRGBType)
            nBytesOfPixcel = 2;
        else
            nBytesOfPixcel = 4;
        int nRndStride = ((m_nTextureWidth + 3) & ~3) * nBytesOfPixcel;

        if (NULL == m_pFrameData)
        {
            m_pFrameData = new unsigned char[nRndStride * m_nTextureHeight];
        }
        
        if (NULL == m_pFrameData)
            return QC_ERR_FAILED;
        
        qcColorConvert_c (pVideoBuffer->pBuff[0], pVideoBuffer->pBuff[1], pVideoBuffer->pBuff[2], pVideoBuffer->nStride[0],
                          m_pFrameData, nRndStride, m_nInputWidth, m_nInputHeight, pVideoBuffer->nStride[1], pVideoBuffer->nStride[2]);
    }
    
    return QC_ERR_NONE;
}


int COpenGLRndES1::RenderToScreen(QC_VIDEO_BUFF* pVideoBuffer)
{
    glClear(GL_COLOR_BUFFER_BIT);
    if (565 == m_nRGBType)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_nTextureWidth, m_nTextureHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, m_pFrameData);
    else// RGB32
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_nTextureWidth, m_nTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pFrameData);
    
    glVertexPointer(2, GL_FLOAT, 0, m_fVertices);        // 只有x，y坐标，没有z，所以一个点只用2个数字描述
    glTexCoordPointer(2, GL_FLOAT, 0, m_fCoordinates);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);            // 画四个点，矩形
    // GL_TRIANGLE_STRIP 模式,使用头两个顶点，然后对于后续顶点，它都将和之前的两个顶点组成一个三角形
    return QC_ERR_NONE;
}

int COpenGLRndES1::RenderCommit()
{
    [m_pContext presentRenderbuffer: GL_RENDERBUFFER_OES];
    return QC_ERR_NONE;
}


int COpenGLRndES1::Render (QC_DATA_BUFF * pBuff)
{
    // Process BA, the first frame will not be new format flag for Local and RTMP, but HLS does
    if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT && IsFormatChanged((QC_VIDEO_FORMAT *)pBuff->pFormat))
    {
        QC_VIDEO_FORMAT* pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
        QCLOGI("[GL]Video format changed, width %d, height %d", pFmt->nWidth, pFmt->nHeight);
        CBaseVideoRnd::Init(pFmt);
        // process BA
        SetTexture(pFmt->nWidth, pFmt->nHeight);
        //m_nRndCount++;
        //return QC_ERR_NONE;
    }

    if (!IsGLRenderReady())
    {
#if 0
        QCLOGW("[GL]GL not ready, %p, %d, %d, %d, %p", this, m_bInitializing,
               m_nColorRenderBuffer, m_nFrameBuffer, m_pVideoTextureCache);
#endif
        InitInternal(&m_fmtVideo);
        m_nRndCount++;
        return QC_ERR_RETRY;
    }
    
    CBaseVideoRnd::Render (pBuff);
    
    CAutoLock lock (&m_mtDraw);
    QC_VIDEO_BUFF* pVideoBuffer = NULL;
    
    //if((pBuff->uFlag & QC_BUFF_TYPE_Video) == QC_BUFF_TYPE_Video)
    {
        pVideoBuffer = (QC_VIDEO_BUFF *)pBuff->pBuffPtr;
    }
    
    if(!pVideoBuffer)
        return QC_ERR_FAILED;
    
    if (pVideoBuffer->nType != QC_VDT_YUV420_P && pVideoBuffer->nType != QC_VDT_NV12)
    {
        pVideoBuffer = &m_bufVideo;
        if (pVideoBuffer->nType != QC_VDT_YUV420_P)
            return QC_ERR_STATUS;
    }

    if(m_bStop)
        return QC_ERR_STATUS;

    int nRet = Prepare(pVideoBuffer);

    if (![EAGLContext setCurrentContext:m_pContext])
        return QC_ERR_FAILED;

#if 0
    CFAbsoluteTime fTime = CFAbsoluteTimeGetCurrent() * 1000;
#endif

    UpdateDecOutputType(pVideoBuffer->nType);

    nRet = UploadTexture(pVideoBuffer);

    if (QC_ERR_NONE != nRet)
        return nRet;

    RenderToScreen(pVideoBuffer);
    RenderCommit();
    m_nRndCount++;
    
#if 0
    static CFAbsoluteTime g_nTotalTime = 0;
    CFAbsoluteTime fTimeL = CFAbsoluteTimeGetCurrent() * 1000;
    g_nTotalTime += (fTimeL - fTime);
    //if( (fTimeL - fTime) >= 15)
    {
        QCLOGI("[GL]Average render use time:%f, current frame render time:%f\n", g_nTotalTime / m_nRndCount, fTimeL - fTime);
    }
#endif
	
	return QC_ERR_NONE;
}

bool COpenGLRndES1::UpdateRenderSize (void)
{
	bool bRC = CBaseVideoRnd::UpdateRenderSize ();

	return bRC;
}

int COpenGLRndES1::SetupTexture()
{
    if(!m_pBaseInst)
        return QC_ERR_STATUS;

    DeleteTexture();
    CreateTexture();

    return QC_ERR_NONE;
}

int COpenGLRndES1::SetRotation(COpenGLRnd::QC_ROTATIONMODE eType)
{
    CAutoLock lock(&m_mtDraw);
    
    if (NULL == m_pContext || ![EAGLContext setCurrentContext:m_pContext])
    {
        return QC_ERR_FAILED;
    }
    
    if (m_nRotation != eType)
    {
        m_nRotation = eType;
        //UpdateVertices();
        UpdatePosition();
    }
    
    return QC_ERR_NONE;
}

int COpenGLRndES1::SetTexture(int nWidth, int nHeight)
{
    CAutoLock lock(&m_mtDraw);
    
    if (NULL == m_pContext || ![EAGLContext setCurrentContext:m_pContext] || !m_pBaseInst)
    {
        return QC_ERR_FAILED;
    }
    
    //if ((nWidth != m_nTextureWidth) || (nHeight != m_nTextureHeight))
    {
        m_nInputWidth = nWidth;
        m_nInputHeight = nHeight;
        m_nTextureWidth = GetTextureSize(nWidth);
        m_nTextureHeight = GetTextureSize(nHeight);
        OnTextureSizeChange();
        //UpdateVertices();
        UpdatePosition();
    }
    
    return QC_ERR_NONE;
}

int COpenGLRndES1::OnTextureSizeChange()
{
    DeleteFrameData();
    return QC_ERR_NONE;
}

int COpenGLRndES1::UpdateDecOutputType(int nCurrType)
{
    if(nCurrType != m_nDecOutputType)
    {
        m_nDecOutputType = nCurrType;
        SetupTexture();
    }

    return QC_ERR_NONE;
}

int COpenGLRndES1::SetOutputRect(int nLeft, int nTop, int nRight, int nBottom)
{
    CAutoLock lock(&m_mtDraw);
    
    if (NULL == m_pContext || ![EAGLContext setCurrentContext:m_pContext])
    {
        return QC_ERR_FAILED;
    }
    
    m_cAdjustPoint.x = nLeft;
    m_cAdjustPoint.y = nTop;
    m_nOutputWidth = (nRight - nLeft) * [[UIScreen mainScreen] scale];
    m_nOutputHeight = (nBottom - nTop) * [[UIScreen mainScreen] scale];
    
    //UpdateVertices();
    UpdatePosition();
    
    return QC_ERR_NONE;
}

int COpenGLRndES1::ClearGL()
{
    if (NULL == m_pContext || ![EAGLContext setCurrentContext:m_pContext])
        return QC_ERR_FAILED;

    DrawBackgroundColor();

    /* Clear the renderbuffer */
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();

    /* Refresh the screen */
    RenderCommit();
    
    return QC_ERR_NONE;
}


int COpenGLRndES1::RefreshView()
{
    //QCLOGI("[GL]Setup OpenGL, %p", m_pVieoView);
    
    if (!m_pVieoView || !m_pBaseInst)
        return QC_ERR_FAILED;
    
    CAEAGLLayer* eaglLayer = (CAEAGLLayer *)(((UIView*)m_pVieoView).layer);
    eaglLayer.opaque = YES;
    
    if (m_nRGBType == 565)
    {
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool: NO], kEAGLDrawablePropertyRetainedBacking,
                                        kEAGLColorFormatRGB565,
                                        kEAGLDrawablePropertyColorFormat,
                                        nil];
        
    }
    else//RGB32
    {
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool: NO], kEAGLDrawablePropertyRetainedBacking,
                                        kEAGLColorFormatRGBA8,
                                        kEAGLDrawablePropertyColorFormat,
                                        nil];
    }
    
    if (NULL == m_pContext || ![EAGLContext setCurrentContext:m_pContext])
        return QC_ERR_FAILED;

    m_nOutputWidth = (m_rcView.right - m_rcView.left) * [[UIScreen mainScreen] scale];
    m_nOutputHeight = (m_rcView.bottom - m_rcView.top) * [[UIScreen mainScreen] scale];

    // frame buffer first, then renderbuffer
    SetupFrameBuffer();
    SetupRenderBuffer();
    
    //SetupGlViewport();
    SetupView();
    
    CreateTexture();
    
    return QC_ERR_NONE;
}

int COpenGLRndES1::SetupGlViewport()
{
    if(!m_pBaseInst)
        return QC_ERR_STATUS;
    /* Extract renderbuffer's width and height. This should match layer's size */
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &m_nBackingWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &m_nBackingHeight);
    
    /* Set viewport size to match the renderbuffer size */
    glViewport(0, 0, m_nBackingWidth, m_nBackingHeight);
    DrawBackgroundColor();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    QCLOGI("[GL]Setup GL viewport: %d x %d", m_nBackingWidth, m_nBackingHeight);
    
    return QC_ERR_NONE;
}

int COpenGLRndES1::SetupRenderBuffer()
{
    if(!m_pBaseInst)
        return QC_ERR_STATUS;
    
    DeleteRenderBuffer();
    
    /* Generate handles for 1 view- and 1 m_nRenderBuffer. */
    glGenRenderbuffersOES(1, &m_nRenderBuffer);
    
    /* Bind (i.e. select) frame and render buffers. */
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_nRenderBuffer);
    
    if (nil == m_pContext)
    {
        return QC_ERR_STATUS;
    }
    
    /* This call is equiv. of glRenderbufferStorage, that binds renderbuffer to layer's framebuffer */
    [m_pContext renderbufferStorage: GL_RENDERBUFFER_OES fromDrawable: (CAEAGLLayer *)m_pVieoView.layer];
    
    /* Attach a renderbuffer to a framebuffer, color buffer */
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, m_nRenderBuffer);
    
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
        QCLOGE("Framebuffer creation failed: %x\n", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
        return QC_ERR_FAILED;
    }

    return QC_ERR_NONE;
}

int COpenGLRndES1::SetupFrameBuffer()
{
    if(!m_pBaseInst)
        return QC_ERR_STATUS;
    DeleteFrameBuffer();
    
    /* Generate handles for 1 view- and 1 m_nRenderBuffer. */
    glGenFramebuffersOES(1, &m_nFrameBuffer);
    
    /* Bind (i.e. select) frame and render buffers. */
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_nFrameBuffer);
    return QC_ERR_NONE;
}

int COpenGLRndES1::DeleteFrameData()
{
    if (NULL != m_pFrameData)
    {
        delete []m_pFrameData;
        m_pFrameData = NULL;
    }
    
    return QC_ERR_NONE;
}

int COpenGLRndES1::DeleteRenderBuffer()
{
    if (m_nRenderBuffer)
    {
        glDeleteRenderbuffersOES(1, &m_nRenderBuffer);
        m_nRenderBuffer = 0;
    }

    return QC_ERR_NONE;
}

int COpenGLRndES1::DeleteFrameBuffer()
{
    if (m_nFrameBuffer)
    {
        glDeleteFramebuffersOES(1, &m_nFrameBuffer);
        m_nFrameBuffer = 0;
    }
    
    return QC_ERR_NONE;
}

int COpenGLRndES1::DeleteTexture()
{
    if (m_nFrameTexture)
    {
        glDeleteTextures(1, &m_nFrameTexture);
        m_nFrameTexture = 0;
    }
    
    return QC_ERR_NONE;
}

int COpenGLRndES1::GLTexImage2D(GLuint nTexture, unsigned char* pData, int nWidth, int nHeight)
{
    glBindTexture(GL_TEXTURE_2D, nTexture);
    
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, nWidth, nHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pData);
    
    return QC_ERR_NONE;
}


int COpenGLRndES1::Redraw()
{
    return QC_ERR_NONE;
}

bool COpenGLRndES1::IsGLRenderReady()
{
    if (m_bInitializing)
        return false;
    
    if (0 == m_nFrameBuffer)
        return false;
    
    return true;
}

int COpenGLRndES1::ViewContentMode2ZoomMode(int nViewContentMode)
{
    if(UIViewContentModeScaleToFill == nViewContentMode)	// 拉伸填充满窗口
        return COpenGLRnd::QC_FILLWINDOW;
    else if(UIViewContentModeScaleAspectFit == nViewContentMode)  // 按宽高比例，不做裁剪
        return COpenGLRnd::QC_FIXED_RATIO;
    else if(UIViewContentModeScaleAspectFill == nViewContentMode)  // 按宽高比例，裁剪
        return COpenGLRnd::QC_PANSCAN;
    
    return COpenGLRnd::QC_FIXED_RATIO;
}

void COpenGLRndES1::UpdateOpenGLView(bool bCreate)
{
    UIView* pParent = m_pParent;
    
    if(!pParent)
        return;

    if(bCreate)
    {
        if(m_pVieoView)
            return ;
        
        //CGRect rect = pParent.bounds;
        CGRect rect = CGRectMake(m_rcView.left, m_rcView.top, m_rcView.right-m_rcView.left, m_rcView.bottom-m_rcView.top);
        
        m_pVieoView = [[COpenGLView alloc] initWithFrame:rect];
        m_nZoomMode = ViewContentMode2ZoomMode(pParent.contentMode);
        //[m_pVieoView setAutoresizingMask:(UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight)];
        m_pVieoView.backgroundColor = [UIColor blackColor];
    	[pParent insertSubview:m_pVieoView atIndex:0];
        [m_pVieoView release];
        
        QCLOGI("[GL]Create video view, width %f, height %f, mode %d", rect.size.width, rect.size.height, m_nZoomMode);
    }
    else
    {
        m_hView = NULL;
        m_pParent = nil;
        if(!m_pVieoView)
            return;
        [m_pVieoView setDelegate:NULL userData:NULL];
        [m_pVieoView removeFromSuperview];
        m_pVieoView = nil;
    }
}

bool COpenGLRndES1::IsFormatChanged(QC_VIDEO_FORMAT* pFormat)
{
    if(!pFormat)
        return false;
    if(pFormat->nWidth<=0 || pFormat->nHeight<=0)
        return false;
    
    return (pFormat->nWidth != m_fmtVideo.nWidth) || (pFormat->nHeight != m_fmtVideo.nHeight);
}

void COpenGLRndES1::DrawBackgroundColor()
{
//    if(m_pBaseInst)
//    {
//        glClearColor(m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor.fRed, m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor.fGreen,
//                     m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor.fBlue, m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor.fAlpha);
//    }
//    else
        glClearColor(VIDEO_BACK_COLOR);
}

void COpenGLRndES1::InitGL2D()
{
    /* Enable texturing, we do not need depth buffer or alpha handling */
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glDisable(GL_DITHER);
    
    glEnable(GL_TEXTURE_2D);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

void COpenGLRndES1::CreateFramebuffer()
{
    if (0 != m_nFrameBuffer)
    {
        return;
    }
    
    /* Generate handles for 1 view- and 1 m_nRenderBuffer. */
    glGenFramebuffersOES(1, &m_nFrameBuffer);
    glGenRenderbuffersOES(1, &m_nRenderBuffer);
    
    /* Bind (i.e. select) frame and render buffers. */
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_nFrameBuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_nRenderBuffer);
    
    if (nil == m_pContext) {
        return;
    }
    /* This call is equiv. of glRenderbufferStorage, that binds renderbuffer to layer's framebuffer */
    [m_pContext renderbufferStorage: GL_RENDERBUFFER_OES fromDrawable: (CAEAGLLayer *)m_pVieoView.layer];
    
    /* Attach a renderbuffer to a framebuffer, color buffer */
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, m_nRenderBuffer);
    
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
        QCLOGE("Framebuffer creation failed: %x\n", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
        return;
    }
}

void COpenGLRndES1::SetupView()
{
    /* Extract renderbuffer's width and height. This should match layer's size, I assume */
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &m_nBackingWidth);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &m_nBackingHeight);
    
    /* Set viewport size to match the renderbuffer size */
    int glX = 0;
    int glY = 0;
    glViewport(glX, glY, m_nBackingWidth, m_nBackingHeight);
    
    
    /* Setup orthographic projection mode */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //glOrthof(-m_nBackingWidth, m_nBackingWidth, -m_nBackingHeight, m_nBackingHeight, -1.0, 1.0);
    glOrthof(0, m_nBackingWidth, 0, m_nBackingHeight, -1.0, 0);
    ////NSLog(@"GL: glX = %d, glY = %d, m_nBackingWidth = %d, m_nBackingHeight = %d \n\n", -m_nBackingWidth, m_nBackingWidth, -m_nBackingHeight, m_nBackingHeight);
    
    /* Reset the matrix mode to model/view */
    glMatrixMode(GL_MODELVIEW);
    
    ClearGL();
}

int COpenGLRndES1::GetTextureSize(int size)
{
    int outSize = 64;
    while (outSize < size)
        outSize <<= 1;
    return outSize;
}


void COpenGLRndES1::CreateTexture()
{
    DeleteTexture();
    
    glGenTextures(1, (GLuint*)&m_nFrameTexture);
    glBindTexture(GL_TEXTURE_2D, m_nFrameTexture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);//GL_NEAREST
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//GL_NEAREST
}

void COpenGLRndES1::UpdatePosition()
{
    QCLOGI("[GL]UpdateVertices %d %d", m_nTextureWidth, m_nTextureHeight);
    
    if (0 == m_nBackingWidth || 0 == m_nBackingHeight || 0 == m_nTextureWidth || 0 == m_nTextureHeight || !m_pBaseInst)
    {
        QCLOGE("[GL]Status error!!!");
        return;
    }
    
    if (COpenGLRnd::QC_FILLWINDOW == m_nZoomMode)
    {
        // full screen
        m_rcGLFrameDrawArea.size.width = m_nBackingWidth;
        m_rcGLFrameDrawArea.size.height = m_nBackingHeight;
    }
    else if (COpenGLRnd::QC_ORIGINAL == m_nZoomMode)
    {
        // ORIGINAL means just draw by video's original size, just like 176*144
        m_rcGLFrameDrawArea.size.width = m_nInputWidth;
        m_rcGLFrameDrawArea.size.height = m_nInputHeight;
    }
    else if (COpenGLRnd::QC_ZOOMIN == m_nZoomMode)
    {
        m_rcGLFrameDrawArea.size.width = m_nInputWidth;
        m_rcGLFrameDrawArea.size.height = m_nInputHeight;

//        m_rcGLFrameDrawArea.size.width = m_nBackingWidth / (m_fOutRightPer - m_fOutLeftPer);
//        m_rcGLFrameDrawArea.size.height = m_nBackingHeight / (m_fOutBottomPer - m_fOutTopPer);
//        m_rcGLFrameDrawArea.origin.x = -m_fOutLeftPer * m_rcGLFrameDrawArea.size.width;
//        m_rcGLFrameDrawArea.origin.y = -m_fOutTopPer * m_rcGLFrameDrawArea.size.height;
    }
    else
    {
        m_rcGLFrameDrawArea.size.width = m_nBackingWidth;
        m_rcGLFrameDrawArea.size.height = m_nBackingHeight;


        unsigned int    nW = 0;
        unsigned int    nH = 0;
        
        if (m_nRatio == COpenGLRnd::QC_RATIO_00)
        {
//            nW = m_nTextureWidth;
//            nH = m_nTextureHeight;
            nW = m_nInputWidth;
            nH = m_nInputHeight;
        }
        else if (m_nRatio == COpenGLRnd::QC_RATIO_11)
        {
            nW = 1;
            nH = 1;
        }
        else if (m_nRatio == COpenGLRnd::QC_RATIO_43)
        {
            nW = 4;
            nH = 3;
        }
        else if (m_nRatio == COpenGLRnd::QC_RATIO_169)
        {
            nW = 16;
            nH = 9;
        }
        else if (m_nRatio == COpenGLRnd::QC_RATIO_21)
        {
            nW = 2;
            nH = 1;
        }
        else if (m_nRatio == COpenGLRnd::QC_RATIO_2331)
        {
            nW = 233;
            nH = 100;
        }
        else
        {
            // deafult is video's original width and height
//            nW = m_nTextureWidth;
//            nH = m_nTextureHeight;
            nW = m_nInputWidth;
            nH = m_nInputHeight;
        }
        
        if(m_nRotation == COpenGLRnd::QC_ROTATION_90 || m_nRotation == COpenGLRnd::QC_ROTATION_270)
        {
            int nTmp = nW;
            nW = nH;
            nH = nTmp;
        }
        
        if (m_nZoomMode == COpenGLRnd::QC_PANSCAN)
        {
            if (m_rcGLFrameDrawArea.size.width * nH > m_rcGLFrameDrawArea.size.height * nW)
                m_rcGLFrameDrawArea.size.height = m_rcGLFrameDrawArea.size.width * nH / nW;
            else
                m_rcGLFrameDrawArea.size.width = m_rcGLFrameDrawArea.size.height * nW / nH;
        }
        else
        {
            if (m_rcGLFrameDrawArea.size.width * nH > m_rcGLFrameDrawArea.size.height * nW)
                m_rcGLFrameDrawArea.size.width = m_rcGLFrameDrawArea.size.height * nW / nH;
            else
                m_rcGLFrameDrawArea.size.height = m_rcGLFrameDrawArea.size.width * nH / nW;
        }
    }
    
    if (COpenGLRnd::QC_ZOOMIN != m_nZoomMode)
    {
        m_rcGLFrameDrawArea.origin.x = (m_nBackingWidth - m_rcGLFrameDrawArea.size.width) / 2;
        m_rcGLFrameDrawArea.origin.y = (m_nBackingHeight - m_rcGLFrameDrawArea.size.height) / 2;
    }
    
    QCLOGI("[GL]Draw rect: width %f, height %f, left %f, top %f, rotatation %d", m_rcGLFrameDrawArea.size.width, m_rcGLFrameDrawArea.size.height, m_rcGLFrameDrawArea.origin.x, m_rcGLFrameDrawArea.origin.y, (int)m_nRotation);
    
//    GLfloat nLengthX = (GLfloat)(2 * m_rcGLFrameDrawArea.size.width) / m_nBackingWidth;
//    GLfloat nLengthY = (GLfloat)(2 * m_rcGLFrameDrawArea.size.height) / m_nBackingHeight;
//    GLfloat nXLeft = -1 + (GLfloat)(2 * m_rcGLFrameDrawArea.origin.x) / m_nBackingWidth; // from left
//    GLfloat nYTop = 1 - ((GLfloat)(2 * m_rcGLFrameDrawArea.origin.y) / m_nBackingHeight); // from top
    

    CGPoint point = {m_rcGLFrameDrawArea.origin.x, m_rcGLFrameDrawArea.origin.y};
    //CGPoint point = {nXLeft, nYTop};
    m_nOutputWidth = m_rcGLFrameDrawArea.size.width;
    m_nOutputHeight = m_rcGLFrameDrawArea.size.height;
    
    GLfloat maxS = (GLfloat)m_nInputWidth / m_nTextureWidth;
    GLfloat maxT = (GLfloat)m_nInputHeight / m_nTextureHeight;
    
    GLfloat *pc = m_fCoordinates;
    GLfloat *pv = m_fVertices;
    
    if (m_nRotation == COpenGLRnd::QC_ROTATION_0 ||
        m_nRotation == COpenGLRnd::QC_ROTATION_180 ||
        m_nRotation == COpenGLRnd::QC_ROTATION_0FLIP ||
        m_nRotation == COpenGLRnd::QC_ROTATION_180FLIP)
    {
        ////NSLog(@"m_fVertices rotate 0\n");
        *pv++ = 0 + point.x;                // bottom left
        *pv++ = 0 + point.y;
        
        *pv++ = m_nOutputWidth + point.x;        // bottom right
        *pv++ = 0 + point.y;
        
        *pv++ = 0 + point.x;                // top left
        *pv++ = m_nOutputHeight + point.y;
        
        *pv++ = m_nOutputWidth + point.x;        // top right
        *pv++ = m_nOutputHeight + point.y;
    }
    else
    {
        ////NSLog(@"m_fVertices not rotate 0\n");
        *pv++ = -m_nOutputHeight + point.x;
        *pv++ = -m_nOutputWidth + point.y;
        *pv++ = m_nOutputHeight + point.x;
        *pv++ = -m_nOutputWidth + point.y;
        *pv++ = -m_nOutputHeight + point.x;
        *pv++ = m_nOutputWidth + point.y;
        *pv++ = m_nOutputHeight + point.x;
        *pv++ = m_nOutputWidth + point.y;
    }
    
    //纹理的长宽范围为0～1
    switch (m_nRotation)
    {
        case COpenGLRnd::QC_ROTATION_0:  //3412
            ////NSLog(@"m_fCoordinates VO_GL_ROTATION_0 0\n");
            *pc++ = 0;
            *pc++ = maxT;
            *pc++ = maxS;
            *pc++ = maxT;
            *pc++ = 0;
            *pc++ = 0;
            *pc++ = maxS;
            *pc++ = 0;
            break;
            
        case COpenGLRnd::QC_ROTATION_180:  //2143
            ////NSLog(@"m_fCoordinates VO_GL_ROTATION_180 0\n");
            *pc++ = maxS;
            *pc++ = 0;
            *pc++ = 0;
            *pc++ = 0;
            *pc++ = maxS;
            *pc++ = maxT;
            *pc++ = 0;
            *pc++ = maxT;
            break;
            
            //        case ROTATION_90:  //4231
            //            ////NSLog(@"m_fCoordinates ROTATION_90 0\n");
            //            *pc++ = maxS;
            //            *pc++ = maxT;
            //            *pc++ = maxS;
            //            *pc++ = 0;
            //            *pc++ = 0;
            //            *pc++ = maxT;
            //            *pc++ = 0;
            //            *pc++ = 0;
            //            break;
            //
            //        case ROTATION_270:  //1324
            //            ////NSLog(@"m_fCoordinates ROTATION_270 0\n");
            //            *pc++ = 0;
            //            *pc++ = 0;
            //            *pc++ = 0;
            //            *pc++ = maxT;
            //            *pc++ = maxS;
            //            *pc++ = 0;
            //            *pc++ = maxS;
            //            *pc++ = maxT;
            //            break;
            
        case COpenGLRnd::QC_ROTATION_0FLIP:  //4321
            ////NSLog(@"m_fCoordinates VO_GL_ROTATION_0FLIP 0\n");
            *pc++ = maxS;
            *pc++ = maxT;
            *pc++ = 0;
            *pc++ = maxT;
            *pc++ = maxS;
            *pc++ = 0;
            *pc++ = 0;
            *pc++ = 0;
            break;
            
        case COpenGLRnd::QC_ROTATION_180FLIP:  //1234
            ////NSLog(@"m_fCoordinates VO_GL_ROTATION_180FLIP 0\n");
            *pc++ = 0;
            *pc++ = 0;
            *pc++ = maxS;
            *pc++ = 0;
            *pc++ = 0;
            *pc++ = maxT;
            *pc++ = maxS;
            *pc++ = maxT;
            break;
            
            //        case ROTATION_90FLIP:  //3142
            //            ////NSLog(@"m_fCoordinates ROTATION_90FLIP 0\n");
            //            *pc++ = 0;
            //            *pc++ = maxT;
            //            *pc++ = 0;
            //            *pc++ = 0;
            //            *pc++ = maxS;
            //            *pc++ = maxT;
            //            *pc++ = maxS;
            //            *pc++ = 0;
            //            break;
            //
            //        case ROTATION_270FLIP: //2413
            //            ////NSLog(@"m_fCoordinates ROTATION_270FLIP 0\n");
            //            *pc++ = maxS;
            //            *pc++ = 0;
            //            *pc++ = maxS;
            //            *pc++ = maxT;
            //            *pc++ = 0;
            //            *pc++ = 0;
            //            *pc++ = 0;
            //            *pc++ = maxT;
            //            break;
            
        default: // == case ROTATION_90:  //4231
            ////NSLog(@"m_fCoordinates default 0\n");
            *pc++ = maxS;
            *pc++ = maxT;
            *pc++ = maxS;
            *pc++ = 0;
            *pc++ = 0;
            *pc++ = maxT;
            *pc++ = 0;
            *pc++ = 0;
            break;
    }
    
    Redraw();
    
    QCLOGI("[GL]point(%f, %f), m_fVertices: %f, %f, %f, %f, %f, %f, %f, %f \n\n",
    point.x, point.y, m_fVertices[0], m_fVertices[1], m_fVertices[2], m_fVertices[3], m_fVertices[4], m_fVertices[5], m_fVertices[6], m_fVertices[7]);
    
    QCLOGI("[GL]m_fCoordinates: %f, %f, %f, %f, %f, %f, %f, %f \n\n",
    m_fCoordinates[0], m_fCoordinates[1], m_fCoordinates[2], m_fCoordinates[3], m_fCoordinates[4], m_fCoordinates[5], m_fCoordinates[6], m_fCoordinates[7]);

}



