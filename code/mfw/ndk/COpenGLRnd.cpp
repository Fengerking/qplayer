/*******************************************************************************
	File:		COpenGLRnd.cpp

	Contains:	The OpenGL Video render implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2019-05-04		Bangfei			Create file

*******************************************************************************/
#include <dlfcn.h>

#include "qcErr.h"
#include "qcMsg.h"
#include "qcPlayer.h"

#include "COpenGLRnd.h"

#include "ULogFunc.h"
#include "USystemFunc.h"

typedef enum {
    TEXY = 0,
    TEXU,
    TEXV,
    TEXC
}Tex_YUV;

COpenGLRnd::COpenGLRnd(CBaseInst * pBaseInst, void * hInst)
	: CBaseVideoRnd (pBaseInst, hInst)
	, m_pjVM (NULL)
	, m_pjCls (NULL)
	, m_pjObj (NULL)
	, m_fPostEvent (NULL)
	, m_nWidth (544)
	, m_nHeight (960)
	, m_pLastVideo (NULL)
{
	SetObjectName ("COpenGLRnd");
	memset (&m_buffVideo, 0, sizeof (QC_VIDEO_BUFF));

	m_program = 0;
	m_positionHandle = 0;
	m_texCoord = 0;
	m_textureYUV[TEXY] = 0;
	m_textureYUV[TEXU] = 0;
	m_textureYUV[TEXV] = 0;
}

COpenGLRnd::~COpenGLRnd(void)
{
	Uninit ();
}

int COpenGLRnd::SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer)
{
	m_pjVM = jvm;
	m_pjCls = clsPlayer;
	m_pjObj = objPlayer;

	if (m_pjCls != NULL && m_pjObj != NULL)
	{
		JNIEnv * pEnv = env;
		if (pEnv == NULL)
			m_pjVM->AttachCurrentThread(&pEnv, NULL);
		m_fPostEvent = pEnv->GetStaticMethodID(m_pjCls, "postEventFromNative", "(Ljava/lang/Object;IIILjava/lang/Object;)V");
		if (env == NULL)
			m_pjVM->DetachCurrentThread();
	}	

	InitOpenGL ();

	return QC_ERR_NONE;
}

int COpenGLRnd::SetSurface (JNIEnv* pEnv, jobject pSurface)
{
	QCLOGI ("SetSurface 0000");
	//return QC_ERR_NONE;

	//CAutoLock lock (&m_mtDraw);
	m_mtDraw.Lock();
	UpdateVideoView ();
	m_mtDraw.Unlock();
	if (m_pLastVideo != NULL)
		m_pLastVideo = NULL;

	return QC_ERR_NONE;
}

void COpenGLRnd::SetEventDone (bool bEventDone) 
{
}

int COpenGLRnd::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (CBaseVideoRnd::Init (pFmt) != QC_ERR_NONE)
		return QC_ERR_STATUS;

	m_nWidth = pFmt->nWidth;
	m_nHeight = pFmt->nHeight;
	if (m_nWidth == 0 || m_nHeight == 0)
		return QC_ERR_NONE;

m_nWidth = 544;
	//setVideoSize (m_nWidth, m_nHeight);
	
	UpdateVideoSize (pFmt);

	return QC_ERR_NONE;
}

int COpenGLRnd::Uninit (void)
{
	CBaseVideoRnd::Uninit ();
	QC_DEL_A (m_buffVideo.pBuff[0]);
	QC_DEL_A (m_buffVideo.pBuff[0]);
	QC_DEL_A (m_buffVideo.pBuff[0]);	
	memset (&m_buffVideo, 0, sizeof (QC_VIDEO_BUFF));
	return QC_ERR_NONE;
}

