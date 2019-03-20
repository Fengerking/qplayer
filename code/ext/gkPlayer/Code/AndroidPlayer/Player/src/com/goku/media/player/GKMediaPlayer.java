/**
 * @(#)GKMediaPlayer.java
  */
package com.goku.media.player;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup.LayoutParams;
/**
 * Native media engine encapsulate.
 *
 */
public class GKMediaPlayer implements IMediaPlayer {
    private static final float PERCENT = 0.01f;
    private final static String TAG = "GKMediaPlayer";

    static {
        try {
            System.loadLibrary("osal");
            System.loadLibrary("resample");
            System.loadLibrary("mediaplayer");
        } catch (UnsatisfiedLinkError error) {
            error.printStackTrace();
        }
    }

    private static final int MIN_HARDWARE_VOLUME = -990;
    private EventHandler mEventHandler;
    private boolean mPlayerReleased = false;
    private int mNativePlayerPara = 0;

    private Surface mSurface;
    private SurfaceHolder mSurfaceHolder;
    private SurfaceView mSurfaceView;

    private int mWidth = 0;
    private int mHeight = 0;

    private int mViewWidth = 0;
    private int mViewHeight = 0;

    private static GKMediaPlayer sGKMediaPlayer;

    /**
     * constructors.
     *
     * @param headerBytes 不注�?
     * @param plugInPath  动�?�库文件路径.
     */
    public GKMediaPlayer(byte[] headerBytes, String plugInPath) {
        mEventHandler = new EventHandler(this, Looper.myLooper());

        int maxSamplerate = GKAudioTrack.maxOutputSamplerate();
        nativeSetup(this, headerBytes, maxSamplerate, plugInPath);
    }

	/*
     * A native method that is implemented by the 'hello-jni' native library,
	 * which is packaged with this application.
	 */

    private native void nativeSetup(Object weakThis, byte[] headerBytes, int samplerate, String pluginPath);

    private native void nativeRelease(int context);

    private native int nativeSetDataSourceSync(int context, String url, int flag);

    private native int nativeSetDataSourceAsync(int context, String url, int flag);

    private native int nativeSetSurface(int context);

    private native int nativeGetCurFreqAndWave(int context, short[] freqArr, short[] waveArr, int sampleNum);

    private native int nativeGetCurFreq(int context, short[] freqArr, int freqNum);

    private native int nativeGetCurWave(int context, short[] waveArr, int waveNum);

    private native void nativeSetVolume(int context, int lVolume, int rVolume);

    private native int nativeStop(int context);

    private native void nativeSetCacheFilePath(int context, String cacheFilePath);

    private native void nativeSetAudioEffectLowDelay(int context, boolean enable);

    private native void nativeCongfigProxyServer(int context, String ip, int port, String authenkey, boolean useProxy);
    
    private static native void nativeCongfigProxyServerByDomain(String domain, int port, String authenkey, boolean useProxy);

    private native void nativeSetActiveNetWorkType(int context, int type);

    private native void nativeSetDecoderType(int context, int type);

    private native int nativePlay(int context);

    private native void nativePause(int context, boolean isFadeOut);

    private native void nativeResume(int context, boolean isFadeIn);
    
    private native void nativeSetHostMetadata(String filePath);

    private native int nativeSetPosition(int context, int aPos, int flag);

    private native void nativeSetPlayRange(int context, int aStart, int aEnd);

    private native int nativeGetPosition(int context);

    private native int nativeDuration(int context);

    private native int nativeSize(int context);

    private native int nativeBufferedSize(int context);

    private native int nativeBufferBandWidth(int context);

    private native int nativeBufferBandPercent(int context);

    private native int nativeBufferedPercent(int context);

    private native int nativeGetStatus(int context);
    
    private static native void  nativeSetLogOpenSwitch(boolean open);

    /**
     * 设置网络类型
     *
     * @param type 类型
     */
    public void setActiveNetWorkType(int type) {
        nativeSetActiveNetWorkType(mNativePlayerPara, type);
    }


    /**
     * 硬件解码
     */
    public static final int EDECODER_DEFAULT = 0x00;
    /**
     * 软件解码
     */
    public static final int EDECODER_SOFT = 0x01;

