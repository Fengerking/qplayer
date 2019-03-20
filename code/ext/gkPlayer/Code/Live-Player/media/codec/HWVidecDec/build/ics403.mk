# 
# This configure file is just for Linux projects against Android
#

# special macro definitions for building 
TTPREDEF:=-DLINUX -D_LINUX 
TTARM ?= v6
 
# for debug or not: yes for debug, any other for release
TTDBG?=ye
TTOPTIM?=unknown
ifeq ($(TTOPTIM),debug)
TTDBG:=yes
else
ifeq ($(TTOPTIM),release)
TTDBG:=no
endif
endif

TCROOTPATH:=/opt/ics-4.0.4
GCCVER:=4.6
TCPATH:=$(TCROOTPATH)/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6
CCTPRE:=$(TCPATH)/bin/arm-linux-androideabi-
AS:=$(CCTPRE)as
AR:=$(CCTPRE)ar
NM:=$(CCTPRE)nm
CC:=$(CCTPRE)gcc
GG:=$(CCTPRE)g++
LD:=$(CCTPRE)ld
SIZE:=$(CCTPRE)size
STRIP:=$(CCTPRE)strip
RANLIB:=$(CCTPRE)ranlib
OBJCOPY:=$(CCTPRE)objcopy
OBJDUMP:=$(CCTPRE)objdump
READELF:=$(CCTPRE)readelf
STRINGS:=$(CCTPRE)strings

# target product dependcy
# available: dream, generic
TTTP:=generic
CCTLIB:=$(TCROOTPATH)/out/target/product/$(TTTP)/obj/lib
CCTINC:=-I$(TCROOTPATH)/system/core/include \
	-I$(TCROOTPATH)/system/core/include/arch/linux-arm \
	-I$(TCROOTPATH)/hardware/libhardware/include \
	-I$(TCROOTPATH)/hardware/ril/include \
	-I$(TCROOTPATH)/hardware/libhardware_legacy/include \
	-I$(TCROOTPATH)/dalvik/libnativehelper/include \
	-I$(TCROOTPATH)/dalvik/libnativehelper/include/nativehelper \
	-I$(TCROOTPATH)/frameworks/base/include \
	-I$(TCROOTPATH)/frameworks/base/include/media/stagefright/openmax \
	-I$(TCROOTPATH)/frameworks/base/include/media/stagefright \
	-I$(TCROOTPATH)/frameworks/base/include/media/ \
	-I$(TCROOTPATH)/frameworks/base/opengl/include \
	-I$(TCROOTPATH)/frameworks/base/native/include \
	-I$(TCROOTPATH)/frameworks/base/include \
	-I$(TCROOTPATH)/frameworks/base/core/jni \
	-I$(TCROOTPATH)/external/skia/include \
	-I$(TCROOTPATH)/external/openssl/include \
	-I$(TCROOTPATH)/out/target/product/$(TTTP)/obj/include \
	-I$(TCROOTPATH)/bionic/libc/arch-arm/include \
	-I$(TCROOTPATH)/bionic/libc/include \
	-I$(TCROOTPATH)/bionic/libstdc++/include \
	-I$(TCROOTPATH)/bionic/libc/kernel/common \
	-I$(TCROOTPATH)/bionic/libc/kernel/arch-arm \
	-I$(TCROOTPATH)/bionic/libm/include \
	-I$(TCROOTPATH)/bionic/libm/include/arm \
	-I$(TCROOTPATH)/bionic/libthread_db/include \
	-I$(TCROOTPATH)/bionic/libm/arm \
	-I$(TCROOTPATH)/bionic/libm \
	-I$(TCROOTPATH)/frameworks/base/include/android_runtime \
	-I$(TCROOTPATH)/external/webkit/Source/WebCore/bridge \
	-I$(TCROOTPATH)/external/webkit/Source/WebCore/plugins \
	-I$(TCROOTPATH)/external/webkit/Source/WebCore/platform/android \
	-I$(TCROOTPATH)/external/webkit/Source/WebKit/android/plugins

CCTCFLAGS:=-fno-exceptions -Wno-multichar -msoft-float -fpic -ffunction-sections -fdata-sections -funwind-tables -fstack-protector -Wa,--noexecstack -fno-short-enums -Wno-unused-but-set-variable -fno-builtin-sin -fno-strict-volatile-bitfields -Wno-psabi -Wstrict-aliasing=2 -fgcse-after-reload -frerun-cse-after-loop -frename-registers -DANDROID -fmessage-length=0 -W -Wall -Wno-unused -Winit-self -Wpointer-arith -Werror=return-type -Werror=non-virtual-dtor -Werror=address -Werror=sequence-point -fomit-frame-pointer -fno-strict-aliasing

