#import "TTGLRenderES2_FTU.h"
#import "TTLog.h"
#import "TTSysTime.h"
#import "GKMacrodef.h"
#import "GKMsgEnumDef.h"

static const GLchar *G_VERTEX_SHADER_Y_UV = " \
precision mediump float; \
attribute vec4 position; \
attribute vec2 texCoord; \
varying vec2 texCoordVarying; \
uniform mat4 modelViewProjectionMatrix; \
void main() \
{ \
    gl_Position = modelViewProjectionMatrix * position; \
    texCoordVarying = texCoord; \
}";


static const GLchar *G_FRAGMENT_SHADER_Y_UV = " \
precision mediump float; \
uniform sampler2D SamplerY; \
uniform sampler2D SamplerUV; \
uniform sampler2D SamplerRGB; \
uniform float fsphere; \
varying highp vec2 texCoordVarying; \
void main() \
{ \
    mediump vec3 yuv; \
    lowp vec3 rgb; \
    if(fsphere > 0.0){ \
        yuv.x = texture2D(SamplerY, texCoordVarying).r; \
        yuv.yz = texture2D(SamplerUV, texCoordVarying).rg - vec2(0.5, 0.5); \
        rgb = mat3(      1,       1,      1, \
                   0, -.18732, 1.8556, \
                   1.57481, -.46813,      0) * yuv; \
        gl_FragColor = vec4(rgb, 1); \
    } else { \
        gl_FragColor = texture2D(SamplerRGB, texCoordVarying); \
    } \
}";

TTGLRenderES2_FTU::TTGLRenderES2_FTU(EAGLContext* pContext)
:TTGLRenderES2(pContext)
,_lumaTexture(NULL)
,_chromaTexture(NULL)
,videoTextureCache(NULL)
,m_nTextureUniformY(0)
,m_nTextureUniformUV(0)
,m_nTextureUniformRGB(0)
,m_nTextureUniformViewProjectionMaxtrix(0)
,m_nFragmentSphere(0)
,vertices(NULL)
,texCoordsL(NULL)
,texCoordsR(NULL)
,indices(NULL)
,numIndices(0)
,numVertices(0)
,vertexArrayID(0)
,vertexBufferID(0)
,vertexIndicesBufferID(0)
,vertexTexCoordID(0)
{
}

TTGLRenderES2_FTU::~TTGLRenderES2_FTU()
{
    StopDeviceMotion();

    if (_lumaTexture)
    {
        CFRelease(_lumaTexture);
        _lumaTexture = NULL;
    }
    
    if (_chromaTexture)
    {
        CFRelease(_chromaTexture);
        _chromaTexture = NULL;
    }
    
    if (videoTextureCache) {
        CFRelease(videoTextureCache);
        videoTextureCache = 0;
    }

    EsFreeSphere();
    
    FreeSphereContext();
}

int TTGLRenderES2_FTU::EsFreeSphere()
{
    if (vertices) {
        free(vertices);
        vertices = NULL;
    }
    
    if (texCoordsL) {
        free(texCoordsL);
        texCoordsL = NULL;
    }
    
    if (texCoordsR) {
        free(texCoordsR);
        texCoordsR = NULL;
    }
    
    if (indices) {
        free(indices);
        indices = NULL;
    }
    
    return 0;
}

