LOCAL_PATH := $(call my-dir)

MY_PATH_INC		:= $(LOCAL_PATH)/../../../include
MY_PATH_BASE	:= $(LOCAL_PATH)/../../../base
MY_PATH_ANL		:= $(LOCAL_PATH)/../../../anl
MY_PATH_DRM		:= $(LOCAL_PATH)/../../../drm
MY_PATH_UTIL	:= $(LOCAL_PATH)/../../../util
MY_PATH_IO		:= $(LOCAL_PATH)/../../../io
MY_PATH_PARSER	:= $(LOCAL_PATH)/../../../parser
MY_PATH_MFW		:= $(LOCAL_PATH)/../../../mfw
MY_PATH_TEST	:= $(LOCAL_PATH)/../../../autotest
MY_PATH_GK		:= $(LOCAL_PATH)/../../../codec/gk
MY_PATH_HEVC	:= $(LOCAL_PATH)/../../../codec/openHEVC
MY_PATH_SPEEX	:= $(LOCAL_PATH)/../../../codec/speex
MY_PATH_SSL		:= $(LOCAL_PATH)/../../../ext/OpenSSL
MY_PATH_LIBYUV	:= $(LOCAL_PATH)/../../../ext/libyuv
MY_PATH_EXTSRC	:= $(LOCAL_PATH)/../../../projects/win32/TestCode