int COpenGLRnd::Render (QC_DATA_BUFF * pBuff)
{
	//CAutoLock lock (&m_mtDraw);
	m_mtDraw.Lock();
	CBaseVideoRnd::Render (pBuff);
m_nWidth = 544;
//	QCLOGI ("Render 0000");

	if (pBuff->uBuffType != QC_BUFF_TYPE_Video)
		return QC_ERR_UNSUPPORT;

	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		if (pFmt != NULL && (m_nWidth != pFmt->nWidth || m_nHeight != pFmt->nHeight || m_fmtVideo.nNum != pFmt->nNum || m_fmtVideo.nDen != pFmt->nDen))
		{
			Init (pFmt);
		}
	}


	QC_VIDEO_BUFF *  pVideoBuff = (QC_VIDEO_BUFF *)pBuff->pBuffPtr;
	if (pVideoBuff->nType != QC_VDT_YUV420_P)
	{
		pVideoBuff = &m_bufVideo;
		if (pVideoBuff->nType != QC_VDT_YUV420_P)
		{	
			return QC_ERR_STATUS;
		}
	}

	if (m_buffVideo.pBuff[0] == NULL) {
		m_buffVideo.pBuff[0] = new unsigned char[m_nWidth * m_nHeight];
		m_buffVideo.pBuff[1] = new unsigned char[m_nWidth * m_nHeight / 4];
		m_buffVideo.pBuff[2] = new unsigned char[m_nWidth * m_nHeight / 4];	
	}

	m_pLastVideo = pVideoBuff;	
	m_mtDraw.Unlock();
//	while (m_pLastVideo != NULL) {
//		qcSleep (1000);
//	}
	return QC_ERR_NONE;



	for (int i = 0; i < m_nHeight; i++) {
		memcpy (m_buffVideo.pBuff[0] + m_nWidth * i, pVideoBuff->pBuff[0] + i * pVideoBuff->nStride[0], m_nWidth);
	}
	for (int i = 0; i < m_nHeight / 2; i++) {
	//	memcpy (m_buffVideo.pBuff[1] + (m_nWidth / 2) * i, pVideoBuff->pBuff[1] + i * pVideoBuff->nStride[1], m_nWidth / 2);
	//	memcpy (m_buffVideo.pBuff[2] + (m_nWidth / 2) * i, pVideoBuff->pBuff[2] + i * pVideoBuff->nStride[2], m_nWidth / 2);
	}

	displayYUV420pData (m_buffVideo.pBuff[0], m_buffVideo.pBuff[1], m_buffVideo.pBuff[2], m_nWidth, m_nHeight);

	m_nRndCount++;

	return QC_ERR_NONE;
}

void COpenGLRnd::UpdateVideoSize (QC_VIDEO_FORMAT * pFmtVideo)
{
	if (m_fPostEvent == NULL || pFmtVideo == NULL)
		return;

	m_rcView.top = 0;
	m_rcView.left = 0;
	m_rcView.right = m_fmtVideo.nWidth;
	m_rcView.bottom = m_fmtVideo.nHeight;	

	CBaseVideoRnd::UpdateRenderSize ();
	int nWidth = m_rcRender.right - m_rcRender.left;
	int nHeight = m_rcRender.bottom - m_rcRender.top;

	QCLOGI ("Update Video Size: %d X %d  Ratio: %d : %d", pFmtVideo->nWidth, pFmtVideo->nHeight, nWidth, nHeight);

	if (m_fPostEvent != NULL)
	{
		JNIEnv * pEnv = NULL;
		m_pjVM->AttachCurrentThread(&pEnv, NULL);
		pEnv->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, QC_MSG_SNKV_NEW_FORMAT, nWidth, nHeight, NULL);
		m_pjVM->DetachCurrentThread();
	}
}

int	COpenGLRnd::OnStart(void)
{
	int nRC = CBaseVideoRnd::OnStart();
	return nRC;
}

int	COpenGLRnd::OnStop(void)
{
	int nRC = CBaseVideoRnd::OnStop();
	m_pLastVideo = NULL;
	return nRC;
}