int TTGLRenderES2_FTU::EsGenSphere(int numSlices, float radius, int *numVertices_out, int nType)
{
    int i;
    int j;
    int numParallels = numSlices / 2;
    int numVertices = (numParallels + 1) * (numSlices + 1);
    int numIndices = numParallels * numSlices * 6;
    float angleStep = 360.0/numSlices;
    
    // Allocate memory for buffers
    if (!vertices)
        vertices = (GLfloat *)malloc(sizeof(GLfloat) * 3 * numVertices);
    
    if (!texCoordsL)
        texCoordsL = (GLfloat *)malloc(sizeof(GLfloat) * 2 * numVertices);
    
    if(!texCoordsR)
        texCoordsR = (GLfloat *)malloc(sizeof(GLfloat) * 2 * numVertices);
    
    if (!indices)
        indices = (GLushort *)malloc(sizeof(GLuint) * numIndices);
    
    i = 0;
    j = 0;
    angleStep = 360.0/numSlices;
    for (float f1 = -90.0; f1 <= 90.0; f1 += angleStep)
    {
        float f2 = (float)sinf(M_PI * f1 / 180.0);
        float f3 = (float)cosf(M_PI * f1 / 180.0);
        for (float f4 = 0.0; f4 <= 360.0; f4 += angleStep)
        {
            float f5 = (float)cosf(M_PI * f4 / 180.0);
            float f6 = (float)sinf(M_PI * f4 / 180.0);
            vertices[i] = (f3 * f6 * radius);
            vertices[(i + 1)] = (f2 * radius);
            vertices[(i + 2)] = (f3 * f5 * radius);
            
            if(nType & ERenderLR3D) {
                texCoordsL[j] = (1.0 - f4 / 360.0)*0.5;
                texCoordsL[(j + 1)] = (1.0 - (90.0 + f1) / 180.0);
                
                texCoordsR[j] = 0.5 + texCoordsL[j];
                texCoordsR[(j + 1)] = (1.0 - (90.0 + f1) / 180.0);
                
            } else if(nType & ERenderUD3D) {
                texCoordsL[j] = (1.0 - f4 / 360.0);
                texCoordsL[(j + 1)] = (1.0 - (90.0 + f1) / 180.0)*0.5;
                
                texCoordsR[j] = (1.0 - f4 / 360.0);
                texCoordsR[(j + 1)] = texCoordsL[(j + 1)] + 0.5;
            } else {
                texCoordsL[j] = (1.0 - f4 / 360.0);
                texCoordsL[(j + 1)] = (1.0 - (90.0 + f1) / 180.0);
            }
            
            i += 3;
            j += 2;
        }
    }

    // Generate the vertex indices in the vertices array
    if (NULL != indices)
    {
        uint16_t *indexBuf = indices;
        for (i = 0; i < numParallels ; i++)
        {
            for (j = 0; j < numSlices; j++)
            {
                *indexBuf++ = (j + (numSlices + 1) * (i + 1));
                *indexBuf++ = (j + i * (numSlices + 1));
                *indexBuf++ = (1 + (j + (numSlices + 1) * (i + 1)));
                *indexBuf++ = (1 + (j + (numSlices + 1) * (i + 1)));
                *indexBuf++ = (j + i * (numSlices + 1));
                *indexBuf++ = (1 + (j + i * (numSlices + 1)));
            }
        }
    }
    
    if (numVertices_out) {
        *numVertices_out = numVertices;
    }
    
    return numIndices;
}

