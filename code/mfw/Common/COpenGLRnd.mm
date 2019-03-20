/*******************************************************************************
	File:		COpenGLRnd.mm

	Contains:	The OpenGL render implement on iOS code

	Written by:	Jun Lin

	Change History (most recent first):
	2017-03-01		Jun Lin			Create file

*******************************************************************************/
#include "qcErr.h"
#include "COpenGLRnd.h"
#include "ULogFunc.h"
#include "USystemFunc.h"
#include "qcPlayer.h"

#define _STANDRAD_CC_ 1

// Used for NV12 and pixle image
static const GLchar *GLOBAL_VERTEX_SHADER_Y_UV[2] = {" \
    attribute vec4 position; \
    attribute vec2 texCoord; \
    varying vec2 texCoordVarying; \
    void main() \
    { \
    gl_Position = position; \
    texCoordVarying = texCoord; \
    }"
    ,
    " \
    uniform mediump mat4 modelview_matrix; \
    uniform mediump mat4 rotate_matrix; \
    uniform mediump mat4 projection_matrix; \
    attribute vec4 position; \
    attribute vec2 texCoord; \
    varying vec2 texCoordVarying; \
    void main() \
    { \
    gl_Position = projection_matrix * rotate_matrix * modelview_matrix * position; \
    texCoordVarying = texCoord; \
    }"
};

#if _STANDRAD_CC_
static const GLchar *GLOBAL_FRAGMENT_SHADER_Y_UV = " \
uniform sampler2D SamplerY; \
uniform sampler2D SamplerUV; \
varying highp vec2 texCoordVarying; \
void main() \
{ \
mediump vec3 yuv; \
lowp vec3 rgb; \
yuv.x = texture2D(SamplerY, texCoordVarying).r; \
yuv.yz = texture2D(SamplerUV, texCoordVarying).rg - vec2(0.5, 0.5); \
rgb = mat3(      1,       1,      1, \
0, -.18732, 1.8556, \
1.57481, -.46813,      0) * yuv; \
gl_FragColor = vec4(rgb, 1); \
}";
#else
static const GLchar *GLOBAL_FRAGMENT_SHADER_Y_UV = " \
uniform sampler2D SamplerY; \
uniform sampler2D SamplerUV; \
varying highp vec2 texCoordVarying; \
void main() \
{ \
mediump vec3 yuv; \
lowp vec3 rgb; \
yuv.x = texture2D(SamplerY, texCoordVarying).r; \
yuv.yz = texture2D(SamplerUV, texCoordVarying).rg - vec2(0.5, 0.5); \
rgb = mat3(1.0, 1.0, 1.0, \
0.0, -0.213, 2.112, \
1.71, -0.533,   0.0) * yuv; \
gl_FragColor = vec4(rgb, 1); \
}";
#endif

// Useed for YUV420
static const GLchar *GLOBAL_VERTEX_SHADER_Y_U_V = " \
attribute vec4 position; \
attribute mediump vec4 textureCoordinate; \
varying mediump vec2 coordinate; \
void main() \
{ \
gl_Position = position; \
coordinate = textureCoordinate.xy; \
}";

/*
static const GLchar *GLOBAL_FRAGMENT_SHADER_Y_U_V = " \
precision highp float; \
uniform sampler2D SamplerY; \
uniform sampler2D SamplerU; \
uniform sampler2D SamplerV; \
\
varying highp vec2 coordinate; \
\
void main() \
{ \
highp vec3 yuv,yuv1; \
highp vec3 rgb; \
\
yuv.x = texture2D(SamplerY, coordinate).r; \
\
yuv.y = texture2D(SamplerU, coordinate).r-0.5; \
\
yuv.z = texture2D(SamplerV, coordinate).r-0.5 ; \
\
rgb = mat3(      1,       1,      1, \
0, -.34414, 1.772, \
1.402, -.71414,      0) * yuv; \
\
gl_FragColor = vec4(rgb, 1); \
}";
 //R = Y + 1.402V
 //G = Y - 0.34414U - 0.71414V
 //B = Y + 1.772U
*/


#if _STANDRAD_CC_
static const GLchar *GLOBAL_FRAGMENT_SHADER_Y_U_V = " \
precision highp float; \
uniform sampler2D SamplerY; \
uniform sampler2D SamplerU; \
uniform sampler2D SamplerV; \
\
varying highp vec2 coordinate; \
\
void main() \
{ \
highp vec3 yuv,yuv1; \
highp vec3 rgb; \
\
yuv.x = texture2D(SamplerY, coordinate).r-0.0627; \
\
yuv.y = texture2D(SamplerU, coordinate).r-0.5; \
\
yuv.z = texture2D(SamplerV, coordinate).r-0.5 ; \
\
rgb = mat3(1.164, 1.164, 1.164, \
0, -0.392, 2.017, \
1.596, -0.813, 0.0) * yuv; \
gl_FragColor = vec4(rgb, 1); \
}";
/*
 0.0627 = 16.0 / 255.0;
 
 // 709
 rgb = mat3(1.164, 1.164, 1.164, \
 0.0, -0.213, 2.112, \
 1.793, -0.533, 0.0) * yuv; \
 gl_FragColor = vec4(rgb, 1); \
 */
/*
 // 601 video range
 rgb = mat3(1.164, 1.164, 1.164, \
 0, -0.392, 2.017, \
 1.596, -0.813, 0.0) * yuv; \
 gl_FragColor = vec4(rgb, 1); \
 */
/*
 // 601 full range
 rgb = mat3(1.0, 1.0, 1.0, \
 0.0, -0.343, 1.765, \
 1.4, -0.711, 0.0) * yuv; \
 gl_FragColor = vec4(rgb, 1); \
 */
#else
static const GLchar *GLOBAL_FRAGMENT_SHADER_Y_U_V = " \
precision highp float; \
uniform sampler2D SamplerY; \
uniform sampler2D SamplerU; \
uniform sampler2D SamplerV; \
\
varying highp vec2 coordinate; \
\
void main() \
{ \
highp vec3 yuv,yuv1; \
highp vec3 rgb; \
\
yuv.x = texture2D(SamplerY, coordinate).r; \
\
yuv.y = texture2D(SamplerU, coordinate).r-0.5; \
\
yuv.z = texture2D(SamplerV, coordinate).r-0.5 ; \
\
rgb = mat3(1.0, 1.0, 1.0, \
0.0, -0.213, 2.112, \
1.71, -0.533,   0.0) * yuv; \
\
gl_FragColor = vec4(rgb, 1); \
}";
#endif
 

// Color Conversion Constants (YUV to RGB) including adjustment from 16-235/16-240 (video range)

// BT.709, which is the standard for HDTV.
//static const GLfloat kColorConversion709[] = {
//    1.164,  1.164, 1.164,
//    0.0, -0.213, 2.112,
//    1.793, -0.533,   0.0,
//};

// BT.601, which is the standard for SDTV.
//static const GLfloat kColorConversion601[] = {
//    1.164,  1.164, 1.164,
//    0.0, -0.392, 2.017,
//    1.596, -0.813,   0.0,
//};

// BT.601 full range (ref: http://www.equasys.de/colorconversion.html)
//const GLfloat kColorConversion601FullRange[] = {
//    1.0,    1.0,    1.0,
//    0.0,    -0.343, 1.765,
//    1.4,    -0.711, 0.0,
//};



//#define VIDEO_BACK_COLOR 	1.0, 0.5, 0.0, 1.0
#define VIDEO_BACK_COLOR 	0.0, 0.0, 0.0, 1.0