    /**
     * 设置解码类型
     *
     * @param type 解码类型
     */
    public void setDecoderType(int type) {
        nativeSetDecoderType(mNativePlayerPara, type);
    }

    /**
     * 设置缓存文件路径
     *
     * @param cacheFilePath 文件路径
     */
    @Override
    public void setCacheFilePath(String cacheFilePath) {
        nativeSetCacheFilePath(mNativePlayerPara, cacheFilePath);
    }

    /**
     * play stream
     *
     * @return operation status
     */
    public int play() {
        return nativePlay(mNativePlayerPara);
    }

    /**
     * pause stream
     */
    public void pause(boolean isFadeOut) {
        nativePause(mNativePlayerPara, isFadeOut);
    }

    /**
     * resume stream
     */
    public void resume(boolean isFadeIn) {
        nativeResume(mNativePlayerPara, isFadeIn);
    }

    /**
     * seek Operation.
     *
     * @param aPos seek position in milliseconds
     * @param flag 设置标志
     * @return 正数就是设置成功的位置，负数表示不成�?
     */
    public int setPosition(int aPos, int flag) {
        return nativeSetPosition(mNativePlayerPara, aPos, flag);
    }

    /**
     * cue cut setPlayRange Operation.
     *
     * @param aStart 范围�?�?
     * @param aEnd   范围结束
     */
    public void setPlayRange(int aStart, int aEnd) {
        nativeSetPlayRange(mNativePlayerPara, aStart, aEnd);
    }

    /**
     * get current position.
     *
     * @return current position in milliseconds
     */
    public int getPosition() {
        return nativeGetPosition(mNativePlayerPara);
    }

    /**
     * get stream duration.
     *
     * @return stream duration in milliseconds
     */
    public int duration() {
        return nativeDuration(mNativePlayerPara);
    }
    
    public void SetHostMetadata(String hostMeta) {
    	nativeSetHostMetadata(hostMeta);
    }

    /**
     * get stream size.
     *
     * @return stream size in bytes
     */
    public int size() {
        return nativeSize(mNativePlayerPara);
    }

    /**
     * get buffered size.
     *
     * @return buffered size in bytes
     */
    public int bufferedSize() {
        return nativeBufferedSize(mNativePlayerPara);
    }

    /**
     * get buffered Percent.
     *
     * @return stream duration in %
     */
    public int bufferedPercent() {
        return nativeBufferedPercent(mNativePlayerPara);
    }

    /**
     * get stream status.
     *
     * @return stream status
     */
    public int getStatus() {
        return nativeGetStatus(mNativePlayerPara);
    }


    @Override
    public float getBufferPercent() {
        return nativeBufferedPercent(mNativePlayerPara) * PERCENT;
    }

    @Override
    public int getBufferSize() {
        return nativeBufferedSize(mNativePlayerPara);
    }

    /**
     * 网络下载速度
     *
     * @return 下载速度 byte/per second
     */
    public int bufferedBandWidth() {
        return nativeBufferBandWidth(mNativePlayerPara);
    }

    /**
     * 网络下载速度百分�?
     *
     * @return 下载速度 byte/per second
     */
    public int bufferedBandPercent() {
        return nativeBufferBandPercent(mNativePlayerPara);
    }

    /**
     * 获取文件大小
     *
     * @return 文件大小
     */
    @Override
    public int getFileSize() {
        return nativeSize(mNativePlayerPara);
    }

    /**
     * 设置SurfaceView
     *
     * @param sv 当前SurfaceView
     */
    public void setView(SurfaceView sv) {
        if (sv == null) {
            mSurface = null;
            mSurfaceView = null;
            mSurfaceHolder = null;
            nativeSetSurface(mNativePlayerPara);
            return;
        }

        mSurfaceView = sv;
        mSurfaceHolder = mSurfaceView.getHolder();

        if (mSurfaceHolder != null) {
            mSurface = mSurfaceHolder.getSurface();
        } else {
            mSurface = null;
        }

        nativeSetSurface(mNativePlayerPara);
    }
    
    /**
     * 设置当前surfaceView
     *
     * @param sv 当前SurfaceView
     */
    public void setSurface(Surface sf)
    {
    	if (sf == null) {
            mSurface = null;
            nativeSetSurface(mNativePlayerPara);
            return;
        }

        mSurface = sf;

        nativeSetSurface(mNativePlayerPara);
    }


