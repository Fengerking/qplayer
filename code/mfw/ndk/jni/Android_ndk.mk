LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
# qPlayer
LOCAL_MODULE    := QPlayer

include $(LOCAL_PATH)/code_ndk.mk

ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_CFLAGS := -D__GCC32__ -D__QC_OS_NDK__ -DHAVE_CONFIG_H -DNO_CRYPTO -D_RESAMPLE_SMALL_FILTER_ -fsigned-char
else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CFLAGS := -DARM -DLINUX -D_ARM_ARCH_NEON_ -D__QC_OS_NDK__ -DHAVE_CONFIG_H -DNO_CRYPTO -D_RESAMPLE_SMALL_FILTER_ -DHIDDEN_SYMBOL=0 -fsigned-char -fvisibility=hidden -ffunction-sections -fdata-sections -mfpu=neon 
else
LOCAL_CFLAGS := -DARM -DLINUX -D__QC_OS_NDK__ -DHAVE_CONFIG_H -DNO_CRYPTO -D_RESAMPLE_SMALL_FILTER_ -DHIDDEN_SYMBOL=0 -fsigned-char -fvisibility=hidden -ffunction-sections -fdata-sections
endif

LOCAL_LDFLAGS := -llog -lOpenSLES -Wl,--gc-sections
LOCAL_LDFLAGS += -Xlinker --build-id

include $(BUILD_SHARED_LIBRARY)

ifeq ($(TARGET_ARCH),arm)
$(call import-module,android/cpufeatures)
endif