COpenGLRnd::COpenGLRnd(CBaseInst * pBaseInst, void * hInst)
:CBaseVideoRnd (pBaseInst, hInst)
,m_pContext(NULL)
,m_nTextureWidth(0)
,m_nTextureHeight(0)
,m_nBackingWidth(0)
,m_nBackingHeight(0)
,m_pFrameData(NULL)
,m_fOutLeftPer(0)
,m_fOutTopPer(0)
,m_fOutRightPer(0)
,m_fOutBottomPer(0)
,m_nPositionSlot(0)
,m_nTexCoordSlot(0)
,m_nColorRenderBuffer(0)
,m_nFrameBuffer(0)
,m_nProgramHandle(0)
,m_nTexturePlanarY(0)
,m_nTexturePlanarU(0)
,m_nTexturePlanarV(0)
,m_nTextureUniformY(0)
,m_nTextureUniformU(0)
,m_nTextureUniformV(0)
,m_bInitializing(false)
,m_nZoomMode(QC_FIXED_RATIO)
,m_nRotation(QC_ROTATION_0)
,m_bStop(true)
,m_pLumaTexture(NULL)
,m_pChromaTexture(NULL)
,m_pVideoTextureCache(NULL)
,m_nTextureUniformUV(0)
,m_pPixelBuffer(NULL)
,m_nDecOutputType(QC_VDT_YUV420_P)
,m_pVieoView(NULL)
,m_nGlInitTime(0)
,m_pParent(nil)
,m_nDisplayCtrl(QC_PLAY_VideoEnable)
{
    QCLOGI("[GL]Render instance is created. %p, %s %s", this, __TIME__,  __DATE__);
    
    //
    SetObjectName ("COpenGLRnd");
    memset (&m_rcGLFrameDrawArea, 0, sizeof (m_rcGLFrameDrawArea));
    
    //
    m_fTextureVertices[0] = 0;
    m_fTextureVertices[1] = 1;
    
    m_fTextureVertices[2] = 1;
    m_fTextureVertices[3] = 1;
    
    m_fTextureVertices[4] = 0;
    m_fTextureVertices[5] = 0;
    
    m_fTextureVertices[6] = 1;
    m_fTextureVertices[7] = 0;
    
    memset(&m_fSquareVertices, 0, sizeof(m_fSquareVertices));
}

void COpenGLRnd::InitOpenGLInternal()
{
    QCLOGI("[GL]GL +InitOpenGLInternal: %p, parent %p, base %p, %d", this, m_pParent, m_pBaseInst, qcGetSysTime()-m_nGlInitTime);
    if(!m_pParent || !m_pBaseInst)
    {
        QCLOGI("[GL]GL -InitOpenGLInternal: %p, parent %p, base %p", this, m_pParent, m_pBaseInst);
        m_bInitializing = false;
        return;
    }
    
    CAutoLock lock (&m_mtDraw);
    
    int nRet = QC_ERR_NONE;
    
    if(!m_pContext)
    {
        m_pContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
        SetContext(m_pContext);
        nRet = SetupTexture();
        if(nRet != QC_ERR_NONE)
        {
            QCLOGE("[GL]-Init GL failed while setup texture. %p, ctx %p, view %p, parent %p", this, m_pContext, m_pVieoView, m_pParent);
            return;
        }
        
        nRet = CompileAllShaders();
        if(nRet != QC_ERR_NONE)
        {
            QCLOGE("[GL]-Init GL failed while compile shader. %p, ctx %p, view %p, parent %p", this, m_pContext, m_pVieoView, m_pParent);
            return;
        }
    }
    
    UpdateOpenGLView(true);
    RefreshView();
    m_bInitializing = false;
    QCLOGI("[GL]GL -InitOpenGLInternal: %p, %d", this, qcGetSysTime()-m_nGlInitTime);
}

COpenGLRnd::~COpenGLRnd(void)
{
    m_pBaseInst = NULL;
#if 1
    int nWaitCount = 0;
    while (m_bInitializing)
    {
        qcSleep(10*1000);
        nWaitCount++;
        if(nWaitCount >= 200)
        {
            QCLOGW("[GL]Force destroy GL, %p", this);
            break;
        }
        QCLOGI("[GL]Wait GL init complete...... %p", this);
    }
    QCLOGI("[GL]GL begin destroy, %p", this);
#endif
    
    CAutoLock lock (&m_mtDraw);
    
    QCLOGI("[GL]+Render instance is destroy. %p, parent %p, base %p", this, m_pParent, m_pBaseInst);
	UninitInternal ();
    
    //glFinish();
    QCLOGI("[GL]-Render instance is destroy. %p, ctx %p, view %p, parent %p, base %p", this, m_pContext, m_pVieoView, m_pParent, m_pBaseInst);
}

int COpenGLRnd::SetViewInternal(void * hView, RECT * rect)
{
    if(m_pBaseInst && m_pBaseInst->m_bForceClose)
        return QC_ERR_NONE;
    if(IsDisableRender())
        return QC_ERR_NONE;

    CAutoLock lock (&m_mtDraw);
    QCLOGI("[GL]Set view %p", this);
    RECT r = {0, 0, 0, 0};
    RECT* pRect = rect;
    // if user not set draw rect, default is view's size
    if( (!pRect || ((pRect->right-pRect->left)<=0 || (pRect->bottom-pRect->top)<=0)) && hView && [NSRunLoop mainRunLoop] == [NSRunLoop currentRunLoop])
    {
        UIView* tmp = (__bridge UIView*)hView;
        r = {(int)tmp.bounds.origin.x, (int)tmp.bounds.origin.y, (int)tmp.bounds.size.width, (int)tmp.bounds.size.height};
        pRect = &r;
        QCLOGI("[GL]Update view size, (%d,%d,%d,%d), parent %p, %p", r.left, r.top, r.right, r.bottom, tmp, this);
    }
    
    bool bDrawRectChanged = IsDrawRectChanged(pRect);
    bool bZoomModeChanged = false;
    if([NSRunLoop mainRunLoop] == [NSRunLoop currentRunLoop])
        bZoomModeChanged = IsZoomModeChanged((__bridge UIView*)hView);
    
    if (pRect != NULL && (pRect->right-pRect->left)>0 && (pRect->bottom-pRect->top)>0)
        memcpy (&m_rcView, pRect, sizeof (RECT));
//    else
//    {
//        // if user not set draw rect, default is view's size
//        if(hView && [NSRunLoop mainRunLoop] == [NSRunLoop currentRunLoop])
//        {
//            UIView* view = (__bridge UIView*)hView;
//            m_rcView = {(int)view.bounds.origin.x, (int)view.bounds.origin.y, (int)view.bounds.size.width, (int)view.bounds.size.height};
//            QCLOGI("[GL]Update view size, (%d,%d,%d,%d), parent %p, %p", m_rcView.left, m_rcView.top, m_rcView.right, m_rcView.bottom, view, this);
//        }
//    }
    
    if(!m_pParent && hView)
        m_pParent = (__bridge UIView*)hView;
    else if(m_pParent != hView && hView)
    {
        UninitInternal ();
        m_pParent = (__bridge UIView*)hView;
    }
    
    if(pRect && m_pVieoView && bDrawRectChanged)
    {
        if([NSRunLoop mainRunLoop] == [NSRunLoop currentRunLoop])
        	m_pVieoView.frame = CGRectMake(m_rcView.left, m_rcView.top, m_rcView.right-m_rcView.left, m_rcView.bottom-m_rcView.top);
    }
    
    if(hView && bZoomModeChanged)
    {
        if([NSRunLoop mainRunLoop] == [NSRunLoop currentRunLoop])
        	m_nZoomMode = ViewContentMode2ZoomMode(m_pParent.contentMode);
    }
    
    if(bDrawRectChanged || bZoomModeChanged)
    {
        UpdateRenderSize();
        if(IsGLRenderReady())
        {
            RefreshView();
        }
    }

	return QC_ERR_NONE;
}

