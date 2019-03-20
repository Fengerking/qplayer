LOCAL_PATH := $(call my-dir)/../../../

include $(CLEAR_VARS)

LOCAL_CFLAGS := \
				-D_LARGEFILE_SOURCE \
				-DHAVE_AV_CONFIG_H \
				-std=c99

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CFLAGS += -O3 -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -D__QC_OS_NDK__ -D__QC_FF_CODEC__
LOCAL_ARM_MODE := arm 
else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_CFLAGS += -O3 -D__QC_OS_NDK__ -D__QC_FF_CODEC__
LOCAL_ARM_MODE := arm 
else
LOCAL_CFLAGS += -D__GCC32__ -fsigned-char -D__QC_OS_NDK__ -D__QC_FF_CODEC__
endif

LOCAL_MODULE := qcCodec

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_C_INCLUDES := \
				$(LOCAL_PATH)/compat/atomics/gcc		\
				$(LOCAL_PATH)/../../include				\
				$(LOCAL_PATH)/QCP						\
				$(LOCAL_PATH)/QCP/ndk/config_bak/armv7	\

else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_C_INCLUDES := \
				$(LOCAL_PATH)/compat/atomics/gcc		\
				$(LOCAL_PATH)/../../include				\
				$(LOCAL_PATH)/QCP						\
				$(LOCAL_PATH)/QCP/ndk/config_bak/armv8	\
				
else
LOCAL_C_INCLUDES := \
				$(LOCAL_PATH)/compat/atomics/gcc		\
				$(LOCAL_PATH)/../../include				\
				$(LOCAL_PATH)/QCP						\
				$(LOCAL_PATH)/QCP/ndk/config_bak/x86	\

				
endif

LOCAL_CPPFLAGS := -Wno-multichar -fno-exceptions -fno-rtti

include $(LOCAL_PATH)/QCP/ndk/jni/ffmpeg.mk

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES :=	$(AVUTIL_COMM_FILES) 		\
					$(AVUTIL_ARM_FILES) 		\
					$(AVCODEC_COMM_FILES) 		\
					$(AVCODEC_ARM_FILES) 		\
					$(AVCODEC_MJPEG_FILES)		\
					$(AVCODEC_MJPEG_ARM_FILES)	\
					$(AVFORMAT_COMM_FILES) 		\

else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES :=	$(AVUTIL_COMM_FILES) 		\
					$(AVCODEC_COMM_FILES) 		\
					$(AVCODEC_MJPEG_FILES)		\
					$(AVFORMAT_COMM_FILES) 		\

else
LOCAL_SRC_FILES :=	$(AVUTIL_COMM_FILES) 		\
					$(AVUTIL_X86_FILES) 		\
					$(AVCODEC_COMM_FILES) 		\
					$(AVCODEC_X86_FILES) 		\
					$(AVCODEC_MJPEG_FILES)		\
					$(AVCODEC_MJPEG_X86_FILES)	\
					$(AVFORMAT_COMM_FILES) 		\

endif

LOCAL_LDLIBS := -lz  -llog

include $(BUILD_SHARED_LIBRARY)

