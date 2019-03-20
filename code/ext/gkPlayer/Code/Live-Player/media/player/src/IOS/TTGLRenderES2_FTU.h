#ifndef __TT_GLRENDER_ES2_FTU_
#define __TT_GLRENDER_ES2_FTU_

#import "TTGLRenderES2.h"

class TTGLRenderES2_FTU : public TTGLRenderES2 {
    
public:
    TTGLRenderES2_FTU(EAGLContext* pContext);
    virtual ~TTGLRenderES2_FTU();
    
    virtual int SetupFrameBuffer();
    virtual int DeleteFrameBuffer();
    
    virtual int SetupTexture();
    virtual int DeleteTexture();
    
    virtual int TextureSizeChange();
    
    virtual int CompileAllShaders();
    
    virtual int UploadTexture(TTVideoBuffer *pVideoBuffer);
    virtual int RenderToScreen();
    
    virtual bool IsGLRenderReady();
    
    
    virtual int Flush();
    
    virtual int RedrawInner(bool bIsTryGetFrame, void *pData);
    
    virtual int EsGenSphere(int numSlices, float radius, int *numVertices_out, int nType);
    virtual int EsGenSphere_2(int numSlices, float radius, int *numVertices_out, int nType);
    virtual int EsFreeSphere();
    
    virtual int SetupSphereContext();
    virtual int FreeSphereContext();
    virtual int SetupModelViewMatrix();
    
    virtual void StartDeviceMotion();
    virtual void StopDeviceMotion();
    
    virtual void setRendType(TTInt aRenderType);
    virtual void setMotionEnable(bool aEnable);

private:
    CVOpenGLESTextureRef _lumaTexture;
    CVOpenGLESTextureRef _chromaTexture;
	CVOpenGLESTextureCacheRef videoTextureCache;
    
    GLint m_nTextureUniformY;
    GLint m_nTextureUniformUV;
    GLint m_nTextureUniformRGB;
    GLint m_nVertexSphere;
    GLint m_nFragmentSphere;
    GLint m_nTextureUniformViewProjectionMaxtrix;
    
    /*For Creating Sphere*/
    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint vertexTexCoordID;
    GLuint vertexIndicesBufferID;
    GLfloat *vertices;
    GLfloat *texCoordsL;
    GLfloat *texCoordsR;
    GLushort *indices;
    int numVertices;
    int numIndices;
    GLKMatrix4 modelViewProjectionMatrix;
    /*End*/
};

#endif