void COpenGLRnd::UpdateVideoView (void)
{
	QCLOGI ("UpdateVideoView  0000");
m_nWidth = 544;
	if (m_pLastVideo == NULL)
		return;
	QCLOGI ("UpdateVideoView  1111");
	QC_VIDEO_BUFF * pVideoBuff = m_pLastVideo;	

	for (int i = 0; i < m_nHeight; i++) {
		memcpy (m_buffVideo.pBuff[0] + m_nWidth * i, pVideoBuff->pBuff[0] + i * pVideoBuff->nStride[0], m_nWidth);
	}
	for (int i = 0; i < m_nHeight / 2; i++) {
		memcpy (m_buffVideo.pBuff[1] + (m_nWidth / 2) * i, pVideoBuff->pBuff[1] + i * pVideoBuff->nStride[1], m_nWidth / 2);
		memcpy (m_buffVideo.pBuff[2] + (m_nWidth / 2) * i, pVideoBuff->pBuff[2] + i * pVideoBuff->nStride[2], m_nWidth / 2);
	}
	QCLOGI ("UpdateVideoView  2222");
	displayYUV420pData (m_buffVideo.pBuff[0], m_buffVideo.pBuff[1], m_buffVideo.pBuff[2], m_nWidth, m_nHeight);
	QCLOGI ("UpdateVideoView  3333");
}

void COpenGLRnd::displayYUV420pData(void * pY, void *pU, void * pV, int w, int h)
{
    //绑定
    glBindTexture(GL_TEXTURE_2D, m_textureYUV[TEXY]);

    /**
     更新纹理
     @param target#>  指定目标纹理，这个值必须是GL_TEXTURE_2D。 description#>
     @param level#>   执行细节级别。0是最基本的图像级别，n表示第N级贴图细化级别 description#>
     @param xoffset#> 纹理数据的偏移x值 description#>
     @param yoffset#> 纹理数据的偏移y值 description#>
     @param width#>   更新到现在的纹理中的纹理数据的规格宽 description#>
     @param height#>  高 description#>
     @param format#>  像素数据的颜色格式, 不需要和internalformatt取值必须相同。可选的值参考internalformat。 description#>
     @param type#>    颜色分量的数据类型 description#>
     @param pixels#>  指定内存中指向图像数据的指针 description#>
     */

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)w, (GLsizei)h, GL_LUMINANCE, GL_UNSIGNED_BYTE, pY);
    glBindTexture(GL_TEXTURE_2D,m_textureYUV[TEXU]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)w/2, (GLsizei)h/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, pU);
    glBindTexture(GL_TEXTURE_2D,m_textureYUV[TEXV]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)w/2, (GLsizei)h/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, pV);

    //渲染
    Render();
}
void COpenGLRnd::Render()
{
    //把数据显示在这个视窗上
    /*
     我们如果选定(0, 0), (0, 1), (1, 0), (1, 1)四个纹理坐标的点对纹理图像映射的话，就是映射的整个纹理图片。如果我们选择(0, 0), (0, 1), (0.5, 0), (0.5, 1) 四个纹理坐标的点对纹理图像映射的话，就是映射左半边的纹理图片（相当于右半边图片不要了），相当于取了一张320x480的图片。但是有一点需要注意，映射的纹理图片不一定是“矩形”的。实际上可以指定任意形状的纹理坐标进行映射。下面这张图就是映射了一个梯形的纹理到目标物体表面。这也是纹理（Texture）比上一篇文章中记录的表面（Surface）更加灵活的地方。
     */
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    static const GLfloat squareVertices[] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f,  1.0f,
            1.0f,  1.0f,
    };

    static const GLfloat coordVertices[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f,  0.0f,
            1.0f,  0.0f,
    };
    //更新属性值
    glVertexAttribPointer(m_positionHandle, 2, GL_FLOAT, 0, 0, squareVertices);
    //开启定点属性数组
    glEnableVertexAttribArray(m_positionHandle);

    glVertexAttribPointer(m_texCoord, 2, GL_FLOAT, 0, 0, coordVertices);
    glEnableVertexAttribArray(m_texCoord);

    //绘制
    //当采用顶点数组方式绘制图形时，使用该函数。该函数根据顶点数组中的坐标数据和指定的模式，进行绘制。
    //绘制方式,从数组的哪一个点开始绘制(一般为0),顶点个数
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

