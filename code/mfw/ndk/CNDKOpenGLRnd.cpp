/*******************************************************************************
	File:		CNDKOpenGLRnd.cpp

	Contains:	The ndk Video render implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-12		Bangfei			Create file

*******************************************************************************/
#include <dlfcn.h>

#include "qcErr.h"
#include "qcMsg.h"
#include "qcPlayer.h"

#include "CNDKOpenGLRnd.h"
#include "ClConv.h"
#include "libyuv.h"

#include "ULogFunc.h"
#include "USystemFunc.h"

static GLfloat squareVertices[] = {  
        -1.0f, -1.0f,  
        1.0f, -1.0f,  
        -1.0f,  1.0f,  
        1.0f,  1.0f,  
};  

static GLfloat TextureVertices[] = {  
        0.0f, 1.0f,  
        1.0f, 1.0f,  
        0.0f,  0.0f,  
        1.0f,  0.0f,  
}; 

static const char* g_vertextShader =    
      "attribute vec4 position;    \n"  
      "attribute mediump vec4 textureCoordinate;   \n"  
      "varying vec2 tc;    \n"
      "void main()                  \n"  
      "{                            \n"  
      "   gl_Position = position;  \n"
      "   tc  = textureCoordinate.xy;  \n"  
      "}                            \n";

static const char* g_fragmentShader =
    "precision highp float; \n"
    "varying highp vec2 tc;\n"
    "uniform sampler2D SamplerY;\n"
    "uniform sampler2D SamplerU;\n"
    "uniform sampler2D SamplerV;\n"
    "void main(void)\n"
    "{\n"
        "highp vec3 yuv;\n"
        "highp vec3 rgb;\n"
        "yuv.x = texture2D(SamplerY, tc).r;\n"
        "yuv.y = texture2D(SamplerU, tc).r - 0.5;\n"
        "yuv.z = texture2D(SamplerV, tc).r - 0.5;\n"
        "rgb = mat3( 1,   1,   1,\n"
                    "0,       -0.39465,  2.03211,\n"
                    "1.13983,   -0.58060,  0) * yuv;\n"
        "gl_FragColor = vec4(rgb, 1.0); \n"
    "}\n";

CNDKOpenGLRnd::CNDKOpenGLRnd(CBaseInst * pBaseInst, void * hInst)
	: CNDKVideoRnd (pBaseInst, hInst)
{
	SetObjectName ("CNDKOpenGLRnd");

	mDisplay = 0;
	mSurface = 0;
	mConfig = 0;
	mContext = 0;
    
    _widthId = 0;
    _heightId = 0;
}

CNDKOpenGLRnd::~CNDKOpenGLRnd(void)
{
}

int CNDKOpenGLRnd::Render (QC_DATA_BUFF * pBuff)
{
	CAutoLock lock (&m_mtDraw);
	CBaseVideoRnd::Render (pBuff);
	if (m_pNativeWnd == NULL)
		return QC_ERR_STATUS;
	
	if (pBuff->uBuffType != QC_BUFF_TYPE_Video)
		return QC_ERR_UNSUPPORT;

	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		if (pFmt != NULL && (m_nWidth != pFmt->nWidth || m_nHeight != pFmt->nHeight || m_fmtVideo.nNum != pFmt->nNum || m_fmtVideo.nDen != pFmt->nDen))
		{
			m_nFormatTime = qcGetSysTime ();
			Init (pFmt);
		}
	}

	if (m_nFormatTime > 0 && m_nRndCount == 0)
	{
		m_nFormatTime = qcGetSysTime () - m_nFormatTime;
		if (m_nFormatTime < 80)
		{
			m_nFormatTime = 80 - m_nFormatTime;
			qcSleep (m_nFormatTime * 1000);
			QCLOGI ("TestRender sleep %d", m_nFormatTime);
		}	
		m_nFormatTime = 0;
	}

	if (m_pSendBuff != NULL)
	{
		pBuff->nMediaType = QC_MEDIA_Video;
		m_pSendBuff->SendBuff (pBuff);	
		if (m_nInRender == 1)
		{
			m_nRndCount++;		
			return QC_ERR_NONE;
		}
	}

	int	nRc = 0;
	int nRndWidth = m_nWidth;
	int nRndHeight = m_nHeight;

	QC_VIDEO_BUFF *  pVideoBuff = (QC_VIDEO_BUFF *)pBuff->pBuffPtr;
	if (pVideoBuff->nType != QC_VDT_YUV420_P)
	{
		pVideoBuff = &m_bufVideo;
		if (pVideoBuff->nType != QC_VDT_YUV420_P)			
			return QC_ERR_STATUS;
	}	
	m_pLastVideo = pVideoBuff;	

	//QCLOGI ("Render Video buff = %p", pVideoBuff);
	drawFrame (pVideoBuff);

	m_nRndCount++;

	return QC_ERR_NONE;
}

