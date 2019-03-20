LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

MY_SRC_PATH_VIDEODEC:= src/ttCodec
MY_SRC_PUB_VIDEODEC:= src/public

LOCAL_SRC_FILES :=  \
$(MY_SRC_PATH_VIDEODEC)/ttAvpacket.c 			\
$(MY_SRC_PATH_VIDEODEC)/ttBitstream.c			\
$(MY_SRC_PATH_VIDEODEC)/ttCabac.c					\
$(MY_SRC_PATH_VIDEODEC)/ttErrorResilience.c  	\
$(MY_SRC_PATH_VIDEODEC)/ttGolomb.c					\
$(MY_SRC_PATH_VIDEODEC)/ttH264Cabac.c			\
$(MY_SRC_PATH_VIDEODEC)/ttH264Cavlc.c			\
$(MY_SRC_PATH_VIDEODEC)/ttH264chroma.c			\
$(MY_SRC_PATH_VIDEODEC)/ttH264Direct.c		\
$(MY_SRC_PATH_VIDEODEC)/ttH264dsp.c				\
$(MY_SRC_PATH_VIDEODEC)/ttH264idct.c				\
$(MY_SRC_PATH_VIDEODEC)/ttH264Loopfilter.c	\
$(MY_SRC_PATH_VIDEODEC)/ttH264Mb.c					\
$(MY_SRC_PATH_VIDEODEC)/ttH264.c							\
$(MY_SRC_PATH_VIDEODEC)/ttH264Picture.c			\
$(MY_SRC_PATH_VIDEODEC)/ttH264pred.c					\
$(MY_SRC_PATH_VIDEODEC)/ttH264Ps.c					\
$(MY_SRC_PATH_VIDEODEC)/ttH264qpel.c					\
$(MY_SRC_PATH_VIDEODEC)/ttH264Refs.c				\
$(MY_SRC_PATH_VIDEODEC)/ttH264Sei.c					\
$(MY_SRC_PATH_VIDEODEC)/ttH264Slice.c				\
$(MY_SRC_PATH_VIDEODEC)/ttMathTables.c				\
$(MY_SRC_PATH_VIDEODEC)/ttOptions.c					\
$(MY_SRC_PATH_VIDEODEC)/ttPthreadFrame.c		\
$(MY_SRC_PATH_VIDEODEC)/ttPthread.c					\
$(MY_SRC_PATH_VIDEODEC)/ttPthreadSlice.c		\
$(MY_SRC_PATH_VIDEODEC)/ttUtils.c						\
$(MY_SRC_PATH_VIDEODEC)/ttVideodsp.c					\
$(MY_SRC_PATH_VIDEODEC)/ttAtomic.c			\
$(MY_SRC_PATH_VIDEODEC)/ttBuffer.c			\
$(MY_SRC_PATH_VIDEODEC)/ttCpu.c				\
$(MY_SRC_PATH_VIDEODEC)/ttDict.c				\
$(MY_SRC_PATH_VIDEODEC)/ttDisplay.c		\
$(MY_SRC_PATH_VIDEODEC)/ttFrame.c			\
$(MY_SRC_PATH_VIDEODEC)/ttImgutils.c		\
$(MY_SRC_PATH_VIDEODEC)/ttIntmath.c		\
$(MY_SRC_PATH_VIDEODEC)/ttMathematics.c	\
$(MY_SRC_PATH_VIDEODEC)/ttMem.c					\
$(MY_SRC_PATH_VIDEODEC)/ttOpt.c					\
$(MY_SRC_PATH_VIDEODEC)/ttPixdesc.c			\
$(MY_SRC_PATH_VIDEODEC)/ttRational.c			\
$(MY_SRC_PATH_VIDEODEC)/arm/cpu.c									\
$(MY_SRC_PUB_VIDEODEC)/ttVideoDec.c \

LOCAL_C_INCLUDES :=  \
		$(LOCAL_PATH)/src \
		$(LOCAL_PATH)/src/ttCodec/arm \
		$(LOCAL_PATH)/src/ttCodec	\
		$(LOCAL_PATH)/src/public \
		$(LOCAL_PATH)/../../common/inc  \
		$(LOCAL_PATH)/../../info/inc  \
		$(LOCAL_PATH)/../../../osal/inc  \
  
LOCAL_MODULE 	:= H264Dec

LOCAL_CFLAGS := -DNDEBUG -DOPT_X86

include $(BUILD_SHARED_LIBRARY)