# this script aims at generating modules 

VERBOSE?=@

TTMT ?= lib

MAKEFILE := $(lastword $(MAKEFILE_LIST))
MKOBJDIR := $(shell	if [ ! -d $(OBJDIR) ]; then mkdir -p $(OBJDIR); fi;)


# where to place object files 
ifeq ($(TTMT), lib)
LIB_STATIC=$(OBJDIR)/$(TTTARGET).a
LIB_DYNAMIC=$(OBJDIR)/$(TTTARGET).so
LIB_SONAME=$(TTTARGET).so
endif

ifeq ($(TTMT), exe)
TARGET=$(OBJDIR)/$(TTTARGET)
endif

CFLAGS+= $(TTCFLAGS) $(addprefix -I, $(TTSRCDIR)) 
CPPFLAGS+= $(TTCPPFLAGS) $(addprefix -I, $(TTSRCDIR)) 
ASFLAGS=$(TTASFLAGS) $(addprefix -I, $(TTSRCDIR)) 

LDFLAGS:=$(TTLDFLAGS)
TTSTCLIBS?=

vpath %.c $(TTSRCDIR)
vpath %.cpp $(TTSRCDIR)
vpath %.s $(TTSRCDIR)
vpath %.S $(TTSRCDIR)


TTOBJS:=$(addprefix $(OBJDIR)/, $(OBJS)) 
OBJS:=$(TTOBJS) 


#TTMIDDEPS:=$($(TTOBJS):.o=.d)
TTMIDDEPS:=$(patsubst %.o,%.d,$(TTOBJS))

ifeq ($(TTMT), lib)
all: $(LIB_STATIC) $(LIB_DYNAMIC)
else
all: $(TARGET)
endif

#.PRECIOUS: $(OBJS)

$(OBJDIR):
	@if test ! -d $@; then \
		mkdir -p $@; \
	fi;

ifeq ($(TTMT), lib)

$(LIB_STATIC):$(OBJS)
	$(AR) cr $@ $^ $(TTSTCLIBS)
	$(RANLIB) $@
$(LIB_DYNAMIC):$(OBJS)
	$(GG) $(LDFLAGS) -Wl,-soname,$(LIB_SONAME) -o $@ $(CCTCRTBEGIN) $^ -Wl,--whole-archive $(TTSTCLIBS) -Wl,--no-whole-archive $(CCRTEXTRAS) $(TTDEPLIBS) $(TTTLDEPS) 

ifneq ($(TTDBG), yes)
		$(STRIP) $@
endif

else

$(TARGET):$(OBJS)
	$(GG) $(LDFLAGS) -o $@ $(CCTTECRTBEGIN) $^ -Wl,--whole-archive $(TTSTCLIBS) -Wl,--no-whole-archive $(CCRTEXTRAS) $(TTDEPLIBS) $(TTTEDEPS) 

ifneq ($(TTDBG), yes)
	$(STRIP) $@
endif

endif

sinclude $(TTMIDDEPS)

.SUFFIXES: .c .cpp .s .S .d .o

#for building .c
#.c.o:
#	$(VERBOSE) $(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.o:%.c
	$(VERBOSE) $(CC) $(CFLAGS) -o $@ -c $<


$(OBJDIR)/%.d:%.c
	$(VERBOSE) $(CC) -MM $(CFLAGS) $< | sed 's,\($*\)\.o,$(OBJDIR)/\1\.o,g' > $@; \
	echo '$(VERBOSE) $(CC) $(CFLAGS) -o $@ -c $<' | sed 's,^$(VERBOSE),\t$(VERBOSE),g' | sed 's,\($*\)\.d,\1\.o,g' >> $@;

# for building .cpp
#.cpp.o:
#	$(VERBOSE) $(GG) $(CPPFLAGS) -o $@ -c $<

$(OBJDIR)/%.o:%.cpp
	$(VERBOSE) $(GG) $(CPPFLAGS) -o $@ -c $<

$(OBJDIR)/%.d:%.cpp
	$(VERBOSE) $(GG) -MM $(CPPFLAGS) $< | sed 's,\($*\)\.o,$(OBJDIR)/\1\.o,g' > $@; \
	echo '$(VERBOSE) $(GG) $(CPPFLAGS) -o $@ -c $<' | sed 's,^$(VERBOSE),\t$(VERBOSE),g' | sed 's,\($*\)\.d,\1\.o,g' >> $@;

# for building assembly
#.s.o:
#	$(VERBOSE) $(AS) $(ASFLAGS) -o $@ $<
$(OBJDIR)/%.o:%.s
	$(VERBOSE) $(CC) $(TTMM) -E $< | $(AS) $(ASFLAGS) -o $@ 
$(OBJDIR)/%.o:%.S
	$(VERBOSE) $(CC) $(TTMM) -E $< | $(AS) $(ASFLAGS) -o $@ 


.PHONY: clean devel
clean:
ifeq ($(TTMT), lib)
	-rm -f $(OBJS) $(TTMIDDEPS);
	@if test -e "$(LIB_STATIC)"; then rm -f $(LIB_STATIC); fi;
	@if test -e "$(LIB_DYNAMIC)"; then rm -f $(LIB_DYNAMIC); fi;
else
	-rm -f $(OBJS) $(TTMIDDEPS) .*.sw* $(TARGET)
endif







