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

MY_INC_PATH_MEDIACODECJLUGIN:= $(LOCAL_PATH)/MedicCodecJ/inc
MY_SRC_PATH_MEDIACODECJ:= MedicCodecJ/src

MY_INC_PATH_OSALINC:= $(LOCAL_PATH)/../../osal/inc

MY_INC_PATH_AACPLUGIN:= $(LOCAL_PATH)/AACDec/inc
MY_INC_PATH_COMMONIN := $(LOCAL_PATH)/../common/inc
MY_SRC_PATH_AACDEC:= AACDec/src
MY_SRC_PATH_AACDEC_ASM_V6:= AACDec/src/linuxasmv6
MY_SRC_PATH_AACDEC_ASM_V7:= AACDec/src/linuxasmv7

MY_INC_PATH_CLCONV:= $(LOCAL_PATH)/ColorConv/inc
MY_SRC_PATH_CLCONV:= ColorConv/src
MY_SRC_PATH_CLCONV_NEON:= ColorConv/src/gnu


# AACDec
include $(CLEAR_VARS)

#$(warning "###-- $(TARGET_ARCH_ABI)"--###)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_MODULE    := AACDec_v7
else
LOCAL_MODULE    := AACDec
endif

LOCAL_ARM_MODE := arm


ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES :=	$(MY_SRC_PATH_AACDEC)/bitstream.c \
					$(MY_SRC_PATH_AACDEC)/bsac_dec_spectra.c \
					$(MY_SRC_PATH_AACDEC)/decframe.c \
					$(MY_SRC_PATH_AACDEC)/decode_bsac.c \
					$(MY_SRC_PATH_AACDEC)/decoder.c \
					$(MY_SRC_PATH_AACDEC)/downMatrix.c \
					$(MY_SRC_PATH_AACDEC)/Header.c \
					$(MY_SRC_PATH_AACDEC)/ic_predict.c \
					$(MY_SRC_PATH_AACDEC)/latmheader.c \
					$(MY_SRC_PATH_AACDEC)/lc_dequant.c \
					$(MY_SRC_PATH_AACDEC)/lc_huff.c \
					$(MY_SRC_PATH_AACDEC)/lc_hufftab.c \
					$(MY_SRC_PATH_AACDEC)/lc_imdct.c \
					$(MY_SRC_PATH_AACDEC)/lc_mdct.c \
					$(MY_SRC_PATH_AACDEC)/lc_pns.c \
					$(MY_SRC_PATH_AACDEC)/lc_stereo.c \
					$(MY_SRC_PATH_AACDEC)/lc_syntax.c \
					$(MY_SRC_PATH_AACDEC)/lc_tns.c \
					$(MY_SRC_PATH_AACDEC)/ltp_dec.c \
					$(MY_SRC_PATH_AACDEC)/ps_dec.c \
					$(MY_SRC_PATH_AACDEC)/ps_syntax.c \
					$(MY_SRC_PATH_AACDEC)/sam_decode_bsac.c \
					$(MY_SRC_PATH_AACDEC)/sam_fadecode.c \
					$(MY_SRC_PATH_AACDEC)/sbr_dec.c \
					$(MY_SRC_PATH_AACDEC)/sbr_hfadj.c \
					$(MY_SRC_PATH_AACDEC)/sbr_hfgen.c \
					$(MY_SRC_PATH_AACDEC)/sbr_huff.c \
					$(MY_SRC_PATH_AACDEC)/sbr_qmf.c \
					$(MY_SRC_PATH_AACDEC)/sbr_syntax.c \
					$(MY_SRC_PATH_AACDEC)/sbr_utility.c \
					$(MY_SRC_PATH_AACDEC)/table.c \
					$(MY_SRC_PATH_AACDEC)/unit.c \
					$(MY_SRC_PATH_AACDEC_ASM_V7)/qmf.S \
					$(MY_SRC_PATH_AACDEC_ASM_V7)/WinLong_V7.S \
					$(MY_SRC_PATH_AACDEC_ASM_V7)/PostMultiply_V7.S \
					$(MY_SRC_PATH_AACDEC_ASM_V7)/PreMultiply_V7.S \
					$(MY_SRC_PATH_AACDEC_ASM_V7)/R4_Core_V7.S \
					$(MY_SRC_PATH_AACDEC_ASM_V7)/R8FirstPass_v7.S \
					$(MY_SRC_PATH_AACDEC_ASM_V7)/writePCM_ARMV6.S \
					$(MY_SRC_PATH_AACDEC_ASM_V7)/writePCM_ARMV7.S
					
