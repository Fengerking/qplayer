package com.goku.media.player;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import com.goku.media.cardboard.sensors.*;
import com.goku.media.cardboard.proto.*;
import com.goku.media.cardboard.*;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;


@SuppressLint("ViewConstructor")
public class VideoSurfaceView extends GLSurfaceView {
	private static String TAG = "VideoSurfaceView";
    VideoRender mRenderer;
    private static EventHandler mEventHandler;

    
    private OnGLSurfaceViewEventListener mGLSurfaceViewEventListener;
    
    /**
     * Ӳ������
     */
    public static final int GLSURFACEVIEW_CREATE = 0x00;
    /**
     * �������
     */
    public static final int GLSURFACEVIEW_CHANGED= 0x01;

    
    /**
     * Standard View constructor. In order to render something, you
     * must call {@link #setRenderer} to register a renderer.
     */
    public VideoSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        mRenderer = new VideoRender(context);
        setRenderer(mRenderer);
        
        mEventHandler = new EventHandler(this, Looper.myLooper());
    }

    public VideoSurfaceView(Context context) {
        super(context);

        setEGLContextClientVersion(2);
        mRenderer = new VideoRender(context);
        setRenderer(mRenderer);
        
        mEventHandler = new EventHandler(this, Looper.myLooper());
    }

    public void onResume() {
        super.onResume();
        mRenderer.onResume();
    }

    public void onPause() {
        super.onPause();
        mRenderer.onPause();
    }

    public void startRender() {
        mRenderer.startRender();
    }

    public void setChangeXY(float x, float y) { mRenderer.setChangeXY(x, y); }

    public void setRenderType(int renderType) {
        mRenderer.setRenderType(renderType);
    }

    public Surface getSurface() {
        return mRenderer.getSurface();
    }
    
    
    public void setGLSurfaceViewEvenListener(OnGLSurfaceViewEventListener aListener) {
    	mGLSurfaceViewEventListener = aListener;
    }
    
    private class EventHandler extends Handler {
        public EventHandler(VideoSurfaceView mp, Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message aMsg) {
            if (mGLSurfaceViewEventListener != null) {
            	mGLSurfaceViewEventListener.onGLSurfaceViewNotify(aMsg.what, aMsg.arg1, aMsg.arg2);
                return;
            }
        }
    }
    
    public interface OnGLSurfaceViewEventListener {

        void onGLSurfaceViewNotify(int aMsgId, int aArg1, int aArg2);
    }
    
    private static class VideoRender
        implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {

        private static final int FLOAT_SIZE_BYTES = 4;
        private static final int TRIANGLE_VERTICES_DATA_STRIDE_BYTES = 5 * FLOAT_SIZE_BYTES;
        private static final int TRIANGLE_VERTICES_DATA_POS_OFFSET = 0;
        private static final int TRIANGLE_VERTICES_DATA_UV_OFFSET = 3;
        private final float[] mTriangleVerticesData = {
            // X, Y, Z, U, V
            -1.0f, -1.0f, 0, 0.f, 0.f,
            1.0f, -1.0f, 0, 1.0f, 0.f,
            -1.0f,  1.0f, 0, 0.f, 1.0f,
            1.0f,  1.0f, 0, 1.0f, 1.0f,
        };

        private FloatBuffer mTriangleVertices;

        private final String mVertexShader =
                "uniform mat4 uMVPMatrix;\n" +
                "uniform mat4 uSTMatrix;\n" +
                "attribute vec4 aPosition;\n" +
                "attribute vec4 aTextureCoord;\n" +
                "varying vec2 vTextureCoord;\n" +
                "void main() {\n" +
                "  gl_Position = uMVPMatrix * aPosition;\n" +
                "  vTextureCoord = (uSTMatrix * aTextureCoord).xy;\n" +
                "}\n";

        private final String mFragmentShader =
                "#extension GL_OES_EGL_image_external : require\n" +
                "precision mediump float;\n" +
                "varying vec2 vTextureCoord;\n" +
                "uniform samplerExternalOES sTexture;\n" +
                "void main() {\n" +
                "  gl_FragColor = texture2D(sTexture, vTextureCoord);\n" +
                "}\n";

        private float[] mMVPMatrix = new float[16];
        private float[] mSTMatrix = new float[16];

        private int mProgram;
        private int mTextureID;
        private int muMVPMatrixHandle;
        private int muSTMatrixHandle;
        private int maPositionHandle;
        private int maTextureHandle;
        private float mXOffSet = 0.0f;
        private float mYOffSet = 0.0f;

        private SurfaceTexture mSurfaceTexture;
        private Surface mSurface = null;
        private boolean updateSurface = false;

        private HeadTracker mHeadTracker;
        private HeadMountedDisplayManager mHmdManager;
        private final HeadTransform mHeadTransform;
        private final Eye mMonocular;
        private final Eye mLeftEye;
        private final Eye mRightEye;
        private final float[] mLeftEyeTranslate = new float[16];
        private final float[] mRightEyeTranslate = new float[16];
        private boolean mSurfaceCreated;
        private HeadMountedDisplay mHmd;
        private DistortionRenderer mDistortionRenderer;
        private boolean mVRMode;
        private boolean mDistortionCorrectionEnabled;
        private boolean mProjectionChanged;
        private boolean mInvalidSurfaceSize;

        public static final float MAX_Y_ANGLE = 90.0F;
        public static final float MIN_Y_ANGLE = -90.0F;
        private final int COORDS_PER_VERTEX = 3;
        private final int LATITUDE = 180;
        private final int LONGITUDE = 360;
        private final String TAG = "Sphere";
        private int bufferSize = 703;
        private int[] index;
        private IntBuffer indexBuffer;
        private float radius = 2.5F;
        private float[] scale = { 1.0F, 1.0F, 1.0F };
        private boolean single = true;
        private final int step = 10;
        private FloatBuffer texCoordBuffer;
        private FloatBuffer texCoordBufferR;
        private float[] texCoords = null;
        private float[] texCoordsR = null;
        private int[] textureIds = new int[2];
        private FloatBuffer vertexBuffer;
        private int[] vertexIds = new int[3];
        private int vertex_count;
        private int viewWidth = 0;
        private int viewHeight = 0;
        private float fWidth;
        private float fHeight;
        private int nInit = 0;
        private int nRendyType = ERenderSplitView;
        private int nLastType = 0;
        private float[] vertice;
        float[] mHeadView = new float[16];

        float[] mViewMatrix = new float[16];
        float[] mXRotationMatrix = new float[16];
        float[] mYRotationMatrix = new float[16];
        float[] mProjectionMatrix = new float[16];


        private static int GL_TEXTURE_EXTERNAL_OES = 0x8D65;

        public VideoRender(Context context) {
            mTriangleVertices = ByteBuffer.allocateDirect(
                mTriangleVerticesData.length * FLOAT_SIZE_BYTES)
                    .order(ByteOrder.nativeOrder()).asFloatBuffer();
            mTriangleVertices.put(mTriangleVerticesData).position(0);

            this.mHeadTracker = HeadTracker.createFromContext(context);
            this.mHmdManager = new HeadMountedDisplayManager(context);

            this.mHmd = new HeadMountedDisplay(mHmdManager.getHeadMountedDisplay());
            this.mHeadTransform = new HeadTransform();
            this.mMonocular = new Eye(0);
            this.mLeftEye = new Eye(1);
            this.mRightEye = new Eye(2);
            this.updateFieldOfView(this.mLeftEye.getFov(), this.mRightEye.getFov());
            (this.mDistortionRenderer = new DistortionRenderer()).setRestoreGLStateEnabled(true);
            this.mDistortionRenderer.setChromaticAberrationCorrectionEnabled(true);
            this.mDistortionRenderer.setVignetteEnabled(true);

            Matrix.setIdentityM(mSTMatrix, 0);
            initBuffer(120);
        }

        public void setChangeXY(float x, float y)
        {
            mXOffSet += x;
            mYOffSet += y;
        };

        public void setCardboardDeviceParams(final CardboardDeviceParams newParams) {
            final CardboardDeviceParams deviceParams = new CardboardDeviceParams(newParams);
            this.mHmd.setCardboardDeviceParams(deviceParams);
            this.mProjectionChanged = true;
        }

        public void updateCardboardDeviceParams(final CardboardDeviceParams cardboardDeviceParams) {
            if (this.mHmdManager.updateCardboardDeviceParams(cardboardDeviceParams)) {
                setCardboardDeviceParams(this.getCardboardDeviceParams());
            }
        }

        public CardboardDeviceParams getCardboardDeviceParams() {
            return this.mHmdManager.getHeadMountedDisplay().getCardboardDeviceParams();
        }


        public void onResume() {
            this.mHmdManager.onResume();
            setCardboardDeviceParams(this.getCardboardDeviceParams());
            final Phone.PhoneParams phoneParams = PhoneParams.readFromExternalStorage();
            if (phoneParams != null) {
                this.mHeadTracker.setGyroBias(phoneParams.gyroBias);
            }
            this.mHeadTracker.startTracking();
        }

        public void onPause() {
            this.mHmdManager.onPause();
            this.mHeadTracker.stopTracking();
        }

        public void setRenderType(int renderType)
        {
            nRendyType = renderType;
        }

        /**
         * Returns the Surface.
         */
        public Surface getSurface() {
            return mSurface;
        }

        public void startRender() {
            nInit = 1;
        }
        
        @Override
        public void onDrawFrame(GL10 glUnused) {

            if(viewWidth == 0 || viewHeight == 0 || nInit == 0) {
                return;
            }

            synchronized(this) {
                if (updateSurface) {
                	mSurfaceTexture.updateTexImage();
                	mSurfaceTexture.getTransformMatrix(mSTMatrix);
                    updateSurface = false;
                }
                
            }

            boolean nGlobel = ((nRendyType & ERenderGlobelView) > 0) || ((nRendyType & ERender180View) > 0);
            if(nLastType != nRendyType) {
                if(nGlobel){
                    if((nRendyType & ERender180View) > 0) {
                        UpdateBuffer180(120, nRendyType);
                    } else {
                        UpdateBuffer360(120, nRendyType);
                    }

                    if((nRendyType & ERenderGlobelView) > 0) {
                        createVBO(vertexIds, vertice, index, texCoords, vertexBuffer, indexBuffer, texCoordBuffer);
                    }
                } else {
                    destroyVBO(vertexIds);
                }

                mProjectionChanged = true;
                nLastType = nRendyType;

                if((nRendyType&ERenderSplitView) == 0) {
                    mVRMode = false;
                } else {
                    mVRMode = true;
                }

                if((nRendyType&ERenderMeshDistortion) == 0) {
                    mDistortionCorrectionEnabled = false;
                } else {
                    mDistortionCorrectionEnabled = true;
                }
            }

            this.getFrameParams(this.mHeadTransform, this.mLeftEye, this.mRightEye, this.mMonocular);


            //Log.i(TAG, "nGlobel = " +  nGlobel + " nRendyType = " + nRendyType);

            GLES20.glDisable(GLES20.GL_SCISSOR_TEST);

            if(!nGlobel) {
                GLES20.glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
                GLES20.glClear( GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);

                GLES20.glUseProgram(mProgram);
                checkGlError("glUseProgram");

                GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
                GLES20.glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureID);

                mTriangleVertices.position(TRIANGLE_VERTICES_DATA_POS_OFFSET);
                GLES20.glVertexAttribPointer(maPositionHandle, 3, GLES20.GL_FLOAT, false,
                        TRIANGLE_VERTICES_DATA_STRIDE_BYTES, mTriangleVertices);
                checkGlError("glVertexAttribPointer maPosition");
                GLES20.glEnableVertexAttribArray(maPositionHandle);
                checkGlError("glEnableVertexAttribArray maPositionHandle");

                mTriangleVertices.position(TRIANGLE_VERTICES_DATA_UV_OFFSET);
                GLES20.glVertexAttribPointer(maTextureHandle, 3, GLES20.GL_FLOAT, false,
                        TRIANGLE_VERTICES_DATA_STRIDE_BYTES, mTriangleVertices);
                checkGlError("glVertexAttribPointer maTextureHandle");
                GLES20.glEnableVertexAttribArray(maTextureHandle);
                checkGlError("glEnableVertexAttribArray maTextureHandle");
                Matrix.setIdentityM(mMVPMatrix, 0);

                GLES20.glUniformMatrix4fv(muMVPMatrixHandle, 1, false, mMVPMatrix, 0);
                GLES20.glUniformMatrix4fv(muSTMatrixHandle, 1, false, mSTMatrix, 0);

                if((nRendyType & ERenderSplitView) > 0) {
                    GLES20.glViewport(0, 0, viewWidth/2, viewHeight);
                    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
                    GLES20.glViewport(viewWidth/2, 0, viewWidth/2, viewHeight);
                    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
                } else {
                    GLES20.glViewport(0, 0, viewWidth, viewHeight);
                    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
                }
                checkGlError("glDrawArrays");
            } else {
                bindVBO(vertexIds);

                mHeadTransform.getHeadView(mHeadView, 0);
                SetupModelViewMatrix();

                if((nRendyType & ERenderSplitView) > 0) {
                    if((nRendyType&ERenderMeshDistortion) > 0) {

                        this.mDistortionRenderer.beforeDrawFrame();

                        //GLES20.glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
                        //GLES20.glClear( GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);

                        GLES20.glUseProgram(mProgram);
                        checkGlError("glUseProgram");

                        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
                        GLES20.glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureID);

                        GLES20.glUniformMatrix4fv(muMVPMatrixHandle, 1, false, mMVPMatrix, 0);
                        GLES20.glUniformMatrix4fv(muSTMatrixHandle, 1, false, mSTMatrix, 0);

                        GLES20.glEnable(GLES20.GL_SCISSOR_TEST);
                        mLeftEye.getViewport().setGLViewport();
                        mLeftEye.getViewport().setGLScissor();

                        GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, 4 * texCoords.length, texCoordBuffer, GLES20.GL_STATIC_DRAW);

                        GLES20.glDrawElements(GLES20.GL_TRIANGLES, vertex_count, GLES20.GL_UNSIGNED_INT, 0);

                        mRightEye.getViewport().setGLViewport();
                        mRightEye.getViewport().setGLScissor();

                        if ((nRendyType & (ERenderLR3D | ERenderUD3D)) > 0) {
                            GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, 4 * texCoordsR.length, texCoordBufferR, GLES20.GL_STATIC_DRAW);
                        }

                        GLES20.glDrawElements(GLES20.GL_TRIANGLES, vertex_count, GLES20.GL_UNSIGNED_INT, 0);

                        this.mDistortionRenderer.afterDrawFrame();

                    } else {
                        GLES20.glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
                        GLES20.glClear( GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);

                        GLES20.glUseProgram(mProgram);
                        checkGlError("glUseProgram");

                        GLES20.glUniformMatrix4fv(muMVPMatrixHandle, 1, false, mMVPMatrix, 0);
                        GLES20.glUniformMatrix4fv(muSTMatrixHandle, 1, false, mSTMatrix, 0);


                        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
                        GLES20.glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureID);

                        GLES20.glViewport(0, 0, viewWidth / 2, viewHeight);

                        GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, 4 * texCoords.length, texCoordBuffer, GLES20.GL_STATIC_DRAW);

                        GLES20.glDrawElements(GLES20.GL_TRIANGLES, vertex_count, GLES20.GL_UNSIGNED_INT, 0);

                        GLES20.glViewport(viewWidth / 2, 0, viewWidth / 2, viewHeight);

                        if ((nRendyType & (ERenderLR3D | ERenderUD3D)) > 0) {
                            GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, 4 * texCoordsR.length, texCoordBufferR, GLES20.GL_STATIC_DRAW);
                        }

                        GLES20.glDrawElements(GLES20.GL_TRIANGLES, vertex_count, GLES20.GL_UNSIGNED_INT, 0);
                    }
                } else {
                    GLES20.glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
                    GLES20.glClear( GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);

                    GLES20.glUseProgram(mProgram);
                    checkGlError("glUseProgram");

                    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
                    GLES20.glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureID);

                    GLES20.glUniformMatrix4fv(muMVPMatrixHandle, 1, false, mMVPMatrix, 0);
                    GLES20.glUniformMatrix4fv(muSTMatrixHandle, 1, false, mSTMatrix, 0);

                    GLES20.glViewport(0, 0, viewWidth, viewHeight);
                    GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, 4 * texCoords.length, texCoordBuffer, GLES20.GL_STATIC_DRAW);

                    GLES20.glDrawElements(GLES20.GL_TRIANGLES, vertex_count, GLES20.GL_UNSIGNED_INT, 0);
                }

                checkGlError("glDrawElements");
            }

            GLES20.glFinish();
        }

        @Override
        public void onSurfaceChanged(GL10 glUnused, int width, int height) {
        	Log.i(TAG, "onSurfaceChanged :width" +  width + "height" + height);
            viewWidth = width;
            viewHeight = height;

            fWidth = viewWidth;
            fHeight = viewHeight;

            GLES20.glViewport(0, 0, width, height);
        	
        	if (mEventHandler != null) {
                Message m = mEventHandler.obtainMessage(GLSURFACEVIEW_CHANGED, width, height);
                mEventHandler.sendMessage(m);
            }
        }

        public void initBuffer(int numSlices)
        {
            int i = 0;
            int j = 0;
            int k = 0;

            int numParallels = numSlices / 2;
            int numVertices = (numParallels + 1) * (numSlices + 1);
            int numIndices = numParallels * numSlices * 6;
            float angleStep = 360.0F/numSlices;

            bufferSize = numVertices;

            vertice = new float[3 * bufferSize];
            texCoords = new float[2 * bufferSize];
            texCoordsR = new float[2 * bufferSize];
            index = new int[6 * bufferSize];
            Log.i("info", "alloc vertice lenth " + vertice.length);
            vertexBuffer = ByteBuffer.allocateDirect(4 * vertice.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
            texCoordBuffer = ByteBuffer.allocateDirect(4 * texCoords.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
            texCoordBufferR = ByteBuffer.allocateDirect(4 * texCoordsR.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
            indexBuffer = ByteBuffer.allocateDirect(4 * index.length).order(ByteOrder.nativeOrder()).asIntBuffer();

            for (float f1 = -90.0F; f1 <= 90.0F; f1 += angleStep)
            {
                float f2 = (float)Math.sin(Math.PI * f1 / 180.0D);
                float f3 = (float)Math.cos(Math.PI * f1 / 180.0D);
                for (float f4 = 0.0F; f4 <= 360.0F; f4 += angleStep)
                {
                    float f5 = (float)Math.cos(Math.PI * f4 / 180.0D);
                    float f6 = (float)Math.sin(Math.PI * f4 / 180.0D);
                    vertice[i] = f3 * f6;
                    vertice[(i + 1)] = f2 ;
                    vertice[(i + 2)] = f3 * f5;
                    texCoords[j] = (1.0F - f4 / 360.0F);
                    texCoords[(j + 1)] = (90.0F + f1) / 180.0F;
                    i += 3;
                    j += 2;
                }
            }

            for (int m = 0; m < numParallels; m++) {
                for (int n = 0; n < numSlices; n++) {
                    index[k] = (n + (numSlices + 1) * (m + 1));
                    index[(k + 1)] = (n + m * (numSlices + 1));
                    index[(k + 2)] = (1 + (n + (numSlices + 1) * (m + 1)));
                    index[(k + 3)] = (1 + (n + (numSlices + 1) * (m + 1)));
                    index[(k + 4)] = (n + m * (numSlices + 1));
                    index[(k + 5)] = (1 + (n + m * (numSlices + 1)));
                    k += 6;
                }
            }
            vertex_count = k;
            vertexBuffer.put(vertice).position(0);
            texCoordBuffer.put(texCoords).position(0);
            indexBuffer.put(index).position(0);
        }

        public void UpdateBuffer360(int numSlices, int aRendyType)
        {
            int i = 0;
            int j = 0;
            int k = 0;

            int numParallels = numSlices / 2;
            int numVertices = (numParallels + 1) * (numSlices + 1);
            int numIndices = numParallels * numSlices * 6;
            float angleStep = 360.0F/numSlices;

            bufferSize = numVertices;

            for (float f1 = -90.0F; f1 <= 90.0F; f1 += angleStep)
            {
                float f2 = (float)Math.sin(Math.PI * f1 / 180.0D);
                float f3 = (float)Math.cos(Math.PI * f1 / 180.0D);
                for (float f4 = 0.0F; f4 <= 360.0F; f4 += angleStep)
                {
                    float f5 = (float)Math.cos(Math.PI * f4 / 180.0D);
                    float f6 = (float)Math.sin(Math.PI * f4 / 180.0D);
                    vertice[i] = f3 * f6;
                    vertice[(i + 1)] = f2 ;
                    vertice[(i + 2)] = f3 * f5;

                    if((aRendyType & ERenderLR3D) > 0) {
                        texCoords[j] = (1.0F - f4 / 360.0F)*0.5F;
                        texCoords[(j + 1)] = ((90.0F + f1) / 180.0F);

                        texCoordsR[j] = 0.5F + texCoords[j];
                        texCoordsR[(j + 1)] = ((90.0F + f1) / 180.0F);

                    } else if((aRendyType & ERenderUD3D) > 0) {
                        texCoords[j] = (1.0F - f4 / 360.0F);
                        texCoords[(j + 1)] = ((90.0F + f1) / 180.0F)*0.5F;

                        texCoordsR[j] = (1.0F - f4 / 360.0F);
                        texCoordsR[(j + 1)] = texCoords[(j + 1)] + 0.5F;
                    } else {
                        texCoords[j] = (1.0F - f4 / 360.0F);
                        texCoords[(j + 1)] = (90.0F + f1) / 180.0F;
                    }

                    i += 3;
                    j += 2;
                }
            }

            for (int m = 0; m < numParallels; m++) {
                for (int n = 0; n < numSlices; n++) {
                    index[k] = (n + (numSlices + 1) * (m + 1));
                    index[(k + 1)] = (n + m * (numSlices + 1));
                    index[(k + 2)] = (1 + (n + (numSlices + 1) * (m + 1)));
                    index[(k + 3)] = (1 + (n + (numSlices + 1) * (m + 1)));
                    index[(k + 4)] = (n + m * (numSlices + 1));
                    index[(k + 5)] = (1 + (n + m * (numSlices + 1)));
                    k += 6;
                }
            }
            vertex_count = k;
            vertexBuffer.put(vertice).position(0);
            texCoordBuffer.put(texCoords).position(0);
            texCoordBufferR.put(texCoordsR).position(0);
            indexBuffer.put(index).position(0);
        }

        public void UpdateBuffer180(int numSlices, int aRendyType)
        {
            int i = 0;
            int j = 0;
            int k = 0;

            int numParallels = numSlices / 2;
            int numVertices = (numParallels + 1) * (numSlices + 1);
            int numIndices = numParallels * numSlices * 6;
            float angleStep = 360.0F/numSlices;

            bufferSize = numVertices;

            for (float f1 = -90.0F; f1 <= 90.0F; f1 += angleStep)
            {
                float f2 = (float)Math.sin(Math.PI * f1 / 180.0D);
                float f3 = (float)Math.cos(Math.PI * f1 / 180.0D);
                for (float f4 = 0.0F; f4 <= 360.0F; f4 += angleStep)
                {
                    float f5 = (float)Math.cos(Math.PI * f4 / 180.0D);
                    float f6 = (float)Math.sin(Math.PI * f4 / 180.0D);
                    float f7 = f4;

                    vertice[i] = f3 * f6;
                    vertice[(i + 1)] = f2 ;
                    vertice[(i + 2)] = f3 * f5;
                    if(f7 < 90) {
                        f7 = 90 - f7;
                        if((aRendyType & ERenderLR3D) > 0) {
                            texCoords[j] = 0;
                            texCoords[(j + 1)] = 0;

                            texCoordsR[j] = 0;
                            texCoordsR[(j + 1)] = 0;

                        } else if((aRendyType & ERenderUD3D) > 0) {
                            texCoords[j] = 0;
                            texCoords[(j + 1)] = 0;

                            texCoordsR[j] = 0;
                            texCoordsR[(j + 1)] = 0;
                        } else {
                            texCoords[j] = 0;
                            texCoords[(j + 1)] = 0;
                        }
                    } else if(f7 >= 90 && f7 <= 270) {
                        f7 = f7 - 90;

                        if((aRendyType & ERenderLR3D) > 0) {
                            texCoords[j] = (1.0F - f7 / 180.0F)*0.5F;
                            texCoords[(j + 1)] = ((90.0F + f1) / 180.0F);

                            texCoordsR[j] = 0.5F + texCoords[j];
                            texCoordsR[(j + 1)] = ((90.0F + f1) / 180.0F);

                        } else if((aRendyType & ERenderUD3D) > 0) {
                            texCoords[j] = (1.0F - f7 / 180.0F);
                            texCoords[(j + 1)] = ((90.0F + f1) / 180.0F)*0.5F;

                            texCoordsR[j] = (1.0F - f7 / 180.0F);
                            texCoordsR[(j + 1)] = texCoords[(j + 1)] + 0.5F;
                        } else {
                            texCoords[j] = (1.0F - f7 / 180.0F);
                            texCoords[(j + 1)] = ((90.0F + f1) / 180.0F);
                        }

                    } else {
                        f7 = 360 - (f7 - 90);

                        if((aRendyType & ERenderLR3D) > 0) {
                            texCoords[j] = 0;
                            texCoords[(j + 1)] = 0;

                            texCoordsR[j] = 0;
                            texCoordsR[(j + 1)] = 0;

                        } else if((aRendyType & ERenderUD3D) > 0) {
                            texCoords[j] = 0;
                            texCoords[(j + 1)] = 0;

                            texCoordsR[j] = 0;
                            texCoordsR[(j + 1)] = 0;
                        } else {
                            texCoords[j] = 0;
                            texCoords[(j + 1)] = 0;
                        }
                    }

                    i += 3;
                    j += 2;
                }
            }

            for (int m = 0; m < numParallels; m++) {
                for (int n = 0; n < numSlices; n++) {
                    index[k] = (n + (numSlices + 1) * (m + 1));
                    index[(k + 1)] = (n + m * (numSlices + 1));
                    index[(k + 2)] = (1 + (n + (numSlices + 1) * (m + 1)));
                    index[(k + 3)] = (1 + (n + (numSlices + 1) * (m + 1)));
                    index[(k + 4)] = (n + m * (numSlices + 1));
                    index[(k + 5)] = (1 + (n + m * (numSlices + 1)));
                    k += 6;
                }
            }
            vertex_count = k;
            vertexBuffer.put(vertice).position(0);
            texCoordBuffer.put(texCoords).position(0);
            texCoordBufferR.put(texCoordsR).position(0);
            indexBuffer.put(index).position(0);
        }


        public void SetupModelViewMatrix()
        {
            float viewWidth = fWidth;
            if((nRendyType & ERenderSplitView) > 0) {
                viewWidth *= 0.5;
            }
            float ratio = (viewWidth / fHeight);

            float fRoll = 0;
            float fYaw = 0;


            fRoll -= mYOffSet*0.05;
            fYaw -= mXOffSet*0.05;

            Matrix.setLookAtM(mViewMatrix, 0, 0.0F, 0.0F, radius, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F);
            Matrix.scaleM(mViewMatrix, 0, mViewMatrix, 0, radius, radius, radius);
            Matrix.frustumM(mProjectionMatrix, 0, -ratio, ratio, -1.0F, 1.0F, radius, 100.0F*radius);
            Matrix.multiplyMM(mMVPMatrix, 0, mProjectionMatrix, 0, mViewMatrix, 0);
            Matrix.multiplyMM(mMVPMatrix, 0, mMVPMatrix, 0, mHeadView, 0);

            if(fYaw > 0.000001 || fYaw < -0.000001) {
                Matrix.setRotateM(mYRotationMatrix, 0, fYaw, 0.0F, 1.0F, 0.0F);
                Matrix.multiplyMM(mMVPMatrix, 0, mMVPMatrix, 0, mYRotationMatrix, 0);
            }

            if(fRoll > 0.000001 || fRoll < -0.000001) {
                Matrix.setRotateM(mXRotationMatrix, 0, fRoll, 1.0F, 0.0F, 0.0F);
                Matrix.multiplyMM(mMVPMatrix, 0, mMVPMatrix, 0, mXRotationMatrix, 0);
            }

            //Matrix.multiplyMM(mMVPMatrix, 0, mMVPMatrix, 0, mZRotationMatrix, 0);
        }


        public void bindVBO(int[] paramArrayOfInt)
        {
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, paramArrayOfInt[0]);
            GLES20.glVertexAttribPointer(maPositionHandle, 3, GLES20.GL_FLOAT, false, 0, 0);
            GLES20.glBindBuffer(GLES20.GL_ELEMENT_ARRAY_BUFFER, paramArrayOfInt[1]);
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, paramArrayOfInt[2]);
            GLES20.glVertexAttribPointer(maTextureHandle, 2, GLES20.GL_FLOAT, false, 0, 0);
        }

        public void createVBO(int[] paramArrayOfInt1, float[] paramArrayOfFloat1, int[] paramArrayOfInt2, float[] paramArrayOfFloat2, FloatBuffer paramFloatBuffer1, IntBuffer paramIntBuffer, FloatBuffer paramFloatBuffer2)
        {
            GLES20.glGenBuffers(paramArrayOfInt1.length, paramArrayOfInt1, 0);
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, paramArrayOfInt1[0]);
            GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, 4 * paramArrayOfFloat1.length, paramFloatBuffer1, GLES20.GL_STATIC_DRAW);
            GLES20.glEnableVertexAttribArray(maPositionHandle);
            GLES20.glVertexAttribPointer(maPositionHandle, 3, GLES20.GL_FLOAT, false, 0, 0);
            GLES20.glBindBuffer(GLES20.GL_ELEMENT_ARRAY_BUFFER, paramArrayOfInt1[1]);
            GLES20.glBufferData(GLES20.GL_ELEMENT_ARRAY_BUFFER, 4 * paramArrayOfInt2.length, paramIntBuffer, GLES20.GL_STATIC_DRAW);
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, paramArrayOfInt1[2]);
            GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, 4 * paramArrayOfFloat2.length, paramFloatBuffer2, GLES20.GL_STATIC_DRAW);
            GLES20.glEnableVertexAttribArray(maTextureHandle);
            GLES20.glVertexAttribPointer(maTextureHandle, 2, GLES20.GL_FLOAT, false, 0, 0);
        }

        private void getFrameParams(final HeadTransform head, final Eye leftEye, final Eye rightEye, final Eye monocular) {
            final CardboardDeviceParams cdp = this.mHmd.getCardboardDeviceParams();
            final ScreenParams screen = this.mHmd.getScreenParams();
            mHeadTracker.getLastHeadView(head.getHeadView(), 0);
            final float halfInterpupillaryDistance = cdp.getInterLensDistance() * 0.5f;
            if (this.mVRMode) {
                Matrix.setIdentityM(this.mLeftEyeTranslate, 0);
                Matrix.setIdentityM(this.mRightEyeTranslate, 0);
                Matrix.translateM(this.mLeftEyeTranslate, 0, halfInterpupillaryDistance, 0.0f, 0.0f);
                Matrix.translateM(this.mRightEyeTranslate, 0, -halfInterpupillaryDistance, 0.0f, 0.0f);
                Matrix.multiplyMM(leftEye.getEyeView(), 0, this.mLeftEyeTranslate, 0, head.getHeadView(), 0);
                Matrix.multiplyMM(rightEye.getEyeView(), 0, this.mRightEyeTranslate, 0, head.getHeadView(), 0);
            }
            else {
                System.arraycopy(head.getHeadView(), 0, monocular.getEyeView(), 0, head.getHeadView().length);
            }
            if (this.mProjectionChanged) {
                monocular.getViewport().setViewport(0, 0, screen.getWidth(), screen.getHeight());
                //CardboardView.this.mUiLayer.updateViewport(monocular.getViewport());
                if (!this.mVRMode) {
                    this.updateMonocularFieldOfView(monocular.getFov());
                }
                else if (this.mDistortionCorrectionEnabled) {
                    this.updateFieldOfView(leftEye.getFov(), rightEye.getFov());
                    this.mDistortionRenderer.onFovChanged(this.mHmd, leftEye.getFov(), rightEye.getFov(), this.getVirtualEyeToScreenDistance());
                }
                else {
                    this.updateUndistortedFovAndViewport();
                }
                leftEye.setProjectionChanged();
                rightEye.setProjectionChanged();
                monocular.setProjectionChanged();
                this.mProjectionChanged = false;
            }
            if (this.mDistortionCorrectionEnabled && this.mDistortionRenderer.haveViewportsChanged()) {
                this.mDistortionRenderer.updateViewports(leftEye.getViewport(), rightEye.getViewport());
            }
        }

        private void updateFieldOfView(final FieldOfView leftEyeFov, final FieldOfView rightEyeFov) {
            final CardboardDeviceParams cdp = this.mHmd.getCardboardDeviceParams();
            final ScreenParams screen = this.mHmd.getScreenParams();
            final Distortion distortion = cdp.getDistortion();
            final float eyeToScreenDist = this.getVirtualEyeToScreenDistance();
            final float outerDist = (screen.getWidthMeters() - cdp.getInterLensDistance()) / 2.0f;
            final float innerDist = cdp.getInterLensDistance() / 2.0f;
            final float bottomDist = cdp.getVerticalDistanceToLensCenter() - screen.getBorderSizeMeters();
            final float topDist = screen.getHeightMeters() + screen.getBorderSizeMeters() - cdp.getVerticalDistanceToLensCenter();
            final float outerAngle = (float)Math.toDegrees(Math.atan(distortion.distort(outerDist / eyeToScreenDist)));
            final float innerAngle = (float)Math.toDegrees(Math.atan(distortion.distort(innerDist / eyeToScreenDist)));
            final float bottomAngle = (float)Math.toDegrees(Math.atan(distortion.distort(bottomDist / eyeToScreenDist)));
            final float topAngle = (float)Math.toDegrees(Math.atan(distortion.distort(topDist / eyeToScreenDist)));
            leftEyeFov.setLeft(Math.min(outerAngle, cdp.getLeftEyeMaxFov().getLeft()));
            leftEyeFov.setRight(Math.min(innerAngle, cdp.getLeftEyeMaxFov().getRight()));
            leftEyeFov.setBottom(Math.min(bottomAngle, cdp.getLeftEyeMaxFov().getBottom()));
            leftEyeFov.setTop(Math.min(topAngle, cdp.getLeftEyeMaxFov().getTop()));
            rightEyeFov.setLeft(leftEyeFov.getRight());
            rightEyeFov.setRight(leftEyeFov.getLeft());
            rightEyeFov.setBottom(leftEyeFov.getBottom());
            rightEyeFov.setTop(leftEyeFov.getTop());
        }

        private void updateMonocularFieldOfView(final FieldOfView monocularFov) {
            final ScreenParams screen = this.mHmd.getScreenParams();
            final float monocularBottomFov = 22.5f;
            final float monocularLeftFov = (float)Math.toDegrees(Math.atan(Math.tan(Math.toRadians(monocularBottomFov)) * screen.getWidthMeters() / screen.getHeightMeters()));
            monocularFov.setLeft(monocularLeftFov);
            monocularFov.setRight(monocularLeftFov);
            monocularFov.setBottom(monocularBottomFov);
            monocularFov.setTop(monocularBottomFov);
        }

        private void updateUndistortedFovAndViewport() {
            final ScreenParams screen = this.mHmd.getScreenParams();
            final CardboardDeviceParams cdp = this.mHmd.getCardboardDeviceParams();
            final float halfLensDistance = cdp.getInterLensDistance() / 2.0f;
            final float eyeToScreen = this.getVirtualEyeToScreenDistance();
            final float left = screen.getWidthMeters() / 2.0f - halfLensDistance;
            final float right = halfLensDistance;
            final float bottom = cdp.getVerticalDistanceToLensCenter() - screen.getBorderSizeMeters();
            final float top = screen.getBorderSizeMeters() + screen.getHeightMeters() - cdp.getVerticalDistanceToLensCenter();
            final FieldOfView leftEyeFov = this.mLeftEye.getFov();
            leftEyeFov.setLeft((float)Math.toDegrees(Math.atan2(left, eyeToScreen)));
            leftEyeFov.setRight((float)Math.toDegrees(Math.atan2(right, eyeToScreen)));
            leftEyeFov.setBottom((float)Math.toDegrees(Math.atan2(bottom, eyeToScreen)));
            leftEyeFov.setTop((float)Math.toDegrees(Math.atan2(top, eyeToScreen)));
            final FieldOfView rightEyeFov = this.mRightEye.getFov();
            rightEyeFov.setLeft(leftEyeFov.getRight());
            rightEyeFov.setRight(leftEyeFov.getLeft());
            rightEyeFov.setBottom(leftEyeFov.getBottom());
            rightEyeFov.setTop(leftEyeFov.getTop());
            this.mLeftEye.getViewport().setViewport(0, 0, screen.getWidth() / 2, screen.getHeight());
            this.mRightEye.getViewport().setViewport(screen.getWidth() / 2, 0, screen.getWidth() / 2, screen.getHeight());
        }

        private float getVirtualEyeToScreenDistance() {
            return this.mHmd.getCardboardDeviceParams().getScreenToLensDistance();
        }

        public void destroyVBO(int[] paramArrayOfInt)
        {
            GLES20.glDeleteBuffers(paramArrayOfInt.length, paramArrayOfInt, 0);
        }
        
        @Override
        public void onSurfaceCreated(GL10 glUnused, EGLConfig config) {
      	
        	mProgram = createProgram(mVertexShader, mFragmentShader);
            if (mProgram == 0) {
                return;
            }
            
            maPositionHandle = GLES20.glGetAttribLocation(mProgram, "aPosition");
            checkGlError("glGetAttribLocation aPosition");
            if (maPositionHandle == -1) {
                throw new RuntimeException("Could not get attrib location for aPosition");
            }
            maTextureHandle = GLES20.glGetAttribLocation(mProgram, "aTextureCoord");
            checkGlError("glGetAttribLocation aTextureCoord");
            if (maTextureHandle == -1) {
                throw new RuntimeException("Could not get attrib location for aTextureCoord");
            }

            muMVPMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uMVPMatrix");
            checkGlError("glGetUniformLocation uMVPMatrix");
            if (muMVPMatrixHandle == -1) {
                throw new RuntimeException("Could not get attrib location for uMVPMatrix");
            }

            muSTMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uSTMatrix");
            checkGlError("glGetUniformLocation uSTMatrix");
            if (muSTMatrixHandle == -1) {
                throw new RuntimeException("Could not get attrib location for uSTMatrix");
            }

            //createVBO(vertexIds, vertice, index, texCoords, vertexBuffer, indexBuffer, texCoordBuffer);

            int[] textures = new int[1];
            GLES20.glGenTextures(1, textures, 0);

            mTextureID = textures[0];
            GLES20.glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureID);
            checkGlError("glBindTexture mTextureID");

            GLES20.glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER,
                                   GLES20.GL_LINEAR);
            GLES20.glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER,
                                   GLES20.GL_LINEAR);

            GLES20.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
            GLES20.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);

            /*
             * Create the SurfaceTexture that will feed this textureID,
             * and pass it to the MediaPlayer
             */
            mSurfaceTexture = new SurfaceTexture(mTextureID);
            mSurfaceTexture.setOnFrameAvailableListener(this);

            mSurface = new Surface(mSurfaceTexture);
            
            Log.i(TAG, "onSurfaceCreated:::::");
            
            synchronized(this) {
                updateSurface = false;
            }

            if (mEventHandler != null) {
                Message m = mEventHandler.obtainMessage(GLSURFACEVIEW_CREATE, 0, 0);
                mEventHandler.sendMessage(m);
            }
        }

        synchronized public void onFrameAvailable(SurfaceTexture surface)
        {
            updateSurface = true;

            long nowTime = System.currentTimeMillis();

            //Log.i(TAG, "onFrameAvailable::::: Time Diff " + (nowTime - lastArrived));
        }

        private int loadShader(int shaderType, String source) {
            int shader = GLES20.glCreateShader(shaderType);
            if (shader != 0) {
                GLES20.glShaderSource(shader, source);
                GLES20.glCompileShader(shader);
                int[] compiled = new int[1];
                GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
                if (compiled[0] == 0) {
                    Log.e(TAG, "Could not compile shader " + shaderType + ":");
                    Log.e(TAG, GLES20.glGetShaderInfoLog(shader));
                    GLES20.glDeleteShader(shader);
                    shader = 0;
                }
            }
            return shader;
        }

        private int createProgram(String vertexSource, String fragmentSource) {
            int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexSource);
            if (vertexShader == 0) {
                return 0;
            }
            int pixelShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentSource);
            if (pixelShader == 0) {
                return 0;
            }

            int program = GLES20.glCreateProgram();
            if (program != 0) {
                GLES20.glAttachShader(program, vertexShader);
                checkGlError("glAttachShader");
                GLES20.glAttachShader(program, pixelShader);
                checkGlError("glAttachShader");
                GLES20.glLinkProgram(program);
                int[] linkStatus = new int[1];
                GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linkStatus, 0);
                if (linkStatus[0] != GLES20.GL_TRUE) {
                    Log.e(TAG, "Could not link program: ");
                    Log.e(TAG, GLES20.glGetProgramInfoLog(program));
                    GLES20.glDeleteProgram(program);
                    program = 0;
                }
            }
            return program;
        }

        private void checkGlError(String op) {
            int error;
            while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
                Log.e(TAG, op + ": glError " + error);
            //    throw new RuntimeException(op + ": glError " + error);
            }
        }
       
    }  // End of class VideoRender.


    public static final int ERenderDefault = 0;
    public static final int ERenderLR3D = 0x1;
    public static final int ERenderUD3D = 0x2;
    public static final int ERenderGlobelView = 0x4;
    public static final int ERenderSplitView = 0x8;
    public static final int ERender180View = 0x10;
    public static final int ERenderMeshDistortion = 0x20;


}  // End of class VideoSurfaceView.