int COpenGLRnd::SetView (void* hView, RECT * pRect)
{
    if([NSRunLoop mainRunLoop] != [NSRunLoop currentRunLoop])
        QCLOGW("[GL]Call SetView from a non-main thread!");
#if 0
    if([NSRunLoop mainRunLoop] == [NSRunLoop currentRunLoop])
    {
        SetViewInternal(hView, pRect);
    }
    else
    {
        QCLOGW("[GL]GL set view on sub-thread, dangerous %p", this);
        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            SetViewInternal(hView, pRect);
        }];
    }
#else
    SetViewInternal(hView, pRect);
#endif
    
    return QC_ERR_NONE;
}

bool COpenGLRnd::IsZoomModeChanged(UIView* hView)
{
    if(!hView)
        return false;
    
    int nNewMode = ViewContentMode2ZoomMode(((UIView*)hView).contentMode);
    return nNewMode != m_nZoomMode;
}

bool COpenGLRnd::IsDrawRectChanged(RECT* pNewRect)
{
	if(pNewRect == NULL || (pNewRect->right-pNewRect->left)<=0 || (pNewRect->bottom-pNewRect->top)<=0)
        return false;
    
    if(pNewRect->left!=m_rcView.left || pNewRect->top!=m_rcView.top
       || pNewRect->right!=m_rcView.right || pNewRect->bottom!=m_rcView.bottom)
        return true;
    
    return false;
}


int COpenGLRnd::Start (void)
{
    m_bStop = false;
    return CBaseVideoRnd::Start();
}

int COpenGLRnd::Stop (void)
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

int COpenGLRnd::InitGLOnMainThread()
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

        //dispatch_async(dispatch_get_main_queue(), ^{
          //  InitOpenGLInternal();
        //});
    }

    return QC_ERR_NONE;
}

