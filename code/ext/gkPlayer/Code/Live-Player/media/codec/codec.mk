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

MY_INC_PATH_CLCONV:= $(LOCAL_PATH)/ColorConv/inc
MY_SRC_PATH_CLCONV:= ColorConv/src


# AACDec
include $(CLEAR_VARS)

LOCAL_MODULE    := AACDec

LOCAL_SRC_FILES := $(MY_SRC_PATH_AACDEC)/bitstream.c \
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
					$(MY_SRC_PATH_AACDEC)/unit.c
					
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
					$(MY_INC_PATH_AACPLUGIN) \
					$(MY_INC_PATH_AFLIB) 


LOCAL_CFLAGS	:= -D__GCC32__ -fsigned-char


LOCAL_SHARED_LIBRARIES := mediaplayer resample osal

include $(BUILD_SHARED_LIBRARY)

# Resample module
include $(CLEAR_VARS)
MY_INC_PATH_AFLIB:=  $(LOCAL_PATH)/../audiodecoder/src/aflib
MY_SRC_PATH_AFLIB:= ../audiodecoder/src/aflib
MY_INC_PATH_TDSLIB:=  $(LOCAL_PATH)/../audiodecoder/src/tStretch
MY_SRC_PATH_TDSLIB:= ../audiodecoder/src/tStretch

LOCAL_MODULE    := resample

LOCAL_SRC_FILES :=  $(MY_SRC_PATH_AFLIB)/aflibConverter.cpp	\
										$(MY_SRC_PATH_TDSLIB)/TDStretch.cpp

LOCAL_C_INCLUDES :=	$(MY_INC_PATH_AFLIB)	\
										$(MY_INC_PATH_TDSLIB)

LOCAL_CFLAGS := -D_RESAMPLE_SMALL_FILTER_

include $(BUILD_SHARED_LIBRARY)


# Color Conversion
include $(CLEAR_VARS)

LOCAL_MODULE   := clconv

LOCAL_SRC_FILES := 	\
					$(MY_SRC_PATH_CLCONV)/clconv.c
					
								
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
					$(MY_INC_PATH_COMMONIN)  \
					$(MY_INC_PATH_CLCONV) \
					$(LOCAL_PATH)/../../osal/inc \

LOCAL_SHARED_LIBRARIES := mediaplayer osal
include $(BUILD_SHARED_LIBRARY)



#MediaCodecDec
include $(CLEAR_VARS)

LOCAL_MODULE    := MediaCodecJDec

LOCAL_SRC_FILES :=  \
$(MY_SRC_PATH_MEDIACODECJ)/MediaCodecJava.cpp 			\
$(MY_SRC_PATH_MEDIACODECJ)/ttMediaCodecDec.cpp			\
../../osal/src/TTJniEnvUtil.cpp	 \

LOCAL_C_INCLUDES :=  \
		$(MY_INC_PATH_MEDIACODECJLUGIN) \
		$(MY_INC_PATH_COMMONIN)		\
		$(LOCAL_PATH)/../info/inc  \
		$(LOCAL_PATH)/../../osal/inc  \

LOCAL_CFLAGS := -DNDEBUG

LOCAL_LDLIBS	:= -llog 

LOCAL_SHARED_LIBRARIES := mediaplayer osal

include $(BUILD_SHARED_LIBRARY)