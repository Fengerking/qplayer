package com.ali.music.media.cameracollection;

import java.util.Iterator;
import java.util.List;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

@SuppressLint("NewApi")
public class CollectionControl {

	private static String TAG = "CollectionControl";
	private RtmpPublish mRtmpPush;
	private AudioRecordClient m_audio = null;
	private AVEncoder m_videoEncoder = null;
	private Camera mCamera = null;
	private int m_samplrate;
	private int m_channel;
	private int m_videoWidth;
	private int m_videoHight;
	private int m_framerate;
	private int m_bitrate;
	private int m_codecType;
	private boolean mPreviewRunning = false;
	private boolean mSupport = false;
	private int mSysfps = 0;
    private byte[] mRotateArray ;
	private MediaCodecInfo.CodecCapabilities m_colorCapab;
	private int m_CameraType;
	private boolean m_resolution = false;
	//private int[] CurPreRange = new int[2];
	private long m_preSecond = 0;
	private int m_tick =0;
	private boolean m_ajust = false;
	private int m_discardType ;
	private int m_discardCnt = 0;
	private int m_Cnt = 0;
	private SurfaceHolder mholder = null;
	private PreviewCallback mcb = null;
	private boolean swapflag = false; 
	private boolean mHardEncoder = true;
	private int swapCnt = 0; 
	private boolean m_startLive = false;
	private RtmpPublish.OnMediaNotifyEventListener mListen = null;
	private int m_color;
	private String m_url;
	//private int mtest = 0;
	private boolean mSetCallback = true;
	private int mType = 0;
	private int mDisaplayOritation = 90;
	private int mDefaultOritation;
	private int mVideoCount = 0;
	//private boolean mResetEncode = false;
	
    public static final int CAMERA_FRONT  = 0x00;
    public static final int CAMERA_BACK   = 0x01;
    public static final int DISCARD_1PER5  = 0x01;
    public static final int DISCARD_2PER3  = 0x02;
    public static final int DISCARD_1PER2  = 0x03;
    
    public static final int SWAP_DISCARD_MAX  = 8;
	
    public void setCameraType(int type){
    	m_CameraType = type;
    }
    
	public void setVideoParameter(int videoWidth, int videoHight,
			int framerate, int bitrate, int ncodecType) {
		if (videoWidth > m_videoHight){
			m_videoWidth = videoHight;
			m_videoHight = videoWidth;
		}
		else{
			m_videoWidth = videoWidth;
			m_videoHight = videoHight;
		}
		
		m_framerate = framerate;
		m_bitrate = bitrate;
		m_codecType = ncodecType;
	}

	public void setAudioParameter(int samplrate, int channel) {
		m_samplrate = samplrate;
		m_channel = channel;
	}

	public void setNotifyEventListener(
			RtmpPublish.OnMediaNotifyEventListener listen) {
		mListen = listen;
		if (mRtmpPush != null)
			mRtmpPush.setOnMediaNotifyEventListener(listen);
	}

	public void setPublishUrl(String url) {
		m_url = url;
	}
	
	public boolean init() {
		CheckDevice();
		if (!mSupport)
			return mSupport;
		
		mRtmpPush = new RtmpPublish();
		m_audio = new AudioRecordClient();
		m_audio.setPublishObj(mRtmpPush);
		if (false == m_audio.init(m_samplrate, m_channel))
			return false;
		
		mRtmpPush.SetcollectType(mType);
		mRtmpPush.SetBitrate(m_bitrate);
		mRtmpPush.setVideoCodecType(m_codecType);
		
		if (mRtmpPush != null && mListen != null)
			mRtmpPush.setOnMediaNotifyEventListener(mListen);
		
		if (mHardEncoder)
			m_videoEncoder = new AVEncoder(mRtmpPush); 
		
		Log.e(TAG, "Audio encode init ok");
		
		if (mHardEncoder == false){
			if (mRtmpPush != null) {
				mRtmpPush.setVideoWidthHeight(m_videoWidth, m_videoHight);
				mRtmpPush.setVideoFpsBitrate(m_framerate, m_bitrate);
				mRtmpPush.VideoEncoderInit();
			}
		}
		else{
			if (m_videoEncoder != null){
				m_videoEncoder.setVideoParameter(m_videoWidth,m_videoHight, m_framerate, m_bitrate, m_codecType);
				m_videoEncoder.videoEncodeInit(m_color);
			}
		}
		
		m_audio.start();
		
		return true;
	}

