TARGET ?= $(shell gcc -dumpmachine)

DEBUG ?= FALSE

ifneq (,$(findstring android,$(TARGET)))
# have if which works in termux
ifneq (,$(wildcard ../cpoint))
    CCPOINT=../cpoint
else
    CCPOINT=cpoint
endif

else
IS_WINDOWS = false


ifeq ($(OS), Windows_NT)
IS_WINDOWS = true
endif

ifneq (,$(findstring mingw,$(TARGET)))
IS_WINDOWS = true
endif

ifeq ($(IS_WINDOWS), true)
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

#SRC := $(wildcard *.cpoint) $(wildcard */*.cpoint)
SRC := $(wildcard *.cpoint)
OBJECTS := $(patsubst %.cpoint, %.o, $(SRC))
#TESTS_EXE := $(patsubst %.cpoint, %.test, $(SRC))
TESTS_EXE := math.test path.test utils.test alloc.test mem.test ascii.test env.test
TEST_DEPENDENCIES = expect.cpoint print.cpoint panic.cpoint
TEST_DEPENDENCIES_OBJ = $(patsubst %.cpoint, %.o, $(TEST_DEPENDENCIES))
SRC_C := $(wildcard c_api/*.c)
OBJECTS_C := $(patsubst c_api/%.c, c_api/%.o, $(SRC_C))
SRC_CORE := $(wildcard core/*.cpoint)
OBJECTS_CORE := $(patsubst core/%.cpoint, core/%.o, $(SRC_CORE))
CPOINTFLAGS=-c -no-std -target-triplet $(TARGET)
CPOINTFLAGS += -with-gc

ifeq ($(DEBUG),TRUE)
	CPOINTFLAGS += -d
endif

ifeq ($(DEBUGINFOS),TRUE)
	CPOINTFLAGS += -g
endif

ifeq ($(NO_GC),TRUE)
#	CPOINTFLAGS += -DNO_STD
# TODO ?
endif

all: libstd.a

debug: CPOINTFLAGS += -g
debug: MAKETARGET = debug
debug: libstd.a

set_test_flag:
	$(eval CPOINTFLAGS += -test)

libstd.a: $(OBJECTS) capi core_lib
	llvm-ar -rc $@ $(OBJECTS) $(OBJECTS_C) $(OBJECTS_CORE)

capi:
	+TARGET=$(TARGET) NO_GC=$(NO_GC) make $(MAKETARGET) -C c_api

%.o: %.cpoint
	$(CCPOINT) $(CPOINTFLAGS) $< -o $@

clean-objects:
	rm -f ./*.o

clean: clean-objects
	make -C c_api clean
	make -C core clean
	rm -f ./*.a ./*.ll ./*.log ./*.temp ./*.test


%.test: %.o $(TEST_DEPENDENCIES_OBJ)
	clang -o $@ $^ c_api/libc_api.a ../bdwgc_prefix/lib/libgc.a -lm

test: clean-objects capi $(TEST_DEPENDENCIES_OBJ) set_test_flag $(TESTS_EXE)

run-test:
	@for program in $(TESTS_EXE) ; do \
		./$$program || exit 1 ; \
	done


core_lib:
	+TARGET=$(TARGET) NO_GC=$(NO_GC) make $(MAKETARGET) -C core