int	CNDKOpenGLRnd::OnStart(void)
{
	int nRC = CBaseVideoRnd::OnStart();
	InitialEGL ();
	nRC =  Setup();
	return nRC;
}

int	CNDKOpenGLRnd::OnStop(void)
{
	int nRC = CBaseVideoRnd::OnStop();
	if (m_pSendBuff != NULL)
		m_pSendBuff->OnStop ();
	m_pLastVideo = NULL;

	DeinitEGL();
    Destroy();

	return nRC;
}

void CNDKOpenGLRnd::UpdateVideoView (void)
{
	if (m_bPlay || m_pLastVideo == NULL || m_pNativeWnd == NULL)
		return;
	
	int nRndWidth = m_nWidth;
	int nRndHeight = m_nHeight;
	int nRC = m_fANativeWindow_lock(m_pNativeWnd, (void*)&m_buffer, NULL);			
	if (nRC == 0) 
	{
		if (nRndWidth > m_buffer.width)
			nRndWidth = m_buffer.width;
		if (nRndHeight > m_buffer.height)
			nRndHeight = m_buffer.height;

		m_bufRender.nWidth = nRndWidth;
		m_bufRender.nHeight = nRndHeight;
		m_bufRender.pBuff[0] = (unsigned char *)m_buffer.bits;
		m_bufRender.nStride[0] = m_buffer.stride * 4;
		if (m_fColorCvtR != NULL)
			m_fColorCvtR(m_pLastVideo, &m_bufRender, 0);
							
	}	
	else
	{
		QCLOGI ("Lock window buffer failed! return %08X", nRC);
	}	

	nRC = m_fANativeWindow_unlockAndPost (m_pNativeWnd);
}

int CNDKOpenGLRnd::drawFrame(QC_VIDEO_BUFF* dstBuffer)
{    
	//LOGE("mLeftOffset %d mTopOffset %d", mLeftOffset, mTopOffset);
    //glViewport(0, 0, mViewWidth, mViewHeight);
	//QCLOGI ("View size = %d  %d", m_nWidth, m_nHeight);
	//glViewport(0, 0, m_nWidth, m_nHeight / 2);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    
    EGLint width = m_nWidth;
    EGLint height = m_nHeight;
    
    float fWidth = (float)m_nWidth;
    width = dstBuffer->nStride[0];
    fWidth = (fWidth - 1)/width;

    TextureVertices[2] = fWidth;
    TextureVertices[6] = fWidth;
    
	//QCLOGI ("W X H  %d  %d   %.2f", width, height, fWidth);
	bindTexture(_texYId, dstBuffer->pBuff[0], width, height);  
	bindTexture(_texUId, dstBuffer->pBuff[1], width/2, height/2);
	bindTexture(_texVId, dstBuffer->pBuff[2], width/2, height/2);
    
	RenderToScreen();
    
    if(mSurface != EGL_NO_SURFACE && mDisplay != EGL_NO_DISPLAY) {
        eglSwapBuffers(mDisplay, mSurface);
    }

    return 0;
}

