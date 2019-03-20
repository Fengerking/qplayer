/*******************************************************************************
	File:		MediaPlayer.java

	Contains:	player wrap implement file.

	Written by:	Fenger King

	Change History (most recent first):
	2018-07-04		Fenger			Create file

*******************************************************************************/
package com.qiniu.qplayer.mediaEngine;

import android.annotation.TargetApi;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.view.Surface;
import java.lang.ref.WeakReference;

public class MediaPlayer implements BasePlayer {
	private static final String TAG = "QMediaPlayer";
	private static final String ACTION_NET = "android.net.conn.CONNECTIVITY_CHANGE";

	static final int QC_FLAG_NETWORK_CHANGED 	= 0X20000000;
	static final int QC_FLAG_NETWORK_TYPE 		= 0X20000001;
	static final int QC_FLAG_ISP_NAME 			= 0X20000002;
	static final int QC_FLAG_WIFI_NAME 			= 0X20000003;
	static final int QC_FLAG_SIGNAL_DB 			= 0X20000004;
	static final int QC_FLAG_SIGNAL_LEVEL 		= 0X20000005;
	static final int QC_FLAG_APP_VERSION 		= 0X21000001;
	static final int QC_FLAG_APP_NAME 			= 0X21000002;
	static final int QC_FLAG_SDK_ID 			= 0X21000003;
	static final int QC_FLAG_SDK_VERSION 		= 0X21000004;

	private Context 	m_context = null;
	private long 		m_NativeContext = 0;
	private String		m_strSDKVersion = null;
	private Surface 	m_NativeSurface = null;

	private int 		m_nStreamNum = 0;
	private int 		m_nStreamPlay = -1;
	private int			m_nStreamBitrate = 0;

	private int 		m_nVideoWidth = 0;
	private int 		m_nVideoHeight = 0;
	private int 		m_nSampleRate = 0;
	private int 		m_nChannels = 0;
	private int 		m_nBTOffset = 0;
	private int			m_nPlaySpeed = 0;

	private Object				m_pObjPlayer = null;
	private String				m_strApkPath = null;
	private String				m_strURL = null;
	private AudioRender			m_AudioRender = null;
	private onEventListener 	m_EventListener = null;
	private msgHandler 			m_hHandler = null;
	private String				m_strNetworkName = null;

	static {
		System.loadLibrary("QPlayer");
	}

	public int Init(Context context, int nFlag) {
		m_context = context;
		String appPath = context.getApplicationContext().getFilesDir().getAbsolutePath();
		appPath = appPath.substring(0, appPath.lastIndexOf('/'));
		appPath = appPath + "/lib/";
		m_pObjPlayer = new WeakReference<MediaPlayer>(this);
		m_NativeContext = nativeInit(m_pObjPlayer, appPath, nFlag);
		if (m_NativeContext == 0)
			return -1;
		if (m_NativeSurface != null)
			nativeSetView(m_NativeContext, m_NativeSurface);

		int nSDKVer = GetParam(BasePlayer.PARAM_PID_QPLAYER_VERSION, 0, null);
		m_strSDKVersion = String.format("qpalyer_v%d.%d.%d.%d", nSDKVer>>24, (nSDKVer>>16)&0XFF, (nSDKVer>>8)&0XFF, nSDKVer&0XFF);

		recordBasicInfo ();
		recordNetworkInfo();

		m_hHandler = new msgHandler();
		m_strApkPath = appPath;
		AudioManager am = (AudioManager)context.getSystemService(context.AUDIO_SERVICE);
		if (am != null && am.isBluetoothA2dpOn())
			m_nBTOffset = 250;

		if (mNetworkStatusReceiver != null) {
			context.registerReceiver(mNetworkStatusReceiver, new IntentFilter(ACTION_NET));
		}

		return 0;
	}

	public void setEventListener(onEventListener listener) {
		m_EventListener = listener;
	}

	public void SetView(Surface sv) {
		if (sv != null) {
			m_NativeSurface = sv;
		} else {
			m_NativeSurface = null;
		}
		if (m_NativeContext != 0)
			nativeSetView(m_NativeContext, m_NativeSurface);
	}

	public int Open(String strPath, int nFlag) {
		m_strURL = strPath;
		return nativeOpen(m_NativeContext, strPath, nFlag);
	}

	public void Play() {
		nativePlay(m_NativeContext);
	}

	public void Pause() {
		nativePause(m_NativeContext);
	}

	public void Stop() {
		nativeStop(m_NativeContext);
	}

	public long GetDuration() {
		return nativeGetDuration(m_NativeContext);
	}

	public long GetPos() {
		return nativeGetPos(m_NativeContext);
	}

