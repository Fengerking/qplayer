LOCAL_PATH := $(call my-dir)/../../../

include $(CLEAR_VARS)
LOCAL_MODULE    := libcrypto
LOCAL_SRC_FILES += $(LOCAL_PATH)/lib/ndk/$(TARGET_ARCH_ABI)/libcrypto.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libssl
LOCAL_SRC_FILES += $(LOCAL_PATH)/lib/ndk/$(TARGET_ARCH_ABI)/libssl.a
include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -std=c99 -fPIC -DOPENSSL_NO_COMP -D__QC_OS_NDK__ 

LOCAL_MODULE := qcOpenSSL

LOCAL_C_INCLUDES := \
				$(LOCAL_PATH)/../../include				\
				$(LOCAL_PATH)/include					\

LOCAL_CPPFLAGS := -Wno-multichar -fno-exceptions -fno-rtti

LOCAL_SRC_FILES := wrap/qcOpenSSL.c

LOCAL_LDLIBS := -llog -lz
LOCAL_STATIC_LIBRARIES += libssl libcrypto

include $(BUILD_SHARED_LIBRARY)

