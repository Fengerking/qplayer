# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

# Media jni

include $(CLEAR_VARS)

MY_INC_PATH_INFO := $(LOCAL_PATH)/../info/inc
MY_SRC_PATH_INFO := ../info/src

MY_INC_PATH_PLAYER := $(LOCAL_PATH)/inc
MY_SRC_PATH_PLAYER := src

MY_INC_PATH_COMMON := $(LOCAL_PATH)/../common/inc
MY_SRC_PATH_COMMON := ../common/src

MY_INC_PATH_AFLIB := $(LOCAL_PATH)/../audiodecoder/src/aflib

MY_INC_PATH_TDSLIB := $(LOCAL_PATH)/../audiodecoder/src/tStretch

LOCAL_MODULE    := mediaplayer

LOCAL_ARM_MODE := arm
 
LOCAL_SRC_FILES := 	\
					TTMediaPlayer_jni.cpp \
					$(MY_SRC_PATH_COMMON)/TTAudioPlugin.cpp \
					$(MY_SRC_PATH_COMMON)/TTFFT.cpp \
					$(MY_SRC_PATH_COMMON)/TTVideoPlugin.cpp \
					$(MY_SRC_PATH_PLAYER)/TTAudioDecode.cpp \
					$(MY_SRC_PATH_PLAYER)/TTAudioProcess.cpp \
					$(MY_SRC_PATH_PLAYER)/TTBaseAudioSink.cpp \
					$(MY_SRC_PATH_PLAYER)/TTBaseVideoSink.cpp \
					$(MY_SRC_PATH_PLAYER)/TTSrcDemux.cpp \
					$(MY_SRC_PATH_PLAYER)/TTVideoDecode.cpp \
					$(MY_SRC_PATH_PLAYER)/GKMediaPlayer.cpp \
					$(MY_SRC_PATH_PLAYER)/GKMediaPlayerFactory.cpp \
					$(MY_SRC_PATH_PLAYER)/Android/TTAndroidAudioSink.cpp \
					$(MY_SRC_PATH_PLAYER)/Android/TTAndroidVideoSink.cpp \
					$(MY_SRC_PATH_INFO)/TTIntReader.cpp \
					$(MY_SRC_PATH_INFO)/TTID3Tag.cpp \
					$(MY_SRC_PATH_INFO)/TTAPETag.cpp \
					$(MY_SRC_PATH_INFO)/TTMediaParser.cpp \
					$(MY_SRC_PATH_INFO)/TTHttpAACParser.cpp \
					$(MY_SRC_PATH_INFO)/TTAACHeader.cpp \
					$(MY_SRC_PATH_INFO)/TTAACParser.cpp \
					$(MY_SRC_PATH_INFO)/TTMP4Parser.cpp \
					$(MY_SRC_PATH_INFO)/TTMediainfoProxy.cpp \
					$(MY_SRC_PATH_INFO)/TTBufferManager.cpp \
					$(MY_SRC_PATH_INFO)/TTStreamQueue.cpp \
					$(MY_SRC_PATH_INFO)/TTFLVTag.cpp \
					$(MY_SRC_PATH_INFO)/TTFLVParser.cpp \
					$(MY_SRC_PATH_INFO)/../hls/TTHLSInfoProxy.cpp \
					$(MY_SRC_PATH_INFO)/../hls/TTLiveSession.cpp \
					$(MY_SRC_PATH_INFO)/../hls/TTM3UParser.cpp \
					$(MY_SRC_PATH_INFO)/../hls/TTPackedaudioParser.cpp \
					$(MY_SRC_PATH_INFO)/../hls/TTPlaylistManager.cpp \
					$(MY_SRC_PATH_INFO)/../hls/TTString.cpp \
					$(MY_SRC_PATH_INFO)/../ts/TTTSParserProxy.cpp \
					$(MY_SRC_PATH_INFO)/../rtmp/rtmpamf.c \
					$(MY_SRC_PATH_INFO)/../rtmp/rtmp.c \
					$(MY_SRC_PATH_INFO)/../rtmp/rtmparseurl.c \
					$(MY_SRC_PATH_INFO)/../rtmp/TTRtmpDownload.cpp \
					$(MY_SRC_PATH_INFO)/../rtmp/TTRtmpInfoProxy.cpp 
					
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
					$(MY_INC_PATH_PLAYER) \
					$(MY_INC_PATH_COMMON) \
					$(MY_INC_PATH_INFO) \
					$(MY_INC_PATH_INFO)/../hls/ \
					$(MY_INC_PATH_INFO)/../ts/ \
					$(MY_INC_PATH_INFO)/../rtmp/ \
					$(MY_INC_PATH_TAG) \
					$(MY_INC_PATH_PLAYER)/Android/ \
					$(MY_INC_PATH_AFLIB) \
					$(MY_INC_PATH_TDSLIB)
					
LOCAL_EXPORT_C_INCLUDES := \
					$(MY_INC_PATH_INFO) \
					$(MY_INC_PATH_COMMON) \
					$(MY_INC_PATH_PLAYER)

LOCAL_LDLIBS	:= -ldl

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS	+=-D_ARM_ARCH_VFP_ -DCPU_ARM -DNO_CRYPTO -ffunction-sections -fdata-sections
LOCAL_CPPFLAGS	+=-D_ARM_ARCH_VFP_ -DCPU_ARM -DNO_CRYPTO -ffunction-sections -fdata-sections
LOCAL_STATIC_LIBRARIES += cpufeatures
else
LOCAL_CFLAGS	+= -DNO_CRYPTO
LOCAL_CPPFLAGS	+=-DNO_CRYPTO
endif

LOCAL_LDFLAGS := -Wl,--gc-sections
					
LOCAL_SHARED_LIBRARIES := osal audiofx resample

include $(BUILD_SHARED_LIBRARY)

