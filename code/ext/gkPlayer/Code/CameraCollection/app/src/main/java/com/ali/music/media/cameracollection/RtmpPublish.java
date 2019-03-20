package com.ali.music.media.cameracollection;

import android.net.TrafficStats;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public class RtmpPublish {
    static {
    	try {
    		System.loadLibrary("H264Encoder");
    		System.loadLibrary("rtmppush");
        } catch (UnsatisfiedLinkError error) {
            error.printStackTrace();
        }
    }
    
    public RtmpPublish() {
    	mEventHandler = new EventHandler(this, Looper.myLooper());
        nativeSetup(this);
    }
    private static long m_startPTS= 0; 
    
    private int mNativeRtmpPara = 0;
    private native void nativeSetup(Object weakThis);
    private native int nativeStartPublish(int context, String url);
    private native void nativeStop(int context);
    private native void nativeRelease(int context);
    private native int nativeSendAudioPacket(int context,byte[] array, int size, long pts);
     
    private native int nativeSendAudioConfig(int context,byte[] array, int size);
    
    //soft use
    private native void nativeSetWidthHeight(int context, int Width,int Height);
    private native void nativeSetFpsBitrate(int context, int fps,int bitrate);
    private native void nativeSetVideoCodecType(int context, int codeType);
    private native void nativeVideoEncoderInit(int context);
    private native int nativeSendVideoRawData(int context,byte[] array, int size, long pts, int RotateType);
    
    private native void nativeGetLastPic(int context,int[] array, int size);
    
    //hard use
    private native int nativeSendVideoConfig(int context,byte[] array, int size);
    private native int nativeSendVideoPacket(int context,byte[] array, int size, long pts);
    private native void nativeHandleRawData(int context,byte[] src, byte[] dst,int with, int heith,int type);
    private native void nativeSetcolorFormart(int context, int color);
    private native void nativeSetcollectType(int context, int type);
    
    //flow monitor use
    private native void nativeSetBitrate(int context,int bitrate); 

    private EventHandler mEventHandler;
    private OnMediaNotifyEventListener mMediaNotifyEventListener;
    
    public static final int ENotifyUrlParseError = 1;
    public static final int ENotifySokcetConnectFailed = 2;
    public static final int ENotifyStreamConnectFailed = 3;
    public static final int ENotifyTransferError = 4;
    public static final int ENotifyOpenSucess = 5;
    public static final int ENotifyServerIP = 6;
    public static final int ENotifyOpenStart = 7;
    public static final int ENotifyReconn_OpenStart = 8;
    public static final int ENotifyReconn_OpenSucess = 9;
    public static final int ENotifyReconn_SokcetConnectFailed = 10;
    public static final int ENotifyReconn_StreamConnectFailed = 11;
    public static final int ENotifyNetBadCondition = 12;
    public static final int ENotifyNetReconnectUpToMax = 13;
    
    public static final int ENotifyRecord_FILEOPEN_OK = 14;
    public static final int ENotifyRecord_FILEOPEN_FAIL = 15;
    public static final int ENotifyRecord_START = 16;
    
    //public static final int ENotifyResetEncoder = 20;
    
    public static final int Collection_Live = 0;
    public static final int Collection_Record = 1;
    private class EventHandler extends Handler {
        public EventHandler(RtmpPublish mp, Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message aMsg) {
            if (mMediaNotifyEventListener != null) {
            	mMediaNotifyEventListener.onMediaNotify(aMsg.what, aMsg.arg1, aMsg.arg2);
                return;
            }
        }
    }
    
    /**
     * MediaListener
     * @author 
     * @version 
     * @since 2015-11-25
     */
    public interface OnMediaNotifyEventListener {
        /**
         * @param aMsgId ��ϢID
         * @param aArg1  ���Ͳ���1
         */
        void onMediaNotify(int aMsgId, int aArg1, int aArg2);
    }
    
    /**
     * set Media Listener
     *
     * @param aListener Listener reference.
     */
    public void setOnMediaNotifyEventListener(OnMediaNotifyEventListener aListener) {
    	mMediaNotifyEventListener = aListener;
    }
    
    /**
     * @param aMediaplayerRef
     * @param aMsg
     * @param aArg1  error code
     */
    private static void postEventFromNative(Object aMediaplayerRef, int aMsg, int aArg1, int aArg2) {
    	RtmpPublish mp = (RtmpPublish) aMediaplayerRef;
        if (mp == null) {
            return;
        }

        if (mp.mEventHandler != null) {
            Message m = mp.mEventHandler.obtainMessage(aMsg, aArg1, aArg2, null);
            mp.mEventHandler.sendMessage(m);
        }
    }
    
    public void release() {
        synchronized (this) {
            nativeRelease(mNativeRtmpPara);
            mNativeRtmpPara = 0;
        }
    }
 
    public void startPublish(String aUrl) {
         nativeStartPublish(mNativeRtmpPara, aUrl);
    }
    
    public void stop() {
    	nativeStop(mNativeRtmpPara);
    	mNativeRtmpPara = 0;
   }
    
    public  void SendAudioPacket(byte[] array, int size, long pts){
    	nativeSendAudioPacket(mNativeRtmpPara, array, size, pts);
    }
    
    public  void SendAudioConfig(byte[] array, int size){
    	nativeSendAudioConfig(mNativeRtmpPara, array, size);
    }
    
    public  void SendVideoRawData(byte[] array, int size, long pts, int RotateType){
    	nativeSendVideoRawData(mNativeRtmpPara, array, size, pts, RotateType);
    }
    
    public void setVideoWidthHeight(int width, int height){
    	nativeSetWidthHeight(mNativeRtmpPara,width,height);
    }
    
    public void setVideoCodecType(int codecType){
    	nativeSetVideoCodecType(mNativeRtmpPara,codecType);
    }
    
    public void setVideoFpsBitrate(int framerate, int bitrate){
    	nativeSetFpsBitrate(mNativeRtmpPara,framerate,bitrate);
    }
    
    public void VideoEncoderInit(){
    	nativeVideoEncoderInit(mNativeRtmpPara);
    }
    
    public void GetLastPic(int[] array, int size){
    	nativeGetLastPic(mNativeRtmpPara,array,size);
    }
    
    public  void SendVideoConfig(byte[] array, int size){
    	nativeSendVideoConfig(mNativeRtmpPara, array, size);
    }
    
    public  void SendVideoPacket(byte[] array, int size, long pts){
    	nativeSendVideoPacket(mNativeRtmpPara, array, size, pts);
    }
    
    public  void HandleRawData(byte[] src, byte[] dst,int with, int heith,int type){
    	nativeHandleRawData(mNativeRtmpPara, src, dst, with,heith,type);
    }
    
    public  void SetcolorFormart(int color){
    	nativeSetcolorFormart(mNativeRtmpPara, color);
    }
    
    public  void SetcollectType(int type){
    	nativeSetcollectType(mNativeRtmpPara, type);
    }
    
    public  void SetBitrate(int bitrate){
    	 nativeSetBitrate(mNativeRtmpPara, bitrate);
    }

}
