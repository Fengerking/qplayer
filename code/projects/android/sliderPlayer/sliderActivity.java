package com.qiniu.sampleplayer;

import android.os.Environment;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.qiniu.qplayer.mediaEngine.*;

import java.io.File;

public class sliderActivity extends AppCompatActivity
        implements SurfaceHolder.Callback, BasePlayer.onEventListener {
    private SurfaceView[]         m_svVideo;
    private MediaPlayer         m_Player = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_slider);
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null)
            actionBar.hide();

        m_svVideo = new SurfaceView[3];
        m_svVideo[0] = (SurfaceView) findViewById(R.id.svVideo1);
        m_svVideo[0].getHolder().addCallback(this);
    }

    protected void onResume() {
        super.onResume();
    }

    protected void onPause() {
        super.onPause();
    }

    public void surfaceCreated(SurfaceHolder surfaceholder) {

    }

    public void surfaceChanged(SurfaceHolder surfaceholder, int format, int w, int h) {
    }

    public void surfaceDestroyed(SurfaceHolder surfaceholder) {
        if (m_Player != null)
            m_Player.SetView(null);
    }


    public int onEvent(int nID, int nArg1, int nArg2, Object obj) {
        if (m_Player == null)
            return 0;

        return 0;
    }

    public int OnRevData(byte[] pData, int nSize, int nFlag) {
        return 0;
    }

    private void InitPlayer() {
        int nFlag = 0;
        //nFlag = BasePlayer.QCPLAY_OPEN_VIDDEC_HW;
        int nRet = 0;
        if (m_Player == null) {
            m_Player = new MediaPlayer();
            m_Player.SetView(m_svVideo[0].getHolder().getSurface());
            nRet = m_Player.Init(this, nFlag);
            if (nRet != 0) {
                ClosePlayer();
                return;
            }

            String strHost = "play-sg.magicmovie.video";
            m_Player.SetParam(BasePlayer.QCPLAY_PID_DNS_DETECT, 0, strHost);
            m_Player.SetParam(BasePlayer.QCPLAY_PID_Playback_Loop, 1, null);

            //m_Player.SetParam(BasePlayer.QCPLAY_PID_Prefer_Protocol, BasePlayer.QC_IOPROTOCOL_HTTPPD, null);
            File file = Environment.getExternalStorageDirectory();
            String strPDPath = file.getPath() + "/QPlayer/PDFile";
            m_Player.SetParam(BasePlayer.QCPLAY_PID_PD_Save_Path, 0, strPDPath);

        }
        m_Player.setEventListener(this);
    }

    private void ClosePlayer() {
        if (m_Player != null) {
            m_Player.SetParam(BasePlayer.QCPLAY_PID_STOP_SAVE_FILE, 0, null);
            m_Player.Stop();
            m_Player.Uninit();
            m_Player = null;
        }
    }

}
