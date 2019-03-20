LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
include $(LOCAL_PATH)/codec_armv7.mk
endif

ifeq ($(TARGET_ARCH_ABI),armeabi)
include $(LOCAL_PATH)/codec_armv6.mk
endif


ifeq ($(TARGET_ARCH_ABI),x86)
include $(LOCAL_PATH)/codec.mk
endif