	public boolean StartLive() {
		if (!mSupport)
			return mSupport;
		
		if (mRtmpPush == null){
			mRtmpPush = new RtmpPublish();
			mRtmpPush.SetcollectType(mType);
			mRtmpPush.SetBitrate(m_bitrate);
			mRtmpPush.setVideoCodecType(m_codecType);
			if (mRtmpPush != null && mListen != null)
				mRtmpPush.setOnMediaNotifyEventListener(mListen);
			if (mHardEncoder == false){
				mRtmpPush.setVideoWidthHeight(m_videoWidth, m_videoHight);
				mRtmpPush.setVideoFpsBitrate(m_framerate, m_bitrate);
				mRtmpPush.VideoEncoderInit();
			}
		}
		
		mRtmpPush.setVideoCodecType(m_codecType);
		
		if (m_audio == null){
			m_audio = new AudioRecordClient();
			m_audio.setPublishObj(mRtmpPush);
			if (false == m_audio.init(m_samplrate, m_channel))
				return false;
			m_audio.start();
		}
		
		if (mHardEncoder && m_videoEncoder == null){
			m_videoEncoder = new AVEncoder(mRtmpPush); 
			m_videoEncoder.setVideoParameter(m_videoWidth,m_videoHight, m_framerate, m_bitrate, m_codecType);
			m_videoEncoder.videoEncodeInit(m_color);
		}
		
		if (m_videoEncoder != null){
			m_videoEncoder.start();
		}
		m_audio.startEncode();
		mRtmpPush.startPublish(m_url);
		m_startLive = true;
		return mSupport;
	}
	
	public void swap(){
		Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
		if(m_CameraType == CAMERA_FRONT){
			for(int i =0; i<Camera.getNumberOfCameras();i++){
				Camera.getCameraInfo(i, cameraInfo); 
				if ( cameraInfo.facing ==Camera.CameraInfo.CAMERA_FACING_BACK) { 
					mCamera.stopPreview();
					mCamera.release();
					mCamera = null;
					mCamera = Camera.open(i);
					calOritation(cameraInfo);
					InitCamera();
					m_CameraType = CAMERA_BACK;
					break;
				}
			}
		}
		else{
			for(int i =0; i<Camera.getNumberOfCameras();i++){
				Camera.getCameraInfo(i, cameraInfo); 
				if ( cameraInfo.facing ==Camera.CameraInfo.CAMERA_FACING_FRONT) { 
					mCamera.stopPreview();
					mCamera.release();
					mCamera = null;
					mCamera = Camera.open(i);
					calOritation(cameraInfo);
					InitCamera();
					m_CameraType = CAMERA_FRONT;
					break;
				}
			}
		}
		swapflag = true;
	}

	public void SetSufaceParameter(SurfaceHolder holder, PreviewCallback cb, Activity act) {
		mholder = holder;
		mcb = cb;
		if (act != null){
			mDefaultOritation = act.getWindowManager().getDefaultDisplay().getRotation();
		}
		else{
			mDefaultOritation = Surface.ROTATION_0;
		}
	}