MFW_COMM_FILES := \
				$(MY_PATH_UTIL)/UAVParser.cpp  				\
				$(MY_PATH_UTIL)/UAVFormatFunc.cpp  			\
				$(MY_PATH_UTIL)/ULogFunc.c					\
				$(MY_PATH_UTIL)/USystemFunc.cpp				\
				$(MY_PATH_UTIL)/UBuffTrace.cpp 				\
				$(MY_PATH_UTIL)/UFileFunc.cpp     			\
				$(MY_PATH_UTIL)/UIntReader.cpp    			\
				$(MY_PATH_UTIL)/ULibFunc.cpp      			\
				$(MY_PATH_UTIL)/UMsgMng.cpp       			\
				$(MY_PATH_UTIL)/USourceFormat.cpp 			\
				$(MY_PATH_UTIL)/UThreadFunc.cpp   			\
				$(MY_PATH_UTIL)/UUrlParser.cpp    			\
				$(MY_PATH_UTIL)/USocketFunc.cpp    			\
				$(MY_PATH_BASE)/CBaseObject.cpp 			\
				$(MY_PATH_BASE)/CBaseSetting.cpp 			\
				$(MY_PATH_BASE)/CBitReader.cpp				\
				$(MY_PATH_BASE)/CBuffMng.cpp				\
				$(MY_PATH_BASE)/CBuffTrace.cpp				\
				$(MY_PATH_BASE)/CIOReader.cpp				\
				$(MY_PATH_BASE)/CMemFile.cpp				\
				$(MY_PATH_BASE)/CMsgMng.cpp					\
				$(MY_PATH_BASE)/CMutexLock.cpp				\
				$(MY_PATH_BASE)/CNodeList.cpp				\
				$(MY_PATH_BASE)/CNDKSysInfo.cpp				\
				$(MY_PATH_BASE)/CThreadSem.cpp 				\
				$(MY_PATH_BASE)/CThreadWork.cpp				\
				$(MY_PATH_ANL)/CAnalysisMng.cpp				\
				$(MY_PATH_ANL)/CAnalBase.cpp				\
				$(MY_PATH_ANL)/CAnalDataSender.cpp			\
				$(MY_PATH_ANL)/CAnalJedi.cpp				\
				$(MY_PATH_ANL)/CAnalPili.cpp				\
				$(MY_PATH_ANL)/CAnalPandora.cpp				\
				$(MY_PATH_DRM)/aes128/AES_CBC.cpp			\
				$(MY_PATH_DRM)/aes128/aes-internal.c			\
				$(MY_PATH_DRM)/aes128/aes-internal-dec.c		\
				$(MY_PATH_DRM)/normal_hls/CNormalHLSDrm.cpp	\
				$(MY_PATH_IO)/qcIO.cpp						\
				$(MY_PATH_IO)/CBaseIO.cpp					\
				$(MY_PATH_IO)/CFileIO.cpp					\
				$(MY_PATH_IO)/CQCLibIO.cpp					\
				$(MY_PATH_IO)/CDNSLookUp.cpp				\
				$(MY_PATH_IO)/http2/CDNSCache.cpp			\
				$(MY_PATH_IO)/http2/CHTTPClient.cpp			\
				$(MY_PATH_IO)/http2/COpenSSL.cpp			\
				$(MY_PATH_IO)/http2/CHTTPIO2.cpp			\
				$(MY_PATH_IO)/http2/CPDFileIO.cpp			\
				$(MY_PATH_IO)/http2/CPDData.cpp				\
				$(MY_PATH_IO)/rtmp/amf.c					\
				$(MY_PATH_IO)/rtmp/hashswf.c				\
				$(MY_PATH_IO)/rtmp/log.c					\
				$(MY_PATH_IO)/rtmp/parseurl.c				\
				$(MY_PATH_IO)/rtmp/rtmp.c					\
				$(MY_PATH_IO)/rtmp/CRTMPIO.cpp				\
				$(MY_PATH_IO)/CExtIO.cpp				\
				$(MY_PATH_PARSER)/qcParser.cpp					\
				$(MY_PATH_PARSER)/CBaseParser.cpp				\
				$(MY_PATH_PARSER)/flv/CFLVParser.cpp			\
				$(MY_PATH_PARSER)/flv/CFLVTag.cpp				\
				$(MY_PATH_PARSER)/mp4/CMP4Parser.cpp			\
				$(MY_PATH_PARSER)/mp4/CMP4ParserBase.cpp		\
				$(MY_PATH_PARSER)/mp4/CMP4ParserMoov.cpp		\
				$(MY_PATH_PARSER)/mp4/CMP4ParserFrag.cpp		\
				$(MY_PATH_PARSER)/ts/CADTSFrameSpliter.cpp		\
				$(MY_PATH_PARSER)/ts/CCheckTimestampCache.cpp	\
				$(MY_PATH_PARSER)/ts/CFrameSpliter.cpp			\
				$(MY_PATH_PARSER)/ts/CTSParser.cpp				\
				$(MY_PATH_PARSER)/ts/tsbase.cpp					\
				$(MY_PATH_PARSER)/ts/tsparser.cpp				\
				$(MY_PATH_PARSER)/ts/CH2645FrameSpliter.cpp		\
				$(MY_PATH_PARSER)/m3u8/CAdaptiveStreamHLS.cpp	\
				$(MY_PATH_PARSER)/m3u8/CHls_entity.cpp			\
				$(MY_PATH_PARSER)/m3u8/CHls_manager.cpp			\
				$(MY_PATH_PARSER)/m3u8/CHls_parser.cpp			\
				$(MY_PATH_PARSER)/AdaptiveStreamParser/CDataBox.cpp	\
				$(MY_PATH_PARSER)/ba/CAdaptiveStreamBA.cpp			\
				$(MY_PATH_GK)/tStretch/TDStretch.cpp				\
				$(MY_PATH_GK)/audioRSMP/aflibConverter.cpp			\
				$(MY_PATH_MFW)/common/CBaseAudioDec.cpp				\
				$(MY_PATH_MFW)/common/CBaseAudioRnd.cpp				\
				$(MY_PATH_MFW)/common/CBaseClock.cpp				\
				$(MY_PATH_MFW)/common/CBaseSource.cpp				\
				$(MY_PATH_MFW)/common/CBaseVideoDec.cpp				\
				$(MY_PATH_MFW)/common/CBaseVideoRnd.cpp				\
				$(MY_PATH_MFW)/common/CGKAudioDec.cpp				\
				$(MY_PATH_MFW)/common/CQCAdpcmDec.cpp				\
				$(MY_PATH_MFW)/common/CGKVideoDec.cpp				\
				$(MY_PATH_MFW)/common/CQCAudioDec.cpp				\
				$(MY_PATH_MFW)/common/CQCSpeexDec.cpp				\
				$(MY_PATH_MFW)/common/CQCVideoDec.cpp				\
				$(MY_PATH_MFW)/common/CQCVideoEnc.cpp				\
				$(MY_PATH_MFW)/common/CQCSource.cpp					\
				$(MY_PATH_MFW)/common/CQCFFConcat.cpp				\
				$(MY_PATH_MFW)/common/CQCFFSource.cpp				\
				$(MY_PATH_MFW)/common/CExtAVSource.cpp			\
				$(MY_PATH_MFW)/ombox/CBoxAudioDec.cpp				\
				$(MY_PATH_MFW)/ombox/CBoxAudioRnd.cpp				\
				$(MY_PATH_MFW)/ombox/CBoxBase.cpp					\
				$(MY_PATH_MFW)/ombox/CBoxMonitor.cpp				\
				$(MY_PATH_MFW)/ombox/CBoxRender.cpp					\
				$(MY_PATH_MFW)/ombox/CBoxSource.cpp					\
				$(MY_PATH_MFW)/ombox/CBoxVideoDec.cpp				\
				$(MY_PATH_MFW)/ombox/CBoxVideoRnd.cpp				\
				$(MY_PATH_MFW)/ombox/CBoxVDecRnd.cpp				\
				$(MY_PATH_MFW)/ombox/COMBoxMng.cpp					\
				$(MY_PATH_MFW)/qcPlayerEng.cpp						\
				$(MY_PATH_MFW)/ndk/CNDKSendBuff.cpp					\
				$(MY_PATH_MFW)/ndk/CNDKAudioRnd.cpp					\
				$(MY_PATH_MFW)/ndk/COpenSLESRnd.cpp					\
				$(MY_PATH_MFW)/ndk/CNDKVideoRnd.cpp					\
				$(MY_PATH_MFW)/ndk/CNDKVDecRnd.cpp					\
				$(MY_PATH_MFW)/ndk/CMediaCodecDec.cpp				\
				$(MY_PATH_MFW)/ndk/CJniEnvUtil.cpp					\
				$(MY_PATH_MFW)/ndk/CNDKPlayer.cpp					\
				$(MY_PATH_MFW)/ndk/jniPlayer.cpp					\
				$(MY_PATH_TEST)/CTestMng.cpp						\
				$(MY_PATH_TEST)/CTestBase.cpp						\
				$(MY_PATH_TEST)/CTestInst.cpp						\
				$(MY_PATH_TEST)/CTestTask.cpp						\
				$(MY_PATH_TEST)/CTestItem.cpp						\
				$(MY_PATH_TEST)/CTestPlayer.cpp						\
				$(MY_PATH_EXTSRC)/CExtSource.cpp				\	
			
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES :=	$(MFW_COMM_FILES) 	\

