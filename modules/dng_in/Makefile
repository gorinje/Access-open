include ../../config.mak

vpath %.c $(SRC_PATH)/modules/img_in

CFLAGS= $(OPTFLAGS) -I"$(SRC_PATH)/include"

ifeq ($(DEBUGBUILD), yes)
CFLAGS+=-g
LDFLAGS+=-g
endif

ifeq ($(GPROFBUILD), yes)
CFLAGS+=-pg
LDFLAGS+=-pg
endif

NEED_LOCAL_LIB="no"
LOCAL_LIB=../../bin/gcc
ifneq ($(STATICBUILD),yes)
LINKLIBS= -lgpac
else
LINKLIBS= -lgpac_static
endif

#common objects
OBJS=raw_dec.o

#libraw config
ifeq ($(CONFIG_RAW), no)
else
OBJS+=dng_in.o
LINKLIBS+= -lraw
#local openjpeg lib
ifeq ($(CONFIG_RAW), local)
NEED_LOCAL_LIB="yes"
CFLAGS+= -I"$(LOCAL_INC_PATH)/libraw"
endif
endif

#add local lib path
ifeq ($(NEED_LOCAL_LIB), "yes")
LOCAL_LIB+=-L../../extra_lib/lib/gcc
endif



SRCS := $(OBJS:.o=.c) 

LIB=gm_dng_in$(DYN_LIB_SUFFIX)
ifeq ($(CONFIG_WIN32),yes)
LINKLIBS+=-lwinmm
#LDFLAGS+=-export-symbols img_in.def 
endif

all: $(LIB)


$(LIB): $(OBJS)
	$(CC) $(SHFLAGS) $(LDFLAGS) -o ../../bin/gcc/$@ $(OBJS) $(EXTRALIBS) -L$(LOCAL_LIB) $(LINKLIBS)
ifeq ($(STATICBUILD),yes)
	$(CC) $(SHFLAGS) $(LDFLAGS) -o ../../bin/gcc/gm_img_in-static$(DYN_LIB_SUFFIX) $(OBJS) $(EXTRALIBS) -L$(LOCAL_LIB) $(LINKLIBS)
endif

clean: 
	rm -f $(OBJS) ../../bin/gcc/$(LIB)

dep: depend

depend:
	rm -f .depend	
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

distclean: clean
	rm -f Makefile.bak .depend

-include .depend
