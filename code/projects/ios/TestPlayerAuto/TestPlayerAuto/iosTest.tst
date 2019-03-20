;[EXTSRC1]
;URL=rtmp://media3.sinovision.net:1935/live/livestream
;URL=http://skydvn-nowtv-atv-prod.skydvn.com/atv/skynews/1404/live/04.m3u8
;EXTSRC=1
;ACTION=exit:60000

[EXTSRC_AV]
URL=http://demo-videos.qnsdk.com/movies/qiniu.mp4
;URL=http://pf1nvq2id.bkt.clouddn.com/testg711.m3u8
;URL=http://devimages.apple.com/iphone/samples/bipbop/gear2/prog_index.m3u8
;URL=rtmp://media3.sinovision.net:1935/live/livestream
;URL=rtmp://fc_video.bsgroup.com.hk:443/webcast/bshdlive-pc
EXTSRC=1

;说明
;muxstart:5000:mux0.mp4 第5秒开始录制
;muxstop:20000:1 第20秒暂停
;muxstop:30000:2 第30秒继续录制
;muxstop:40000:0 第40秒停止录制

ACTION=muxstart:5000:mux0.mp4
ACTION=muxstop:10000:1
ACTION=muxstop:30000:2
ACTION=muxstop:40000:0
ACTION=exit:50000

;[EXTSRC_IO]
;URL=camera.ts
;URL=vlc.NFL.500sm.ts
;URL=g711.ts
;EXTSRC=0
;ACTION=exit:20000



