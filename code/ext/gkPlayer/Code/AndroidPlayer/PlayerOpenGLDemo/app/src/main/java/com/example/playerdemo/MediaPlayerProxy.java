package com.example.playerdemo;

import android.util.Log;

import com.goku.media.player.*;
import android.view.SurfaceView;
import android.view.Surface;

public final class MediaPlayerProxy {

	public static final int HUNDRED = 100;
	private final int  K_FREQUENCY_NUMBER = 512;
	private short[] mFreqBuffer = new short[K_FREQUENCY_NUMBER];
	private PlayStatus mPlayStatus = PlayStatus.STATUS_STOPPED;
	private volatile GKMediaPlayer mGKMediaPlayer;
	private IMediaPlayer mCurMediaPlayer;
	private static final String TAG = "MediaPlayerDemo";
	
	private SurfaceView			mSurfaceView = null;
	
	private int 				mWidth = 0;
	private int					mHeight = 0;	
		
	private int 				mScreenWidth = 0;
	private int					mScreenHeight = 0;
	
	private boolean				mIsMV = false;
    /**
     * 锟斤拷锟届函锟斤拷
     *
     * @param context context
     */
    public MediaPlayerProxy(String pluginPath) {
    	mCurMediaPlayer = buildGKMediaPlayer(pluginPath);
    }
    
    /**
     * 锟斤拷锟矫诧拷锟斤拷锟斤拷路锟斤拷
     *
     * @param sourcePath 源路锟斤拷
     * @param songId     锟斤拷锟斤拷id
     * @throws Exception Exception
     */
    public void play(String sourcePath) throws Exception {
    	if(mCurMediaPlayer != null)
    		mCurMediaPlayer.setDataSourceAsync(sourcePath, GKMediaPlayer.OPEN_BUFFER);
    }
    
    /**
     * 锟斤拷锟斤拷锟斤拷锟竭革拷锟斤拷锟斤拷锟斤拷锟斤拷路锟斤拷
     *
     * @param sourcePath    源路锟斤拷
     * @param songId        锟斤拷锟斤拷id
     * @param cacheFilePath 锟斤拷锟斤拷锟侥硷拷锟斤拷锟斤拷路锟斤拷
     * @throws Exception Exception
     */
    public void play(String sourcePath, String cacheFilePath) throws Exception {
    	if(mCurMediaPlayer != null) {
    		mCurMediaPlayer.setCacheFilePath(cacheFilePath);
    		mCurMediaPlayer.setDataSourceAsync(sourcePath, GKMediaPlayer.OPEN_DEFAULT);
    	}
    }
    
    public void stop() {
        //if(mCurMediaPlayer != null)
        	mCurMediaPlayer.stop();
        mPlayStatus = PlayStatus.STATUS_STOPPED;
    }
    
    public void palySong(String sourcePath) throws Exception{
    	stop();
        play(sourcePath);
        mPlayStatus = PlayStatus.STATUS_STOPPED;
    }
    
    public void palyVideo(String sourcePath, int  nFlag) throws Exception{
    	if(mCurMediaPlayer != null)
    		mCurMediaPlayer.setDataSourceAsync(sourcePath, nFlag);
         mPlayStatus = PlayStatus.STATUS_STOPPED;
    }
    
    public void palySong(String sourcePath, String cacheFilePath) throws Exception{
    	stop();
        play(sourcePath, cacheFilePath);
        mPlayStatus = PlayStatus.STATUS_STOPPED;
    }
    
    public void setViewSize(int width, int height) {
    	if(mCurMediaPlayer != null)
    		mCurMediaPlayer.setViewSize(width, height);
    	Log.e(TAG, "setViewSize: width" + width + "height" + height);
    }
    /**
     * 锟斤拷停
     */
    public void pause(boolean bFadeOut) {
    	if(mCurMediaPlayer != null)
    		mCurMediaPlayer.pause(bFadeOut);
        mPlayStatus = PlayStatus.STATUS_PAUSED;
    }
    
    
    public int duration() {
    	return mCurMediaPlayer.duration();
    }
    
    public boolean isMV() {
    	return mIsMV;
    }
    
    public void setView(SurfaceView sv) 
    {
    	 mSurfaceView = sv;
    	 mCurMediaPlayer.setView(sv);
    }
    