int CNDKOpenGLRnd::InitialEGL()
{
	// Get the built in display
	// TODO: check for external HDMI displays
	mDisplay = eglGetDisplay( EGL_DEFAULT_DISPLAY );

	// Initialize EGL
	EGLint majorVersion;
	EGLint minorVersion;
	eglInitialize( mDisplay, &majorVersion, &minorVersion );
	QCLOGI( "eglInitialize gives majorVersion %i, minorVersion %i", majorVersion, minorVersion);

	const char * eglVendorString = eglQueryString( mDisplay, EGL_VENDOR );
	QCLOGI( "EGL_VENDOR: %s", eglVendorString );
	const char * eglClientApisString = eglQueryString( mDisplay, EGL_CLIENT_APIS );
	QCLOGI( "EGL_CLIENT_APIS: %s", eglClientApisString );
	const char * eglVersionString = eglQueryString( mDisplay, EGL_VERSION );
	QCLOGI( "EGL_VERSION: %s", eglVersionString );
	const char * eglExtensionString = eglQueryString( mDisplay, EGL_EXTENSIONS );
	QCLOGI( "EGL_EXTENSIONS:" );


	mConfig = ChooseColorConfig(mDisplay);
	if ( mConfig == 0 )
	{
		QCLOGE( "No acceptable EGL color configs." );
		return -1;
	}

	EGLint contextAttribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE, EGL_NONE,
		EGL_NONE };

    QCLOGI( "%ld:eglCreateContext+++", (long)m_pNativeWnd);
	mContext = eglCreateContext( mDisplay, mConfig, EGL_NO_CONTEXT, contextAttribs );
	if ( mContext == EGL_NO_CONTEXT )
	{
        eglTerminate(mDisplay);
        mDisplay = EGL_NO_DISPLAY;
		QCLOGE( "eglCreateContext failed");
		return -1;
	}
    QCLOGI( "%ld:eglCreateContext---", (long)m_pNativeWnd);
    
    const EGLint surfaceAttrs[] =
    {
        EGL_RENDER_BUFFER,
        EGL_BACK_BUFFER,
        EGL_NONE
    };
    
    QCLOGI( "%ld:eglCreateWindowSurface+++", (long)m_pNativeWnd);
	mSurface = eglCreateWindowSurface( mDisplay, mConfig, m_pNativeWnd, surfaceAttrs);
	if (mSurface == EGL_NO_SURFACE )
	{
		QCLOGE( "eglCreateWindowSurface failed");
		eglDestroyContext( mDisplay, mContext );
		mContext = EGL_NO_CONTEXT;
        eglTerminate(mDisplay);
        mDisplay = EGL_NO_DISPLAY;
		return -1;
	}
    QCLOGI( "%ld:eglCreateWindowSurface---", (long)m_pNativeWnd);

	if (eglMakeCurrent( mDisplay, mSurface, mSurface, mContext ) == EGL_FALSE )
	{
		QCLOGE( "eglMakeCurrent mSurface failed");
		eglDestroySurface( mDisplay, mSurface );
        mSurface = EGL_NO_SURFACE;
		eglDestroyContext( mDisplay, mContext );
		mContext = EGL_NO_CONTEXT;
        eglTerminate(mDisplay);
        mDisplay = EGL_NO_DISPLAY;
		return -1;
	}
   
	return 0;
}

int CNDKOpenGLRnd::DeinitEGL()
{
    //LOGE( "%ld:eglMakeCurrent+++", (long)m_pNativeWnd);
	if ( eglMakeCurrent( mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) == EGL_FALSE ){
		QCLOGE( "eglMakeCurrent: failed");
	}
    //LOGE( "%ld:eglMakeCurrent---", (long)m_pNativeWnd);

    //LOGE( "%ld:eglDestroyContext+++", (long)m_pNativeWnd);
    if (mContext != EGL_NO_CONTEXT ) {
        if ( eglDestroyContext( mDisplay, mContext ) == EGL_FALSE ){
            QCLOGE( "eglDestroyContext: failed" );
        }
    }
    //LOGE( "%ld:eglDestroyContext---", (long)m_pNativeWnd);

    //LOGE( "%ld:eglDestroySurface+++", (long)m_pNativeWnd);
    if (mSurface != EGL_NO_SURFACE ) {
        if ( eglDestroySurface( mDisplay, mSurface ) == EGL_FALSE ) {
            QCLOGE( "eglDestroySurface: failed" );
        }
    }
    //LOGE( "%ld:eglDestroySurface---", (long)m_pNativeWnd);
    
    if(mDisplay != EGL_NO_DISPLAY) {
        eglTerminate(mDisplay);
    }

	mDisplay = 0;
	mSurface = 0;		
	mConfig = 0;
	mContext = 0;

	return 0;
}