int COpenGLRnd::Init (QC_VIDEO_FORMAT * pFmt)
{
    if([NSRunLoop mainRunLoop] != [NSRunLoop currentRunLoop])
        QCLOGW("[GL]Call Init from a non-main thread!");

    if(!IsFormatChanged((QC_VIDEO_FORMAT *)pFmt) && IsGLRenderReady())
        return QC_ERR_NONE;
    else if(IsFormatChanged((QC_VIDEO_FORMAT *)pFmt) && IsGLRenderReady())
    {
        CBaseVideoRnd::Init(pFmt);
        UpdateRenderSize();
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

int COpenGLRnd::InitInternal (QC_VIDEO_FORMAT * pFmt)
{
    m_nGlInitTime		= qcGetSysTime();
    m_nTextureWidth     = pFmt->nWidth;
    m_nTextureHeight    = pFmt->nHeight;
    
    InitGLOnMainThread();
    return QC_ERR_NONE;
}

void COpenGLRnd::UninitInternal()
{
    CAutoLock lock (&m_mtDraw);
    
    if(m_pContext)
        SetContext(m_pContext);
    
    if (m_pLumaTexture)
    {
        CFRelease(m_pLumaTexture);
        m_pLumaTexture = NULL;
    }
    
    if (m_pChromaTexture)
    {
        CFRelease(m_pChromaTexture);
        m_pChromaTexture = NULL;
    }
    
    if (m_pVideoTextureCache)
    {
        CFRelease(m_pVideoTextureCache);
        m_pVideoTextureCache = 0;
    }
    
    if (NULL != m_pPixelBuffer)
    {
        CVPixelBufferRelease(m_pPixelBuffer);
        m_pPixelBuffer = NULL;
    }
    
    DeleteRenderBuffer();
    DeleteFrameBuffer();
    DeleteTexture();
    
    if (m_nProgramHandle)
    {
        glDeleteProgram(m_nProgramHandle);
        m_nProgramHandle = 0;
    }
    
    DeleteFrameData();
    
    //UpdateOpenGLView(false);
    
    //
    if(m_pContext)
    {
        [EAGLContext setCurrentContext: nil];
        [m_pContext release];
        m_pContext = nil;
    }
    
    UpdateOpenGLView(false);
    m_pParent = nil;
}

int COpenGLRnd::Uninit (void)
{
    return QC_ERR_NONE;
}

int COpenGLRnd::Prepare (QC_VIDEO_BUFF* pVideoBuffer)
{
    if(QC_VDT_YUV420_P == pVideoBuffer->nType)
    {
        if(m_nRotation != QC_ROTATION_0)
        {
        	m_nRotation = QC_ROTATION_0;
            UpdateVertices();
        }
        return QC_ERR_NONE;
    }
    else if (QC_VDT_NV12 == pVideoBuffer->nType)
    {
        if (m_pPixelBuffer)
            CVPixelBufferRelease(m_pPixelBuffer);
        
        m_pPixelBuffer = (CVImageBufferRef)(pVideoBuffer->pBuff[0]);
        // it's more safe for decoder retain it while GetOutput
        //CVPixelBufferRetain(m_pPixelBuffer);
        
        if(UpdateSelfRotate())
        {
            UpdateVertices();
        }
        
//        CFTypeRef colorAttachments = CVBufferGetAttachment(m_pPixelBuffer, kCVImageBufferYCbCrMatrixKey, NULL);
//        if (colorAttachments != NULL)
//        {
//            if(CFStringCompare((CFStringRef)colorAttachments, kCVImageBufferYCbCrMatrix_ITU_R_601_4, 0) == kCFCompareEqualTo)
//            {
//            }
//            else
//            {
//            }
//        }
        return QC_ERR_NONE;
    }
    
    return QC_ERR_NONE;
}


int COpenGLRnd::Render (QC_DATA_BUFF * pBuff)
{
    if(IsDisableRender())
       return QC_ERR_STATUS;
       
    // Process BA, the first frame will not be new format flag for Local and RTMP, but HLS does
    if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT && IsFormatChanged((QC_VIDEO_FORMAT *)pBuff->pFormat))
    {
        QC_VIDEO_FORMAT* pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
        QCLOGI("[GL]Video format changed, width %d, height %d", pFmt->nWidth, pFmt->nHeight);
        CBaseVideoRnd::Init(pFmt);
        UpdateRenderSize();
        // process BA
        SetTexture(pFmt->nWidth, pFmt->nHeight);
        //m_nRndCount++;
        //return QC_ERR_NONE;
    }

    if (!IsGLRenderReady())
    {
        QCLOGW("[GL]GL not ready, %p, %d, %d, %d, %p", this, m_bInitializing,
               m_nColorRenderBuffer, m_nFrameBuffer, m_pVideoTextureCache);
        InitInternal(&m_fmtVideo);
        m_nRndCount++;
        return QC_ERR_RETRY;
    }
    
    CBaseVideoRnd::Render (pBuff);
    
    CAutoLock lock (&m_mtDraw);
    
    if(IsDisableRender())
        return QC_ERR_STATUS;
    
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
    
    if (!SetContext(m_pContext))
        return QC_ERR_FAILED;
    
#if 0
    CFAbsoluteTime fTime = CFAbsoluteTimeGetCurrent() * 1000;
#endif
    
    UpdateDecOutputType(pVideoBuffer->nType);
    
    nRet = UploadTexture(pVideoBuffer);
    
    if (QC_ERR_NONE != nRet)
        return nRet;
    
    if(QC_ERR_NONE != RenderToScreen())
        return QC_ERR_STATUS;
    if(QC_ERR_NONE != RenderCommit())
        return QC_ERR_STATUS;
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

bool COpenGLRnd::UpdateRenderSize (void)
{
	bool bRC = CBaseVideoRnd::UpdateRenderSize ();

	return bRC;
}

int COpenGLRnd::SetupTexture()
{
    if(!m_pBaseInst)
        return QC_ERR_STATUS;

    if(m_nDecOutputType == QC_VDT_YUV420_P)
    {
        DeleteTexture();
        
        glGenTextures(1, &m_nTexturePlanarY);
        glGenTextures(1, &m_nTexturePlanarU);
        glGenTextures(1, &m_nTexturePlanarV);
        
        if ((0 == m_nTexturePlanarY) || (0 == m_nTexturePlanarU) || (0 == m_nTexturePlanarV))
        {
            QCLOGE("SetupTexture fail, %d, %d, %d", m_nTexturePlanarY, m_nTexturePlanarU, m_nTexturePlanarV);
            return QC_ERR_FAILED;
        }
    }
    else if(m_nDecOutputType == QC_VDT_NV12)
    {
    }

    return QC_ERR_NONE;
}


int COpenGLRnd::CompileAllShaders()
{
    if (m_nProgramHandle)
    {
        glDeleteProgram(m_nProgramHandle);
        m_nProgramHandle = 0;
    }
    
    // Setup program
    m_nProgramHandle = glCreateProgram();
    if (0 == m_nProgramHandle)
    {
        QCLOGE("[GL]glCreateProgram fail, %d", m_nProgramHandle);
        return QC_ERR_FAILED;
    }

    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
    
    if(m_nDecOutputType == QC_VDT_YUV420_P)
    {
        vertexShader = CompileShader(GLOBAL_VERTEX_SHADER_Y_U_V, GL_VERTEX_SHADER);
        fragmentShader = CompileShader(GLOBAL_FRAGMENT_SHADER_Y_U_V, GL_FRAGMENT_SHADER);
    }
    else if(m_nDecOutputType == QC_VDT_NV12)
    {
        //int index = m_bIsSphericalVideo? 1 : 0;
        vertexShader = CompileShader(GLOBAL_VERTEX_SHADER_Y_UV[0], GL_VERTEX_SHADER);
        fragmentShader = CompileShader(GLOBAL_FRAGMENT_SHADER_Y_UV, GL_FRAGMENT_SHADER);
    }
    
    if (0 == vertexShader || 0 == fragmentShader)
    {
        QCLOGI("[GL]glCreateShader fail, vertexShader:%d, fragmentShader:%d, %d", vertexShader, fragmentShader, m_nDecOutputType);
        return QC_ERR_FAILED;
    }
    
    glAttachShader(m_nProgramHandle, vertexShader);
    glAttachShader(m_nProgramHandle, fragmentShader);
    glLinkProgram(m_nProgramHandle);
    
    // Link program
    GLint linkSuccess;
    glGetProgramiv(m_nProgramHandle, GL_LINK_STATUS, &linkSuccess);
    
    if (linkSuccess == GL_FALSE)
    {
        if (vertexShader)
        {
            glDetachShader(m_nProgramHandle, vertexShader);
            glDeleteShader(vertexShader);
        }
        if (fragmentShader) {
            glDetachShader(m_nProgramHandle, fragmentShader);
            glDeleteShader(fragmentShader);
        }
        
        GLchar messages[256];
        glGetProgramInfoLog(m_nProgramHandle, sizeof(messages), 0, &messages[0]);
        NSString *messageString = [NSString stringWithUTF8String:messages];
        QCLOGE("%s", [messageString UTF8String]);
        
        return QC_ERR_FAILED;
    }
    
    // Use Program
    glUseProgram(m_nProgramHandle);
    
    // Get Attrib
    m_nPositionSlot = glGetAttribLocation(m_nProgramHandle, "position");
    glEnableVertexAttribArray(m_nPositionSlot);
    
    if(m_nDecOutputType == QC_VDT_YUV420_P)
    {
        m_nTexCoordSlot = glGetAttribLocation(m_nProgramHandle, "textureCoordinate");
        glEnableVertexAttribArray(m_nTexCoordSlot);
        
        m_nTextureUniformY = glGetUniformLocation(m_nProgramHandle, "SamplerY");
        m_nTextureUniformU = glGetUniformLocation(m_nProgramHandle, "SamplerU");
        m_nTextureUniformV = glGetUniformLocation(m_nProgramHandle, "SamplerV");
        
        // Wrong init
        if ((m_nTextureUniformY == m_nTextureUniformU) || (m_nTextureUniformU == m_nTextureUniformV) || (m_nTextureUniformY == m_nTextureUniformV))
        {
            QCLOGE("Error Y:%d, U:%d, V:%d", m_nTextureUniformY, m_nTextureUniformU, m_nTextureUniformV);
            return QC_ERR_FAILED;
        }
    }
    else if(m_nDecOutputType == QC_VDT_NV12)
    {
        m_nTexCoordSlot = glGetAttribLocation(m_nProgramHandle, "texCoord");
        glEnableVertexAttribArray(m_nTexCoordSlot);
        
        m_nTextureUniformY = glGetUniformLocation(m_nProgramHandle, "SamplerY");
        m_nTextureUniformUV = glGetUniformLocation(m_nProgramHandle, "SamplerUV");
        
        if(m_nTextureUniformY == m_nTextureUniformUV)
        {
            QCLOGE("Error Y:%d, UV:%d", m_nTextureUniformY, m_nTextureUniformUV);
            return QC_ERR_FAILED;
        }
    }
    
    // Release vertex and fragment shaders.
    if (vertexShader)
    {
        glDetachShader(m_nProgramHandle, vertexShader);
        glDeleteShader(vertexShader);
    }
    
    if (fragmentShader)
    {
        glDetachShader(m_nProgramHandle, fragmentShader);
        glDeleteShader(fragmentShader);
    }
    
    QCLOGI("[GL]m_nPositionSlot:%d, m_nTexCoordSlot:%d, Y:%d, U:%d, V:%d", m_nPositionSlot, m_nTexCoordSlot, m_nTextureUniformY, m_nTextureUniformU, m_nTextureUniformV);
    
    return QC_ERR_NONE;
}

int COpenGLRnd::SetRotation(QC_ROTATIONMODE eType)
{
    CAutoLock lock(&m_mtDraw);
    
    if (NULL == m_pContext || !SetContext(m_pContext))
    {
        return QC_ERR_FAILED;
    }
    
    if (m_nRotation != eType)
    {
        m_nRotation = eType;
        UpdateVertices();
    }
    
    return QC_ERR_NONE;
}

int COpenGLRnd::SetTexture(int nWidth, int nHeight)
{
    CAutoLock lock(&m_mtDraw);
    
    if (NULL == m_pContext || !SetContext(m_pContext) || !m_pBaseInst)
    {
        return QC_ERR_FAILED;
    }
    
    //if ((nWidth != m_nTextureWidth) || (nHeight != m_nTextureHeight))
    {
        m_nTextureWidth = nWidth;
        m_nTextureHeight = nHeight;
        OnTextureSizeChange();
        UpdateVertices();
    }
    
    return QC_ERR_NONE;
}

int COpenGLRnd::OnTextureSizeChange()
{
    DeleteFrameData();
    return QC_ERR_NONE;
}

int COpenGLRnd::UpdateDecOutputType(int nCurrType)
{
    if(nCurrType != m_nDecOutputType)
    {
        m_nDecOutputType = nCurrType;
        SetupTexture();
        CompileAllShaders();
    }

    return QC_ERR_NONE;
}

int COpenGLRnd::SetOutputRect(int nLeft, int nTop, int nRight, int nBottom)
{
    CAutoLock lock(&m_mtDraw);
    
    if (NULL == m_pContext || !SetContext(m_pContext))
    {
        return QC_ERR_FAILED;
    }
    
    m_fOutLeftPer = (float)nLeft / m_nTextureWidth;
    m_fOutTopPer = (float)nTop / m_nTextureHeight;
    m_fOutRightPer = (float)nRight / m_nTextureWidth;
    m_fOutBottomPer = (float)nBottom / m_nTextureHeight;
    
    UpdateVertices();
    
    return QC_ERR_NONE;
}

int COpenGLRnd::ClearGL()
{
    if (NULL == m_pContext || !SetContext(m_pContext))
        return QC_ERR_FAILED;
    
    DrawBackgroundColor();
    
    /* Clear the renderbuffer */
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    
    /* Refresh the screen */
    [m_pContext presentRenderbuffer: GL_RENDERBUFFER];
    
    return QC_ERR_NONE;
}


int COpenGLRnd::RefreshView()
{
    //QCLOGI("[GL]Setup OpenGL, %p", m_pVieoView);
    
    if (!m_pVieoView || !m_pBaseInst)
        return QC_ERR_FAILED;
    
    if (NULL == m_pContext || !SetContext(m_pContext))
    {
        return QC_ERR_FAILED;
    }
    
    CAEAGLLayer* _eaglLayer = (CAEAGLLayer *)(((UIView*)m_pVieoView).layer);
    _eaglLayer.opaque = YES;
    
    SetupRenderBuffer();
    SetupFrameBuffer();
    SetupGlViewport();
    
    UpdateVertices();
    
    return QC_ERR_NONE;
}

int COpenGLRnd::SetupGlViewport()
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

int COpenGLRnd::SetupRenderBuffer()
{
    if(!m_pBaseInst)
        return QC_ERR_STATUS;

    DeleteRenderBuffer();
    glGenRenderbuffers(1, &m_nColorRenderBuffer);
    
    if (0 == m_nColorRenderBuffer)
    {
        return QC_ERR_FAILED;
    }
    
    glBindRenderbuffer(GL_RENDERBUFFER, m_nColorRenderBuffer);
    
    if([NSRunLoop mainRunLoop] != [NSRunLoop currentRunLoop])
    {
        QCLOGW("[GL]Call [EAGLContext renderbufferStorage] from a non-main thread!");
    }
    else
    {
        QCLOGI("[GL]Call [EAGLContext renderbufferStorage] from main thread!");
    }
    
    [m_pContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)(((UIView*)m_pVieoView).layer)];
    
    return QC_ERR_NONE;
}

int COpenGLRnd::SetupFrameBuffer()
{
    if(!m_pBaseInst)
        return QC_ERR_STATUS;

    DeleteFrameBuffer();
    
    //
    CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, m_pContext, NULL, &m_pVideoTextureCache);
    
    if (err)
    {
        QCLOGE("[GL]CVOpenGLESTextureCacheCreate %d", err);
        return QC_ERR_FAILED;
    }
    
    //
    glGenFramebuffers(1, &m_nFrameBuffer);
    
    if (0 == m_nFrameBuffer)
    {
        QCLOGE("[GL]glGenFramebuffers fail");
        return QC_ERR_FAILED;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, m_nFrameBuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, m_nColorRenderBuffer);
    
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        QCLOGE("[GL]glCheckFramebufferStatus fail with framebuffer generation");
        return QC_ERR_FAILED;
    }
    
    return QC_ERR_NONE;
}