    public void setSurface(Surface sf) 
    {
    	 mCurMediaPlayer.setSurface(sf);
    }
    /**
     * 锟斤拷始锟斤拷锟斤拷
     */
    public void start() {
        mCurMediaPlayer.play();
        mPlayStatus = PlayStatus.STATUS_PLAYING;
    }
    
    
    /**
     * resume
     */
    public void resume(boolean bFadeIn) {
        mCurMediaPlayer.resume(bFadeIn);
        mPlayStatus = PlayStatus.STATUS_PLAYING;         
    }
    
    /**
     * 锟斤拷取锟斤拷锟斤拷状态
     *
     * @return 锟斤拷锟斤拷状态
     */
    public PlayStatus getPlayStatus() {
        return mPlayStatus;
    }
    
    
    /**
     * 锟斤拷锟斤拷位锟斤拷
     *
     * @return 锟斤拷锟斤拷位锟斤拷 锟斤拷位锟斤拷锟斤拷锟斤拷
     */
    public int getPosition() {
        if (mCurMediaPlayer != null) {  //锟斤拷锟竭筹拷锟斤拷要锟劫界处锟斤拷
            return mCurMediaPlayer.getPosition();
        }
        return 0;
    }
    
    /**
     * 锟斤拷取锟斤拷锟斤拷锟斤拷劝俜直锟�
     *
     * @return 锟斤拷锟斤拷锟斤拷劝俜直锟�
     */
    public float getBufferPercent() {
        if (mPlayStatus == PlayStatus.STATUS_PLAYING || mPlayStatus == PlayStatus.STATUS_PAUSED) {
            return mCurMediaPlayer.getBufferPercent();
        } else {
            return 0.0f;
        }
    }
    
    public void videoSizeChanged(int width, int height) {
    	if(mCurMediaPlayer != null)
    		mCurMediaPlayer.videoSizeChanged(width, height);
    }    
	/**
	 * 锟斤拷锟矫诧拷锟斤拷位锟矫ｏ拷seek锟斤拷锟斤拷
	 * 
	 * @param position
	 *            锟斤拷锟斤拷位锟矫ｏ拷锟斤拷位锟斤拷锟斤拷锟斤拷
	 */
	public void setPosition(int position, int flag) {
		mCurMediaPlayer.setPosition(position, flag);
	}
	
    /**
     * 锟斤拷锟矫诧拷锟斤拷锟斤拷锟斤拷
     *
     * @param aStart 锟斤拷始时锟斤拷
     * @param aEnd   锟斤拷锟斤拷时锟斤拷
     */
    public void setPlayRange(int aStart, int aEnd) {
        mCurMediaPlayer.setPlayRange(aStart, aEnd);
    }

	private OnStateChangeListener mStateChangeListener = new OnStateChangeListener() {

		@Override
		public void onPrepared(int error, int av) {
			// mediaItem ;

			int lastPlayPosition = 0;// getLastPlayPosition();
			if (lastPlayPosition != 0) {
				setPosition(lastPlayPosition, GKMediaPlayer.SEEK_FAST);
			}
			
			int nDuration = mCurMediaPlayer.duration();
			Log.e(TAG, "nDuration " + nDuration);
			
			if((av&2) != 0)
				mIsMV = true;
			
			// 锟斤拷锟斤拷锟斤拷锟斤拷
			start();
		}

		@Override
		public void onStarted() {
		}

		@Override
		public void onPaused() {
			// saveLastPlayPosition
		}
		
		@Override
		public void onSeekCompleted() {
			// select next song
		}


		@Override
		public void onCompleted() {
			// select next song
		}

		@Override
		public void onError(int error, final int httpCode, final MediaPlayerNotificationInfo ip) {
			Log.e(TAG, "onError:" + error);
			// processPlayError
		}

		@Override
		public void onBufferingStarted() {
			Log.e(TAG, "onBufferingStarted++++++");
		}

		@Override
		public void onBufferingDone() {
			Log.e(TAG, "onBufferingDone-----");
		}

		@Override
		public void onBufferFinished() {
			// save cached file
		}
		
		public void onVideoFormatchanged(int width, int height) {
			videoSizeChanged(width, height);
		}
	};

	/**
	 * 媒锟斤拷时锟斤拷锟斤拷锟斤拷通知锟接匡拷
	 */
	public interface OnMediaDurationUpdateListener {
		/**
		 * 媒锟斤拷时锟斤拷锟斤拷锟斤拷
		 * 
		 * @param duration
		 *            锟斤拷位:锟斤拷锟斤拷
		 */
		void onMediaDurationUpdated(int duration);
	}


