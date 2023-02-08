CXX=g++
DESTDIR ?= /usr/bin
PREFIX ?= /usr/local
NO_OPTI ?= false

ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint.exe
else
OUTPUTBIN = cpoint
endif
CXXFLAGS = -c -g -Wall $(shell llvm-config --cxxflags)
ifeq ($(NO_OPTI),true)
CXXFLAGS += -O0
else
CXXFLAGS += -O2
endif

ifneq ($(shell echo | $(CXX) -dM -E - | grep clang),"")
CXXFLAGS += -frtti
endif

LDFLAGS = $(shell llvm-config --ldflags --system-libs --libs core)
SRCDIR=src

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SRCS))


all: std_lib_plus_compiler_plus_gc

std_lib_plus_compiler_plus_gc: gc


gc: std_lib
	cd bdwgc && ./autogen.sh && ./configure --enable-cplusplus
	+make -C bdwgc

std_lib: $(OUTPUTBIN)
	+make -C std

$(OUTPUTBIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o:$(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

clean-build:
ifeq ($(OS),Windows_NT)
	Remove-Item -Recurse -Force -Path .\src\*.o
	Remove-Item -Recurse -Force -Path .\src\*.temp
else
	rm -f ./src/*.o ./src/*.temp
endif

install:
	cp cpoint $(DESTDIR)/
	rm -rf $(PREFIX)/lib/cpoint
	mkdir $(PREFIX)/lib/cpoint
	mkdir $(PREFIX)/lib/cpoint/std
	mkdir $(PREFIX)/lib/cpoint/packages
	mkdir $(PREFIX)lib/cpoint/gc
	cp -r std/* $(PREFIX)/lib/cpoint/std
	cp -r gc/* $(PREFIX)lib/cpoint/gc
	chmod a=rwx -R $(PREFIX)/lib/cpoint/


run:
	./cpoint -std ./std tests/test2.cpoint

clean: clean-build
	make -C std/c_api clean
	make -C std clean
	make -C tests clean
	rm -rf cpoint out.ll out.ll.* cpoint.* a.out out.o
	make -C bdwgc clean

test:
	make -C tests python
#	make -C tests run