int TTGLRenderES2_FTU::EsGenSphere_2(int numSlices, float radius, int *numVertices_out, int nType)
{
    int i;
    int j;
    int numParallels = numSlices / 2;
    int numVertices = (numParallels + 1) * (numSlices + 1);
    int numIndices = numParallels * numSlices * 6;
    float angleStep = 360.0/numSlices;
    
    // Allocate memory for buffers
    if (!vertices)
        vertices = (GLfloat *)malloc(sizeof(GLfloat) * 3 * numVertices);
    
    if (!texCoordsL)
        texCoordsL = (GLfloat *)malloc(sizeof(GLfloat) * 2 * numVertices);
    
    if(!texCoordsR)
        texCoordsR = (GLfloat *)malloc(sizeof(GLfloat) * 2 * numVertices);
    
    if (!indices)
        indices = (GLushort *)malloc(sizeof(GLuint) * numIndices);
    
    i = 0;
    j = 0;
    angleStep = 360.0/numSlices;
    for (float f1 = -90.0; f1 <= 90.0; f1 += angleStep)
    {
        float f2 = (float)sinf(M_PI * f1 / 180.0);
        float f3 = (float)cosf(M_PI * f1 / 180.0);
        for (float f4 = 0.0; f4 <= 360.0; f4 += angleStep)
        {
            float f5 = (float)cosf(M_PI * f4 / 180.0);
            float f6 = (float)sinf(M_PI * f4 / 180.0);
            float f7 = f4;
            vertices[i] = (f3 * f6 * radius);
            vertices[(i + 1)] = (f2 * radius);
            vertices[(i + 2)] = (f3 * f5 * radius);
            
            if(f7 < 90) {
                f7 = 90 - f7;
                if(nType & ERenderLR3D) {
                    texCoordsL[j] = 0;
                    texCoordsL[(j + 1)] = 0;
                    
                    texCoordsR[j] = 0;
                    texCoordsR[(j + 1)] = 0;
                    
                } else if(nType & ERenderUD3D) {
                    texCoordsL[j] = 0;
                    texCoordsL[(j + 1)] = 0;
                    
                    texCoordsR[j] = 0;
                    texCoordsR[(j + 1)] = 0;
                } else {
                    texCoordsL[j] = 0;
                    texCoordsL[(j + 1)] = 0;
                }
            } else if(f7 >= 90 && f7 <= 270) {
                f7 = f7 - 90;
                
                if(nType & ERenderLR3D) {
                    texCoordsL[j] = (1.0 - f7 / 180.0)*0.5;
                    texCoordsL[(j + 1)] = (1.0 - (90.0 + f1) / 180.0);
                    
                    texCoordsR[j] = 0.5 + texCoordsL[j];
                    texCoordsR[(j + 1)] = (1.0 - (90.0 + f1) / 180.0);
                    
                } else if(nType & ERenderUD3D) {
                    texCoordsL[j] = (1.0 - f7 / 180.0);
                    texCoordsL[(j + 1)] = (1.0 - (90.0 + f1) / 180.0)*0.5;
                    
                    texCoordsR[j] = (1.0 - f7 / 180.0);
                    texCoordsR[(j + 1)] = texCoordsL[(j + 1)] + 0.5;
                } else {
                    texCoordsL[j] = (1.0 - f7 / 180.0);
                    texCoordsL[(j + 1)] = (1.0 - (90.0 + f1) / 180.0);
                }
                
            } else {
                f7 = 360 - (f7 - 90);
                
                if(nType & ERenderLR3D) {
                    texCoordsL[j] = 0;
                    texCoordsL[(j + 1)] = 0;
                    
                    texCoordsR[j] = 0;
                    texCoordsR[(j + 1)] = 0;
                    
                } else if(nType & ERenderUD3D) {
                    texCoordsL[j] = 0;
                    texCoordsL[(j + 1)] = 0;
                    
                    texCoordsR[j] = 0;
                    texCoordsR[(j + 1)] = 0;
                } else {
                    texCoordsL[j] = 0;
                    texCoordsL[(j + 1)] = 0;
                }
            }
            
//            if(nType & ERenderLR3D) {
//                texCoordsL[j] = (1.0 - f7 / 180.0)*0.5;
//                texCoordsL[(j + 1)] = (1.0 - (90.0 + f1) / 180.0);
//                
//                texCoordsR[j] = 0.5 + texCoordsL[j];
//                texCoordsR[(j + 1)] = (1.0 - (90.0 + f1) / 180.0);
//                
//            } else if(nType & ERenderUD3D) {
//                texCoordsL[j] = (1.0 - f7 / 180.0);
//                texCoordsL[(j + 1)] = (1.0 - (90.0 + f1) / 180.0)*0.5;
//                
//                texCoordsR[j] = (1.0 - f7 / 180.0);
//                texCoordsR[(j + 1)] = texCoordsL[(j + 1)] + 0.5;
//            } else {
//                texCoordsL[j] = (1.0 - f7 / 180.0);
//                texCoordsL[(j + 1)] = (1.0 - (90.0 + f1) / 180.0);
//            }
            
            i += 3;
            j += 2;
        }
    }
    // Generate the vertex indices in the vertices array
    if (NULL != indices)
    {
        uint16_t *indexBuf = indices;
        for (i = 0; i < numParallels ; i++)
        {
            for (j = 0; j < numSlices; j++)
            {
                *indexBuf++ = (j + (numSlices + 1) * (i + 1));
                *indexBuf++ = (j + i * (numSlices + 1));
                *indexBuf++ = (1 + (j + (numSlices + 1) * (i + 1)));
                *indexBuf++ = (1 + (j + (numSlices + 1) * (i + 1)));
                *indexBuf++ = (j + i * (numSlices + 1));
                *indexBuf++ = (1 + (j + i * (numSlices + 1)));
            }
        }
    }
    
    if (numVertices_out) {
        *numVertices_out = numVertices;
    }
    
    return numIndices;
}

int TTGLRenderES2_FTU::FreeSphereContext()
{
    if(vertexBufferID != 0) {
        glDeleteBuffers(1, &vertexBufferID);
    }
    vertexBufferID = 0;
    
    if(vertexTexCoordID != 0) {
        glDeleteBuffers(1, &vertexTexCoordID);
    }
    vertexTexCoordID = 0;
    
    if(vertexIndicesBufferID != 0) {
        glDeleteBuffers(1, &vertexIndicesBufferID);
    }
    vertexIndicesBufferID = 0;
    
    if(mVertexArray != 0) {
        glDeleteVertexArraysOES(1, &mVertexArray);
    }
    mVertexArray = 0;

    
    return 0;
}

