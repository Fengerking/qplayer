package com.ali.music.media.cameracollection;


import java.io.File;
import java.io.FileOutputStream;

import android.app.Activity;
import android.hardware.Camera.PreviewCallback;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SurfaceHolder.Callback;


import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.Size;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;

public class CameraActivity extends Activity implements Callback, PreviewCallback {

	private static String TAG = "CameraActivity";
	private Button btn_start;
	private Button btn_stop;
	private Button btn_swap;
	private Button btn_opencamera;
	private Button btn_closecamera;
	 
	private SurfaceView mSurfaceView = null;  
    private SurfaceHolder mSurfaceHolder = null;   
	private CollectionControl m_control;
 
	private int videoWidth = 640;
	private int videoHight = 480;
    int framerate = 20;
    int codecType = 264;
    int bitrate = 512000;
    
    private boolean				mVisibility = true;
    private View mProgressTouchArea;
    
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_camera);
		
		//��ҳ���������
        Bundle bundle = this.getIntent().getExtras();
        //����nameֵ
        String codec = bundle.getString("codec");
        if(codec.equals("HEVC")) {
        	codecType = 265;
        } else {
        	codecType = 264;
        }

		videoWidth = bundle.getInt("width");
		videoHight = bundle.getInt("height");
		bitrate = bundle.getInt("bitrate");
		bitrate = bitrate*1000;
        
        Log.e(TAG, "codecType = "+codecType);
		Log.e(TAG, "width = "+videoWidth + " Height = " + videoHight + " bitrate = " + bitrate);
   
        btn_stop = (Button)findViewById(R.id.button_first);
        btn_swap = (Button)findViewById(R.id.btn_swap);
        btn_start = (Button)findViewById(R.id.button1);
        //btn_opencamera = (Button)findViewById(R.id.button2);
        //btn_closecamera = (Button)findViewById(R.id.button3);
       
        btn_stop.setOnClickListener(new StopListener());
        btn_swap.setOnClickListener(new SwapListener());
        btn_start.setOnClickListener(new StartListener());
        //btn_opencamera.setOnClickListener(new openListener());
        //btn_closecamera.setOnClickListener(new closeListener());
        
        mSurfaceView = (SurfaceView) this.findViewById(R.id.surfaceView1);  
        mSurfaceHolder = mSurfaceView.getHolder();  
        mSurfaceHolder.addCallback(this);  
        //mSurfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        
        initTouchArea();
        
        m_control = new CollectionControl();
        m_control.setCameraType(CollectionControl.CAMERA_BACK);
 
        //m_control.SetCollectionType(RtmpPublish.Collection_Live);
        m_control.setAudioParameter(44100, 2);
        m_control.setVideoParameter(videoWidth, videoHight,framerate, bitrate, codecType);
        m_control.setNotifyEventListener(mNotifyEventListener);
        if(codecType == 264) {
        	m_control.setPublishUrl("rtmp://push.ws.live.dongting.com/live/8bf0c474-3687-499a-864b-02b7413cf27d");
        } else {
        	//m_control.setPublishUrl("rtmp://push.aliyun.dongting.com/alimusic/e5e28469-e637-471c-a061-13487dc07aa9");
        	m_control.setPublishUrl("rtmp://push.ws.live.dongting.com/live/ebae6269-0be1-4948-82a9-26c34ee2200d");
        }
        //m_control.setFilePath("/sdcard/uu.flv");
        m_control.init();
    }
	/*
	class openListener implements OnClickListener {
		public void onClick(View v) {
			if (m_control != null) {
				m_control.CreateCamera();
			}
		}
	};

	class closeListener implements OnClickListener {
		public void onClick(View v) {
			if (m_control != null) {
				m_control.CloseCamera();
			}
		}
	};*/

	class StartListener implements OnClickListener
	    {
	        public void onClick(View v)
	        {    
				if (m_control != null) {
					m_control.StartLive();
				}
	        }
	    };
	
    class StopListener implements OnClickListener
    {
        public void onClick(View v)
        {    
			if (m_control != null) {
				Bitmap aa = m_control.getLastPic();
				FileOutputStream fOut = null;
				File f = new File("/sdcard/2.jpg");
				try {
					f.createNewFile();
					fOut = new FileOutputStream(f);
				} catch (Exception e) {
					// TODO Auto-generated catch block
				}
				aa.compress(Bitmap.CompressFormat.JPEG, 100, fOut);
				try {
					fOut.flush();
					fOut.close();
				} catch (Exception e) {
					// TODO Auto-generated catch block
				}

				m_control.StopPush();
				//m_control.CloseCamera();
				//m_control = null;
			}
        }
    };
    
    class SwapListener implements OnClickListener
    {
        public void onClick(View v)
        {    
        	if (m_control != null){
        		m_control.swap();
        	} 
        }
    };
    
	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		if (m_control != null){
			m_control.SetSufaceParameter(holder,this,this);
			m_control.CreateCamera();
		}
		
		LayoutParams lp = mSurfaceView.getLayoutParams();

		int nWidth = videoHight;
		int nHeight = videoWidth;
		int nMaxOutW = mSurfaceView.getWidth();
		int nMaxOutH = mSurfaceView.getHeight();
		int w = 0, h = 0;
		
		//Log.e(TAG, " nWidth = "+nWidth+", nHeight = "+nHeight + " nMaxOutW = "+nMaxOutW+", nMaxOutH = "+ nMaxOutH);

		if (nMaxOutW * nHeight > nWidth * nMaxOutH)
		{
			h = nMaxOutH;
			w = nMaxOutH * nWidth / nHeight;
		}
		else
		{
			w = nMaxOutW;
			h = nMaxOutW * nHeight / nWidth;
		}
		//w &= ~0x7;
		//h &= ~0x3;
		
		//Log.e(TAG, " nWidth = "+nWidth+", nHeight = "+nHeight + " nMaxOutW = "+nMaxOutW+", nMaxOutH = "+ nMaxOutH + " w = "+w+", h = "+ h);

		lp.width = w;
		lp.height = h;
		
		mSurfaceView.setLayoutParams(lp);
	}


	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width,
			int height) {
		//initCamera(holder);
		//Log.e(TAG, " width = "+width+", height = "+height);
	}


	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		// TODO Auto-generated method stub
		if (m_control != null){
			m_control.CloseCamera();
			m_control.StopPush();
			//m_control = null;
		}
	}
	
	public void onConfigurationChanged(Configuration newConfig) {  
        try {  
            super.onConfigurationChanged(newConfig);  
            if (this.getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {  
            } else if (this.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {  
            }  
        } catch (Exception ex) {  
        }  
    }
	
	@Override
	public void onPreviewFrame(byte[] data, Camera camera) {
		if (data == null) {
			// It appears that there is a bug in the camera driver that is asking for a buffer size bigger than it should
			Parameters params = camera.getParameters();
			Size size = params.getPreviewSize();
			int bufferSize = (size.width * size.height * ImageFormat.getBitsPerPixel(params.getPreviewFormat())) / 8;
			bufferSize += bufferSize / 20;
			camera.addCallbackBuffer(new byte[bufferSize]);
		} else {
			if (m_control != null)
				m_control.FeedVideoData(data);

			camera.addCallbackBuffer(data);
		}
	}  	

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.camera, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
	
	private RtmpPublish.OnMediaNotifyEventListener mNotifyEventListener = new RtmpPublish.OnMediaNotifyEventListener() {
		@Override
		public void onMediaNotify(int aMsgId, int aArg1, int aArg2) {
			Log.e(TAG, "aMsgId = "+aMsgId);
		switch (aMsgId) {
		/*case RtmpPublish.ENotifyResetEncoder:
			Log.e(TAG, "aMsgId = "+aMsgId+"  ,arg1 = "+aArg1);
			m_control.ResetEncode(15, aArg1);*/
		case RtmpPublish.ENotifyUrlParseError:
			break;
		case RtmpPublish.ENotifyOpenSucess:
			Log.e(TAG, "ENotifyOpenSucess");
			break;
		case RtmpPublish.ENotifyOpenStart:
			Log.e(TAG, "ENotifyOpenStart");
			break;
		case RtmpPublish.ENotifyNetReconnectUpToMax	:
			Log.e(TAG, "ENotifyNetReconnectUpToMax");
			break;
		case RtmpPublish.ENotifySokcetConnectFailed:
			Log.e(TAG, "SokcetConnectFailed" + " errortype= "+aArg1 +" Sokcetcode = "+aArg2);
			break;
		case RtmpPublish.ENotifyStreamConnectFailed:
			Log.e(TAG, "StreamConnectFailed"+ " Sokcetcode = "+aArg2);
			break;
		case RtmpPublish.ENotifyTransferError:
			Log.e(TAG, "TransferError"+ " Sokcetcode = "+aArg2);
			break;
		}
		}
	};
	
    private void initTouchArea() {
        mProgressTouchArea = findViewById(R.id.touch_area);
        mProgressTouchArea.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                int width = mProgressTouchArea.getWidth();

                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        Log.v(TAG, " onTouch ACTION_DOWN +++++");
                        Log.v(TAG, " action down");
                        break;
                    case MotionEvent.ACTION_UP:
                    case MotionEvent.ACTION_CANCEL:
                    	Log.v(TAG, " onTouch ACTION_UP +++++");
                      	
                    	if(mVisibility) {
                    		mVisibility = false;
                    		
                    		btn_start.setVisibility(View.GONE);
                    		btn_stop.setVisibility(View.GONE);
                    		btn_swap.setVisibility(View.GONE);
                    	} else {
                    		mVisibility = true;
                    		
                    		btn_start.setVisibility(View.VISIBLE);
                    		btn_stop.setVisibility(View.VISIBLE);
                    		btn_swap.setVisibility(View.VISIBLE);
                    	}

                        break;
                    case MotionEvent.ACTION_MOVE:
             
                        break;
                    default:
                        break;
                }
                return true;
            }
        });
    }
}
