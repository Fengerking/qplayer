package com.ali.music.media.cameracollection;


import java.io.File;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import android.annotation.SuppressLint;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Environment;
import android.os.Bundle;
import android.util.Log;

@SuppressLint("NewApi")
public class AVEncoder {

	private MediaCodec mediaCodec = null;
	private static String TAG = "AVEncoder";
	int m_width;
	int m_height;
	int m_color;
	int m_framerate;
	int m_vbitrate;
	int m_abitrate;
	int m_samplerate;
	int m_channel;
	int m_codecType;
	int m_limit = 0;
	int m_videoCount = 0;
	FileOutputStream mf = null;
	private RtmpPublish mPush = null;

	private byte[] m_inputAudio = null;
	byte[] packet = new byte[7];
	boolean  m_Config = false;
	private static long m_startTime = 0; 
	boolean dumpfile = false;
	
	public AVEncoder(RtmpPublish aPush) {
		mPush = aPush;
		dumpfile = false;
	}
	
    public void setVideoParameter(int width, int height, int framerate, int bitrate, int nCodecType)
	{
		m_width = width;
		m_height = height;
		m_framerate = framerate;
		m_vbitrate = bitrate;
		m_codecType = nCodecType;
		
		if (m_width == 360 && m_height == 640 ){
			m_width = 352;
			m_height = 640;
		}
		
		//test
		try {
			if (dumpfile){
				File file = new File(Environment.getExternalStorageDirectory(),
						"h2");
				if(file.exists())
				{
					file.delete();
				}
				file.createNewFile(); 
				mf = new FileOutputStream(file);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	public void setAudioParameter(int samplerate, int channel, int bitrate)
	{
		m_samplerate = samplerate;
		m_channel = channel;
		m_abitrate = bitrate;

		//test
		try {
			if (dumpfile){
				File file = new File(Environment.getExternalStorageDirectory(),
						"a2");
				if(file.exists())
				{
					file.delete();
				}
				file.createNewFile(); 
				mf = new FileOutputStream(file);
				// af.write(info);
				// af.flush();
			}
		} catch (Exception e) {
			e.printStackTrace();
		}		
	}
	
	public void audioEncodeInit() {
		try{
			mediaCodec = MediaCodec.createEncoderByType("audio/mp4a-latm");
			MediaFormat mediaFormat = new MediaFormat();
			mediaFormat.setString(MediaFormat.KEY_MIME, "audio/mp4a-latm");
			mediaFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
			mediaFormat.setInteger(MediaFormat.KEY_CHANNEL_COUNT, m_channel);
			mediaFormat.setInteger(MediaFormat.KEY_SAMPLE_RATE, m_samplerate);
			mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, m_abitrate);
			mediaCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
			//mediaCodec.start();
		} catch (Exception e) {
			e.printStackTrace();
		}
		
	}
	
	public void start(){
		mediaCodec.start();
	}
	
	public void videoEncodeInit(int color) {
		m_color = color;
		Log.e(TAG, " m_color = "+m_color+" w = "+m_width+" h = "+m_height);
		try {
			mPush.SetcolorFormart(color);
			mPush.setVideoWidthHeight(m_width, m_height);
			MediaFormat mediaFormat = null;
			if(m_codecType == 265) {
				mediaCodec = MediaCodec.createEncoderByType("video/hevc");
				mediaFormat = MediaFormat.createVideoFormat(
						"video/hevc", m_width, m_height);
			} else if(m_codecType == 264) {
				mediaCodec = MediaCodec.createEncoderByType("video/avc");
				mediaFormat = MediaFormat.createVideoFormat(
						"video/avc", m_width, m_height);
			}
			
			mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, m_vbitrate);
			mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, m_framerate);
			if (m_color > 0)
				mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, m_color);

			mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 2); // 
			if(m_codecType == 264) 
			{
				mediaFormat.setInteger("profile", MediaCodecInfo.CodecProfileLevel.AVCProfileBaseline);
				mediaFormat.setInteger("level", MediaCodecInfo.CodecProfileLevel.AVCLevel3);
			}
			mediaCodec.configure(mediaFormat, null, null,
					MediaCodec.CONFIGURE_FLAG_ENCODE);
			
		} catch (Exception e) {
			Log.e(TAG, " paramenter error !");
			e.printStackTrace();
		}
	}

	@SuppressLint("NewApi")
	public void close() {
		try {
			if(mediaCodec != null){
				mediaCodec.stop();
				mediaCodec.release();
			}

			//test
			if(mf != null)
			{
				mf.close();
				mf = null;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	public int feedDataToAudioEncoder(byte[] input)
	{
		int pos = 0;
		long diff;
		try {
			ByteBuffer[] inputBuffers = mediaCodec.getInputBuffers();
			ByteBuffer[] outputBuffers = mediaCodec.getOutputBuffers();
			int inputBufferIndex = mediaCodec.dequeueInputBuffer(-1);
			
			if (m_startTime == 0)
				m_startTime = System.currentTimeMillis();
		 
			if (inputBufferIndex >= 0) {
				ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
				inputBuffer.clear();
				
				if(m_limit == 0){
					m_limit = inputBuffer.limit();
					diff = System.currentTimeMillis() - m_startTime;
					if (input.length > m_limit){
						m_inputAudio = new byte[m_limit];
						System.arraycopy(input, 0, m_inputAudio, 0, m_limit);
						inputBuffer.put(m_inputAudio);
						mediaCodec.queueInputBuffer(inputBufferIndex, 0, m_limit,diff*1000, 0);
						pos = m_limit;
					}
					else{
						inputBuffer.put(input);
						mediaCodec.queueInputBuffer(inputBufferIndex, 0, input.length,diff*1000, 0);
					}
				}
				else{
					inputBuffer.put(input);
					diff = System.currentTimeMillis() - m_startTime;
					mediaCodec.queueInputBuffer(inputBufferIndex, 0, input.length,diff*1000, 0);
				}

			}

			MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
			int outputBufferIndex;
			while(true){
				outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo,0);
				if (outputBufferIndex >= 0 ){
					ByteBuffer outputBuffer = outputBuffers[outputBufferIndex];
					if(outputBuffer == null)
						break;
					
					byte[] outData = new byte[bufferInfo.size];
					outputBuffer.get(outData);
					Log.e(TAG, "audio E = " + bufferInfo.size + "audio flags = " + bufferInfo.flags);
					//Log.e(TAG, "audio E = " + bufferInfo.size);
					if (bufferInfo.flags == 0 || bufferInfo.flags == 1)
					{
						if (dumpfile){
							addADTStoPacket(outData.length+7);
							mf.write(packet);
							mf.write(outData);
							mf.flush();
						}

						if (outData.length > 0)
							mPush.SendAudioPacket(outData, outData.length, bufferInfo.presentationTimeUs/1000);
					}
					else if(bufferInfo.flags == 2){
						Log.e(TAG, "set audio config ");
						mPush.SendAudioConfig(outData, outData.length);
					}
					mediaCodec.releaseOutputBuffer(outputBufferIndex, false);
				}
				else
					break;
			}
		} catch (Throwable t) {
			Log.e(TAG, " -- exception happen! --");
			t.printStackTrace();
		}
		
		return pos;
	}
	
	
	@SuppressLint("NewApi")
	public int feedDataToVideoEncoder(byte[] input) {
		int pos = 0;
		long diff;
		try {
			if(m_videoCount >= 2*m_framerate) {
				Bundle params = new Bundle();
				params.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);

				mediaCodec.setParameters(params);
				
				m_videoCount = 0;
			}
			
			
			ByteBuffer[] inputBuffers = mediaCodec.getInputBuffers();
			ByteBuffer[] outputBuffers = mediaCodec.getOutputBuffers();
			int inputBufferIndex = mediaCodec.dequeueInputBuffer(1000);//(-1);
			
			m_videoCount++;
			
			if (m_startTime == 0)
				m_startTime = System.currentTimeMillis();
			
			if (inputBufferIndex >= 0) {
				ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
				inputBuffer.clear();
				inputBuffer.put(input);
				
				diff = System.currentTimeMillis() - m_startTime;
				mediaCodec.queueInputBuffer(inputBufferIndex, 0, input.length,
						diff*1000, 0);
				//Log.e(TAG, "input = "+input.length);
			}
			else{
				//Log.e(TAG, "inputBufferIndex= "+inputBufferIndex);
				//return 0;
			}

			MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
			int outputBufferIndex ;
			
			while(true){
				outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo,0);
				if (outputBufferIndex >= 0 ){
					ByteBuffer outputBuffer = outputBuffers[outputBufferIndex];
					if (outputBuffer == null)
						break;
					
					byte[] outData = new byte[bufferInfo.size];
					outputBuffer.get(outData);
					Log.e(TAG, "video E = " + bufferInfo.size + "video flags = " + bufferInfo.flags);
					
					if (bufferInfo.flags == 0 || bufferInfo.flags == 1){
						if (outData.length > 0) {
							int nSize = bufferInfo.size;
							if(bufferInfo.flags == 1) {
								nSize = nSize|0x80000000;
							}
							mPush.SendVideoPacket(outData, nSize, bufferInfo.presentationTimeUs/1000);
						}
						
						if (dumpfile){
							try {
								mf.write(outData);
								mf.flush();
							} catch (Exception e) {
								e.printStackTrace();
							}
						}
					}else if(bufferInfo.flags == 2){
						m_Config = true;
						Log.e(TAG, "set video config ");
						mPush.SendVideoConfig(outData, outData.length);
						if (dumpfile){
							try {
								mf.write(outData);
								mf.flush();
							} catch (Exception e) {
								e.printStackTrace();
							}
						}
					}
					 
					mediaCodec.releaseOutputBuffer(outputBufferIndex, false);
				}
				else
					break;
				 
				
			}
		} catch (Throwable t) {
			t.printStackTrace();
		}

		return pos;
	}

	public static long getCurrentTime(){
		if (m_startTime == 0)
			m_startTime = System.currentTimeMillis();
		
		long diff = System.currentTimeMillis() - m_startTime;
		
		return diff;
	}
	
	//test only
	private void addADTStoPacket( int packetLen) {
	    int profile = 2;  //AAC LC
	                      //39=MediaCodecInfo.CodecProfileLevel.AACObjectELD;
	    int freqIdx = 4;  //44.1KHz
	    int chanCfg = 2;  //CPE 
	    // fill in ADTS data
	    packet[0] = (byte)0xFF;
	    packet[1] = (byte)0xF9;
	    packet[2] = (byte)(((profile-1)<<6) + (freqIdx<<2) +(chanCfg>>2));
	    packet[3] = (byte)(((chanCfg&3)<<6) + (packetLen>>11));
	    packet[4] = (byte)((packetLen&0x7FF) >> 3);
	    packet[5] = (byte)(((packetLen&7)<<5) + 0x1F);
	    packet[6] = (byte)0xFC;
	}
}