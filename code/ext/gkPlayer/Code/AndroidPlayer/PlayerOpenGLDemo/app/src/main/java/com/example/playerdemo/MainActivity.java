package com.example.playerdemo;

import com.goku.media.player.GKMediaPlayer;
import com.goku.media.player.VideoSurfaceView;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.view.View.OnClickListener;
import android.view.*;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.graphics.PixelFormat;
import android.view.ViewGroup;
import android.util.DisplayMetrics;
import android.content.Intent;  
import android.content.IntentFilter;  
import android.content.res.Configuration;
import android.util.Log;
import android.opengl.GLSurfaceView;


public class MainActivity extends Activity {
	private static final String TAG = "@@@Player Example";
	MediaPlayerProxy mPlayProxy = null;
	VideoSurfaceView mGlSurfaceView = null;
	SDCardListener	 mSDCardListener = null;	
	Button btnPauseResume;
	String defaulturl;
	String cachePath ;
	EditText urlEdit;

	Button btn_play;
	Button btn_pause;
	Button btn_stop;
	Button btn_seek;
	Button btn_render;

	private static int position;

	private int mSCURl = 0;
	int	   seekn = 0;
	private boolean				mOnlineSrc = false;
	
	private boolean				mLowDelay = true;

	private boolean				mVisibility = true;

	private int					mRenderType = 0;

	private int					mCount = 0;

	private SurfaceView			surfaceView;
	
	private View mProgressTouchArea;

    
	@SuppressWarnings("deprecation")
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getWindow().setFormat(PixelFormat.UNKNOWN);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		
		//initGravitySensor();
		
		setContentView(R.layout.activity_main);
		String pluginPath = "/data/data/com.example.playerdemo/lib";
		Log.i(TAG, "onCreate" + position + "mPlayProxy :" + mPlayProxy);
		if(mPlayProxy != null) {
			mPlayProxy.release();
			mPlayProxy = null;
		}
		mPlayProxy = new MediaPlayerProxy(pluginPath);

		btn_play = (Button) findViewById(R.id.button1);
		btn_pause = (Button) findViewById(R.id.button2);
		btn_stop = (Button) findViewById(R.id.button3);
		btn_seek = (Button) findViewById(R.id.buttonSeek);
		btn_render = (Button) findViewById(R.id.Render);

		String checkpath = Environment.getExternalStorageDirectory().getPath();
		
		checkpath = checkpath + "/ttpod/song";
		
		Log.i(TAG, "SDCardListener checkpath" + checkpath);
		
		mSDCardListener	= new SDCardListener(checkpath);
		
		mSDCardListener.startWatching();
		
		btnPauseResume = (Button) findViewById(R.id.button2);

		mGlSurfaceView = (VideoSurfaceView) findViewById(R.id.VideoView);
		
		mGlSurfaceView.setGLSurfaceViewEvenListener(mGLSurfaceNotifyEventListener);

		//setContentView(mGlSurfaceView);
	
		//surfaceView = (SurfaceView) this.findViewById(R.id.surfaceView);
		//surfaceView.getHolder().addCallback(new SurfaceCallback());

		//mPlayProxy.setView(surfaceView);
		        
    	DisplayMetrics dm  = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(dm);
		mPlayProxy.setViewSize(dm.widthPixels, dm.heightPixels);

		Log.i(TAG, "dm.widthPixels:" + dm.widthPixels + " dm.heightPixels " + dm.heightPixels);

		//mOnlineSrc = true;
		defaulturl = "/sdcard/MTV.mp4";
		mOnlineSrc = false;
		cachePath = "/sdcard/audio5.tmp";

		initTouchArea();

		//play
		((Button)findViewById(R.id.button1)).setOnClickListener(new OnClickListener(){
			public void onClick(View view){
				try {
					mGlSurfaceView.setRenderType(mRenderType);
					mPlayProxy.setSurface(mGlSurfaceView.getSurface());
					mPlayProxy.palyVideo(defaulturl, 1);
					mGlSurfaceView.startRender();
				} catch (Exception e) {

				}
			}
		});

		// pause
		btnPauseResume.setOnClickListener(new OnClickListener() {
					public void onClick(View view) {
						try {
							String str = btnPauseResume.getText().toString();
							if (str.equals("Pause")) {
								mPlayProxy.pause(true);
								btnPauseResume.setText("Resume");
							} else {
								mPlayProxy.resume(true);
								btnPauseResume.setText("Pause");
							}
						} catch (Exception e) {
						}
					}
				});

