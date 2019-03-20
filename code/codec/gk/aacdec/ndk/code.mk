LOCAL_PATH := $(call my-dir)

MY_INC_PATH_CODEC:= $(LOCAL_PATH)/../../include
MY_INC_PATH_AACDEC:= $(LOCAL_PATH)/../inc
MY_SRC_PATH_AACDEC:= $(LOCAL_PATH)/../src
MY_SRC_PATH_AACDEC_ASM_V6:= ../src/linuxasmv6
MY_SRC_PATH_AACDEC_ASM_V7:= ../src/linuxasmv7

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_MODE := arm
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
					
else ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_ARM_MODE := arm
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
					
else ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_ARM_MODE := thumb
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
endif


#$(warning "###-- $(LOCAL_SRC_FILES)"--###)
					
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
					$(MY_INC_PATH_CODEC) \
					$(MY_INC_PATH_AACDEC) \