    /**
     * get current wave and spectrum data
     *
     * @param aFreqarr   spectrum data	output
     * @param aWavearr   wave data		output
     * @param aSampleNum samples num
     * @return operation status
     */
    @Override
    public int getCurFreqAndWave(short[] aFreqarr, short[] aWavearr, int aSampleNum) {
        int nRet = -1;
        synchronized (this) {
            if (!mPlayerReleased) {
                nRet = nativeGetCurFreqAndWave(mNativePlayerPara, aFreqarr, aWavearr, aSampleNum);
            }
        }

        return nRet;
    }

    /**
     * get current spectrum data
     *
     * @param aFreqarr spectrum data	output
     * @param aFreqNum number of frequency bands   input
     * @return operation status
     */
    @Override
    public int getCurFreq(short[] aFreqarr, int aFreqNum) {
        int nRet = -1;
        synchronized (this) {
            if (!mPlayerReleased) {
                nRet = nativeGetCurFreq(mNativePlayerPara, aFreqarr, aFreqNum);
            }
        }

        return nRet;
    }

    /**
     * get current wave data
     *
     * @param aWavearr wave data		output
     * @param aWaveNum waveform points   input
     * @return operation status
     */
    @Override
    public int getCurWave(short[] aWavearr, int aWaveNum) {
        int nRet = -1;
        synchronized (this) {
            if (!mPlayerReleased) {
                nRet = nativeGetCurWave(mNativePlayerPara, aWavearr, aWaveNum);
            }
        }

        return nRet;
    }

    /**
     * set audio volume.
     *
     * @param aLVolume left channel volume.
     * @param aRVolume right channel volume.
     */
    @Override
    public void setVolume(float aLVolume, float aRVolume) {
        int leftVolume = (int) ((1.0f - aLVolume) * MIN_HARDWARE_VOLUME);
        int rightVolume = (int) ((1.0f - aRVolume) * MIN_HARDWARE_VOLUME);
        nativeSetVolume(mNativePlayerPara, leftVolume, rightVolume);
    }

    /**
     * stop stream
     */
    @Override
    public void stop() {
        if (0 != nativeStop(mNativePlayerPara)) {
            throw new IllegalStateException("can not be stopped!");
        }
    }

    private int setDataSource(String aUrl, int flag, boolean aSync) {
        int nErr;
        if (aSync) {
            nErr = nativeSetDataSourceSync(mNativePlayerPara, aUrl, flag);
        } else {
            nErr = nativeSetDataSourceAsync(mNativePlayerPara, aUrl, flag);
        }
        return nErr;
    }

    /**
     * set stream source synchronized
     *
     * @param aUrl stream source path.
     * @param flag 打开标志
     * @return operation status
     */
    public int setDataSource(String aUrl, int flag) {
        return setDataSource(aUrl, flag, true);
    }

    /**
     * set stream source asynchronized
     *
     * @param aUrl stream source path.
     * @param flag 打开标志
     */
    @Override
    public void setDataSourceAsync(String aUrl, int flag) {
        if (0 != setDataSource(aUrl, flag, false)) {
            throw new IllegalStateException("can not setDataSourceAsync !");
        }
    }

    /**
     * release player resource
     */
    @Override
    public void release() {
        synchronized (this) {
            nativeRelease(mNativePlayerPara);
            mPlayerReleased = true;
        }
    }

    @Override
    public void setChannelBalance(float balance) {
        setVolume(1.0f - balance, 1.0f + balance);
    }

    /**
     * 默认播放方式
     */
    public static final int OPEN_DEFAULT = 0x00;
    /**
     * 播放在线视频
     */
    public static final int OPEN_BUFFER = 0x01;
    /**
     * 预加�?
     */
    public static final int OPEN_PRE_LOADING = 0x02;
    /**
     * 预加载完�?
     */
    public static final int OPEN_PRE_LOADED = 0x04;
    /**
     * 切换音视�?
     */
    public static final int OPEN_SWITCH = 0x08;

    /**
     * seek到最近点，用来控制拖动视频则够了
     */
    public static final int SEEK_FAST = 0x00;

    /**
     * seek到某�?个可以播放的点，比较准确
     */
    public static final int SEEK_CORRECT = 0x01;

