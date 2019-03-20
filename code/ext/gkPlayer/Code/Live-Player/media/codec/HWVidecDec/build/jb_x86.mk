# 
# This configure file is just for Linux projects against Android
#

# special macro definitions for building 
TTPREDEF:=-DLINUX -D_LINUX 

TTARM ?= x86
 
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


TCROOTPATH:=/opt/jb_atom_x86_4.3
TCPATH:=$(TCROOTPATH)/prebuilts/gcc/linux-x86/x86/i686-linux-android-4.7
CCTPRE:=$(TCPATH)/bin/i686-linux-android-
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
TTTP:=generic_x86
CCTLIB:=$(TCROOTPATH)/out/target/product/$(TTTP)/obj/lib
CCTINC:= -I$(TCROOTPATH)/out/target/product/$(TTTP)/obj/include \
	-I$(TCROOTPATH)/system/core/include \
	-I$(TCROOTPATH)/hardware/libhardware/include \
	-I$(TCROOTPATH)/hardware/ril/include \
	-I$(TCROOTPATH)/hardware/libhardware_legacy/include \
	-I$(TCROOTPATH)/libnativehelper/include \
	-I$(TCROOTPATH)/libnativehelper/include/nativehelper \
	-I$(TCROOTPATH)/frameworks/native/include \
	-I$(TCROOTPATH)/frameworks/native/include/media/openmax \
	-I$(TCROOTPATH)/frameworks/native/include/media/hardware \
	-I$(TCROOTPATH)/frameworks/native/opengl/include \
	-I$(TCROOTPATH)/frameworks/av/include \
	-I$(TCROOTPATH)/frameworks/base/include \
	-I$(TCROOTPATH)/frameworks/base/core/jni \
	-I$(TCROOTPATH)/frameworks/av/services/audioflinger \
	-I$(TCROOTPATH)/external/skia/include \
	-I$(TCROOTPATH)/external/openssl/include \
	-I$(TCROOTPATH)/bionic/libc/arch-x86/include \
	-I$(TCROOTPATH)/bionic/libc/include \
	-I$(TCROOTPATH)/bionic/libstdc++/include \
	-I$(TCROOTPATH)/bionic/libc/kernel/common \
	-I$(TCROOTPATH)/bionic/libc/kernel/arch-x86 \
	-I$(TCROOTPATH)/bionic/libm/include \
	-I$(TCROOTPATH)/bionic/libm/include/i387 \
	-I$(TCROOTPATH)/bionic/libthread_db/include \
	-I$(TCROOTPATH)/bionic/libm/i386 \
	-I$(TCROOTPATH)/bionic/libm \
	-I$(TCROOTPATH)/frameworks/base/include/android_runtime \
	-I$(TCROOTPATH)/external/webkit/Source/WebCore/bridge \
	-I$(TCROOTPATH)/external/webkit/Source/WebCore/plugins \
	-I$(TCROOTPATH)/external/webkit/Source/WebCore/platform/android \
	-I$(TCROOTPATH)/external/webkit/Source/WebKit/android/plugins

CCTCFLAGS:=-O2 -Ulinux -Wa,--noexecstack -Wstrict-aliasing=2 -fPIC -ffunction-sections -finline-functions -finline-limit=300 -fno-inline-functions-called-once -fno-short-enums -fstrict-aliasing -funswitch-loops -funwind-tables -march=atom -mstackrealign -DUSE_SSSE3 -DUSE_SSE2 -mfpmath=sse -mbionic -D__ANDROID__ -DANDROID -fmessage-length=0 -W -Wall -Wno-unused -Winit-self -Wpointer-arith -O2 -g -fno-strict-aliasing -DNDEBUG -UDEBUG -fno-use-cxa-atexit -DANDROID -fmessage-length=0 -W -Wall -Wno-unused -Winit-self -Wpointer-arith -Wsign-promo -fno-rtti -DANDROID_TARGET_BUILD -finline-limit=64 -finline-functions -fno-inline-functions-called-once -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -O2 -fomit-frame-pointer -Wall -W -Wno-unused-parameter -Wwrite-strings  -fno-exceptions   -fno-rtti -Woverloaded-virtual -Wno-sign-promo

TTLDFLAGS:=-m32 -Wl,-z,noexecstack -Wl,--gc-sections -nostdlib -Wl,-soname,libmtp.so -shared -Bsymbolic -fno-exceptions -Wno-multichar -O2 -Ulinux -Wa,--noexecstack -Werror=format-security -Wstrict-aliasing=2 -fPIC -ffunction-sections -finline-functions -finline-limit=300 -fno-inline-functions-called-once -fno-short-enums -fstrict-aliasing -funswitch-loops -funwind-tables -include $(TCROOTPATH)/build/core/combo/include/arch/target_linux-x86/AndroidConfig.h -march=atom -mstackrealign -DUSE_SSSE3 -DUSE_SSE2 -mfpmath=sse -mbionic -D__ANDROID__ -DANDROID -fmessage-length=0 -W -Wall -Wno-unused -Winit-self -Wpointer-arith -Werror=return-type -Werror=non-virtual-dtor -Werror=address -Werror=sequence-point -O2 -g -fno-strict-aliasing -DNDEBUG -UDEBUG -Wl,--whole-archive   -Wl,--no-whole-archive 

LOCAL_PRELINK_MODULE := false
LIBGCCA:=$(TCROOTPATH)/prebuilts/gcc/linux-x86/x86/i686-linux-android-4.7/lib/gcc/i686-linux-android/4.7/libgcc.a

# for target exe
TELDFLAGS:=-m32 -Wl,-z,noexecstack -Wl,--gc-sections -nostdlib -Bdynamic -Wl,-dynamic-linker,/system/bin/linker -Wl,-z,nocopyreloc -Wl,-v

TTTEDEPS:=$(TCROOTPATH)/out/target/product/$(TTTP)/obj/lib/crtbegin_dynamic.o $(LIBGCCA) $(TCROOTPATH)/out/target/product/$(TTTP)/obj/lib/crtend_android.o -lc -lm


# for target lib
CCTCRTBEGIN:=$(TCROOTPATH)/out/target/product/$(TTTP)/obj/lib/crtbegin_so.o

TTTLDEPS:=-L$(CCTLIB) -Wl,--no-whole-archive -Wl,--no-undefined $(LIBGCCA) $(TCROOTPATH)/out/target/product/$(TTTP)/obj/lib/crtend_so.o -lm -lc

ifeq ($(TTARM), x86)
TTCFLAGS:=-march=i686 -mbionic
TTASFLAGS:=-march=i686 -mbionic
endif

#global compiling options for ARM target
ifneq ($(TTARM), pc)
TTASFLAGS+=--strip-local-absolute -R
endif 

TTCFLAGS+=-include $(TCROOTPATH)/build/core/combo/include/arch/target_linux-x86/AndroidConfig.h

ifeq ($(TTDBG), yes)
ifeq ($(TTOPTIM),unknown)
TTCFLAGS+=-D_DEBUG -g -D_LINUX_ANDROID -DDEBUG
else
ifeq ($(TTOPTIM),debug)
TTCFLAGS+=-DNDEBUG -O3 -D_LINUX_ANDROID -D_LINUX_ANDROID
endif
endif
OBJDIR:=debug
else
TTCFLAGS+=-DNDEBUG -O3 -D_LINUX_ANDROID
OBJDIR:=release
endif

TTCFLAGS+=$(TTPREDEF) $(TTMM)

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