int COpenGLRnd::DeleteFrameData()
{
    if (NULL != m_pFrameData)
    {
        delete []m_pFrameData;
        m_pFrameData = NULL;
    }
    
    if (NULL != m_pPixelBuffer)
    {
        CVPixelBufferRelease(m_pPixelBuffer);
        m_pPixelBuffer = NULL;
    }
    
    return QC_ERR_NONE;
}

int COpenGLRnd::DeleteRenderBuffer()
{
    if (m_nColorRenderBuffer)
    {
        glDeleteRenderbuffers(1, &m_nColorRenderBuffer);
        m_nColorRenderBuffer = 0;
    }
    return QC_ERR_NONE;
}

int COpenGLRnd::DeleteFrameBuffer()
{
    if (m_pVideoTextureCache)
    {
        CFRelease(m_pVideoTextureCache);
        m_pVideoTextureCache = 0;
    }

    if (m_nFrameBuffer)
    {
        glDeleteFramebuffers(1, &m_nFrameBuffer);
        m_nFrameBuffer = 0;
    }
    
    return QC_ERR_NONE;
}

int COpenGLRnd::DeleteTexture()
{
    if (m_nTexturePlanarY)
    {
        glDeleteTextures(1, &m_nTexturePlanarY);
        m_nTexturePlanarY = 0;
    }
    
    if (m_nTexturePlanarU)
    {
        glDeleteTextures(1, &m_nTexturePlanarU);
        m_nTexturePlanarU = 0;
    }
    
    if (m_nTexturePlanarV)
    {
        glDeleteTextures(1, &m_nTexturePlanarV);
        m_nTexturePlanarV = 0;
    }
    
    return QC_ERR_NONE;
}

GLuint COpenGLRnd::CompileShader(const GLchar *pBuffer, GLenum shaderType)
{
    NSString* shaderString = [NSString stringWithFormat:@"%s", pBuffer];
    
    //
    GLuint shaderHandle = glCreateShader(shaderType);
    
    //
    const char* shaderStringUTF8 = [shaderString UTF8String];
    int shaderStringLength = (int)[shaderString length];
    glShaderSource(shaderHandle, 1, &shaderStringUTF8, &shaderStringLength);
    
    //
    glCompileShader(shaderHandle);
    
    //
    GLint compileSuccess;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);
    if (compileSuccess == GL_FALSE)
    {

        GLchar messages[256];
        glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]);
        NSString *messageString = [NSString stringWithUTF8String:messages];
        QCLOGE("[GL]%s", [messageString UTF8String]);
        return -1;
    }
    
    return shaderHandle;
}

int COpenGLRnd::GLTexImage2D(GLuint nTexture, unsigned char* pData, int nWidth, int nHeight)
{
    glBindTexture(GL_TEXTURE_2D, nTexture);
    
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, nWidth, nHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pData);
    
    return QC_ERR_NONE;
}