	private void InitCamera() {
		if (mSupport == false)
			return ;
		
		if (mPreviewRunning)
			mCamera.stopPreview();

		int width = m_videoWidth;
		int Height = m_videoHight;
		if(m_videoWidth == 360 && m_videoHight == 640) {
			width = 720;
			Height = 1280;
		}

		// probe resolution
		List<Camera.Size> sizeList = mCamera.getParameters()
				.getSupportedPreviewSizes();
	
		if (sizeList.size() > 1) {
			Iterator<Camera.Size> itor = sizeList.iterator();
			while (itor.hasNext()) {
				Camera.Size cur = itor.next();
				//Log.e(TAG, "cw = "+cur.width + "ch = "+cur.height);
				if (width== cur.height && Height == cur.width) {
					m_resolution = true;
					//break;
				}
			}
		}

		if (m_resolution == false) {
			mSupport = false;
			Log.e(TAG, "resolution set failed");
			return;
		}

		Camera.Parameters parameters = mCamera.getParameters();
		parameters.setPreviewSize(Height,width);
		 
		parameters.setPreviewFormat(ImageFormat.NV21);
		//Configuration.ORIENTATION_LANDSCAPE
		parameters.set("orientation", "portrait"); //"landscape"
		//parameters.set("rotation", 0); //
		mCamera.setDisplayOrientation(mDisaplayOritation); //(0)
		
		// probe FpsRange mSysfps
		//mSysfps = parameters.getPreviewFrameRate();
		//if (mSysfps >= 20)//15
		//	parameters.setPreviewFrameRate(20);
		
		List<int[]> SupPreFpsRangeList = parameters.getSupportedPreviewFpsRange();  
        int SupPreRange[] = new int[2];
        int MinPreRange[] = new int[2];
        
        int nDiff = 0;
        int nMinDiff = 0xffff;
  
        Log.e(TAG, "Support Preview FPS Range List :  " );  
        for(int num = 0; num < SupPreFpsRangeList.size(); num++)  
        {  
            SupPreRange = SupPreFpsRangeList.get(num);    
            Log.e(TAG, "< " + num + " >" + " Min = " + SupPreRange[0]  
                    + "  Max = " + SupPreRange[1]);
            int nMin = SupPreRange[0] - m_framerate*1000;
            if(nMin < 0) nMin = -nMin; 
            int nMax = SupPreRange[0] - m_framerate*1000;
            if(nMax < 0) nMax = -nMax;
            nDiff = nMin + nMax;
            if(nDiff < nMinDiff) {
            	MinPreRange[0] = SupPreRange[0];
            	MinPreRange[1] = SupPreRange[1];
            	nMinDiff = nDiff;
            }
        }
        
        Log.e(TAG, "< FpsRange >" + " Min = " + MinPreRange[0]  
                + "  Max = " + MinPreRange[1]);
        
        parameters.setPreviewFpsRange(MinPreRange[0], MinPreRange[1]);
		
		parameters.setZoom(0);
	
		mCamera.setParameters(parameters);
		mCamera.addCallbackBuffer(new byte[width * Height * 3 / 2]);

		mCamera.setPreviewCallbackWithBuffer(mcb);
		mSetCallback = true;
		
		try {
			mCamera.setPreviewDisplay(mholder);
		} catch (Exception ex) {
		}
		mCamera.startPreview();
		mPreviewRunning = true;
		
		// probe camera fps
        //mCamera.getParameters().getPreviewFpsRange(CurPreRange);
        //CurPreRange[0] = CurPreRange[0]/1000;CurPreRange[1] = CurPreRange[1]/1000;
	}
	
	public boolean CreateCamera() {
		if (mholder == null || mcb==null){
			return false;
		}
		Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
		for (int i = 0; i < Camera.getNumberOfCameras(); i++) {
			Camera.getCameraInfo(i, cameraInfo); 
			if ( cameraInfo.facing ==Camera.CameraInfo.CAMERA_FACING_FRONT  && m_CameraType == CAMERA_FRONT) { 
				mCamera = Camera.open(i);
				break;
			}
			else if ( cameraInfo.facing ==Camera.CameraInfo.CAMERA_FACING_BACK  && m_CameraType == CAMERA_BACK) { 
				mCamera = Camera.open(i);
				break;
			}
		}

		if(mCamera == null){
			mSupport = false;
		}
		
		calOritation(cameraInfo);

		InitCamera();

		return mSupport;
	}
	
	private void calOritation(Camera.CameraInfo info){
		int degrees = 0;  
	    switch (mDefaultOritation) {  
	        case Surface.ROTATION_0:  
	            degrees = 0;  
	            break;  
	        case Surface.ROTATION_90:  
	            degrees = 90;  
	            break;  
	        case Surface.ROTATION_180:  
	            degrees = 180;  
	            break;  
	        case Surface.ROTATION_270:  
	            degrees = 270;  
	            break;  
	    }  

	    if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {  
	    	mDisaplayOritation = (info.orientation + degrees) % 360;  
	    	mDisaplayOritation = (360 - mDisaplayOritation) % 360;   // compensate the mirror   
	    } else {  
	        // back-facing   
	    	mDisaplayOritation = ( info.orientation - degrees + 360) % 360;  
	    } 
	    
	    //Log.e(TAG, " result = " + mDisaplayOritation + " degrees = "+degrees+" c = "+info.orientation);
	}
	 
	public void CloseCamera() {
		if (mCamera != null) {
			if (mSetCallback){
				mCamera.setPreviewCallback(null);
				mSetCallback = false;
			}
			mCamera.stopPreview();
			mPreviewRunning = false;
			mCamera.release();
			mCamera = null;
		}
		
		if (m_audio != null){
			m_audio.free();
			m_audio = null;
		}
	}
	