else

LOCAL_SRC_FILES :=	$(MY_SRC_PATH_AACDEC)/bitstream.c \
					$(MY_SRC_PATH_AACDEC)/bsac_dec_spectra.c \
					$(MY_SRC_PATH_AACDEC)/decframe.c \
					$(MY_SRC_PATH_AACDEC)/decode_bsac.c \
					$(MY_SRC_PATH_AACDEC)/decoder.c \
					$(MY_SRC_PATH_AACDEC)/downMatrix.c \
					$(MY_SRC_PATH_AACDEC)/Header.c \
					$(MY_SRC_PATH_AACDEC)/ic_predict.c \
					$(MY_SRC_PATH_AACDEC)/latmheader.c \
					$(MY_SRC_PATH_AACDEC)/lc_dequant.c \
					$(MY_SRC_PATH_AACDEC)/lc_huff.c \
					$(MY_SRC_PATH_AACDEC)/lc_hufftab.c \
					$(MY_SRC_PATH_AACDEC)/lc_imdct.c \
					$(MY_SRC_PATH_AACDEC)/lc_mdct.c \
					$(MY_SRC_PATH_AACDEC)/lc_pns.c \
					$(MY_SRC_PATH_AACDEC)/lc_stereo.c \
					$(MY_SRC_PATH_AACDEC)/lc_syntax.c \
					$(MY_SRC_PATH_AACDEC)/lc_tns.c \
					$(MY_SRC_PATH_AACDEC)/ltp_dec.c \
					$(MY_SRC_PATH_AACDEC)/ps_dec.c \
					$(MY_SRC_PATH_AACDEC)/ps_syntax.c \
					$(MY_SRC_PATH_AACDEC)/sam_decode_bsac.c \
					$(MY_SRC_PATH_AACDEC)/sam_fadecode.c \
					$(MY_SRC_PATH_AACDEC)/sbr_dec.c \
					$(MY_SRC_PATH_AACDEC)/sbr_hfadj.c \
					$(MY_SRC_PATH_AACDEC)/sbr_hfgen.c \
					$(MY_SRC_PATH_AACDEC)/sbr_huff.c \
					$(MY_SRC_PATH_AACDEC)/sbr_qmf.c \
					$(MY_SRC_PATH_AACDEC)/sbr_syntax.c \
					$(MY_SRC_PATH_AACDEC)/sbr_utility.c \
					$(MY_SRC_PATH_AACDEC)/table.c \
					$(MY_SRC_PATH_AACDEC)/unit.c \
					$(MY_SRC_PATH_AACDEC_ASM_V6)/PostMultiply_V6.S \
					$(MY_SRC_PATH_AACDEC_ASM_V6)/qmf.S \
					$(MY_SRC_PATH_AACDEC_ASM_V6)/PreMultiply_V6.S \
					$(MY_SRC_PATH_AACDEC_ASM_V6)/R4_Core_v6.S \
					$(MY_SRC_PATH_AACDEC_ASM_V6)/R8FirstPass_v6.S \
					$(MY_SRC_PATH_AACDEC_ASM_V6)/writePCM_ARMV6.S
endif