		btn_render.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {
				try {
					mCount++;
					if(mCount == 4) {
						mCount = 0;
					}

					if(mCount == 0) {
						mRenderType = VideoSurfaceView.ERenderGlobelView;
					} else if(mCount == 1) {
						mRenderType = VideoSurfaceView.ERenderGlobelView | VideoSurfaceView.ERenderSplitView;
					} else if(mCount == 2) {
						mRenderType = VideoSurfaceView.ERenderDefault;
					} else if(mCount == 3) {
						mRenderType = VideoSurfaceView.ERenderSplitView;
					}

					mGlSurfaceView.setRenderType(mRenderType);

				} catch (Exception e) {
				}
			}
		});

		// stop
		((Button) findViewById(R.id.button3))
				.setOnClickListener(new OnClickListener() {
					public void onClick(View view) {
						try {
							mPlayProxy.stop();
							position = 0;
						} catch (Exception e) {

						}
					}
				});
		
		// seek
		((Button) findViewById(R.id.buttonSeek))
				.setOnClickListener(new OnClickListener() {
						public void onClick(View view) {
						try {
							int pos = mPlayProxy.getPosition();

							if(seekn == 0) {
								if(pos + 50000 > mPlayProxy.duration()) {
									pos -= 50000;
									if(pos <= 0) pos = 0;
									seekn = 1;
								} else {
									pos += 50000;
								}
							} else {
								if(pos - 50000 <= 0) {
									pos += 50000;
									if(pos >= mPlayProxy.duration()) pos = mPlayProxy.duration();
									seekn = 0;
								} else {
									pos -= 50000;
								}
							}

							Log.i(TAG, "Current Position:" + mPlayProxy.getPosition() + " Set Position " + pos);

							mPlayProxy.setPosition(pos, GKMediaPlayer.SEEK_FAST);
						} catch (Exception e) {

						}
					}
				});

	}

	 @Override  
	 protected void onResume() {  
	       super.onResume();
	       mGlSurfaceView.onResume();

	       Log.i(TAG, "onResume");
	  }  
	  
	   @Override  
	   protected void onPause() {  
	       super.onPause();
	       mPlayProxy.pause(false);
	       btnPauseResume.setText("Resume");
	       mPlayProxy.setSurface(null);
	       mGlSurfaceView.onPause();
	   }  
	
	
	private final class SurfaceCallback implements SurfaceHolder.Callback{
		@Override
		public void surfaceCreated(SurfaceHolder holder) {
			Log.i(TAG, "Surface Created... position:" + position);
			if(position>0){
				try {
						Log.i(TAG, "Surface Created, set resume");	
						if(mPlayProxy.getPlayStatus() == PlayStatus.STATUS_PAUSED){
							mPlayProxy.setView(surfaceView);
							mPlayProxy.setPosition(position, GKMediaPlayer.SEEK_CORRECT);
						}
						else
						{
							try {
								String playurl = urlEdit.getText().toString();
								mPlayProxy.palyVideo(playurl, 1);
								mPlayProxy.setPosition(position, GKMediaPlayer.SEEK_CORRECT);
							} catch (Exception e) {

							}
						}						
						position = 0;						
				} catch (Exception e){

				}
			}
		}
		@Override
		public void surfaceChanged(SurfaceHolder holder, int format, int width,
				int height) {
		}
		@Override
		public void surfaceDestroyed(SurfaceHolder holder) {
			Log.i(TAG, "Surface Destroyed...");
			if(mPlayProxy != null && (mPlayProxy.getPlayStatus() == PlayStatus.STATUS_PLAYING || mPlayProxy.getPlayStatus() == PlayStatus.STATUS_PAUSED)){
				mPlayProxy.setView(null);
				mPlayProxy.pause(false);
				position = mPlayProxy.getPosition();
				
				Log.i(TAG, "Surface Destroyed, set pause, and position:" + position);
			}
		}    	
    }
	
	public void onConfigurationChanged(Configuration newConfig) {  
		  
	    super.onConfigurationChanged(newConfig);  
	  
	    if (this.getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {  
	        // 加入横屏要处理的代码  
	    } else if (this.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {  
	        // 加入竖屏要处理的代码  
	    } 
	    
	    Log.i(TAG, "onConfigurationChanged++++++");
	}  
	
	protected void onDestroy() 
	{
		if(mPlayProxy != null) {
			mPlayProxy.release();
			mPlayProxy = null;
		}
		
		if(mSDCardListener != null) {
			mSDCardListener.stopWatching();
		}
		position = 0;
		super.onDestroy(); 		
		// TODO Auto-generated method stub
		Log.i(TAG, "Player onDestroy Completed!");
	}
	

	public boolean onKeyDown(int keyCode, KeyEvent event) 
	{
		// TODO Auto-generated method stub
		Log.v(TAG, "Key click is " + keyCode);
		if (keyCode ==KeyEvent.KEYCODE_BACK)
		{
			Log.v(TAG, "Key click is Back key");
			mPlayProxy.stop();
			return super.onKeyDown(keyCode, event);
		} else if(keyCode == KeyEvent.KEYCODE_POWER) {
			Log.v(TAG, "Key click is POWER key");
			if(mLowDelay) {
				mLowDelay = false;
				mPlayProxy.setAudioEffectLowDelay(mLowDelay);
			} else {
				mLowDelay = true;
				mPlayProxy.setAudioEffectLowDelay(mLowDelay);
			}
		} 

		return super.onKeyDown(keyCode, event);
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	private VideoSurfaceView.OnGLSurfaceViewEventListener mGLSurfaceNotifyEventListener = new VideoSurfaceView.OnGLSurfaceViewEventListener() {
		@Override
		public void onGLSurfaceViewNotify(int aMsgId, int aArg1, int aArg2) {
			Log.d(TAG, "NotifyEventMsgId:" + aMsgId);
			switch (aMsgId) {
			case VideoSurfaceView.GLSURFACEVIEW_CREATE:
				mPlayProxy.setSurface(mGlSurfaceView.getSurface());
				if(mPlayProxy.getPlayStatus() == PlayStatus.STATUS_PAUSED){
					int position = mPlayProxy.getPosition();
					mPlayProxy.setPosition(position, GKMediaPlayer.SEEK_CORRECT);
				} else if(mPlayProxy.getPlayStatus() == PlayStatus.STATUS_PLAYING) {
				}
				
				break;

			case VideoSurfaceView.GLSURFACEVIEW_CHANGED:

				break;

			default:
				break;
			}
		}
	};

	private void initTouchArea() {
		mProgressTouchArea = findViewById(R.id.touch_area);
		mProgressTouchArea.setOnTouchListener(new View.OnTouchListener() {
			private float mTouchX;
			private float mTouchY;
			private float mDiffX;
			private float mDiffY;
			private float mStartX;
			private float mStartY;

			@Override
			public boolean onTouch(View v, MotionEvent event) {

				int width = mProgressTouchArea.getWidth();

				switch (event.getAction()) {
					case MotionEvent.ACTION_DOWN:
						mTouchX = event.getX();
						mStartX = mTouchX;
						mTouchY = event.getY();
						mStartY = mTouchY;
						Log.v(TAG, " onTouch ACTION_DOWN +++++");
						Log.v(TAG, " action down");
						break;
					case MotionEvent.ACTION_UP:
					case MotionEvent.ACTION_CANCEL:
						Log.v(TAG, " onTouch ACTION_UP +++++");
						if(mVisibility) {
							mVisibility = false;

							btn_play.setVisibility(View.GONE);
							btn_pause.setVisibility(View.GONE);
							btn_stop.setVisibility(View.GONE);
							btn_seek.setVisibility(View.GONE);
							btnPauseResume.setVisibility(View.GONE);
							btn_render.setVisibility(View.GONE);
						} else {
							mVisibility = true;

							btn_play.setVisibility(View.VISIBLE);
							btn_pause.setVisibility(View.VISIBLE);
							btn_stop.setVisibility(View.VISIBLE);
							btn_seek.setVisibility(View.VISIBLE);
							btnPauseResume.setVisibility(View.VISIBLE);
							btn_render.setVisibility(View.VISIBLE);
						}

						break;
					case MotionEvent.ACTION_MOVE:
						mTouchX = event.getX();
						mTouchY = event.getY();
						mDiffX = mTouchX - mStartX;
						mDiffY = mTouchY - mStartY;
						mStartX = mTouchX;
						mStartY = mTouchY;

						mGlSurfaceView.setChangeXY(mDiffX, mDiffY);
						Log.v(TAG, " onTouch ACTION_MOVE mDiffX " + mDiffX + " mDiffY " + mDiffY);

						break;
					default:
						break;
				}
				return true;
			}
		});
	}

}