int COpenGLRnd::UploadTexture(QC_VIDEO_BUFF* pVideoBuffer)
{
    //QCLOGI("[GL]Texture width %d, height %d", m_nTextureWidth, m_nTextureHeight);
    //    QCLOGI("YUV comes, time %lld, %p(%d), %p(%d), %p(%d)", pBuff->llTime, pVideoBuffer->pBuff[0], pVideoBuffer->nStride[0],
    //                                    pVideoBuffer->pBuff[1], pVideoBuffer->nStride[1], pVideoBuffer->pBuff[2], pVideoBuffer->nStride[2]);
    
    if(pVideoBuffer->nType == QC_VDT_YUV420_P)
    {
        if (NULL == m_pFrameData)
        {
            m_pFrameData = new unsigned char[m_nTextureWidth * m_nTextureHeight * 3 / 2];
        }
        
        if (NULL == m_pFrameData)
        {
            return QC_ERR_FAILED;
        }
        
        int i = 0;
        int nWidthUV = m_nTextureWidth / 2;
        int nHeightUV = m_nTextureHeight / 2;
        
        for (i = 0; i < m_nTextureHeight; i++)
            memcpy (m_pFrameData + m_nTextureWidth * i, pVideoBuffer->pBuff[0] + pVideoBuffer->nStride[0] * i, m_nTextureWidth);
        
        unsigned char* pBuffer = m_pFrameData + (m_nTextureWidth * m_nTextureHeight);
        for (i = 0; i < nHeightUV; i++)
            memcpy (pBuffer + (nWidthUV * i), pVideoBuffer->pBuff[1] + pVideoBuffer->nStride[1] * i, nWidthUV);
        
        pBuffer = pBuffer + (nWidthUV * nHeightUV);
        for (i = 0; i < nHeightUV; i++)
            memcpy (pBuffer + (nWidthUV * i), pVideoBuffer->pBuff[2] + pVideoBuffer->nStride[2] * i, nWidthUV);
        
        if(IsDisableRender())
            return QC_ERR_STATUS;
        
        GLTexImage2D(m_nTexturePlanarY, m_pFrameData, m_nTextureWidth, m_nTextureHeight);
        GLTexImage2D(m_nTexturePlanarU, pBuffer - (nWidthUV * nHeightUV), nWidthUV, nHeightUV);
        GLTexImage2D(m_nTexturePlanarV, pBuffer, nWidthUV, nHeightUV);
        
#if 0
        static FILE* hFile = NULL;
        
        if (NULL == hFile)
        {
            char szTmp[256];
            qcGetAppPath(NULL, szTmp, 256);
            strcat(szTmp, "dump.yuv420");
            hFile = fopen(szTmp, "wb");
        }
        
        if (NULL != hFile)
        {
            fwrite(m_pFrameData, 1, m_nTextureWidth * m_nTextureHeight * 3 / 2, hFile);
        }
#endif
    }
    else if(pVideoBuffer->nType == QC_VDT_NV12)
    {
        if (NULL == m_pPixelBuffer)
            return QC_ERR_FAILED;
        
        if (m_pLumaTexture)
        {
            CFRelease(m_pLumaTexture);
            m_pLumaTexture = NULL;
        }
        
        if (m_pChromaTexture)
        {
            CFRelease(m_pChromaTexture);
            m_pChromaTexture = NULL;
        }
        
        if(IsDisableRender())
            return QC_ERR_STATUS;

        // Periodic texture cache flush every frame
        CVOpenGLESTextureCacheFlush(m_pVideoTextureCache, 0);
        
        // Create a CVOpenGLESTexture from the CVImageBuffer
        size_t frameWidth = CVPixelBufferGetWidth(m_pPixelBuffer);
        size_t frameHeight = CVPixelBufferGetHeight(m_pPixelBuffer);
        
        CVReturn err = 0;
        glActiveTexture(GL_TEXTURE0);
        err = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                           m_pVideoTextureCache,
                                                           m_pPixelBuffer,
                                                           NULL,
                                                           GL_TEXTURE_2D,
                                                           GL_RED_EXT,
                                                           frameWidth,
                                                           frameHeight,
                                                           GL_RED_EXT,
                                                           GL_UNSIGNED_BYTE,
                                                           0,
                                                           &m_pLumaTexture);
        if (err)
        {
            QCLOGE("Error at CVOpenGLESTextureCacheCreateTextureFromImage %d", err);
        }
        
        glBindTexture(CVOpenGLESTextureGetTarget(m_pLumaTexture), CVOpenGLESTextureGetName(m_pLumaTexture));
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // UV-plane
        glActiveTexture(GL_TEXTURE1);
        err = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                           m_pVideoTextureCache,
                                                           m_pPixelBuffer,
                                                           NULL,
                                                           GL_TEXTURE_2D,
                                                           GL_RG_EXT,
                                                           frameWidth/2,
                                                           frameHeight/2,
                                                           GL_RG_EXT,
                                                           GL_UNSIGNED_BYTE,
                                                           1,
                                                           &m_pChromaTexture);
        if (err)
        {
            QCLOGE("Error at CVOpenGLESTextureCacheCreateTextureFromImage %d", err);
        }
        
        glBindTexture(CVOpenGLESTextureGetTarget(m_pChromaTexture), CVOpenGLESTextureGetName(m_pChromaTexture));
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    return QC_ERR_NONE;
}


int COpenGLRnd::RenderToScreen()
{
    if(IsDisableRender())
        return QC_ERR_STATUS;

    if(m_nDecOutputType == QC_VDT_YUV420_P)
    {
        DrawBackgroundColor();
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_nTexturePlanarY);
        glUniform1i(m_nTextureUniformY, 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_nTexturePlanarU);
        glUniform1i(m_nTextureUniformU, 1);
        
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_nTexturePlanarV);
        glUniform1i(m_nTextureUniformV, 2);
        
        // Update attribute values.
        glVertexAttribPointer(m_nPositionSlot, 2, GL_FLOAT, 0, 0, m_fSquareVertices);
        glVertexAttribPointer(m_nTexCoordSlot, 2, GL_FLOAT, 0, 0, m_fTextureVertices);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    else if(m_nDecOutputType == QC_VDT_NV12)
    {
        DrawBackgroundColor();
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUniform1i(m_nTextureUniformY, 0);
        glUniform1i(m_nTextureUniformUV, 1);
        
        // Update attribute values.
        glVertexAttribPointer(m_nPositionSlot, 2, GL_FLOAT, 0, 0, m_fSquareVertices);
        glVertexAttribPointer(m_nTexCoordSlot, 2, GL_FLOAT, 0, 0, m_fTextureVertices);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    return QC_ERR_NONE;
}

int COpenGLRnd::RenderCommit()
{
#if 0
    dispatch_sync(openGLESSerialQueue, ^{
    });
#else
    if(IsDisableRender())
        return QC_ERR_STATUS;
    [m_pContext presentRenderbuffer:GL_RENDERBUFFER];
#endif
    
    return QC_ERR_NONE;
}