#$(warning "###-- $(LOCAL_SRC_FILES)"--###)
					
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
					$(MY_INC_PATH_AACPLUGIN) \
					$(MY_INC_PATH_COMMONIN) \
					$(MY_INC_PATH_AFLIB) \
					$(MY_INC_PATH_OSALINC)  \

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_NEON := true
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CFLAGS := -DARM -DARMV6 -DARMV7 -DLINUX -DHIDDEN_SYMBOL=0 -O2 -DNDEBUG -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a8 -mfpu=neon -fsigned-char -fvisibility=hidden -ffunction-sections -fdata-sections
else
LOCAL_CFLAGS := -DARM -DARMV6 -DLINUX -DHIDDEN_SYMBOL=0  -O2 -DNDEBUG -mfloat-abi=soft -march=armv6j -mtune=arm1136jf-s -mfpu=vfp -fsigned-char -fvisibility=hidden -ffunction-sections -fdata-sections
endif
LOCAL_LDFLAGS := -llog -Wl,--gc-sections

#$(warning "###-- $(LOCAL_CFLAGS)"--###)

LOCAL_SHARED_LIBRARIES := mediaplayer osal

include $(BUILD_SHARED_LIBRARY)

# Resample module
include $(CLEAR_VARS)
MY_INC_PATH_AFLIB:=  $(LOCAL_PATH)/../audiodecoder/src/aflib
MY_SRC_PATH_AFLIB:= ../audiodecoder/src/aflib
MY_INC_PATH_TDSLIB:=  $(LOCAL_PATH)/../audiodecoder/src/tStretch
MY_SRC_PATH_TDSLIB:= ../audiodecoder/src/tStretch

LOCAL_MODULE    := resample

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES :=  $(MY_SRC_PATH_AFLIB)/aflibConverter.cpp	\
										$(MY_SRC_PATH_TDSLIB)/TDStretch.cpp

LOCAL_C_INCLUDES :=	$(MY_INC_PATH_AFLIB)	\
										$(MY_INC_PATH_TDSLIB)

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS := -D_RESAMPLE_SMALL_FILTER_ -O2 -ffunction-sections -fdata-sections
LOCAL_LDFLAGS := -Wl,--gc-sections
endif

include $(BUILD_SHARED_LIBRARY)

# Color Conversion
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_MODULE    := clconv_v7
else
LOCAL_MODULE    := clconv
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_NEON := true
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES := 	\
					$(MY_SRC_PATH_CLCONV)/clconv.c \
					$(MY_SRC_PATH_CLCONV_NEON)/YUV420toRGB32_8nx2n_armv7.S
						
endif					

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CFLAGS := -D_ARM_ARCH_NEON_ -mfloat-abi=soft -mfpu=neon -march=armv7-a -mtune=cortex-a8 -fsigned-char -O2 -ffast-math  -nostdlib -enable-int-quality -mandroid				
else
LOCAL_CFLAGS := -mfloat-abi=soft -march=armv6j -mtune=arm1136jf-s -fsigned-char -O2 -ffast-math  -nostdlib -enable-int-quality -mandroid
endif 

ifneq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES := 	\
					$(MY_SRC_PATH_CLCONV)/clconv.c

 
endif							
								
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
					$(MY_INC_PATH_COMMONIN)  \
					$(MY_INC_PATH_CLCONV)   \
					$(MY_INC_PATH_OSALINC)  \

LOCAL_SHARED_LIBRARIES := mediaplayer osal

include $(BUILD_SHARED_LIBRARY)

# FLACDec
include $(CLEAR_VARS)

#MediaCodecDec
include $(CLEAR_VARS)

LOCAL_MODULE    := MediaCodecJDec

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES :=  \
$(MY_SRC_PATH_MEDIACODECJ)/MediaCodecJava.cpp 			\
$(MY_SRC_PATH_MEDIACODECJ)/ttMediaCodecDec.cpp			\
../../osal/src/TTJniEnvUtil.cpp	 \

LOCAL_C_INCLUDES :=  \
		$(MY_INC_PATH_MEDIACODECJLUGIN) \
		$(MY_INC_PATH_COMMONIN)		\
		$(LOCAL_PATH)/../info/inc  \
		$(MY_INC_PATH_OSALINC)  \

LOCAL_CFLAGS := -DNDEBUG -ffunction-sections -fdata-sections
LOCAL_LDFLAGS := -Wl,--gc-sections

LOCAL_LDLIBS	:= -llog 

LOCAL_SHARED_LIBRARIES := mediaplayer osal

include $(BUILD_SHARED_LIBRARY)