int COpenGLRnd::InitOpenGL()
{
    //glClearColor(0.1f, 0.4f, 0.6f, 1.0f);
    float data[] = {
            -0.2f,-0.2f,-0.6f,1.0f,
            0.2f,-0.2f,-0.6f,1.0f,
            0.0f,0.2f,-0.6f,1.0f
    };
	QCLOGI ("InitOpenGL  0000");
    SetupYUVTextures();
    unsigned char* shaderCode = VsStr();
	QCLOGI ("InitOpenGL  1111");
    GLuint vsShader = CompileShader(GL_VERTEX_SHADER, (char*)shaderCode);
    //  delete(shaderCode);

		QCLOGI ("InitOpenGL  2222");
    shaderCode = FsStr();
    GLuint fsShader = CompileShader(GL_FRAGMENT_SHADER, (char*)shaderCode);
    //  delete(shaderCode);
		QCLOGI ("InitOpenGL  3333");
    m_program = CreateProgram(vsShader, fsShader);
		QCLOGI ("InitOpenGL  4444");
    glDeleteShader(vsShader);
    glDeleteShader(fsShader);
		QCLOGI ("InitOpenGL  5555");
    m_positionHandle = (GLuint)glGetAttribLocation(m_program, "position");
    m_texCoord = (GLuint)glGetAttribLocation(m_program, "TexCoordIn");
	QCLOGI ("InitOpenGL  6666");
    //像素数据对齐,第二个参数默认为4,一般为1或4
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		QCLOGI ("InitOpenGL  7777");
    //使用着色器
    glUseProgram(m_program);
		QCLOGI ("InitOpenGL  8888");
    //获取一致变量的存储位置
    GLint textureUniformY = glGetUniformLocation(m_program, "SamplerY");
    GLint textureUniformU = glGetUniformLocation(m_program, "SamplerU");
    GLint textureUniformV = glGetUniformLocation(m_program, "SamplerV");
    //对几个纹理采样器变量进行设置
    glUniform1i(textureUniformY, 0);
    glUniform1i(textureUniformU, 1);
    glUniform1i(textureUniformV, 2);
    setVideoSize(m_nWidth, m_nHeight);
	QCLOGI ("InitOpenGL  9999");
	return QC_ERR_NONE;
}

GLuint COpenGLRnd::CompileShader(GLenum shaderType, const char*shaderCode)
{
    GLuint shader = glCreateShader(shaderType);
    if (shader == 0){
        QCLOGW("glCreateShader failed! shaderCode = %s", shaderCode);
        return 0;
    }
    if (shaderCode == NULL){
        glDeleteShader(shader);
        return 0;
    }
    glShaderSource(shader, 1, &shaderCode, NULL);
    glCompileShader(shader);
    GLint compileResult = GL_TRUE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);
    if (compileResult == GL_FALSE){
        char errorBuf[1024] = { 0 };
        GLsizei logLen = 0;
        glGetShaderInfoLog(shader, 1024, &logLen, errorBuf);
        //printf("Compile Shader fail error log : %s \nshader code :\n%s\n", logLen, shaderCode);
        glDeleteShader(shader);
        shader = 0;
    }
    // delete shaderCode;
    return shader;
}

GLuint COpenGLRnd::CreateProgram(GLuint vsShader, GLuint fsShader)
{
    GLuint  program = glCreateProgram();
    glAttachShader(program,vsShader);
    glAttachShader(program,fsShader);
    glLinkProgram(program);
    glDetachShader(program,vsShader);
    glDetachShader(program,fsShader);
    GLint ret;
    glGetProgramiv(program,GL_LINK_STATUS,&ret);
    if (ret == GL_FALSE){
        char errorBuf[1024] = {0};
        GLsizei writed = 0;
        glGetProgramInfoLog(program,1024,&writed,errorBuf);
        QCLOGW("create gpu program fail,link error:%s!",errorBuf);
        glDeleteProgram(program);
        program = 0;
    }
    return program;
}