else
LOCAL_SRC_FILES :=	$(MFW_COMM_FILES) 	\

endif

LOCAL_C_INCLUDES := $(MY_PATH_INC) 							\
					$(MY_PATH_BASE) 						\
					$(MY_PATH_ANL)							\
					$(MY_PATH_DRM)/aes128 					\
					$(MY_PATH_DRM)/normal_hls 				\
					$(MY_PATH_UTIL) 						\
					$(MY_PATH_IO)	 						\
					$(MY_PATH_IO)/http2						\
					$(MY_PATH_IO)/rtmp						\
					$(MY_PATH_PARSER) 						\
					$(MY_PATH_PARSER)/AdaptiveStreamParser 	\
					$(MY_PATH_PARSER)/ts					\
					$(MY_PATH_PARSER)/ba					\
					$(MY_PATH_GK)/tStretch 					\
					$(MY_PATH_GK)/audioRSMP 				\
					$(MY_PATH_MFW) 							\
					$(MY_PATH_MFW)/ombox 					\
					$(MY_PATH_MFW)/common					\
					$(MY_PATH_GK)/include 					\
					$(MY_PATH_SPEEX)/inc 					\
					$(MY_PATH_HEVC)/include					\
					$(MY_PATH_SSL)/include					\
					$(MY_PATH_LIBYUV)/include				\
					$(MY_PATH_TEST) 						\
					$(MY_PATH_EXTSRC)						\
					$(LOCAL_PATH)/..						\