//Make sure be called after esGenSphere & CompileAllShaders
int TTGLRenderES2_FTU::SetupSphereContext()
{
    FreeSphereContext();
    
    glGenVertexArraysOES(1, &mVertexArray);
    glBindVertexArrayOES(mVertexArray);
    
    // 1
    glGenBuffers(1, &vertexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, numVertices*3*sizeof(GLfloat),
                 vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(m_nPositionSlot);
    glVertexAttribPointer(m_nPositionSlot, 3, GL_FLOAT,
                          GL_FALSE, sizeof(GLfloat)*3, NULL);
    
    // 2
    glGenBuffers(1, &vertexTexCoordID);
    glBindBuffer(GL_ARRAY_BUFFER, vertexTexCoordID);
    glBufferData(GL_ARRAY_BUFFER, numVertices*2*sizeof(GLfloat),
                 texCoordsL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(m_nTexCoordSlot);
    glVertexAttribPointer(m_nTexCoordSlot, 2, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat)*2, NULL);
    
    // 3
    glGenBuffers(1, &vertexIndicesBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIndicesBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices*1*sizeof(GLushort),
                 indices, GL_STATIC_DRAW);
    return TTKErrNone;
}

int TTGLRenderES2_FTU::SetupModelViewMatrix()
{
    if(mRenderType & ERenderGlobelView) {
        mHeadTransform->setHeadView(mHeadTracker->lastHeadView());

        float nScale = m_pVideoView.overRate;
        modelViewProjectionMatrix = GLKMatrix4MakeLookAt(0.0, 0.0, nScale, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
        modelViewProjectionMatrix = GLKMatrix4Scale(modelViewProjectionMatrix, nScale, nScale, nScale);
        float viewWidth = m_nBackingWidth;
        if(mRenderType & ERenderSplitView) {
            viewWidth *= 0.5;
        }
        
        float fRatio = viewWidth/m_nBackingHeight;
 

        GLKMatrix4  mFrustum = GLKMatrix4MakeFrustum( -1.0 * fRatio,
                                                     1.0 * fRatio,
                                                     -1.0,
                                                     1.0,
                                                     nScale,
                                                     100.0*nScale);
        
        modelViewProjectionMatrix = GLKMatrix4Multiply(mFrustum, modelViewProjectionMatrix);
        
        GLKMatrix4 headview = mHeadTransform->headView();
        
        modelViewProjectionMatrix = GLKMatrix4Multiply(modelViewProjectionMatrix, headview);
        
        float rotateDegreeX = m_pVideoView.fingerRotationX;
        GLKMatrix4 mRotateX = GLKMatrix4MakeXRotation(rotateDegreeX);
        modelViewProjectionMatrix = GLKMatrix4Multiply(modelViewProjectionMatrix, mRotateX);
    
        float rotateDegreeY = m_pVideoView.fingerRotationY;
        GLKMatrix4 mRotateY = GLKMatrix4MakeRotation(rotateDegreeY, 0, 1, 0);
    
        modelViewProjectionMatrix = GLKMatrix4Multiply(modelViewProjectionMatrix, mRotateY);
    } else {
        modelViewProjectionMatrix = GLKMatrix4Identity;
    }
    
    glUniformMatrix4fv(m_nTextureUniformViewProjectionMaxtrix, 1, GL_FALSE, modelViewProjectionMatrix.m);
    
    
    return TTKErrNone;
}

#pragma mark Protect function: GL setting.
int TTGLRenderES2_FTU::SetupFrameBuffer()
{
    int nRet = TTGLRenderES2::SetupFrameBuffer();

    if (TTKErrNone != nRet) {
        return nRet;
    }
    //Creates a new Core Video texture cache.
    CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, m_pContext, NULL, &videoTextureCache);
    if (err) {
        LOGE("Error at CVOpenGLESTextureCacheCreate %d", err);
        return TTKErrArgument;
    }
    
    return TTKErrNone;
}

int TTGLRenderES2_FTU::DeleteFrameBuffer()
{
    // Periodic texture cache flush every frame
    if (videoTextureCache) {
        CFRelease(videoTextureCache);
        videoTextureCache = 0;
    }
    
    return TTGLRenderES2::DeleteFrameBuffer();
}

bool TTGLRenderES2_FTU::IsGLRenderReady() {
    
    if (!TTGLRenderES2::IsGLRenderReady()) {
        return false;
    }
    
    if (0 == videoTextureCache) {
        return false;
    }
    
    return true;
}

int TTGLRenderES2_FTU::TextureSizeChange()
{
    return TTKErrNone;
}

int TTGLRenderES2_FTU::SetupTexture()
{
    return TTKErrNone;
}

int TTGLRenderES2_FTU::DeleteTexture()
{
    return TTKErrNone;
}

int TTGLRenderES2_FTU::CompileAllShaders()
{
    if (m_nProgramHandle) {
        glDeleteProgram(m_nProgramHandle);
        m_nProgramHandle = 0;
    }
    
    // Setup program
    m_nProgramHandle = glCreateProgram();
    if (0 == m_nProgramHandle) {
        LOGE("CompileAllShaders fail m_nProgramHandle:%d", m_nProgramHandle);
        return TTKErrArgument;
    }
    
    GLuint vertexShader = CompileShader(G_VERTEX_SHADER_Y_UV, GL_VERTEX_SHADER);
    GLuint fragmentShader = CompileShader(G_FRAGMENT_SHADER_Y_UV, GL_FRAGMENT_SHADER);
    
    if (0 == vertexShader || 0 == fragmentShader) {
        LOGE("CompileAllShaders fail vertexShader:%d, fragmentShader:%d", vertexShader, fragmentShader);
        return TTKErrArgument;
    }
    
    glAttachShader(m_nProgramHandle, vertexShader);
    glAttachShader(m_nProgramHandle, fragmentShader);
    glLinkProgram(m_nProgramHandle);
    
    // Link program
    GLint linkSuccess;
    glGetProgramiv(m_nProgramHandle, GL_LINK_STATUS, &linkSuccess);
    if (linkSuccess == GL_FALSE) {
        
        if (vertexShader) {
            glDetachShader(m_nProgramHandle, vertexShader);
            glDeleteShader(vertexShader);
        }
        if (fragmentShader) {
            glDetachShader(m_nProgramHandle, fragmentShader);
            glDeleteShader(fragmentShader);
        }
        return TTKErrArgument;
    }
    
    // Use Program
    glUseProgram(m_nProgramHandle);
    
    // Get Attrib
    m_nPositionSlot = glGetAttribLocation(m_nProgramHandle, "position");
    glEnableVertexAttribArray(m_nPositionSlot);
    
    m_nTexCoordSlot = glGetAttribLocation(m_nProgramHandle, "texCoord");
    glEnableVertexAttribArray(m_nTexCoordSlot);
    
    m_nTextureUniformY = glGetUniformLocation(m_nProgramHandle, "SamplerY");
    m_nTextureUniformUV = glGetUniformLocation(m_nProgramHandle, "SamplerUV");
    m_nTextureUniformRGB = glGetUniformLocation(m_nProgramHandle, "SamplerRGB");
    m_nTextureUniformViewProjectionMaxtrix = glGetUniformLocation(m_nProgramHandle, "modelViewProjectionMatrix");
    
    m_nFragmentSphere = glGetUniformLocation(m_nProgramHandle, "fsphere");
    
    // Release vertex and fragment shaders.
    if (vertexShader) {
        glDetachShader(m_nProgramHandle, vertexShader);
        glDeleteShader(vertexShader);
    }
    if (fragmentShader) {
        glDetachShader(m_nProgramHandle, fragmentShader);
        glDeleteShader(fragmentShader);
    }
    
    // Wrong init
    if (m_nTextureUniformY == m_nTextureUniformUV) {
        
        LOGE("Error Y:%d, UV:%d", m_nTextureUniformY, m_nTextureUniformUV);
        return TTKErrArgument;
    }
    
    LOGI("m_nPositionSlot:%d, m_nTexCoordSlot:%d, Y:%d, UV:%d", m_nPositionSlot, m_nTexCoordSlot, m_nTextureUniformY, m_nTextureUniformUV);
    
    if(mRenderType & ERender180View) {
        numIndices = EsGenSphere_2(120, 1.0f, &numVertices, mRenderType);
    } else {
        numIndices = EsGenSphere(120, 1.0f, &numVertices, mRenderType);
    }
    
    if(mRenderType & ERenderGlobelView) {
        SetupSphereContext();
    }
    
    if(mMotionEnable) {
        StartDeviceMotion();
    }
    
    glBindVertexArrayOES(0);

    return TTKErrNone;
}

void TTGLRenderES2_FTU::setRendType(TTInt aRenderType)
{
    if((aRenderType & ERenderGlobelView) || (aRenderType & ERender180View)){
        if(aRenderType & ERender180View) {
            numIndices = EsGenSphere_2(120, 1.0f, &numVertices, aRenderType);
        } else {
            numIndices = EsGenSphere(120, 1.0f, &numVertices, aRenderType);
        }
    
        if(aRenderType & ERenderGlobelView) {
            SetupSphereContext();
        }
    } else {
        FreeSphereContext();
    }
    
    mRenderType = aRenderType;
}

int TTGLRenderES2_FTU::Flush()
{
    if (_lumaTexture)
    {
        CFRelease(_lumaTexture);
        _lumaTexture = NULL;
    }
    
    if (_chromaTexture)
    {
        CFRelease(_chromaTexture);
        _chromaTexture = NULL;
    }
    
    return TTKErrNone;
}

void TTGLRenderES2_FTU::setMotionEnable(bool aEnable)
{
    if(mMotionEnable != aEnable) {
        if(aEnable) {
            StartDeviceMotion();
        } else {
            StopDeviceMotion();
        }
    }
    
    mMotionEnable = aEnable;
}

#pragma mark Protect function: GL Draw
int TTGLRenderES2_FTU::UploadTexture(TTVideoBuffer *pVideoBuffer)
{
    CVImageBufferRef pixelBuffer = (CVImageBufferRef)pVideoBuffer->Buffer[0];
    if (NULL == pixelBuffer) {
        return TTKErrArgument;
    }
    
    if (_lumaTexture)
    {
        CFRelease(_lumaTexture);
        _lumaTexture = NULL;
    }
    
    if (_chromaTexture)
    {
        CFRelease(_chromaTexture);
        _chromaTexture = NULL;
    }
    
    // Periodic texture cache flush every frame
    CVOpenGLESTextureCacheFlush(videoTextureCache, 0);
    
    // Create a CVOpenGLESTexture from the CVImageBuffe
    
    CVReturn err = 0;
    glActiveTexture(GL_TEXTURE0);
    err = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                       videoTextureCache,
                                                       pixelBuffer,
                                                       NULL,
                                                       GL_TEXTURE_2D,
                                                       GL_RED_EXT,
                                                       m_nTextureWidth,
                                                       m_nTextureHeight,
                                                       GL_RED_EXT,
                                                       GL_UNSIGNED_BYTE,
                                                       0,
                                                       &_lumaTexture);
    if (err)
    {
        LOGE("Error at CVOpenGLESTextureCacheCreateTextureFromImage %d", err);
    }
    
    glBindTexture(CVOpenGLESTextureGetTarget(_lumaTexture), CVOpenGLESTextureGetName(_lumaTexture));
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // UV-plane
    glActiveTexture(GL_TEXTURE1);
    err = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                       videoTextureCache,
                                                       pixelBuffer,
                                                       NULL,
                                                       GL_TEXTURE_2D,
                                                       GL_RG_EXT,
                                                       m_nTextureWidth/2,
                                                       m_nTextureHeight/2,
                                                       GL_RG_EXT,
                                                       GL_UNSIGNED_BYTE,
                                                       1,
                                                       &_chromaTexture);
    if (err)
    {
        LOGE("Error at CVOpenGLESTextureCacheCreateTextureFromImage %d", err);
    }
    
    glBindTexture(CVOpenGLESTextureGetTarget(_chromaTexture), CVOpenGLESTextureGetName(_chromaTexture));
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    return TTKErrNone;
}

