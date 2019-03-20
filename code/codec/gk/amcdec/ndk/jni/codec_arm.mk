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

include $(CLEAR_VARS)

MY_INC_PATH_MEDIACODECJ := $(LOCAL_PATH)/../../inc
MY_SRC_PATH_MEDIACODECJ := $(LOCAL_PATH)/../../src


#MediaCodecDec
include $(CLEAR_VARS)

LOCAL_MODULE    := amcDec

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES :=  \
				$(MY_SRC_PATH_MEDIACODECJ)/MediaCodecJava.cpp 		\
				$(MY_SRC_PATH_MEDIACODECJ)/ttMediaCodecDec.cpp		\
				$(MY_SRC_PATH_MEDIACODECJ)/TTJniEnvUtil.cpp			\


LOCAL_C_INCLUDES :=  \
		$(MY_INC_PATH_MEDIACODECJ) 		\
		$(LOCAL_PATH)/../../../include		\

LOCAL_CFLAGS := -DNDEBUG -ffunction-sections -fdata-sections
LOCAL_LDFLAGS := -Wl,--gc-sections

LOCAL_LDLIBS	:= -llog 

include $(BUILD_SHARED_LIBRARY)