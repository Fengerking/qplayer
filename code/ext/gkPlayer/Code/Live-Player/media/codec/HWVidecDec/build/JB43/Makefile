						#################################################################
						#  									Makefile Template  							 	#
						#																																#
						#						(Please Keep Below Options Order Unchanged)					#
						#################################################################

# target type
# available: pc, v4(armv4), v5(armv5), v5x(armv5 xscale), v6(armv6), v7(cortex-a8 neon)
TTARM:=v6
 
# module type
# please specify the type of your module: lib or exe
TTMT:=lib


# module macros
# please append the additional macro definitions here for your module if necessary. 
TTMM:= -D_LINUX_ANDROID -D_TT_JB43_


# please specify the name of your module
TTTARGET:=libHWVideoDec43


# please modify here to be sure to see the eclair.mk
include ../jb4.3.mk 


# dependent libraries.
TTDEPLIBS:= -lbinder -lui -lmedia -lstagefright -ldl -lstdc++ -lutils -lcutils -llog

# module source
# please modify here to be sure to see the ms.mk which specifies all source info of your module
include ../ms_ics.mk


# please modify here to be sure to see the doit.mk
include ../doit.mk

