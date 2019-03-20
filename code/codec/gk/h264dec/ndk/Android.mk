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

# H264Dec
include $(CLEAR_VARS)

LOCAL_MODULE    := qcH264Dec

include $(LOCAL_PATH)/code.mk

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CFLAGS := -DNDEBUG -mfloat-abi=soft -mfpu=neon -march=armv7-a -mtune=cortex-a8 -fsigned-char -O2 -ffast-math  -nostdlib -enable-int-quality -mandroid -fvisibility=hidden -ffunction-sections -fdata-sections
LOCAL_CFLAGS += -DOPT_ARMV7_ANDROID=1  
else ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_CFLAGS := -DNDEBUG -mfloat-abi=soft -mfpu=vfp -marm -fsigned-char -O2 -ffast-math  -nostdlib -enable-int-quality -mandroid -fvisibility=hidden -ffunction-sections -fdata-sections			
else ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_CFLAGS	:= -D__GCC32__ -fsigned-char -DNDEBUG -DOPT_X86
endif
LOCAL_LDFLAGS := -llog -Wl,--gc-sections

#$(warning "###-- $(LOCAL_CFLAGS)"--###)

include $(BUILD_SHARED_LIBRARY)

ifeq ($(TARGET_ARCH),arm)
$(call import-module,android/cpufeatures)
endif