int CNDKOpenGLRnd::Setup() 
{
    const char* fragmentShader = g_fragmentShader;
    
    _program = createProgram(g_vertextShader, fragmentShader);
    if (!_program) {
        QCLOGE("Could not create program");
        return -1;
    }

	glUseProgram(_program);

	glGenTextures(1, &_texYId);
	glGenTextures(1, &_texUId);  
	glGenTextures(1, &_texVId);  

	_sample_y = glGetUniformLocation(_program, "SamplerY");  
    _sample_u = glGetUniformLocation(_program, "SamplerU");  
    _sample_v = glGetUniformLocation(_program, "SamplerV");

    m_nPositionSlot = glGetAttribLocation(_program, "position");
    glEnableVertexAttribArray(m_nPositionSlot);
    
    m_nTexCoordSlot = glGetAttribLocation(_program, "textureCoordinate");
    glEnableVertexAttribArray(m_nTexCoordSlot);

    return 0;
}

int CNDKOpenGLRnd::Destroy()
{
	glDeleteProgram(_program);
}

EGLConfig CNDKOpenGLRnd::ChooseColorConfig(EGLDisplay	aDisplay)
{
	static const int MAX_CONFIGS = 1024;
	EGLConfig 	configs[MAX_CONFIGS];
	EGLint		configsCount[MAX_CONFIGS];
	EGLint  	numConfigs = 0;

	if ( EGL_FALSE == eglGetConfigs( aDisplay,
			configs, MAX_CONFIGS, &numConfigs ) ) {
		return NULL;
	}
	//LOGE( "eglGetConfigs() = %i configs", numConfigs );

	const EGLint configAttribs[] = {
            EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,  //指定渲染api类别
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 16,
            EGL_STENCIL_SIZE, 8,
            EGL_NONE
	};

	int confSize = 1;
	eglChooseConfig(aDisplay, configAttribs, configs, confSize, configsCount);

	if(configsCount[0] > 0) {
		return configs[0];
	}
}

EGLConfig CNDKOpenGLRnd::EglConfigForConfigID( const EGLDisplay display, const GLint configID )
{
	static const int MAX_CONFIGS = 1024;
	EGLConfig 	configs[MAX_CONFIGS];
	EGLint  	numConfigs = 0;

	if ( EGL_FALSE == eglGetConfigs( display,
			configs, MAX_CONFIGS, &numConfigs ) )
	{
		QCLOGE( "eglGetConfigs() failed" );
		return NULL;
	}

	for ( int i = 0; i < numConfigs; i++ )
	{
		EGLint	value = 0;

		eglGetConfigAttrib( display, configs[i], EGL_CONFIG_ID, &value );
		if ( value == configID )
		{
			return configs[i];
		}
	}

	return NULL;
}

void CNDKOpenGLRnd::RenderToScreen()
{
	glVertexAttribPointer(m_nPositionSlot, 2, GL_FLOAT, 0, 0, squareVertices);
	glVertexAttribPointer(m_nTexCoordSlot, 2, GL_FLOAT, 0, 0, TextureVertices);

    glActiveTexture(GL_TEXTURE0);  
    glBindTexture(GL_TEXTURE_2D, _texYId);  
    glUniform1i(_sample_y, 0);  

    glActiveTexture(GL_TEXTURE1);  
    glBindTexture(GL_TEXTURE_2D, _texUId);  
    glUniform1i(_sample_u, 1);  
  
    glActiveTexture(GL_TEXTURE2);  
    glBindTexture(GL_TEXTURE_2D, _texVId);  
    glUniform1i(_sample_v, 2);  
    
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); 
}

GLuint CNDKOpenGLRnd::bindTexture(GLuint texture,unsigned char *buffer, GLuint w , GLuint h)
{   
    glBindTexture ( GL_TEXTURE_2D, texture );  

    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );  
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );  
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );  
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );  

	glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);  
  
    return texture;  
}  

GLuint CNDKOpenGLRnd::loadShader(GLenum shaderType, const char* pSource)
{
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE(" Could not compile shader %d: %s", shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint CNDKOpenGLRnd::createProgram(const char* pVertexSource,
                                       const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }
    
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!fragmentShader) {
        return 0;
    }
    
    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE(" Could not link program: %s", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