	private GKMediaPlayer buildGKMediaPlayer(String pluginPath) {
		byte[] headerBytes = null;
		GKMediaPlayer mediaPlayer = new GKMediaPlayer(headerBytes, pluginPath);
		mediaPlayer.setOnMediaPlayerNotifyEventListener(mMediaPlayerNotifyEventListener);
		return mediaPlayer;
	}

	private void releaseGKMediaPlayer() {
		if (mGKMediaPlayer != null) {
			mGKMediaPlayer.release();
			mGKMediaPlayer = null;
		}
	}
	
    /**
     * 锟酵凤拷MediaPlayer锟斤拷源锟斤拷锟斤拷锟矫猴拷锟斤拷锟斤拷使锟斤拷
     */
    public void release() {
        releaseGKMediaPlayer();
        mCurMediaPlayer = null;
    }
    
    public void setAudioEffectLowDelay(boolean enable) {
    	if (mGKMediaPlayer != null) {
			mGKMediaPlayer.setAudioEffectLowDelay(enable);
		}
    }

	private GKMediaPlayer.OnMediaPlayerNotifyEventListener mMediaPlayerNotifyEventListener = new GKMediaPlayer.OnMediaPlayerNotifyEventListener() {
		@Override
		public void onMediaPlayerNotify(int aMsgId, int aArg1, int aArg2,
				Object aObj) {
			Log.d(TAG, "MsgId:" + aMsgId);
			switch (aMsgId) {
			case GKMediaPlayer.MEDIA_PREPARE:
				if (mStateChangeListener != null) {
					mStateChangeListener.onPrepared(aArg1, aArg2);
				}

				break;

			case GKMediaPlayer.MEDIA_PAUSE:
				mPlayStatus = PlayStatus.STATUS_PAUSED;
				if (mStateChangeListener != null) {
					mStateChangeListener.onPaused();
				}
				break;

			case GKMediaPlayer.MEDIA_PLAY:
				mPlayStatus = PlayStatus.STATUS_PLAYING;
				if (mStateChangeListener != null) {
					mStateChangeListener.onStarted();
				}
				break;

			case GKMediaPlayer.MEDIA_COMPLETE:
				mPlayStatus = PlayStatus.STATUS_STOPPED;
	
				if (mStateChangeListener != null) {
					mStateChangeListener.onCompleted();
				}

				break;
				
			case GKMediaPlayer.MEDIA_SEEK_COMPLETED:
				if (mStateChangeListener != null) {
					mStateChangeListener.onSeekCompleted();
				}
				
				break;

			case GKMediaPlayer.MEDIA_EXCEPTION:
				//mPlayStatus = PlayStatus.STATUS_STOPPED;

				if (mStateChangeListener != null) {
					mStateChangeListener.onError(aArg1, aArg2, null);
				}

				break;

			case GKMediaPlayer.MEDIA_BUFFERING_START:
				if (mStateChangeListener != null) {
					mStateChangeListener.onBufferingStarted();
				}

				break;

			case GKMediaPlayer.MEDIA_BUFFERING_DONE:
				if (mStateChangeListener != null) {
					mStateChangeListener.onBufferingDone();
				}
				break;

			case GKMediaPlayer.MEDIA_UPDATE_DURATION:
//				if (mMediaDurationUpdateListener != null) {
//					mMediaDurationUpdateListener.onMediaDurationUpdated(mCurMediaPlayer.duration());
//				}
				break;

			case GKMediaPlayer.MEDIA_CACHE_COMPLETED:
				if (mStateChangeListener != null) {
					mStateChangeListener.onBufferFinished();
				}

				break;
			case GKMediaPlayer.MEDIA_VIDEOFORMAT_CHANGED:
				if (mStateChangeListener != null) {
					mStateChangeListener.onVideoFormatchanged(aArg1, aArg2);
				}

				break;				

			case GKMediaPlayer.MEDIA_CLOSE:
				Log.e(TAG, "MEDIA_CLOSE: proxysize " + aArg2);
				break;
				
			case GKMediaPlayer.MEDIA_START_RECEIVE_DATA:

				break;
			case GKMediaPlayer.MEDIA_PREFETCH_COMPLETED:
	
				break;

			case GKMediaPlayer.MEDIA_DNS_DONE:

				break;
			case GKMediaPlayer.MEDIA_CONNECT_DONE:
				
				break;
			case GKMediaPlayer.MEDIA_HTTP_HEADER_RECEIVED:
				
				break;

			default:
				break;
			}
		}
	};
	
  
    private void setVolume(float leftVolume, float rightVolume) {
        mCurMediaPlayer.setVolume(leftVolume, rightVolume);
    }
   
}
