LOCAL_PATH := $(call my-dir)

MY_INC_PATH_CODEC:= $(LOCAL_PATH)/../../include
MY_SRC_PATH_VIDEODEC:= $(LOCAL_PATH)/../src
MY_SRC_PUB_VIDEODEC:= $(LOCAL_PATH)/../

ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_ARM_MODE := thumb
LOCAL_SRC_FILES :=  \
				$(MY_SRC_PATH_VIDEODEC)/ttAvpacket.c 			\
				$(MY_SRC_PATH_VIDEODEC)/ttBitstream.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttCabac.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttErrorResilience.c  	\
				$(MY_SRC_PATH_VIDEODEC)/ttGolomb.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Cabac.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Cavlc.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264chroma.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Direct.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264dsp.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264idct.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Loopfilter.c		\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Mb.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Picture.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264pred.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Ps.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264qpel.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Refs.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Sei.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Slice.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttMathTables.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttOptions.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttPthreadFrame.c		\
				$(MY_SRC_PATH_VIDEODEC)/ttPthread.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttPthreadSlice.c		\
				$(MY_SRC_PATH_VIDEODEC)/ttUtils.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttVideodsp.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttAtomic.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttBuffer.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttCpu.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttDict.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttDisplay.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttFrame.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttImgutils.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttIntmath.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttMathematics.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttMem.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttOpt.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttPixdesc.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttRational.c			\
				$(MY_SRC_PATH_VIDEODEC)/arm/cpu.c				\
				$(MY_SRC_PUB_VIDEODEC)/ttVideoDec.c

else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_MODE := arm				
LOCAL_SRC_FILES :=  \
				$(MY_SRC_PATH_VIDEODEC)/ttAvpacket.c 			\
				$(MY_SRC_PATH_VIDEODEC)/ttBitstream.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttCabac.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttErrorResilience.c  	\
				$(MY_SRC_PATH_VIDEODEC)/ttGolomb.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Cabac.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Cavlc.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264chroma.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Direct.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264dsp.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264idct.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Loopfilter.c		\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Mb.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Picture.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264pred.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Ps.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264qpel.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Refs.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Sei.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Slice.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttMathTables.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttOptions.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttPthreadFrame.c		\
				$(MY_SRC_PATH_VIDEODEC)/ttPthread.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttPthreadSlice.c		\
				$(MY_SRC_PATH_VIDEODEC)/ttUtils.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttVideodsp.c				\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264chromaInitArm.c	\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264cmcNeon.S			\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264dspInitArm.c		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264dspNeon.S			\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264idctNeon.S		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264predInitArm.c		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264predNeon.S		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264qpelInitArm.c		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264qpelNeon.S		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttHpeldspNeon.S			\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttVideodspArmv5te.S		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttVideodspInitArm.c		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttVideodspInitArmv5te.c	\
				$(MY_SRC_PATH_VIDEODEC)/ttAtomic.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttBuffer.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttCpu.c						\
				$(MY_SRC_PATH_VIDEODEC)/ttDict.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttDisplay.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttFrame.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttImgutils.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttIntmath.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttMathematics.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttMem.c						\
				$(MY_SRC_PATH_VIDEODEC)/ttOpt.c						\
				$(MY_SRC_PATH_VIDEODEC)/ttPixdesc.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttRational.c				\
				$(MY_SRC_PATH_VIDEODEC)/arm/cpu.c					\
				$(MY_SRC_PUB_VIDEODEC)/ttVideoDec.c

else ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_ARM_MODE := arm	
LOCAL_SRC_FILES :=  \
				$(MY_SRC_PATH_VIDEODEC)/ttAvpacket.c 			\
				$(MY_SRC_PATH_VIDEODEC)/ttBitstream.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttCabac.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttErrorResilience.c  	\
				$(MY_SRC_PATH_VIDEODEC)/ttGolomb.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Cabac.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Cavlc.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264chroma.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Direct.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264dsp.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264idct.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Loopfilter.c		\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Mb.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Picture.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264pred.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Ps.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264qpel.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Refs.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Sei.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttH264Slice.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttMathTables.c			\
				$(MY_SRC_PATH_VIDEODEC)/ttOptions.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttPthreadFrame.c		\
				$(MY_SRC_PATH_VIDEODEC)/ttPthread.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttPthreadSlice.c		\
				$(MY_SRC_PATH_VIDEODEC)/ttUtils.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttVideodsp.c			\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264chromaInitArm.c	\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264dspInitArm.c		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264predInitArm.c		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttH264qpelInitArm.c		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttVideodspArmv5te.S		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttVideodspInitArm.c		\
				$(MY_SRC_PATH_VIDEODEC)/arm/ttVideodspInitArmv5te.c	\
				$(MY_SRC_PATH_VIDEODEC)/ttAtomic.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttBuffer.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttCpu.c						\
				$(MY_SRC_PATH_VIDEODEC)/ttDict.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttDisplay.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttFrame.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttImgutils.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttIntmath.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttMathematics.c				\
				$(MY_SRC_PATH_VIDEODEC)/ttMem.c						\
				$(MY_SRC_PATH_VIDEODEC)/ttOpt.c						\
				$(MY_SRC_PATH_VIDEODEC)/ttPixdesc.c					\
				$(MY_SRC_PATH_VIDEODEC)/ttRational.c				\
				$(MY_SRC_PATH_VIDEODEC)/arm/cpu.c					\
				$(MY_SRC_PUB_VIDEODEC)/ttVideoDec.c 				\		
endif

LOCAL_C_INCLUDES :=  \
				$(LOCAL_PATH)/../../include \
				$(LOCAL_PATH)/../inc  		\
				$(LOCAL_PATH)/../ 			\
				$(LOCAL_PATH)/../src 		\
				$(LOCAL_PATH)/../src/arm 	\
				
		  