LIBGCCA:=$(TCROOTPATH)/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6/lib/gcc/arm-linux-androideabi/4.6.x-google/libgcc.a

# for target exe
TELDFLAGS:=-nostdlib -Bdynamic -Wl,-dynamic-linker,/system/bin/linker -Wl,--gc-sections -Wl,-z,nocopyreloc -Wl,--no-undefined -Wl,-rpath-link=$(CCTLIB) -L$(CCTLIB) -m32

TTTEDEPS:=$(TCROOTPATH)/out/target/product/$(TTTP)/obj/lib/crtbegin_dynamic.o $(LIBGCCA) $(TCROOTPATH)/out/target/product/$(TTTP)/obj/lib/crtend_android.o -lc -lm


# for target lib
CCTCRTBEGIN:=$(TCROOTPATH)/out/target/product/$(TTTP)/obj/lib/crtbegin_so.o
TLLDFLAGS:=-nostdlib -Wl,-T,$(TCROOTPATH)/build/core/armelf.xsc -Wl,--gc-sections -Wl,-shared,-Bsymbolic -m32

TTTLDEPS:=-L$(CCTLIB) -Wl,--no-whole-archive -Wl,--no-undefined $(LIBGCCA) $(TCROOTPATH)/out/target/product/$(TTTP)/obj/lib/crtend_so.o -lm -lc


ifeq ($(TTARM), v4)
TTCFLAGS:=-mtune=arm9tdmi -march=armv4t
TTASFLAGS:=-march=armv4t -mfpu=softfpa
endif

ifeq ($(TTARM), v5)
TTCFLAGS:=-march=armv5te
TTASFLAGS:=-march=armv5te -mfpu=vfp
endif

ifeq ($(TTARM), v5x)
TTCFLAGS:=-march=iwmmxt2 -mtune=iwmmxt2 
TTASFLAGS:=-march=iwmmxt2 -mfpu=vfp
endif

ifeq ($(TTARM), v6)
#TTCFLAGS:=-march=armv6 -mtune=arm1136jf-s 
#TTASFLAGS:=-march=armv6
TTCFLAGS:=-march=armv6j -mtune=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp -mapcs -mlong-calls
TTASFLAGS:=-march=armv6j -mcpu=arm1136jf-s -mfpu=arm1136jf-s -mfloat-abi=softfp -mapcs-float -mapcs-reentrant
endif

#
# global link options
TTLDFLAGS:=-Wl,-x,-X,--as-needed


ifeq ($(TTARM), v7)
TTCFLAGS+=-march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
TTASFLAGS+=-march=armv7-a -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp
TTLDFLAGS+=-Wl,-z,noexecstack -Wl,--icf=safe -Wl,--fix-cortex-a8
endif

#global compiling options for ARM target
ifneq ($(TTARM), pc)
TTASFLAGS+=--strip-local-absolute -R
endif 

TTCFLAGS+=-include $(TCROOTPATH)/system/core/include/arch/linux-arm/AndroidConfig.h

ifeq ($(TTDBG), yes)
ifeq ($(TTOPTIM),unknown)
TTCFLAGS+=-D_DEBUG -g -D_LINUX_ANDROID
else
ifeq ($(TTOPTIM),debug)
TTCFLAGS+=-DNDEBUG -O3 -D_LINUX_ANDROID -D_LINUX_ANDROID
endif
endif
OBJDIR:=debug
else
TTCFLAGS+=-DNDEBUG -O3 -D_LINUX_ANDROID -D__ARM_EABI__
OBJDIR:=release
endif

TTCFLAGS+=$(TTPREDEF) $(TTMM) -Wall -fsigned-char -fomit-frame-pointer -fno-leading-underscore -fpic -fPIC -pipe -ftracer -fforce-addr -fno-bounds-check   #-ftree-loop-linear -mthumb -nostdinc -dD -fprefetch-loop-arrays


ifneq ($(TTARM), pc)
TTCFLAGS+=$(CCTCFLAGS) $(CCTINC)
TTCPPFLAGS:=-fno-rtti $(TTCFLAGS)

ifeq ($(TTMT), exe)
TTLDFLAGS+=$(TELDFLAGS)
endif

ifeq ($(TTMT), lib)
TTLDFLAGS+=$(TLLDFLAGS)
endif
else
TTCPPFLAGS:=$(TTCFLAGS)
ifeq ($(TTMT), lib)
TTLDFLAGS+=-shared
endif
endif

# where to place object files 
