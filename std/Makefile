TARGET ?= $(shell gcc -dumpmachine)

ifneq (,$(findstring android,$(TARGET)))
# have if which works in termux
ifneq (,$(wildcard ../cpoint))
    CCPOINT=../cpoint
else
    CCPOINT=cpoint
endif

else
ifeq ($(OS), Windows_NT)
#$(warning $(shell Test-Path -Path ..\cpoint.exe -PathType Leaf))
#ifeq ($(shell Test-Path -Path ..\cpoint.exe -PathType Leaf),True)
	CCPOINT=../cpoint.exe
#else
#	CCPOINT=cpoint.exe
#endif

else

ifeq ($(shell test ! -f ../cpoint || echo 'yes'),yes)
    CCPOINT=../cpoint
else
    CCPOINT=cpoint
endif

endif

endif

SRC := $(wildcard *.cpoint) $(wildcard */*.cpoint)
OBJECTS := $(patsubst %.cpoint, %.o, $(SRC))
SRC_C := $(wildcard c_api/*.c)
OBJECTS_C := $(patsubst c_api/%.c, c_api/%.o, $(SRC_C))
CPOINTFLAGS=-c -nostd -target-triplet $(TARGET)
CPOINTFLAGS += -with-gc

all: libstd.a

debug: CPOINTFLAGS += -g
debug: MAKETARGET = debug
debug: libstd.a

libstd.a: $(OBJECTS) capi
	llvm-ar -rc $@ $(OBJECTS) $(OBJECTS_C)

capi:
	+TARGET=$(TARGET) make $(MAKETARGET) -C c_api

%.o: %.cpoint
	$(CCPOINT) $(CPOINTFLAGS) $< -o $@

clean:
	make -C c_api clean
	rm -f ./*.o ./*.a ./*.ll ./*.log ./*.temp