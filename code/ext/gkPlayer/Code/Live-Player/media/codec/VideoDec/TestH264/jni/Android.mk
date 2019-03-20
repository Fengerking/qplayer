LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := TestVideoDec

CSRC_PATH:=../

LOCAL_SRC_FILES := \
        $(CSRC_PATH)/process.c	 	
	
LOCAL_C_INCLUDES := \
	../inc \

VOMM:= -DANDROID -DLINUX 

LOCAL_CFLAGS := -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
LOCAL_LDLIBS := -llog

include $(BUILD_EXECUTABLE)