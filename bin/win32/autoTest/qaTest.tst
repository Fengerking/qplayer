[Play_RTMP_IP]
URL=rtmp://202.69.69.180:443/webcast/bshdlive-pc?domain=fc_video.bsgroup.com.hk
ACTION=exit:15000

[Play_RTMP_URL]
URL=rtmp://fc_video.bsgroup.com.hk:443/webcast/bshdlive-pc
ACTION=exit:15000

[Play_Video_Only]
URL=http://demo-videos.qnsdk.com/movies/snowday.mp4
ACTION=exit:15000
ACTION=seek:1000:500000
ACTION=seek:5000:150000

[Play_HTTPS_MP4]
URL=https://www.bldimg.com/videos/ticktocks/27680892/27680892_1544012406.mp4
ACTION=exit:15000
ACTION=seek:1000:1000
ACTION=seek:5000:10000

[Play_MP4_Index_End]
URL=http://demo-videos.qnsdk.com/movies/apple.mp4
ACTION=exit:15000
ACTION=seek:1000:500000
ACTION=seek:5000:150000

[Play_MP3]
URL=http://cdn01.sun.qiniuts.com/Pursue.mp3
ACTION=exit:15000
ACTION=seek:1000:30000
ACTION=seek:5000:60000

[Play_H265]
URL=http://ojpjb7lbl.bkt.clouddn.com/h265/2000k/265_test.m3u8
ACTION=exit:15000

[Play_M3U8]
URL=http://video.mb.moko.cc/2017-11-27/f10f19fe-64a8-4340-bc90-ab59bbafb857.mp4/be0fe625-6d3a-46e2-951d-8aa413df555d/AUTO.m3u8
ACTION=exit:15000
ACTION=seek:1000:500000
ACTION=seek:5000:150000

[Play_MP4_PLayLoop]
URL=http://musically.muscdn.com/reg02/2017/02/02/08/191487643451367424.mp4
ACTION=exit:30000
PLAYLOOP=1

[Play_MP4_Cache_1]
URL=http://musically.muscdn.com/reg02/2017/02/02/08/191487643451367424.mp4
PLAYCOMPLETE=0
PREPROTOCOL=6
PREFERFORMAT=0
SAVEPATH=c:/temp/qplayer/pdfile
EXTNAME=mp4

[Play_MP4_Cache_2]
URL=http://demo-videos.qnsdk.com/movies/apple.mp4
PLAYCOMPLETE=0
PREPROTOCOL=6
PREFERFORMAT=0
SAVEPATH=c:/temp/qplayer/pdfile
EXTNAME=mp4
ACTION=exit:30000

[Play_MP4_Cache_3]
URL=http://demo-videos.qnsdk.com/movies/apple.mp4
PLAYCOMPLETE=0
PREPROTOCOL=6
PREFERFORMAT=0
SAVEPATH=/sdcard/qplayer/pdfile
EXTNAME=mp4
ACTION=exit:30000

[Play_RTMP_URL_HW]
URL=rtmp://fc_video.bsgroup.com.hk:443/webcast/bshdlive-pc
HWDEC=0X01000000
ACTION=exit:15000

[Play_Video_Only_HW]
URL=http://demo-videos.qnsdk.com/movies/snowday.mp4
HWDEC=0X01000000
ACTION=exit:15000
ACTION=seek:1000:500000
ACTION=seek:5000:150000

[Play_HTTPS_MP4_HW]
URL=https://www.bldimg.com/videos/ticktocks/27680892/27680892_1544012406.mp4
HWDEC=0X01000000
ACTION=exit:15000
ACTION=seek:1000:500000
ACTION=seek:5000:150000

[Play_MP4_Index_End_HW]
URL=http://demo-videos.qnsdk.com/movies/apple.mp4
HWDEC=0X01000000
ACTION=exit:15000
ACTION=seek:1000:500000
ACTION=seek:5000:150000

[Play_H265_HW]
URL=http://ojpjb7lbl.bkt.clouddn.com/h265/2000k/265_test.m3u8
HWDEC=0X01000000
ACTION=exit:15000

[Play_M3U8_HW]
URL=http://video.mb.moko.cc/2017-11-27/f10f19fe-64a8-4340-bc90-ab59bbafb857.mp4/be0fe625-6d3a-46e2-951d-8aa413df555d/AUTO.m3u8
HWDEC=0X01000000
ACTION=exit:15000
ACTION=seek:1000:500000
ACTION=seek:5000:150000