	public int SetPos(long lPos) {
		return nativeSetPos(m_NativeContext, lPos);
	}

	public int GetParam(int nParamId, int nParam, Object objParam) {
		return nativeGetParam(m_NativeContext, nParamId, nParam, objParam);
	}

	public int SetParam(int nParamId, int nParam, Object objParam) {
		/*
		if (nParamId == QCPLAY_PID_Speed) {
			m_nPlaySpeed = nParam;
			if (m_AudioRender != null)
				m_AudioRender.setSpeed(nParam);
			return 0;
		}*/
		return nativeSetParam(m_NativeContext, nParamId, nParam, objParam);
	}

	public void Uninit() {
		if (m_context != null && mNetworkStatusReceiver != null) {
			m_context.unregisterReceiver(mNetworkStatusReceiver);
			mNetworkStatusReceiver = null;
		}
		nativeUninit(m_NativeContext);
		if (m_AudioRender != null)
			m_AudioRender.closeTrack();
		m_AudioRender = null;
		m_NativeContext = 0;
	}

	public int GetVideoWidth() {
		return m_nVideoWidth;
	}

	public int GetVideoHeight() {
		return m_nVideoHeight;
	}

	public void SetVolume(int nVolume) {
		SetParam (PARAM_PID_AUDIO_VOLUME, nVolume, null);
	}

	public int GetStreamNum() {
		if (m_nStreamNum == 0)
			GetParam (QCPLAY_PID_StreamNum, 0, this);
		return m_nStreamNum;

	}
	public int SetStreamPlay (int nStream) {
		if (nStream == m_nStreamPlay)
			return nStream;
		m_nStreamPlay = nStream;
		SetParam (QCPLAY_PID_StreamPlay, m_nStreamPlay, null);
		return m_nStreamPlay;
	}

	public int GetStreamPlay () {
		GetParam (QCPLAY_PID_StreamPlay, 0, this);
		return m_nStreamPlay;
	}

	public int GetStreamBitrate (int nStream) {
		GetParam (QCPLAY_PID_StreamInfo, nStream, this);
		return m_nStreamBitrate;
	}

	public void OnOpenComplete () {
		SetParam (QCPLAY_PID_Clock_OffTime,  m_nBTOffset, null);
	}

	private void recordBasicInfo() {
		String deviceId = BaseUtil.replaceNull(BaseUtil.getDeviceId(m_context));
		String appName = BaseUtil.replaceNull(BaseUtil.appName(m_context));
		String appVersion = BaseUtil.replaceNull(BaseUtil.appVersion(m_context));
		SetParam(QC_FLAG_SDK_ID, 0, deviceId);
		SetParam(QC_FLAG_SDK_VERSION, 0, m_strSDKVersion);
		SetParam(QC_FLAG_APP_NAME, 0, appName);
		SetParam(QC_FLAG_APP_VERSION, 0, appVersion);
	}

	private void recordNetworkInfo() {
		String netType = BaseUtil.netType(m_context);
		String wifiName = null;
		String ispName = null;
		int signalDb = 0;
		int signalLevel = 0;
		boolean bWifi = netType.equals("WIFI");
		boolean bNetNone = netType.equals("None");
		if (bWifi) { // wifi
			String[] wifiInfo = BaseUtil.getWifiInfo(m_context);
			if (wifiInfo != null && wifiInfo.length >= 2) {
				wifiName = wifiInfo[0];
				if (BaseUtil.isNumber(wifiInfo[1])) {
					signalDb = Integer.parseInt(wifiInfo[1]);
				}
			}
		} else if (!bNetNone) {  // mobile
			String[] phoneInfo = BaseUtil.getPhoneInfo(m_context);
			if (phoneInfo != null && phoneInfo.length >= 2) {
				ispName = phoneInfo[0];
				if (BaseUtil.isNumber(phoneInfo[1])) {
					signalLevel = Integer.parseInt(phoneInfo[1]);
				}
			}
		}
		SetParam(QC_FLAG_NETWORK_TYPE, 0, BaseUtil.replaceNull(netType));
		SetParam(QC_FLAG_ISP_NAME, 0, BaseUtil.replaceNull(ispName));
		SetParam(QC_FLAG_WIFI_NAME, 0, BaseUtil.replaceNull(wifiName));
		SetParam(QC_FLAG_SIGNAL_DB, signalDb, null);
		SetParam(QC_FLAG_SIGNAL_LEVEL, signalLevel, null);
	}