int TTGLRenderES2_FTU::RenderToScreen()
{
    SetupModelViewMatrix();
    
    glUniform1i(m_nTextureUniformY, 0);
    glUniform1i(m_nTextureUniformUV, 1);
    
    // Update attribute values.
    glUniform1f(m_nFragmentSphere, 1.0f);
    
    if(mRenderType & ERenderSplitView) {
        
        if(mRenderType & ERenderMeshDistortion) {
            glEnable(GL_SCISSOR_TEST);
            mLeftEye->viewport()->setGLViewport();
            mLeftEye->viewport()->setGLScissor();
        } else {
            glViewport (0, 0, m_nBackingWidth*0.5, m_nBackingHeight);
        }
        
        if(mRenderType & ERenderGlobelView) {
            if(mRenderType & (ERenderLR3D | ERenderUD3D)) {
                glBufferData(GL_ARRAY_BUFFER, numVertices*2*sizeof(GLfloat),
                             texCoordsL, GL_DYNAMIC_DRAW);
            }
            glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, 0);
        } else {
            glVertexAttribPointer(m_nPositionSlot, 2, GL_FLOAT, 0, 0, m_fSquareVertices);
            glVertexAttribPointer(m_nTexCoordSlot, 2, GL_FLOAT, 0, 0, m_fTextureVertices);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        
        if(mRenderType & ERenderMeshDistortion) {
            mRightEye->viewport()->setGLViewport();
            mRightEye->viewport()->setGLScissor();
        } else {
            glViewport (m_nBackingWidth*0.5, 0, m_nBackingWidth*0.5, m_nBackingHeight);
        }
        
        if(mRenderType & ERenderGlobelView) {
            if(mRenderType & (ERenderLR3D | ERenderUD3D)) {
                glBufferData(GL_ARRAY_BUFFER, numVertices*2*sizeof(GLfloat),
                             texCoordsR, GL_DYNAMIC_DRAW);
            }
            glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, 0);
        } else {
            glVertexAttribPointer(m_nPositionSlot, 2, GL_FLOAT, 0, 0, m_fSquareVertices);
            glVertexAttribPointer(m_nTexCoordSlot, 2, GL_FLOAT, 0, 0, m_fTextureVertices);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    } else {
        glViewport (0, 0, m_nBackingWidth, m_nBackingHeight);
        if(mRenderType & ERenderGlobelView) {
            glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, 0);
        } else {
            glVertexAttribPointer(m_nPositionSlot, 2, GL_FLOAT, 0, 0, m_fSquareVertices);
            glVertexAttribPointer(m_nTexCoordSlot, 2, GL_FLOAT, 0, 0, m_fTextureVertices);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }
    
    return TTKErrNone;
}