	public void StopPush() {
		if (m_startLive == false)
			return;
		
		if (mSetCallback){
			mCamera.setPreviewCallback(null);
			mSetCallback = false;
		}
		m_startLive = false;
		
		if (m_audio != null){
			m_audio.free();
			m_audio = null;
		}
		
		if (m_videoEncoder != null){
			m_videoEncoder.close();
			m_videoEncoder = null;
		}
		if (mRtmpPush != null){
			mRtmpPush.stop();
			mRtmpPush = null;
		}
	}
	
	public long stopRecord()
	{
		long time = AVEncoder.getCurrentTime();
		StopPush();
		return time;
	}

	public void FeedVideoData(byte[] data) {
		if (mHardEncoder) {
			if (mRotateArray == null) {
				int lenth = data.length;
				if (m_videoHight == 640 && m_videoWidth == 360 )
					lenth = 640*352*3/2;
				mRotateArray = new byte[lenth];
			}
		}
		
		if (m_startLive == false)
			return;
		
		/*if(mResetEncode == true){
			mResetEncode = false;
			InnerEncoderReset();
			return;
		}*/
		
		if (IsDiscardingFrame() ||  (m_resolution == false)){
			//Log.e(TAG, " discarding... ");
			return ;
		}
		
		if (swapflag == true){
			swapCnt++;
			if(swapCnt > SWAP_DISCARD_MAX){
				swapflag = false;
				swapCnt = 0;
			}
			else{
				return;
			}
		}
		
		int type = m_CameraType;
		if(mDisaplayOritation == 270)
			type = CAMERA_BACK;
		
		if (mHardEncoder){
			mRtmpPush.HandleRawData(data,mRotateArray,m_videoHight,m_videoWidth,type);//m_CameraType
			if (m_videoEncoder != null)
				m_videoEncoder.feedDataToVideoEncoder(mRotateArray);
		}
		else{
			if (mRtmpPush != null) {
				mRtmpPush.SendVideoRawData(data,data.length,AVEncoder.getCurrentTime(),type);
			}
		}
		
		mVideoCount++;
			
		//if(mVideoCount >= 30) {
		//	mVideoCount = 0;
		//	InnerEncoderReset();
		//}
	}
	
	public void ResetEncode(int fps, int bitrate){
		//mResetEncode = true;
		m_framerate = fps;
		m_bitrate = bitrate;
	}
	
	private void InnerEncoderReset()
	{
		//suport for hard encoder
		if (m_videoEncoder != null){
			m_videoEncoder.close();
			m_videoEncoder = null;
		}
		
		if (mHardEncoder && m_videoEncoder == null){
			m_videoEncoder = new AVEncoder(mRtmpPush); 
			mRtmpPush.SetBitrate(m_bitrate);
			m_videoEncoder.setVideoParameter(m_videoWidth,m_videoHight, m_framerate, m_bitrate, m_codecType);
			m_videoEncoder.videoEncodeInit(m_color);
		}
		
		if (m_videoEncoder != null){
			m_videoEncoder.start();
		}
	}
	
	private boolean IsDiscardingFrame(){
		boolean ret = false;
		long currentTime = System.currentTimeMillis();
		if (m_preSecond == 0){
			m_preSecond = currentTime;
			m_discardType = 0;
			m_ajust = false;
		}
	 
		long diff = currentTime - m_preSecond;
		m_tick++;
		m_Cnt++;
		if (diff > 1000){
			//Log.e(TAG, " 1s gone, " + diff+" ,useCnt = "+mtest+ ", total ="+m_tick);
			//mtest = 0;
			m_preSecond = currentTime;
			m_ajust = true;
			m_Cnt = 0;
			if (m_framerate == 15)
			{
				ajustfor15fps();
			}
			else if(m_framerate == 20){
				ajustfor20fps();
			}
			else{
				m_framerate = 15;
			}
			m_tick  = 0;
			return false;
		}
		//Log.e(TAG, " delta =  " + delta);
		if (m_ajust){
			if (m_framerate == 20){
				if (m_Cnt%m_discardCnt == 0)
					ret = true;
				else
					ret = false;
			}
			else{
				if (m_discardType == DISCARD_1PER2 || m_discardType == DISCARD_2PER3){
					if (m_Cnt%m_discardCnt == 0)
						ret = false;
					else
						ret = true;
				}
				else if (m_discardType == DISCARD_1PER5){
					if (m_Cnt%m_discardCnt == 0){
							ret = true;
					}
					else{
							ret = false;
					}
				}
			}
		}
		//if (ret == false) 
		//	mtest++;
		 
		return ret;
	}
	
