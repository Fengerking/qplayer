LOCAL_PATH := $(call my-dir)/../../../

include $(CLEAR_VARS)
LOCAL_MODULE    := libavutil
LOCAL_SRC_FILES += $(LOCAL_PATH)/ext/ffmpeg/android/lib/$(TARGET_ARCH_ABI)/libavutil.a
include $(PREBUILT_STATIC_LIBRARY)

LOCAL_MODULE    := libswresample
LOCAL_SRC_FILES += $(LOCAL_PATH)/ext/ffmpeg/android/lib/$(TARGET_ARCH_ABI)/libswresample.a
include $(PREBUILT_STATIC_LIBRARY)

LOCAL_MODULE    := libavcodec
LOCAL_SRC_FILES += $(LOCAL_PATH)/ext/ffmpeg/android/lib/$(TARGET_ARCH_ABI)/libavcodec.a
include $(PREBUILT_STATIC_LIBRARY)

LOCAL_MODULE    := libavformat
LOCAL_SRC_FILES += $(LOCAL_PATH)/ext/ffmpeg/android/lib/$(TARGET_ARCH_ABI)/libavformat.a
include $(PREBUILT_STATIC_LIBRARY)

LOCAL_MODULE    := libswscale
LOCAL_SRC_FILES += $(LOCAL_PATH)/ext/ffmpeg/android/lib/$(TARGET_ARCH_ABI)/libswscale.a
include $(PREBUILT_STATIC_LIBRARY)

LOCAL_MODULE    := libyuv
LOCAL_SRC_FILES += $(LOCAL_PATH)/ext/libyuv/lib/ndk/$(TARGET_ARCH_ABI)/libyuv.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := QPlayer

include $(LOCAL_PATH)/mfw/ndk/jni/onelib_ndk.mk

ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_CFLAGS := -D__QC_CPU_X86__ -D__GCC32__ -fsigned-char
else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CFLAGS := -D__QC_CPU_ARMV7__ -DARM -DLINUX -D_ARM_ARCH_NEON_ -DHIDDEN_SYMBOL=0 -fsigned-char -fvisibility=hidden -ffunction-sections -fdata-sections -mfpu=neon 
else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_CFLAGS := -D__QC_CPU_ARMV8__ -DARM -DLINUX -DHIDDEN_SYMBOL=0 -fsigned-char -fvisibility=hidden -ffunction-sections -fdata-sections
else
LOCAL_CFLAGS := -D__QC_CPU_ARMV6__ -DARM -DLINUX -DHIDDEN_SYMBOL=0 -fsigned-char -fvisibility=hidden -ffunction-sections -fdata-sections
endif

LOCAL_CFLAGS += -D__QC_OS_NDK__ -D__QC_FF_CODEC__ -DHAVE_CONFIG_H -DNO_CRYPTO -D__QC_LIB_ONE__ -D_RESAMPLE_SMALL_FILTER_
LOCAL_CPPFLAGS := -Wno-multichar -fno-exceptions -fno-rtti

LOCAL_LDFLAGS := -llog -lOpenSLES -Wl,--gc-sections
LOCAL_LDFLAGS += -Xlinker --build-id

LOCAL_LDLIBS := -lz  -llog
LOCAL_STATIC_LIBRARIES += libswresample libswscale libavformat libavcodec libavutil libyuv

include $(BUILD_SHARED_LIBRARY)

ifeq ($(TARGET_ARCH),arm)
$(call import-module,android/cpufeatures)
endif
