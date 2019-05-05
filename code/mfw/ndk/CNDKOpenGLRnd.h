/*******************************************************************************
	File:		CNDKOpenGLRnd.h

	Contains:	The ndk Video render header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-12		Bangfei			Create file

*******************************************************************************/
#ifndef __CNDKOpenGLRnd_H__
#define __CNDKOpenGLRnd_H__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "CNDKVideoRnd.h"

class CNDKOpenGLRnd : public CNDKVideoRnd
{
public:
	CNDKOpenGLRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~CNDKOpenGLRnd(void);

	virtual int		Render (QC_DATA_BUFF * pBuff);
	
	virtual int		OnStart(void);
	virtual int		OnStop(void);

protected:
	virtual void	UpdateVideoView (void);

private:
	int     		InitialEGL();
	int				Setup();
	int				Destroy();
	int				DeinitEGL();

	int 			drawFrame(QC_VIDEO_BUFF* dstBuffer);

	GLuint			loadShader(GLenum shaderType, const char* pSource);
	GLuint			createProgram(const char* pVertexSource, const char* pFragmentSource);
	GLuint  		bindTexture(GLuint texture, unsigned char *buffer, GLuint w , GLuint h);
	void			RenderToScreen();
	EGLConfig 		ChooseColorConfig(EGLDisplay	aDisplay);
	EGLConfig 		EglConfigForConfigID( const EGLDisplay display, const GLint configID);


protected:
	EGLDisplay			mDisplay;
	EGLSurface			mSurface;		
	EGLConfig			mConfig;
	EGLContext			mContext;
    
    int         		mInitSet;
    
	GLuint 				_program;

	GLuint 				_texYId;  
	GLuint 				_texUId;  
	GLuint 				_texVId;
    
    GLuint 				_widthId;
    GLuint 				_heightId;

	GLint 				_sample_y;
	GLint 				_sample_u ;
	GLint 				_sample_v;
	int 				m_nPositionSlot;
	int 				m_nTexCoordSlot;	
};

#endif // __CNDKOpenGLRnd_H__