void COpenGLRnd::setVideoSize(int width, int height)
{
//    videoH = (GLuint)height;
//    videoW = (GLuint)width;
    //开辟内存空间
    size_t length = (size_t)(width * height);
    void *blackDataY = malloc(length);
    void *blackDataU = malloc(length/4);
    void *blackDataV = malloc(length/4);
    if (blackDataY){
        /**
         对内存空间清零,作用是在一段内存块中填充某个给定的值，它是对较大的结构体或数组进行清零操作的一种最快方法
         @param __b#>   源数据 description#>
         @param __c#>   填充数据 description#>
         @param __len#> 长度 description#>
         @return <#return value description#>
         */
        memset(blackDataY, 0x0, length);
        memset(blackDataU, 0x0, length/4);
        memset(blackDataV, 0x0, length/4);
    }
    //绑定Y纹理
    glBindTexture(GL_TEXTURE_2D, m_textureYUV[TEXY]);
    /**
    根据像素数据,加载纹理
    @param target#>         指定目标纹理，这个值必须是GL_TEXTURE_2D。 description#>
    @param level#>          执行细节级别。0是最基本的图像级别，n表示第N级贴图细化级别 description#>
    @param internalformat#> 指定纹理中的颜色格式。可选的值有GL_ALPHA,GL_RGB,GL_RGBA,GL_LUMINANCE, GL_LUMINANCE_ALPHA 等几种。 description#>
    @param width#>          纹理的宽度 description#>
    @param height#>         高度 description#>
    @param border#>         纹理的边框宽度,必须为0 description#>
    @param format#>         像素数据的颜色格式, 不需要和internalformatt取值必须相同。可选的值参考internalformat。 description#>
    @param type#>           指定像素数据的数据类型。可以使用的值有GL_UNSIGNED_BYTE,GL_UNSIGNED_SHORT_5_6_5,GL_UNSIGNED_SHORT_4_4_4_4,GL_UNSIGNED_SHORT_5_5_5_1等。 description#>
    @param pixels#>         指定内存中指向图像数据的指针 description#>
    @return <#return value description#>
    */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, blackDataY);
    //绑定U纹理
    glBindTexture(GL_TEXTURE_2D, m_textureYUV[TEXU]);
    //加载纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, blackDataU);
    //绑定V数据
    glBindTexture(GL_TEXTURE_2D, m_textureYUV[TEXV]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, blackDataV);
    //释放malloc分配的内存空间
    free(blackDataY);
    free(blackDataU);
    free(blackDataV);
}

void COpenGLRnd::SetupYUVTextures(void)
{
	QCLOGI ("SetupYUVTextures  0000");
    if(m_textureYUV[TEXY]){
        glDeleteTextures(3, m_textureYUV);
    }
    //生成纹理
    glGenTextures(3,m_textureYUV);
    if (!m_textureYUV[TEXY] || !m_textureYUV[TEXU] || !m_textureYUV[TEXV])
    {
		QCLOGW ("glGenTextures  1111  %d, %d, %d", m_textureYUV[TEXY], m_textureYUV[TEXU], m_textureYUV[TEXV]);
        return;
    }
	QCLOGI ("SetupYUVTextures  1111");
    //选择当前活跃单元
    glActiveTexture(GL_TEXTURE0);
    //绑定Y纹理
    glBindTexture(GL_TEXTURE_2D, m_textureYUV[TEXY]);
    //纹理过滤函数
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);//放大过滤
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);//缩小过滤
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);//水平方向
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);//垂直方向

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureYUV[TEXU]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textureYUV[TEXV]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	QCLOGI ("SetupYUVTextures  2222");
}

unsigned char * COpenGLRnd::FsStr()
{
    const char* fsstr = "varying lowp vec2 TexCoordOut;\
             uniform sampler2D SamplerY;\
             uniform sampler2D SamplerU;\
             uniform sampler2D SamplerV;\
             void main(void)\
            {\
            mediump vec3 yuv;\
            lowp vec3 rgb;\
            yuv.x = texture2D(SamplerY, TexCoordOut).r;\
            yuv.y = texture2D(SamplerU, TexCoordOut).r - 0.5;\
            yuv.z = texture2D(SamplerV, TexCoordOut).r - 0.5;\
            rgb = mat3( 1,       1,         1,\
            0,       -0.39465,  2.03211,\
            1.13983, -0.58060,  0) * yuv;\
            gl_FragColor = vec4(rgb, 1);\
            }";
    return (unsigned char*)fsstr;
}

unsigned char * COpenGLRnd::VsStr()
{
    const char* vsstr = "attribute vec4 position;\
            attribute vec2 TexCoordIn;\
            varying vec2 TexCoordOut;\
            void main(void)\
            {\
            gl_Position = position;\
            TexCoordOut = TexCoordIn;\
            }";
    return (unsigned char*)vsstr;
}