	private void ajustfor15fps(){
		if (m_tick < 20)
			m_ajust = false;
		else if(m_tick >= 20 && m_tick <27){
			m_discardType = DISCARD_1PER5;
			m_discardCnt = 5;
		}
		else if(m_tick >=27 && m_tick <40){
			m_discardType = DISCARD_1PER2;
			m_discardCnt = 2;
		}
		else{
			m_discardType = DISCARD_2PER3;
			m_discardCnt = 3;
		}
	}
	
	private void ajustfor20fps(){
		if (m_tick <= 22)
			m_ajust = false;
		else if(m_tick > 22 && m_tick <27){
			m_discardCnt = 6;
		}
		else if(m_tick >=27 && m_tick <32){
			m_discardCnt = 4;
		}
		else if(m_tick >=32 && m_tick <37){
			m_discardCnt = 3;
		}
		else{
			m_discardCnt = 2;
		}
	}

	public boolean IsSupported() {
		return mSupport;
	}

	@SuppressLint("NewApi")
	private void CheckDevice() {
		// first check android OS version
		int version = 0;
		try {
			version = Integer.valueOf(android.os.Build.VERSION.SDK);
		} catch (NumberFormatException e) {
			Log.e(TAG, e.toString());
		}
		if (version < 16) {
			mSupport = false;
			Log.e(TAG, "Android OS version lower than 4.1");
			return;
		}
			// second check device encode sets
		boolean videoEncode = false;
		String mineType = "video/avc";
		
		if(m_codecType == 265) {
			mineType = "video/hevc";
		}
	
		int numCodecs = MediaCodecList.getCodecCount();
		MediaCodecInfo codecInfo = null;
		for (int i = 0; i < numCodecs && codecInfo == null; i++) {
			MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
			if (!info.isEncoder()) {
				continue;
			}
			String[] types = info.getSupportedTypes();

			for (int j = 0; j < types.length; j++) {
				if (types[j].equals(mineType)) {
					videoEncode = true;
					codecInfo = info;
					}
			}
			if (videoEncode)
				break;
		}

		if(!videoEncode){
			//mSupport = false;
			mHardEncoder = false;
			Log.e(TAG, "device hard-encoder not support!");
			return;
		}
		
		m_colorCapab = codecInfo.getCapabilitiesForType(mineType);
		m_color = colorformatQuery();

		mSupport = true;
	}
	
	public Bitmap getLastPic() {
		Bitmap tmp = null;
		if (mRtmpPush != null) {
			if (m_videoHight == 640 && m_videoWidth ==360){
				if (mHardEncoder){
					int[] data = new int[352*640];
					mRtmpPush.GetLastPic(data,352*640);
					tmp = Bitmap.createBitmap(data,352, 640,Config.ARGB_8888);
				}
				else{
					int[] data = new int[m_videoWidth*m_videoHight/4];
					mRtmpPush.GetLastPic(data,m_videoWidth*m_videoHight/4);
					tmp = Bitmap.createBitmap(data,m_videoWidth/2, m_videoHight/2,Config.ARGB_8888);
				}
			}
			else{
				int[] data = new int[m_videoWidth*m_videoHight];
				mRtmpPush.GetLastPic(data,m_videoWidth*m_videoHight);
				tmp = Bitmap.createBitmap(data,m_videoWidth, m_videoHight,Config.ARGB_8888);
			}
		}
		return tmp;
	}
	
	public void SetCollectionType(int type)
	{
		mType = type;
	}
	
	private int colorformatQuery() {		
		int YUV420Planar = 0;
		int YUV420SemiPlanar = 0;
		for (int i = 0; i < m_colorCapab.colorFormats.length; i++) {
			if (m_colorCapab.colorFormats[i] == MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar)
				YUV420Planar++;
			if (m_colorCapab.colorFormats[i] == MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar)
				YUV420SemiPlanar++;
		}

		if (YUV420SemiPlanar > 0){
			return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;
		}
		
		if (YUV420Planar > 0){
			return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar;
		}

//	      COLOR_TI_FormatYUV420PackedSemiPlanar COLOR_FormatSurface
		return -1;
	}
}

 