    /**
     * wrong operation
     */
    public static final int MEDIA_NOP = 0;

    /**
     * prepare operation
     */
    public static final int MEDIA_PREPARE = 1;

    /**
     * play operation
     */
    public static final int MEDIA_PLAY = 2;

    /**
     * play complete operation
     */
    public static final int MEDIA_COMPLETE = 3;

    /**
     * pause operation
     */
    public static final int MEDIA_PAUSE = 4;

    /**
     * close operation
     */
    public static final int MEDIA_CLOSE = 5;

    /**
     * 播放异常，如：网络断了，本地数据读取失败，音频写文件失败
     */
    public static final int MEDIA_EXCEPTION = 6;

    /**
     * update stream duration operation
     */
    public static final int MEDIA_UPDATE_DURATION = 7;

    /**
     * seek completed
     */
    public static final int MEDIA_SEEK_COMPLETED = 11;

    /**
     * audio format changed
     */
    public static final int MEDIA_AUDIOFORMAT_CHANGED = 12;

    /**
     * format changed
     */
    public static final int MEDIA_VIDEOFORMAT_CHANGED = 13;

    /**
     * notify stream is Buffering
     */
    public static final int MEDIA_BUFFERING_START = 16;

    /**
     * notify stream Buffer completed
     */
    public static final int MEDIA_BUFFERING_DONE = 17;

    /**
     * DNS解析完成
     */
    public static final int MEDIA_DNS_DONE = 18;

    /**
     * 连接完成
     */
    public static final int MEDIA_CONNECT_DONE = 19;

    /**
     * 接收Http头成�?
     */
    public static final int MEDIA_HTTP_HEADER_RECEIVED = 20;

    /**
     * �?始接受网络数�?
     */
    public static final int MEDIA_START_RECEIVE_DATA = 21;

    /**
     * �?始拿到网络数�?
     */
    public static final int MEDIA_PREFETCH_COMPLETED = 22;

    /**
     * 视音频的数据缓冲到最�?
     */
    public static final int MEDIA_CACHE_COMPLETED = 23;

    /**
     * �?始播放媒�?
     */
    public static final int MEDIA_START_OPEN = 24;
    /**
     * �?始绘制MV第一�?
     */
    public static final int MEDIA_START_FIRST_FRAME = 25;
    /**
     * 视频画质切换�?�?
     */
    public static final int VIDEO_QUALITY_SWITCH_START = 50;
    /**
     * 视频画质切换�?�?
     */
    public static final int VIDEO_QUALITY_SWITCH_SUCCESS = 51;
    /**
     * 视频画质切换�?�?
     */
    public static final int VIDEO_QUALITY_SWITCH_FAILED = 52;

    /**
     * stream status starting
     */
    public static final int MEDIASTATUS_STARTING = 1;

    /**
     * stream status playing
     */
    public static final int MEDIASTATUS_PLAYING = 2;

    /**
     * stream status paused
     */
    public static final int MEDIASTATUS_PAUSED = 3;

    /**
     * stream status stopped
     */
    public static final int MEDIASTATUS_STOPPED = 4;

    /**
     * stream status prepared
     */
    public static final int MEDIASTATUS_PREPARED = 5;

    /**
     * max sample num to be fetch
     */
    public static final int MAX_SAMPLE_NUM = 1024;

    /**
     * min sample num to be fetch
     */
    public static final int MIN_SAMPLE_NUM = 256;

    private class EventHandler extends Handler {
        public EventHandler(GKMediaPlayer mp, Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message aMsg) {
            if (mMvPlayerNotifyEventListener != null) {
                mMvPlayerNotifyEventListener.onMediaPlayerNotify(aMsg.what, aMsg.arg1, aMsg.arg2, aMsg.obj);
                return;
            }

            if (mMediaPlayerNotifyEventListener != null) {
                mMediaPlayerNotifyEventListener.onMediaPlayerNotify(aMsg.what, aMsg.arg1, aMsg.arg2, aMsg.obj);
            }
        }
    }

