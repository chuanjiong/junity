##################################################################
#
# general makefile for c/c++ project
#
# @chuanjiong
#
##################################################################

#-----------------------------------------------------------------
# check configuration
#-----------------------------------------------------------------

ifeq ($(SRCDIRS), )
$(error SRCDIRS isn't defined!)
endif

ifeq ($(TARGET), )
$(error TARGET isn't defined!)
endif

#-----------------------------------------------------------------
# common rules
#-----------------------------------------------------------------

SRCEXTS  := .c .cpp
SOURCES  := $(foreach dir, $(SRCDIRS), $(wildcard $(addprefix $(dir)/*, $(SRCEXTS))))
OBJECTS  := $(addsuffix .o, $(basename $(SOURCES)))
DEPENDS  := $(addsuffix .d, $(basename $(SOURCES)))

.PHONY: all clean

all: $(TARGET)


$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(DEPENDS) $(OBJECTS) $(TARGET) $(TARGET).exe

%.d:%.cpp
	$(ECHO) "`dirname $@`\c" > $@
	$(ECHO) "/\c" >> $@
	$(CXX) -MM $(CXXFLAGS) $< >> $@

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.d:%.c
	$(ECHO) "`dirname $@`\c" > $@
	$(ECHO) "/\c" >> $@
	$(CC) -MM $(CFLAGS) $< >> $@

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPENDS)
endif