int TTGLRenderES2_FTU::RedrawInner(bool bIsTryGetFrame, void *pData)
{
    GKCAutoLock cAuto(&mCritical);
    
    if (!IsGLRenderReady()) {
        LOGW("RenderYUV not ready");
        return TTKErrNotReady;
    }
    
    if(_lumaTexture == NULL || _chromaTexture == NULL){
        return TTKErrNotReady;
    }
    
    if (![EAGLContext setCurrentContext:m_pContext]) {
        return TTKErrArgument;
    }
    
    if(mRenderType & ERenderSplitView) {
        if(mRenderType & ERenderMeshDistortion) {
            mDistortionRenderer->beforeDrawFrame();
    
            UseProgram();
    
            CVReturn err = 0;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(CVOpenGLESTextureGetTarget(_lumaTexture), CVOpenGLESTextureGetName(_lumaTexture));
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
            // UV-plane
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(CVOpenGLESTextureGetTarget(_chromaTexture), CVOpenGLESTextureGetName(_chromaTexture));
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
            RenderToScreen();
    
            if(mRenderType & ERenderGlobelView) {
                glBindVertexArrayOES(0);
            }
    
            mMonocularEye->viewport()->setGLViewport();
            mMonocularEye->viewport()->setGLScissor();
    
            BindFrameRenderBuffer();
    
            mDistortionRenderer->afterDrawFrame();
    
            RenderCommit();
    
            glUseProgram(0);
        } else {
            UseProgram();
            RenderToScreen();
            BindFrameRenderBuffer();
            RenderCommit();
        }
    } else {
        UseProgram();
        RenderToScreen();
        BindFrameRenderBuffer();
        RenderCommit();
    }

    return TTKErrNone;
}

void TTGLRenderES2_FTU::StartDeviceMotion()
{
    if(mMotionEnable) {
        StopDeviceMotion();
    }
    
    mHeadTracker->startTracking([UIApplication sharedApplication].statusBarOrientation);
    mMagnetSensor->start();
}

void TTGLRenderES2_FTU::StopDeviceMotion()
{
    if(!mMotionEnable) {
        return;
    }
    
    mHeadTracker->stopTracking();
    mMagnetSensor->stop();
}