    /**
     * @param aMediaplayerRef
     * @param aMsg
     * @param aArg1           error code
     * @param aArg2           1表示音频
     *                        2 表示视频
     *                        3 表既有音频也有视�?
     * @param aObj
     */
    private static void postEventFromNative(Object aMediaplayerRef, int aMsg, int aArg1, int aArg2, Object aObj) {
        GKMediaPlayer mp = (GKMediaPlayer) aMediaplayerRef;
        if (mp == null) {
            return;
        }

        if (mp.mEventHandler != null) {
            Message m = mp.mEventHandler.obtainMessage(aMsg, aArg1, aArg2, aObj);
            mp.mEventHandler.sendMessage(m);
        }
    }

    /**
     * MediaPlayerListener
     *
     * @author zhan.liu
     * @version 2.4.0
     * @since 2011-07-25
     */
    public interface OnMediaPlayerNotifyEventListener {

        /**
         * @param aMsgId 消息ID
         * @param aArg1  整型参数1
         * @param aArg2  整型参数2
         * @param aObj   对象参数
         */
        void onMediaPlayerNotify(int aMsgId, int aArg1, int aArg2, Object aObj);
    }

    /**
     * MvPlayerListener
     *
     * @author benpeng.jiang
     * @version 7.7.0
     */
    public interface OnMvPlayerNotifyEventListener {

        /**
         * @param aMsgId 消息ID
         * @param aArg1  整型参数1
         * @param aArg2  整型参数2
         * @param aObj   对象参数
         */
        void onMediaPlayerNotify(int aMsgId, int aArg1, int aArg2, Object aObj);
    }

    /**
     * set MediaPlayer Listener
     *
     * @param aListener Listener reference.
     */
    public void setOnMediaPlayerNotifyEventListener(OnMediaPlayerNotifyEventListener aListener) {
        mMediaPlayerNotifyEventListener = aListener;
    }

    private OnMediaPlayerNotifyEventListener mMediaPlayerNotifyEventListener;


    /**
     * set MvPlayer Listener
     *
     * @param aListener Listener reference.
     */
    public void setOnMvPlayerNotifyEventListener(OnMvPlayerNotifyEventListener aListener) {
        mMvPlayerNotifyEventListener = aListener;
    }

    private OnMvPlayerNotifyEventListener mMvPlayerNotifyEventListener;

    /**
     * enable low delay audio effect processing
     *
     * @param enable enable
     */
    @Override
    public void setAudioEffectLowDelay(boolean enable) {
        nativeSetAudioEffectLowDelay(mNativePlayerPara, enable);
    }
    
    @Override
    public void setHostAddr(String hostAddr) {
        nativeSetHostMetadata(hostAddr);
    }

    /**
     * enable screen size
     *
     * @param width  Width
     * @param height Height
     */
    public void setViewSize(int width, int height) {
        mViewWidth = width;
        mViewHeight = height;
    }

    /**
     * enable width size changed
     *
     * @param width  width
     * @param height height
     */
    public void videoSizeChanged(int width, int height) {
		mWidth = width;
		mHeight = height;
		
		if(mWidth == 0 || mHeight == 0)
			return;
		
		if(mSurfaceView == null)
			return;
		
		LayoutParams lp = mSurfaceView.getLayoutParams();

		int nMaxOutW = mViewWidth;
		int nMaxOutH = mViewHeight;
		int w = 0, h = 0;

		if (nMaxOutW * mHeight > mWidth * nMaxOutH)
		{
			h = nMaxOutH;
			w = nMaxOutH * mWidth / mHeight;
		}
		else
		{
			w = nMaxOutW;
			h = nMaxOutW * mHeight / mWidth;
		}
		//w &= ~0x7;
		//h &= ~0x3;

		lp.width = w;
		lp.height = h;
		
		mSurfaceView.setLayoutParams(lp);
    }

    /**
     * set ProxyServer Config parameter
     *
     * @param ip        ip
     * @param port      port
     * @param authenkey authenkey
     * @param useProxy  useProxy
     */
    @Override
    public void setProxyServerConfig(String ip, int port, String authenkey, boolean useProxy) {
        nativeCongfigProxyServer(mNativePlayerPara, ip, port, authenkey, useProxy);
    }

    public void setProxyServerConfigByDomain(String domain, int port, String authenkey, boolean useProxy) {
        nativeCongfigProxyServerByDomain(domain, port, authenkey, useProxy);
    }    

    @Override
    public boolean isSystemPlayer() { 
        return false;
    }
}