void COpenGLRnd::UpdateVertices()
{
    if(IsDisableRender())
        return;

    QCLOGI("[GL]UpdateVertices %d %d", m_nTextureWidth, m_nTextureHeight);
    
    if (0 == m_nBackingWidth || 0 == m_nBackingHeight || 0 == m_nTextureWidth || 0 == m_nTextureHeight || !m_pBaseInst)
    {
        QCLOGE("[GL]Status error!!!");
        return;
    }
    
    //    m_rcGLFrameDrawArea.size.width = m_nBackingWidth;
    //    m_rcGLFrameDrawArea.size.height = m_nBackingHeight;
    
    if (QC_FILLWINDOW == m_nZoomMode)
    {
        // full screen
        m_rcGLFrameDrawArea.size.width = m_nBackingWidth;
        m_rcGLFrameDrawArea.size.height = m_nBackingHeight;
    }
    else if (QC_ORIGINAL == m_nZoomMode)
    {
        // ORIGINAL means just draw by video's original size, just like 176*144
        m_rcGLFrameDrawArea.size.width = m_nTextureWidth;
        m_rcGLFrameDrawArea.size.height = m_nTextureHeight;
    }
    else if (QC_ZOOMIN == m_nZoomMode)
    {
        if ((m_fOutLeftPer >= m_fOutRightPer) || (m_fOutTopPer >= m_fOutBottomPer))
        {
            QCLOGE("[GL]Setting is invalid.");
            return;
        }
        
        m_rcGLFrameDrawArea.size.width = m_nBackingWidth / (m_fOutRightPer - m_fOutLeftPer);
        m_rcGLFrameDrawArea.size.height = m_nBackingHeight / (m_fOutBottomPer - m_fOutTopPer);
        m_rcGLFrameDrawArea.origin.x = -m_fOutLeftPer * m_rcGLFrameDrawArea.size.width;
        m_rcGLFrameDrawArea.origin.y = -m_fOutTopPer * m_rcGLFrameDrawArea.size.height;
    }
    else
    {
        unsigned int    nW = 0;
        unsigned int    nH = 0;

        //按宽高比例(计算nNum和nDen)，不做裁剪.
        if(QC_FIXED_RATIO == m_nZoomMode)
        {
            int nRndRectWidth = (m_rcRender.right - m_rcRender.left) * [[UIScreen mainScreen] scale];
            int nRndRectHeight = (m_rcRender.bottom - m_rcRender.top) * [[UIScreen mainScreen] scale];
            
            if(nRndRectWidth != 0 && nRndRectHeight != 0)
            {
                m_rcGLFrameDrawArea.size.width = nRndRectWidth;
                m_rcGLFrameDrawArea.size.height = nRndRectHeight;
            }
            else
            {
                m_rcGLFrameDrawArea.size.width = m_nTextureWidth;
                m_rcGLFrameDrawArea.size.height = m_nTextureHeight;
            }
            
            nW = m_rcGLFrameDrawArea.size.width;
            nH = m_rcGLFrameDrawArea.size.height;
        }
        else //不按宽高比例渲染的话，就以全部view的区域的话
        {
            m_rcGLFrameDrawArea.size.width = m_nBackingWidth;
            m_rcGLFrameDrawArea.size.height = m_nBackingHeight;
            //仍然以原始视频宽高比例(不计算nNum和nDen)
            nW = m_nTextureWidth;
            nH = m_nTextureHeight;
        }
        
//        if(m_nRotation == QC_ROTATION_90 || m_nRotation == QC_ROTATION_270)
//        {
//            int nTmp = nW;
//            nW = nH;
//            nH = nTmp;
//        }
        
        if (m_nZoomMode == QC_PANSCAN)
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
    } // End of FITWINDOW

    if (QC_ZOOMIN != m_nZoomMode)
    {
        m_rcGLFrameDrawArea.origin.x = (m_nBackingWidth - m_rcGLFrameDrawArea.size.width) / 2;
        m_rcGLFrameDrawArea.origin.y = (m_nBackingHeight - m_rcGLFrameDrawArea.size.height) / 2;
    }
    
    QCLOGI("[GL]Draw rect: width %f, height %f, left %f, top %f, rotatation %d", m_rcGLFrameDrawArea.size.width, m_rcGLFrameDrawArea.size.height, m_rcGLFrameDrawArea.origin.x, m_rcGLFrameDrawArea.origin.y, (int)m_nRotation);
    
    GLfloat nLengthX = (GLfloat)(2 * m_rcGLFrameDrawArea.size.width) / m_nBackingWidth;
    GLfloat nLengthY = (GLfloat)(2 * m_rcGLFrameDrawArea.size.height) / m_nBackingHeight;
    GLfloat nXLeft = -1 + (GLfloat)(2 * m_rcGLFrameDrawArea.origin.x) / m_nBackingWidth; // from left
    GLfloat nYTop = 1 - ((GLfloat)(2 * m_rcGLFrameDrawArea.origin.y) / m_nBackingHeight); // from top
    
    switch(m_nRotation)
    {
        case QC_ROTATION_0:  // 不旋转
            //2 | 3
            //-   -
            //0 | 1
            // the no.1 point
            m_fSquareVertices[0] = nXLeft;            // left X
            m_fSquareVertices[1] = nYTop - nLengthY;  // bottom Y
            
            // the no.2 point
            m_fSquareVertices[2] = nXLeft + nLengthX; // right X
            m_fSquareVertices[3] = nYTop - nLengthY;  // bottom Y
            
            // the no.3 point
            m_fSquareVertices[4] = nXLeft;            // left X
            m_fSquareVertices[5] = nYTop;             // top Y
            
            // the no.4 point
            m_fSquareVertices[6] = nXLeft + nLengthX; // right X
            m_fSquareVertices[7] = nYTop;             // top Y
            break;
        case QC_ROTATION_0FLIP: // 左右翻转
            //3 | 2
            //-   -
            //1 | 0
            m_fSquareVertices[0] = nXLeft + nLengthX;
            m_fSquareVertices[1] = nYTop - nLengthY;
            
            m_fSquareVertices[2] = nXLeft;
            m_fSquareVertices[3] = nYTop - nLengthY;
            
            m_fSquareVertices[4] = nXLeft + nLengthX;
            m_fSquareVertices[5] = nYTop;
            
            m_fSquareVertices[6] = nXLeft;
            m_fSquareVertices[7] = nYTop;
            break;
        case QC_ROTATION_90:
            //0 | 2
            //-   -
            //1 | 3
            m_fSquareVertices[0] = nXLeft;
            m_fSquareVertices[1] = nYTop;
            
            m_fSquareVertices[2] = nXLeft;
            m_fSquareVertices[3] = nYTop - nLengthY;
            
            m_fSquareVertices[4] = nXLeft + nLengthX;
            m_fSquareVertices[5] = nYTop;
            
            m_fSquareVertices[6] = nXLeft + nLengthX;
            m_fSquareVertices[7] = nYTop - nLengthY;
            break;
//        case QC_ROTATION_90FLIP:
//            //2 | 0
//            //-   -
//            //3 | 1
//            m_fSquareVertices[0] = nXLeft + nLengthX;
//            m_fSquareVertices[1] = nYTop;
//            
//            m_fSquareVertices[2] = nXLeft + nLengthX;
//            m_fSquareVertices[3] = nYTop - nLengthY;
//            
//            m_fSquareVertices[4] = nXLeft;
//            m_fSquareVertices[5] = nYTop;
//            
//            m_fSquareVertices[6] = nXLeft;
//            m_fSquareVertices[7] = nYTop - nLengthY;
//            break;
        case QC_ROTATION_180: // 顺时针旋转180度
            //1 | 0
            //-   -
            //3 | 2
            m_fSquareVertices[0] = nXLeft + nLengthX;
            m_fSquareVertices[1] = nYTop;
            
            m_fSquareVertices[2] = nXLeft;
            m_fSquareVertices[3] = nYTop;
            
            m_fSquareVertices[4] = nXLeft + nLengthX;
            m_fSquareVertices[5] = nYTop - nLengthY;
            
            m_fSquareVertices[6] = nXLeft;
            m_fSquareVertices[7] = nYTop - nLengthY;
            break;
        case QC_ROTATION_180FLIP: // 上下翻转
            //0 | 1
            //-   -
            //2 | 3
            m_fSquareVertices[0] = nXLeft;
            m_fSquareVertices[1] = nYTop;
            
            m_fSquareVertices[2] = nXLeft + nLengthX;
            m_fSquareVertices[3] = nYTop;
            
            m_fSquareVertices[4] = nXLeft;
            m_fSquareVertices[5] = nYTop - nLengthY;
            
            m_fSquareVertices[6] = nXLeft + nLengthX;
            m_fSquareVertices[7] = nYTop - nLengthY;
            break;
        case QC_ROTATION_270: // 顺时针旋转270度
            //3 | 1
            //-   -
            //2 | 0
            m_fSquareVertices[0] = nXLeft + nLengthX;
            m_fSquareVertices[1] = nYTop - nLengthY;
            
            m_fSquareVertices[2] = nXLeft + nLengthX;
            m_fSquareVertices[3] = nYTop;
            
            m_fSquareVertices[4] = nXLeft;
            m_fSquareVertices[5] = nYTop - nLengthY;
            
            m_fSquareVertices[6] = nXLeft;
            m_fSquareVertices[7] = nYTop;
            break;
//        case QC_ROTATION_270FLIP:
//            //1 | 3
//            //-   -
//            //0 | 2
//            m_fSquareVertices[0] = nXLeft;
//            m_fSquareVertices[1] = nYTop - nLengthY;
//            
//            m_fSquareVertices[2] = nXLeft;
//            m_fSquareVertices[3] = nYTop;
//            
//            m_fSquareVertices[4] = nXLeft + nLengthX;
//            m_fSquareVertices[5] = nYTop - nLengthY;
//            
//            m_fSquareVertices[6] = nXLeft + nLengthX;
//            m_fSquareVertices[7] = nYTop;
//            break;
        default:
            //2 | 3
            //-   -
            //0 | 1
            
            // the no.1 point
            m_fSquareVertices[0] = nXLeft;            // left X
            m_fSquareVertices[1] = nYTop - nLengthY;  // bottom Y
            
            // the no.2 point
            m_fSquareVertices[2] = nXLeft + nLengthX; // right X
            m_fSquareVertices[3] = nYTop - nLengthY;  // bottom Y
            
            // the no.3 point
            m_fSquareVertices[4] = nXLeft;            // left X
            m_fSquareVertices[5] = nYTop;             // top Y
            
            // the no.4 point
            m_fSquareVertices[6] = nXLeft + nLengthX; // right X
            m_fSquareVertices[7] = nYTop;             // top Y
            break;
    }
    
    Redraw();
}

