[GENERATE_ANALYSIS_DATA_1]
URL=http://video.pearvideo.com/mp4/third/20171225/11481639_092720-sd-----.mp4
ACTION=exit:10000

[GENERATE_ANALYSIS_DATA_2]
URL=http://video.pearvideo.com/mp4/third/20171225/11481639_092720-sd.mp4
ACTION=exit:10000

[GENERATE_ANALYSIS_DATA_3]
URL=rtmp://live.hkstv.hk.lxdns.com/live/hks
ACTION=exit:15000

[GENERATE_ANALYSIS_DATA_4]
URL=http://ojpjb7lbl.bkt.clouddn.com/bipbopall.m3u8
ACTION=exit:5000

[GENERATE_ANALYSIS_DATA_5]
URL=http://dev.storage.singworld.net/3/34/83215_LOW_20180324124008_0.aac
ACTION=exit:10000

[GENERATE_ANALYSIS_DATA_6]
URL=http://live.hkstv.hk.lxdns.com/live/hks/playlist.m3u8
ACTION=exit:15000

[GENERATE_ANALYSIS_DATA_7_FAIL]
URL=http://live.hkstv.hk.lxdns.com/live/hks/playlist111111111111111.m3u8
ACTION=exit:5000

[GENERATE_ANALYSIS_DATA_8]
URL=https://www.gtbluesky.com/test.mp3
ACTION=exit:10000

[GENERATE_ANALYSIS_DATA_9]
;pure video
URL=http://demo-videos.qnsdk.com/movies/snowday.mp4
ACTION=exit:10000

[GENERATE_ANALYSIS_DATA_10]
;short video, 14s
URL=http://mus-oss.muscdn.com/reg02/2017/07/02/00/245712223036194816.mp4
ACTION=exit:8000

[STREAM_FORMAT_CHECK]
URL=http://video.mb.moko.cc/2017-11-27/f10f19fe-64a8-4340-bc90-ab59bbafb857.mp4/be0fe625-6d3a-46e2-951d-8aa413df555d/AUTO.m3u8
ACTION=exit:10000

[START_POS]
URL=http://video.pearvideo.com/hls/live-cut/pearvideo/1338423-1525662659-sd.m3u8
SEEKMODE=1
STARTPOS=3600000
ACTION=exit:5000

[VFT_SLOW]
URL=http://video.pearvideo.com/mp4/short/20180502/cont-1335588-11989735_pkg-sd.mp4
OPENFLAG=0X02000000
SEEKMODE=1
STARTPOS=8000
ACTION=exit:5000

[PLAYBACK_FAIL]
URL=http://video.pearvideo.com/mp4/short/20180502/cont-1335588-11989735_pkg-sd.mp4
OPENFLAG=0X02000000
SEEKMODE=1
STARTPOS=8000
ACTION=stop:800
ACTION=open:900
ACTION=exit:3000

[STOP_USE_TIME]
URL=http://v.cdn.ibeiliao.com/recordings/z1.ibeiliao-test-tv.ibeiliaovideo9c133506ee8544009b8b420176aeee57/0_1525253004.mp4
ACTION=stop:1000
ACTION=open:2000
ACTION=stop:3000
ACTION=open:4000
ACTION=stop:5000
ACTION=open:6000
ACTION=stop:7000
ACTION=exit:10000

[SWITCH_2_SW]
URL=http://ojpjb7lbl.bkt.clouddn.com/h265/2000k/265_test.m3u8
OPENFLAG=0X01000000
ACTION=exit:10000

[HW_RESOLUTION]
URL=https://oigovwije.qnssl.com/IMG_6437.MOV
HWDEC=0X01000000
ACTION=exit:10000

[BAD_NETWORK_CRASH]
URL=http://dev.storage.singworld.net/3/34/83215_LOW_20180324124008_0.aac
ACTION=exit:10000

[ONLY_ONE_IDR]
URL=http://mus-oss.muscdn.com/reg02/2017/07/02/00/245712223036194816.mp4
ACTION=exit:10000

[AUDIO_NOISE]
URL=https://static.xingnl.tv/o_1bb6c4r3a1aqp1mo5187t1kqr1pagnr.mp4
ACTION=exit:10000

[BADNETWORK]
URL=https://www.bldimg.com/videos/ticktocks/27680892/27680892_1544012406.mp4
HWDEC=0X01000000
ACTION=exit:5000
[BADNETWORK]
URL=https://www.bldimg.com/videos/ticktocks/1385/1385_1545203176.mp4
ACTION=exit:5000
[BADNETWORK]
URL=https://www.bldimg.com/videos/ticktocks/5943676/5943676_1544964441.mp4
HWDEC=0X01000000
ACTION=exit:5000
[BADNETWORK]
URL=https://www.bldimg.com/videos/ticktocks/18831805/18831805_1544788130.mp4
HWDEC=0X01000000
ACTION=exit:5000
[BADNETWORK]
URL=https://www.bldimg.com/videos/ticktocks/5369354/5369354_1545127116.mp4
ACTION=exit:5000