	private BroadcastReceiver mNetworkStatusReceiver = new BroadcastReceiver() {
		@TargetApi(Build.VERSION_CODES.HONEYCOMB)
		@Override
		public void onReceive(Context context, Intent intent) {
			if (!ACTION_NET.equals(intent.getAction())) {
				return;
			}
			String networkName = intent.getStringExtra(ConnectivityManager.EXTRA_EXTRA_INFO);
			if (networkName != null && m_strNetworkName != null && !networkName.equals(m_strNetworkName)) {
				recordNetworkInfo();
				SetParam(QC_FLAG_NETWORK_CHANGED, 0, null);
			}
			m_strNetworkName = networkName;
		}
	};

	private static void postEventFromNative(Object baselayer_ref, int what, int ext1, int ext2, Object obj)
	{
		MediaPlayer player = (MediaPlayer)((WeakReference)baselayer_ref).get();
		if (player == null) 
			return;
		
		if (what == QC_MSG_SNKV_NEW_FORMAT)
		{
			if (player.m_nVideoWidth == ext1 && player.m_nVideoHeight == ext2){
				player.SetParam (PARAM_PID_EVENT_DONE, 0, null);
				return;
			}
			player.m_nVideoWidth = ext1;
			player.m_nVideoHeight = ext2;
		}		
		else if (what == QC_MSG_SNKA_NEW_FORMAT)
		{
			player.m_nSampleRate = ext1;
			player.m_nChannels = ext2;

			if (player.m_nSampleRate == 0 && player.m_nChannels == 0){
				if (player.m_AudioRender != null)
					player.m_AudioRender.closeTrack();
				return;
			}

			if (player.m_AudioRender == null)
				player.m_AudioRender = new AudioRender(player.m_context, player);
			player.m_AudioRender.openTrack (player.m_nSampleRate, player.m_nChannels);
			if (player.m_nPlaySpeed != 0)
				player.m_AudioRender.setSpeed(player.m_nPlaySpeed);
			return;
		}
		else if (what == QC_MSG_RTMP_METADATA) {
			if (player.m_EventListener != null)
				player.m_EventListener.onEvent(what, ext1, ext2, obj);
			return;
		}

		Message msg = player.m_hHandler.obtainMessage(what, ext1, ext2, obj);
		msg.sendToTarget();
	}
		
	private static void audioDataFromNative(Object baselayer_ref, byte[] data, int size, long lTime)
	{
		// Log.v("audioDataFromNative", String.format("Size %d  Time  %d", size, lTime));
		MediaPlayer player = (MediaPlayer)((WeakReference)baselayer_ref).get();
		if (player == null) 
			return;
		if (player.m_AudioRender != null)
			player.m_AudioRender.writeData(data,  size);
	}
	
	private static void videoDataFromNative(Object baselayer_ref, byte[] data, int size, long lTime, int nFlag)
	{
		MediaPlayer player = (MediaPlayer)((WeakReference)baselayer_ref).get();
		if (player == null || player.m_EventListener == null)
			return;
		player.m_EventListener.OnRevData(data, size, nFlag);
	}

	class msgHandler extends Handler {
		public msgHandler() {
		}
		public void handleMessage(Message msg) {
			if (m_NativeContext == 0)
				return;
			if (msg.what == QC_MSG_VIDEO_HWDEC_FAILED) {
				nativeUninit(m_NativeContext);
				m_NativeContext = nativeInit(m_pObjPlayer, m_strApkPath, 0);
				if (m_NativeSurface != null)
					nativeSetView(m_NativeContext, m_NativeSurface);
				nativeOpen (m_NativeContext, m_strURL, 0);
				//return;
			}
			if (msg.what == QC_MSG_PLAY_OPEN_DONE) {
				OnOpenComplete ();
			}
			if (m_EventListener != null) {
				m_EventListener.onEvent(msg.what, msg.arg1, msg.arg2, msg.obj);
			}
			if (msg.what == QC_MSG_SNKV_NEW_FORMAT) {
				SetParam(PARAM_PID_EVENT_DONE, 0, null);
			}
		}
	}

	// the native functions
    private native long	nativeInit(Object player, String apkPath, int nFlag);
    private native int 	nativeUninit(long nNativeContext);
    private native int 	nativeSetView(long nNativeContext, Object view);
    private native int 	nativeOpen(long nNativeContext,String strPath, int nFlag);
    private native int 	nativePlay(long nNativeContext);
    private native int 	nativePause(long nNativeContext);
    private native int 	nativeStop(long nNativeContext);
    private native long	nativeGetPos(long nNativeContext);
    private native int 	nativeSetPos(long nNativeContext,long lPos);
    private native long nativeGetDuration(long nNativeContext);
	private native int 	nativeGetParam(long nNativeContext,int nParamId, int nParam, Object objParam);
    private native int 	nativeSetParam(long nNativeContext,int nParamId, int nParam, Object objParam);
}