int COpenGLRnd::Redraw()
{
    CAutoLock lock (&m_mtDraw);
    
    if (!IsGLRenderReady())
        return QC_ERR_STATUS;
    
    if (NULL == m_pFrameData && !m_pPixelBuffer)
        return QC_ERR_STATUS;
    
    if (!SetContext(m_pContext))
        return QC_ERR_FAILED;
    
    RenderToScreen();
    RenderCommit();

    return QC_ERR_NONE;
}

bool COpenGLRnd::IsGLRenderReady()
{
    if (m_bInitializing)
        return false;
    
    if (0 == m_nColorRenderBuffer)
        return false;
    
    if (0 == m_nFrameBuffer)
        return false;
    
    if (0 == m_pVideoTextureCache)
        return false;
    
    return true;
}

static void onViewEvent(void* pUserData, int nID, void* pValue)
{
    COpenGLRnd* pRender = (COpenGLRnd*)pUserData;
    
    if(pRender)
        pRender->OnViewEvent(nID, pValue);
}

int COpenGLRnd::OnViewEvent(int nID, void * pValue1)
{
    if(nID == UIView_EVT_SIZE_CHANGED)
    {
        CGRect rect = CGRectMake(m_rcView.left, m_rcView.top, m_rcView.right-m_rcView.left, m_rcView.bottom-m_rcView.top);
        m_pParent.frame = rect;

        RefreshView();
    }
    else if(nID == UIView_EVT_APP_INACTIVE)
    {
        if(m_bPlay && m_pBaseInst)
            qcSleepEx(1000*1000, &m_pBaseInst->m_bForceClose);
    }
    else if(nID == UIView_EVT_APP_ACTIVE)
    {
        
    }

    return QC_ERR_NONE;
}

int COpenGLRnd::ViewContentMode2ZoomMode(int nViewContentMode)
{
    if(UIViewContentModeScaleToFill == nViewContentMode)	// 拉伸填充满窗口
        return QC_FILLWINDOW;
    else if(UIViewContentModeScaleAspectFit == nViewContentMode)  // 按宽高比例(包括nNum和nDen)，不做裁剪
        return QC_FIXED_RATIO;
    else if(UIViewContentModeScaleAspectFill == nViewContentMode)  // 按宽高比例，裁剪
        return QC_PANSCAN;
    
    return QC_FIXED_RATIO;
}

void COpenGLRnd::UpdateOpenGLView(bool bCreate)
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
        [m_pVieoView setDelegate:onViewEvent userData:this];
        m_nZoomMode = ViewContentMode2ZoomMode(pParent.contentMode);
        //[m_pVieoView setAutoresizingMask:(UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight)];
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
        [m_pVieoView onDestroy];
        [m_pVieoView removeFromSuperview];
        m_pVieoView = nil;
    }
}

bool COpenGLRnd::IsFormatChanged(QC_VIDEO_FORMAT* pFormat)
{
    if(!pFormat)
        return false;
    if(pFormat->nWidth<=0 || pFormat->nHeight<=0)
        return false;
    
    return (pFormat->nWidth != m_fmtVideo.nWidth) || (pFormat->nHeight != m_fmtVideo.nHeight) || (pFormat->nNum != m_fmtVideo.nNum) || (pFormat->nDen != m_fmtVideo.nDen);
}

void COpenGLRnd::DrawBackgroundColor()
{
    if(m_pBaseInst)
    {
        glClearColor(m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor.fRed, m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor.fGreen,
                     m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor.fBlue, m_pBaseInst->m_pSetting->g_qcs_sBackgroundColor.fAlpha);
    }
    else
        glClearColor(VIDEO_BACK_COLOR);
}

bool COpenGLRnd::UpdateSelfRotate()
{
    if(m_nRotation == QC_ROTATION_0 && m_pBaseInst->m_pSetting->g_qcs_nVideoRotateAngle != 0)
    {
        if (m_pBaseInst->m_pSetting->g_qcs_nVideoRotateAngle == 90)
            m_nRotation = QC_ROTATION_90;
        else if(m_pBaseInst->m_pSetting->g_qcs_nVideoRotateAngle == 270)
        {
            m_nRotation = QC_ROTATION_270;
        }
        else if (m_pBaseInst->m_pSetting->g_qcs_nVideoRotateAngle == 180)
        {
            m_nRotation = QC_ROTATION_180;
        }
        return true;
    }
    else if(m_nRotation != QC_ROTATION_0 && m_pBaseInst->m_pSetting->g_qcs_nVideoRotateAngle == 0)
    {
        m_nRotation = QC_ROTATION_0;
        return true;
    }
    
    return false;
}

int COpenGLRnd::SetParam(int nID, void * pParam)
{
    if (nID == QCPLAY_PID_Disable_Video && pParam)
    {
        m_nDisplayCtrl = *(int*)pParam;
        QCLOGI("[GL]Video render control %d", m_nDisplayCtrl);
    }
    
    return CBaseVideoRnd::SetParam(nID, pParam);
}

bool COpenGLRnd::IsDisableRender()
{
    //QCLOGI("[GL]Checking control %d", m_nDisplayCtrl != QC_PLAY_VideoEnable);
    return (m_nDisplayCtrl != QC_PLAY_VideoEnable);
}

bool COpenGLRnd::SetContext(EAGLContext* pCtx)
{
    return [EAGLContext setCurrentContext:pCtx];
}

@implementation COpenGLView
    
+(Class) layerClass
{
    return [CAEAGLLayer class];
}

-(void)appResignActive
{
    [self sendEvent:UIView_EVT_APP_INACTIVE value:NULL];
}

-(bool)sendEvent:(int)evtID value:(void*)value
{
    if(_userData && _callback)
    {
        _callback(_userData, evtID, value);
    }
    
    return true;
}

- (void)onDestroy
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    
    if (nil != self)
    {
        _callback = NULL;
        _userData = NULL;
        
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appResignActive) name: UIApplicationWillResignActiveNotification object:nil];
    }
    return self;
}

- (void)setDelegate:(UIViewNotifyEvent)pCallback userData:(void*)userData
{
    _callback = pCallback;
    _userData = userData;
}

- (void)layoutSubviews
{
    //NSLog(@"layoutSubviews");
    if(_callback && _userData)
    {
        //_callback(_userData, UIView_EVT_SIZE_CHANGED, NULL);
    }
}

- (void)drawRect:(CGRect)rect
{
    // Drawing code
}

@end



