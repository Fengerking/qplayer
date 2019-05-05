/*******************************************************************************
	File:		COpenGLRnd.h

	Contains:	The ndk Video render header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-12		Bangfei			Create file

*******************************************************************************/
#ifndef __COpenGLRnd_H__
#define __COpenGLRnd_H__
#include <jni.h>
#include <GLES2/gl2.h>

#include "CBaseVideoRnd.h"

class COpenGLRnd : public CBaseVideoRnd
{
public:
	COpenGLRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~COpenGLRnd(void);

	virtual int		SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer);
	virtual int		SetSurface (JNIEnv* pEnv, jobject pSurface);
	virtual void	SetEventDone (bool bEventDone);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Render (QC_DATA_BUFF * pBuff);
	
	virtual int		OnStart(void);
	virtual int		OnStop(void);
	virtual int		ReleaseRnd (void) {return 0;}
	virtual int 	SetParam(JNIEnv * pEnv, int nID, void * pParam) {return QC_ERR_FAILED;}

protected:
	virtual void	UpdateVideoSize (QC_VIDEO_FORMAT * pFmtVideo);
	virtual void	UpdateVideoView (void);

	void 			displayYUV420pData(void * pY, void *pU, void * pV, int w, int h);
	void 			Render();
	int 			InitOpenGL();
	GLuint 			CompileShader(GLenum shaderType, const char*shaderCode);
	GLuint 			CreateProgram(GLuint vsShader, GLuint fsShader);
	void 			setVideoSize(int width, int height);
	void 			SetupYUVTextures(void);
	unsigned char* 	FsStr();
	unsigned char* 	VsStr();

protected:
	JavaVM *							m_pjVM;
	jclass     							m_pjCls;
	jobject								m_pjObj;
	jmethodID							m_fPostEvent;
	
	int									m_nWidth;
	int									m_nHeight;

	QC_VIDEO_BUFF 						m_buffVideo;
	QC_VIDEO_BUFF *						m_pLastVideo;

	GLuint             		m_program;
	GLuint             		m_positionHandle;
	GLuint					m_texCoord;
	GLuint             		m_textureYUV[3];
};

#endif // __COpenGLRnd_H__
