package com.ali.music.media.cameracollection;


import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

public class AudioRecordClient extends Thread {

	protected AudioRecord m_in_rec;
	protected int m_in_buf_size;
	protected byte[] m_in_bytes;
	protected boolean m_keep_running;
	protected String TAG = "AudioRecordClient";
	protected AVEncoder m_encoder;
	private RtmpPublish mPush = null;
	private boolean mStart = false;
	
	//private boolean recordEnable = false;

	int m_tmppos = 0;

	public void setPublishObj(RtmpPublish aPush)
	{
		mPush = aPush;
	}
	public void run() {
		try {
			int readNum;
			int ret;
			int size = m_in_buf_size;
			if (m_in_rec == null)
				return;
			
			try{  
                //��ֹĳЩ�ֻ���������������  
				m_in_rec.startRecording(); 
            }catch (IllegalStateException e){  
                e.printStackTrace();  
            } 
			
			//long wait = 0;  
            //long maxWait = 10;

			while (m_keep_running) {
				readNum = m_in_rec.read(m_in_bytes, 0, size);
				/*wait++;  
				if (readNum >0 && recordEnable == false){
                    if(wait > maxWait && wait <maxWait+10){
						 boolean  nozero = false;  
		                 for (int i = 0; i < size; i++) {  
		                      if(m_in_bytes[i] != 0){
		                    	  nozero = true;
		                    	  break;
		                      }
		                 }  
		                 if(nozero){  
	                            recordEnable = true;  
	                     }  
		                 Log.e(TAG, "recordEnable:" + recordEnable); 
                    }
				}*/
                 
				if (readNum > 0 && mStart == true) {
					ret = m_encoder.feedDataToAudioEncoder(m_in_bytes);//(m_in_bytes);
					if (ret > 0 && size != ret )
					{
						m_in_buf_size = size = ret;
						m_in_bytes = new byte[m_in_buf_size];
					}
				}
			}
			
			//ֹͣ¼��  
            try {  
                //��ֹĳЩ�ֻ�����
            	m_in_rec.stop();  
                //�����ͷ���Դ  
            	m_in_rec.release(); 
            	m_in_rec = null;
            }catch (IllegalStateException e){  
                e.printStackTrace();  
            }  

			m_in_bytes = null;
			
		}
		catch (Exception e) {
			Log.e(TAG," -- audio record exception! --");
			e.printStackTrace();
		}
	}

	public boolean init(int samplerate, int channel) {
		int aChannel = AudioFormat.CHANNEL_IN_STEREO;
		if(channel == 1){
			aChannel = AudioFormat.CHANNEL_IN_MONO;
		}
		m_in_buf_size = AudioRecord.getMinBufferSize(samplerate,
				aChannel,
				AudioFormat.ENCODING_PCM_16BIT);//CHANNEL_IN_STEREO
		
		m_in_rec = new AudioRecord(MediaRecorder.AudioSource.MIC, samplerate,
				aChannel,
				AudioFormat.ENCODING_PCM_16BIT, m_in_buf_size);
		
		if (m_in_rec == null)
			return false;

		m_in_bytes = new byte[m_in_buf_size];
		m_keep_running = true;
		
		//Log.e(TAG," m_in_buf_size = "+m_in_buf_size); 
		m_encoder = new AVEncoder(mPush);
		m_encoder.setAudioParameter(samplerate, channel, 64000);
		m_encoder.audioEncodeInit();
		
		return true;
	}

	public void free() {
		  synchronized(this) {  
			m_keep_running = false;
			m_in_rec.stop();
			mStart = false;
			m_encoder.close();
			m_encoder = null;
		  }
	}
	
	public void startEncode(){
		if (m_encoder != null)
			m_encoder.start();
		mStart = true;
	}
}