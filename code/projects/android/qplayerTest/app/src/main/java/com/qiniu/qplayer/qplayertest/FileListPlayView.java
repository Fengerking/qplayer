package com.qiniu.qplayer.qplayertest;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;
import android.content.pm.PackageManager;

import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.app.Activity;
import android.content.Intent;
import android.text.Layout;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.SimpleAdapter;

import com.qiniu.qplayer.mediaEngine.BasePlayer;
import com.qiniu.qplayer.mediaEngine.MediaPlayer;

public class FileListPlayView extends AppCompatActivity
                              implements SurfaceHolder.Callback, BasePlayer.onEventListener {
    private String							m_strRootPath = null;
    static private String					m_strListPath = null;
    private ListView						m_lstFiles = null;

    private AdapterView.OnItemClickListener m_lvListener = null;

    private SurfaceView             m_svVideo = null;
    private SurfaceHolder 	        m_shVideo = null;
    private RelativeLayout          m_layVideo = null;

    private MediaPlayer             m_Player = null;
    private int                     m_nViewWidth = 0;
    private int                     m_nViewHeight = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_list_play_view);

        initListView ();

        m_strRootPath = "/";
        //m_strListPath = "/mnt/sdcard/";
        if (m_strListPath == null) {
            File file = Environment.getExternalStorageDirectory();
            m_strListPath = file.getPath();
            updateFileList (m_strListPath);
        } else {
            updateFileList (m_strListPath);
        }
        updateDefaultList ();

        String  strVer = "V1.0";
        int     bldVer = 1;
        try {
        strVer = this.getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
        bldVer = this.getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
        } catch (PackageManager.NameNotFoundException e) {
        }
        strVer = "corePlayer V" + strVer + "  B " + bldVer;
        super.setTitle(strVer);
    }

    public void surfaceCreated(SurfaceHolder surfaceholder) {
        if (m_Player != null)
            m_Player.SetView(m_svVideo);
        else
            InitPlayer ();
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

        if (nID == BasePlayer.QC_MSG_PLAY_OPEN_DONE) {
            m_Player.Play ();
            String strLine = null;
            strLine = "http://video.qiniu.3tong.com/720_182584969019785216.mp4";
            m_Player.SetParam(BasePlayer.QCPLAY_PID_DEL_Cache, 0, strLine);
        }
        else if (nID == BasePlayer.QC_MSG_SNKV_NEW_FORMAT) {
            UpdateSurfaceViewPos(nArg1, nArg2);
        }
        else if (nID == BasePlayer.QC_MSG_PLAY_OPEN_FAILED) {
        }
        else if (nID == BasePlayer.QC_MSG_PLAY_COMPLETE) {
            m_Player.SetPos(0);
            //m_Player.Open(m_strFile, 0);
        }
        else if (nID == BasePlayer.QC_MSG_PLAY_SEEK_DONE || nID == BasePlayer.QC_MSG_PLAY_SEEK_FAILED) {
        }
        else if (nID == BasePlayer.QC_MSG_HTTP_DISCONNECTED) {
        }
        else if (nID == BasePlayer.QC_MSG_HTTP_RECONNECT_FAILED) {
        }
        else if (nID == BasePlayer.QC_MSG_HTTP_RECONNECT_SUCESS) {
            return 0;
        }
        else if (nID == BasePlayer.QC_MSG_SNKV_FIRST_FRAME) {
        }
        else if (nID == BasePlayer.QC_MSG_RTMP_METADATA) {
        }

        return 0;
    }

    public int OnSubTT (String strText, long lTime) {
        return 0;
    }

    public int OnImage (byte[] pData, int nSize) {
        return 0;
    }

    private void OpenFile (String strPath) {
        if (m_Player == null)
            return;
        m_Player.Open (strPath, 0);
    }

    private void InitPlayer (){
        SharedPreferences settings = this.getSharedPreferences("Player_Setting", 0);
        int nVideoDec      = settings.getInt("VideoDec", 1);
        int	nFlag = 0;
        if (nVideoDec == 3)
            nFlag = BasePlayer.QCPLAY_OPEN_VIDDEC_HW;

        int nRet;
        if (m_Player == null) {
            m_Player = new MediaPlayer();
            //String apkPath = "/data/data/" + this.getPackageName() + "/lib/";
            String apkPath = getApplicationContext().getFilesDir().getAbsolutePath();
            apkPath = apkPath.substring(0, apkPath.lastIndexOf('/'));
            apkPath = apkPath + "/lib/";
            m_Player.SetView(m_svVideo);
            nRet = m_Player.Init(this, apkPath, nFlag);
            if (nRet != 0) {
                Close();
                return;
            }
            String strDnsServer = "114.114.114.114";
            //String strDnsServer = "127.0.0.1";
            //m_Player.SetParam(BasePlayer.QCPLAY_PID_DNS_SERVER, 0, strDnsServer);
            String strHost = "video.qiniu.3tong.com";
            m_Player.SetParam(BasePlayer.QCPLAY_PID_DNS_DETECT, 0, strHost);
            strHost = "oh4yf3sig.cvoda.com";
            m_Player.SetParam(BasePlayer.QCPLAY_PID_DNS_DETECT, 0, strHost);
            //m_Player.SetParam(BasePlayer.QCPLAY_PID_Playback_Loop, 1, null);

            //m_Player.SetParam(BasePlayer.QCPLAY_PID_Prefer_Protocol, BasePlayer.QC_IOPROTOCOL_HTTPPD, null);
            File file = Environment.getExternalStorageDirectory();
            String strPDPath = file.getPath() + "/QPlayer/PDFile";
            m_Player.SetParam(BasePlayer.QCPLAY_PID_PD_Save_Path, 0, strPDPath);
/*
            String strLine = null;
            strLine = "http://video.qiniu.3tong.com/720_182584969019785216.mp4";
            m_Player.SetParam(BasePlayer.QCPLAY_PID_ADD_Cache, 0, strLine);
            strLine  = "http://video.qiniu.3tong.com/720_179737636708024320.mp4";
            m_Player.SetParam(BasePlayer.QCPLAY_PID_ADD_Cache, 0, strLine);
            strLine  = "http://video.qiniu.3tong.com/720_188810429944823808.mp4";
            m_Player.SetParam(BasePlayer.QCPLAY_PID_ADD_Cache, 0, strLine);
*/
        }
        m_Player.setEventListener(this);
    }
    private void Close () {
        if (m_Player != null) {
            m_Player.Stop();
            m_Player.Uninit();
            m_Player = null;
        }
    }

    private void UpdateSurfaceViewPos (int nW, int nH) {
        if (nW == 0 || nH == 0)
            return;
        RelativeLayout.LayoutParams lpView = (RelativeLayout.LayoutParams)m_svVideo.getLayoutParams();
        RelativeLayout.LayoutParams lpLayout = (RelativeLayout.LayoutParams)m_layVideo.getLayoutParams();
        if (m_nViewWidth == 0 || m_nViewHeight == 0) {
            DisplayMetrics dm = this.getResources().getDisplayMetrics();
            m_nViewWidth = lpLayout.width;
            m_nViewHeight = lpLayout.height;
            if (m_nViewWidth <= 0)
                m_nViewWidth = dm.widthPixels;
            if (m_nViewHeight <= 0)
                m_nViewHeight = dm.heightPixels;
        }
        if (m_nViewWidth * nH > nW * m_nViewHeight) {
            lpView.height = m_nViewHeight;
            lpView.width  = nW * m_nViewHeight / nH;
        } else {
            lpView.width  = m_nViewWidth;
            lpView.height = nH * m_nViewWidth / nW;
        }
        m_svVideo.setLayoutParams(lpView);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
             if (m_strListPath.equals(m_strRootPath)) {
                 Close ();
                //finish ();
                System.exit(0);
            }
            else {
                File file = new File (m_strListPath);
                m_strListPath = file.getParent();
                updateFileList (m_strListPath);
            }
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.activity_file_list_view, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // TODO Auto-generated method stub
        int id = item.getItemId();
        switch (id) {
            case R.id.menu_settings:
                startActivity(new Intent(this, SettingView.class));
                break;
            case R.id.menu_exit:
                Close ();
                finish();
                System.exit(RESULT_OK);
                break;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    private void initListView () {
        m_lvListener = new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                SimpleAdapter adapter = (SimpleAdapter)m_lstFiles.getAdapter();
                Map<String, Object> map = (Map<String, Object>)adapter.getItem(position);
                String  strFile = (String) map.get("path");
                String	strDir = (String) map.get("dir");

                if (strDir.equals("1")) {
                    updateFileList (strFile);
                } else {
                    String strExt = strFile.substring(strFile.length() - 4);
                    if (strExt.equalsIgnoreCase(".url")) {
                        updateURLList (strFile);
                    } else {
                        int nFlag = 0;
                        if (nFlag == 0) {
                            openExtSource();
                            return;
                        }

                        nFlag = BasePlayer.QCPLAY_OPEN_SAME_SOURCE;
                        if (m_Player == null)
                            OpenFile (strFile);
                        else
                           m_Player.Open (strFile, nFlag);
                    }
                }
            }
        };
        m_lstFiles = (ListView)findViewById (R.id.listViewFile);
        m_lstFiles.setOnItemClickListener(m_lvListener);

        m_svVideo = (SurfaceView) findViewById(R.id.svVideo);
        m_shVideo = m_svVideo.getHolder();
        m_shVideo.addCallback(this);

        m_layVideo = (RelativeLayout) findViewById(R.id.videoView);
    }

    private void updateURLList (String strFile) {
        m_strListPath = m_strListPath + "/url";
        ArrayList<Map<String, Object>> list = new ArrayList<Map<String, Object>>();
        HashMap<String, Object> map;

        try {
            FileInputStream fis = new FileInputStream (strFile);
            BufferedReader br = new BufferedReader(new InputStreamReader(fis));
            String line = null;
            while((line = br.readLine())!=null)
            {
                map = new HashMap<String, Object>();
                map.put("name", line);
                map.put("path", line);
                map.put("img", R.drawable.item_video);
                map.put("dir", "2");
                list.add(map);
            }
        }catch (Exception e) {
            e.printStackTrace();
        }

        SimpleAdapter adapter = new SimpleAdapter(this, list, R.layout.list_item,
                new String[]{"name","img"}, new int[]{R.id.name, R.id.img});
        m_lstFiles.setAdapter(adapter);
    }

    private void updateFileList (String strPath){
        m_strListPath = strPath;

        ArrayList<Map<String, Object>> list = getFileList(strPath);
        SimpleAdapter adapter = new SimpleAdapter(this, list, R.layout.list_item,
                new String[]{"name","img"}, new int[]{R.id.name, R.id.img});

        Comparator comp = new nameComparator();
        Collections.sort(list, comp);

        m_lstFiles.setAdapter(adapter);
    }

    private void updateDefaultList () {
        m_strListPath = m_strListPath + "/url";
        ArrayList<Map<String, Object>> list = new ArrayList<Map<String, Object>>();
        HashMap<String, Object> map;

        String strLine = "";
        for (int i = 0; i < 5; i++){
            if (i == 0)
            //   strLine  = "http://mapex.b0.upaiyun.com/2018/03/15/upload_7a563qrzxwyccsodmwovrbfay8lwen3b.mp4";
            //    strLine = "http://wm.video.huajiao.com/vod-wm-huajiao-bj/52523777_4d0083a71-0770-4292-b959-757c019ba7d4.mp4";
            //   strLine = "http://7xnngn.com1.z0.glb.clouddn.com/bbkim_transfer_883c446aeee94c618d6d5d0ddc08a5e4.mp4?e=1522292478&token=8AL1G4RdHBLOGsNafux4Ac5_JeRoNM3fVyJ2ZBEF:IXXJxKzr96g0wkHl8Fph-G2tLjc";
            //     strLine = "http://video.qiniu.3tong.com/720_201883248781950976.mp4";
                 strLine = "rtmp://pull.qianghuixiang.com/maida/global_1_1";
            else if (i == 1)
             //   strLine  = "http://play.youletd.com/v?m=v1&id=364439";
             //   strLine = "http://p5mlo8b4j.bkt.clouddn.com/Record_%E8%A7%86%E9%A2%91%E6%97%8B%E8%BD%AC90%E5%BA%A6.mp4";
             //    strLine = "rtmp://183.146.213.65/live/hks?domain=live.hkstv.hk.lxdns.com";
                  strLine = "http://video.qiniu.3tong.com/720_182584969019785216.mp4";
            else if (i == 2)
                strLine  = "http://video.qiniu.3tong.com/720_179737636708024320.mp4";
            else if (i == 3)
                strLine  = "http://video.qiniu.3tong.com/720_188810429944823808.mp4";
            else if (i == 4)
                //strLine = "http://p5mlo8b4j.bkt.clouddn.com/Record_%E8%A7%86%E9%A2%91%E6%97%8B%E8%BD%AC90%E5%BA%A6.mp4";
                strLine  = "http://oh4yf3sig.cvoda.com/cWEtZGlhbmJvdGVzdDpY54m56YGj6ZifLumfqeeJiC5IRGJ1eHVhbnppZG9uZzAwNC5tcDQ=_q00030002.mp4";
            map = new HashMap<String, Object>();
            map.put("name", strLine);
            map.put("path", strLine);
            map.put("img", R.drawable.item_video);
            map.put("dir", "2");
            list.add(map);
        }

        SimpleAdapter adapter = new SimpleAdapter(this, list, R.layout.list_item,
                new String[]{"name","img"}, new int[]{R.id.name, R.id.img});
        m_lstFiles.setAdapter(adapter);
    }
    private ArrayList<Map<String, Object>> getFileList(String strPath) {
        ArrayList<Map<String, Object>> list = new ArrayList<Map<String, Object>>();
        File fPath = new File(strPath);
        File[] fList = fPath.listFiles();

        HashMap<String, Object> map;
        if (fList != null) {
            for (int i = 0; i < fList.length; i++)
            {
                File file = fList[i];
                if (file.isHidden())
                    continue;
                if (file.isDirectory()){
                    File fSubPath = new File(file.getPath());
                    File[] fSubList = fSubPath.listFiles();
                    if (fSubList != null) {
                        if (fSubList.length <= 0)
                            continue;
                    }
                }

                map = new HashMap<String, Object>();
                map.put("name", file.getName());
                map.put("path", file.getPath());
                if (file.isDirectory()){
                    map.put("img", R.drawable.item_folder);
                    map.put("dir", "1");
                }
                else {
                    map.put("img", R.drawable.item_video);
                    map.put("dir", "0");
                }
                list.add(map);
            }
        }
        return list;
    }

    public class nameComparator implements Comparator<Object> {
        @SuppressWarnings("unchecked")
        public int compare(Object o1, Object o2) {
            HashMap<String, Object> p1 = (HashMap<String, Object>) o1;
            HashMap<String, Object> p2 = (HashMap<String, Object>) o2;
            String strName1, strName2, strDir1, strDir2;
            strName1 = (String) p1.get("name");
            strName2 = (String) p2.get("name");
            strDir1 = (String) p1.get("dir");
            strDir2 = (String) p2.get("dir");

            if (strDir1.equals("1") && strDir2.equals("0") ){
                return -1;
            } else if (strDir1.equals("0") && strDir2.equals("1") ){
                return 1;
            } else {
                if (strName1.compareToIgnoreCase(strName2) > 0) {
                    return 1;
                } else {
                    return -1;
                }
            }
        }
    }

    public void openExtSource () {
        if (m_Player == null)
            return;
        m_Player.SetParam(BasePlayer.QCPLAY_PID_EXT_VIDEO_CODECID, BasePlayer.QC_CODEC_ID_H264, null);
        //m_Player.SetParam(BasePlayer.QCPLAY_PID_EXT_AUDIO_CODECID, BasePlayer.QC_CODEC_ID_AAC, null);
        m_Player.SetParam(BasePlayer.QCPLAY_PID_EXT_AUDIO_CODECID, BasePlayer.QC_CODEC_ID_G711, null);
        m_Player.Open ("EXT_AV", BasePlayer.QCPLAY_OPEN_EXT_SOURCE_AV);
        fillVideoData ();
        fillAudioData ();
    }

    public void fillVideoData () {
        Thread thread = new Thread(){
            public void run() {
                String strFile = "/sdcard/00Files/H264.dat";
                try {
                    File file = new File(strFile);
                    FileInputStream input = new FileInputStream(file);
                    int nFileSize = input.available();
                    int nFilePos = 0;
                    int nDataSize = 0;
                    int nRC = 0;
                    byte[]  byDataInfo = new byte[16];
                    byte[]  byDataBuff = new byte[1024000];

                    ByteBuffer byBuff = ByteBuffer.wrap(byDataInfo);

                    while (nFilePos < nFileSize) {
                        input.read (byDataInfo, 0, 16);
                        nFilePos += 16;

                        nDataSize = byBuff.getInt(0);

                        input.read (byDataBuff, 0, nDataSize);
                        nFilePos += nDataSize;

                        nRC = -1;
                        while (nRC != 0) {
                            nRC = m_Player.SetParam(BasePlayer.QCPLAY_PID_EXT_SOURCE_INFO, 2, byDataInfo);
                        }
                        nRC = -1;
                        while (nRC != 0) {
                            nRC = m_Player.SetParam(BasePlayer.QCPLAY_PID_EXT_SOURCE_DATA, 2, byDataBuff);
                        }
                    }
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
            }
        };
        thread.start();
    }

    public void fillAudioData () {
        Thread thread = new Thread(){
            public void run() {
                boolean bG711 = true;
                String strFile = "/sdcard/00Files/AAC.dat";
                if (bG711)
                    strFile = "/sdcard/00Files/G711.alaw";
                try {
                    File file = new File(strFile);
                    FileInputStream input = new FileInputStream(file);
                    int nFileSize = input.available();
                    int nFilePos = 0;
                    int nDataSize = 200;
                    long lTime = 0;
                    int nRC = 0;
                    byte[]  byDataInfo = new byte[16];
                    byte[]  byDataBuff = new byte[192000];

                    ByteBuffer byBuff = ByteBuffer.wrap(byDataInfo);

                    while (nFilePos < nFileSize) {
                        if (bG711) {
                            byBuff.putInt(0, nDataSize);
                            byBuff.putLong(4, lTime);
                            byBuff.putInt (12, 0);
                            lTime += 50;
                        } else {
                            input.read(byDataInfo, 0, 16);
                            nFilePos += 16;
                            nDataSize = byBuff.getInt(0);
                        }
                        input.read(byDataBuff, 0, nDataSize);
                        nFilePos += nDataSize;

                        nRC = -1;
                        while (nRC != 0) {
                            nRC = m_Player.SetParam(BasePlayer.QCPLAY_PID_EXT_SOURCE_INFO, 1, byDataInfo);
                        }
                        nRC = -1;
                        while (nRC != 0) {
                            nRC = m_Player.SetParam(BasePlayer.QCPLAY_PID_EXT_SOURCE_DATA, 1, byDataBuff);
                        }
                    }
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
            }
        };
        thread.start();
    }
}
