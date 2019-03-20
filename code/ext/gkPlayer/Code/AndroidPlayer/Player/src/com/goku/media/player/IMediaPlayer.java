package com.goku.media.player;

import android.view.Surface;
import android.view.SurfaceView;

/**
 * @version 7.0.0
 */
public interface IMediaPlayer {

    /**
     * 异步设置播放�?
     *
     * @param aUrl  流路�?.
     * @param aflag 打开参数
     * @throws Exception Exception
     */
    public void setDataSourceAsync(String aUrl, int aflag) throws Exception;

    /**
     * 设置网络类型
     *
     * @param type 类型
     */
    public void setActiveNetWorkType(int type);

    /**
     * 设置解码
     *
     * @param type 类型
     */
    public void setDecoderType(int type);

    /**
     * 异步缓存文件路路�?
     *
     * @param cacheFilePath 文件路径.
     */
    public void setCacheFilePath(String cacheFilePath);

    /**
     * 设置当前surfaceView
     *
     * @param sv 当前SurfaceView
     */
    public void setView(SurfaceView sv);
    
    
    /**
     * 设置当前surfaceView
     *
     * @param sv 当前SurfaceView
     */
    public void setSurface(Surface sf);

    /**
     * �?始播�?
     *
     * @return 播放操作状�??
     */
    public int play();

    /**
     * 暂停
     */
    public void pause(boolean isFadeOut);

    /**
     * 继续
     */
    public void resume(boolean isFadeIn);

    /**
     * 停止
     */
    public void stop();

    /**
     * seek操作
     *
     * @param aPos seek到的位置，单位毫�?
     * @param flag 默认为TTMediaPlayer.OPEN_DEFAULT
     * @return 这个不知�?
     */
    public int setPosition(int aPos, int flag);

    /**
     * 设置播放区间
     *
     * @param aStart 起始时间
     * @param aEnd   结束时间
     */
    public void setPlayRange(int aStart, int aEnd);

    /**
     * 获取播放位置
     *
     * @return 播放位置，单位毫�?
     */
    public int getPosition();

    /**
     * 获取缓冲进度
     *
     * @return 百分�?
     */
    public float getBufferPercent();

    /**
     * 获取缓冲大小
     *
     * @return buffer size
     */
    public int getBufferSize();

    /**
     * 获取文件大小
     *
     * @return 文件大小
     */
    public int getFileSize();

    /**
     * 歌曲时长
     *
     * @return 歌曲时长，单位毫�?
     */
    public int duration();

    /**
     * 缓存百分�?
     *
     * @return 百分�?
     */
    public int bufferedPercent();

    /**
     * 网络下载速度
     *
     * @return 下载速度 byte/per second
     */
    public int bufferedBandWidth();

    /**
     * 网络下载速度百分�?
     *
     * @return 下载速度 byte/per second
     */
    public int bufferedBandPercent();

    /**
     * 获取频谱和波�?
     *
     * @param aFreqarr   频谱
     * @param aWavearr   波形
     * @param aSampleNum 采样个数
     * @return 操作�?
     */
    public int getCurFreqAndWave(short[] aFreqarr, short[] aWavearr, int aSampleNum);

    /**
     * 获取频谱
     *
     * @param aFreqarr 频谱
     * @param aFreqNum 采样个数
     * @return 操作�?
     */
    public int getCurFreq(short[] aFreqarr, int aFreqNum);

    /**
     * 获取波形
     *
     * @param aWavearr 波形
     * @param aWaveNum 采样个数
     * @return 操作�?
     */
    public int getCurWave(short[] aWavearr, int aWaveNum);

    /**
     * 设置音量
     *
     * @param aLVolume 左声�?
     * @param aRVolume 右声�?
     */
    public void setVolume(float aLVolume, float aRVolume);


    /**
     * 释放资源，调用后，播放引擎不能使�?
     */
    public void release();

    /**
     * 设置声道平衡
     *
     * @param balance balance
     */
    public void setChannelBalance(float balance);

    /**
     * enable low delay audio effect processing
     *
     * @param enable enable
     */
    public void setAudioEffectLowDelay(boolean enable);

    /**
     * enable screen size
     *
     * @param width  width
     * @param height height
     */
    public void setViewSize(int width, int height);

    /**
     * enable width size changed
     *
     * @param width  width
     * @param height height
     */
    public void videoSizeChanged(int width, int height);

    /**
     * set ProxyServer Config parameter
     *
     * @param ip        ip
     * @param port      port
     * @param authenkey authenkey
     * @param useProxy  useProxy
     */
    public void setProxyServerConfig(String ip, int port, String authenkey, boolean useProxy);
    
    /**
     * 设置主机域名.
     *
     * @param hostAddr 主机域名.
     */
    public void setHostAddr(String hostAddr);

    /**
     * set ProxyServer Config parameter
     * @param domain domain
     * @param port port
     * @param authenkey authenkey
     * @param useProxy useProxy
     */
    public void setProxyServerConfigByDomain(String domain, int port, String authenkey, boolean useProxy);    

    /**
     * 是否为系统播放器
     * @return          true 系统播放�?
     */
    public boolean isSystemPlayer();
    
    public void SetHostMetadata(String hostMeta);
}
