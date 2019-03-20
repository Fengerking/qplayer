LOCAL_PATH := $(call my-dir)/../../../

include $(LOCAL_PATH)/android/lib/Android.mk

include $(CLEAR_VARS)
LOCAL_MODULE    := libyuv
LOCAL_SRC_FILES += $(LOCAL_PATH)/../libyuv/lib/ndk/$(TARGET_ARCH_ABI)/libyuv.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

MY_PATH_SPEEX	:= $(LOCAL_PATH)/../../codec/speex
MY_PATH_LIBYUV	:= $(LOCAL_PATH)/../libyuv

LOCAL_CFLAGS := -D__QC_OS_NDK__ -D__QC_FF_CODEC__ -DHAVE_CONFIG_H

LOCAL_MODULE := qcCodec

LOCAL_C_INCLUDES := \
				$(LOCAL_PATH)/../../include				\
				$(LOCAL_PATH)/../../base				\
				$(LOCAL_PATH)/../../util				\
				$(LOCAL_PATH)/../../codec/speex/inc		\
				$(LOCAL_PATH)/../libyuv/include			\
				$(LOCAL_PATH)/QCP						\
				$(LOCAL_PATH)/android/include			\

LOCAL_CPPFLAGS := -Wno-multichar -fno-exceptions -fno-rtti

LOCAL_SRC_FILES :=	\
					compat/strtod.c						\
					QCP/qcFFLog.c						\
					QCP/qcCodec.c 						\
					QCP/qcColorCvtR.c 					\
					QCP/qcEncoder.c						\
					QCP/qcFormat.cpp 					\
					QCP/CBaseFFParser.cpp 				\
					QCP/CFFMpegParser.cpp 				\
					QCP/CFFMpegInIO.cpp 				\
					QCP/CFFMpegIO.cpp 					\
					QCP/CFFBaseIO.cpp 					\
					QCP/qcFFIO.cpp 						\
					QCP/CBaseFFMuxer.cpp 				\
					QCP/qcFFMuxer.cpp 					\
					$(LOCAL_PATH)/../../util/UAVParser.cpp		\
					$(LOCAL_PATH)/../../base/CBitReader.cpp		\
					$(MY_PATH_SPEEX)/src/bits.c               	\
					$(MY_PATH_SPEEX)/src/cb_search.c          	\
					$(MY_PATH_SPEEX)/src/exc_10_16_table.c    	\
					$(MY_PATH_SPEEX)/src/exc_10_32_table.c   	\
					$(MY_PATH_SPEEX)/src/exc_20_32_table.c  	\
					$(MY_PATH_SPEEX)/src/exc_5_256_table.c  	\
					$(MY_PATH_SPEEX)/src/exc_5_64_table.c   	\
					$(MY_PATH_SPEEX)/src/exc_8_128_table.c   	\
					$(MY_PATH_SPEEX)/src/fftwrap.c          	\
					$(MY_PATH_SPEEX)/src/filters.c      		\
					$(MY_PATH_SPEEX)/src/gain_table.c       	\
					$(MY_PATH_SPEEX)/src/gain_table_lbr.c     	\
					$(MY_PATH_SPEEX)/src/hexc_10_32_table.c    	\
					$(MY_PATH_SPEEX)/src/hexc_table.c         	\
					$(MY_PATH_SPEEX)/src/high_lsp_tables.c    	\
					$(MY_PATH_SPEEX)/src/kiss_fft.c         	\
					$(MY_PATH_SPEEX)/src/kiss_fftr.c         	\
					$(MY_PATH_SPEEX)/src/lpc.c         			\
					$(MY_PATH_SPEEX)/src/lsp.c         			\
					$(MY_PATH_SPEEX)/src/lsp_tables_nb.c       	\
					$(MY_PATH_SPEEX)/src/ltp.c         			\
					$(MY_PATH_SPEEX)/src/modes.c         		\
					$(MY_PATH_SPEEX)/src/modes_wb.c         	\
					$(MY_PATH_SPEEX)/src/nb_celp.c         		\
					$(MY_PATH_SPEEX)/src/quant_lsp.c         	\
					$(MY_PATH_SPEEX)/src/sb_celp.c         		\
					$(MY_PATH_SPEEX)/src/smallft.c         		\
					$(MY_PATH_SPEEX)/src/speex.c         		\
					$(MY_PATH_SPEEX)/src/speex_callbacks.c   	\
					$(MY_PATH_SPEEX)/src/speex_header.c         \
					$(MY_PATH_SPEEX)/src/stereo.c         		\
					$(MY_PATH_SPEEX)/src/vbr.c         			\
					$(MY_PATH_SPEEX)/src/vorbis_psy.c         	\
					$(MY_PATH_SPEEX)/src/vq.c         			\
					$(MY_PATH_SPEEX)/src/window.c         		\					

LOCAL_LDLIBS := -lz  -llog
LOCAL_STATIC_LIBRARIES += libswresample libswscale libavformat libavcodec libavutil libyuv

include $(BUILD_SHARED_LIBRARY)

