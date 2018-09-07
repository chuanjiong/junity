##################################################################
#
# general makefile for c/c++ project
#
# @chuanjiong
#
##################################################################

#-----------------------------------------------------------------
# c c++ linker
#-----------------------------------------------------------------

SRCDIRS  += ./jlib

CFLAGS   += -I./jlib -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
CXXFLAGS += -I./jlib -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE

LDFLAGS  += -lpthread

ifeq ($(MINGW32), 1)
LDFLAGS  += -lws2_32
endif


