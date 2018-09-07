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

SRCDIRS  += ./jpcm

CFLAGS   += -I./jpcm -DOUTSIDE_SPEEX -DRANDOM_PREFIX=speex
CXXFLAGS += -I./jpcm -DOUTSIDE_SPEEX -DRANDOM_PREFIX=speex


