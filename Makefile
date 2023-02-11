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

ifneq ($(OS), Windows_NT)
ifneq ($(shell echo | $(CXX) -dM -E - | grep clang),"")
CXXFLAGS += -frtti
endif
endif

LDFLAGS = $(shell llvm-config --ldflags --system-libs --libs core)
SRCDIR=src

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SRCS))


all: std_lib_plus_compiler_plus_gc

std_lib_plus_compiler_plus_gc: std_lib


gc:
#	cd bdwgc && ./autogen.sh && ./configure --enable-cplusplus
ifneq ($(OS),Windows_NT)
	mkdir -p $(shell pwd)/bdwgc_prefix
ifneq ($(shell test ! -f bdwgc/Makefile || echo 'yes'),yes)	
	cd bdwgc && ./autogen.sh && ./configure --prefix=$(shell pwd)/bdwgc_prefix --disable-threads  --enable-static
endif
else
	rmdir bdwgc_prefix
	mkdir bdwgc_prefix
	cd bdwgc && ./autogen.sh && ./configure --prefix=$(shell pwd)/bdwgc_prefix --disable-threads  --enable-static
endif
	+make -C bdwgc
	make -C bdwgc install

std_lib: gc $(OUTPUTBIN)
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

ifneq ($(OS),Windows_NT)
USERNAME=$(shell logname)
endif

install:
	cp cpoint $(DESTDIR)/
	rm -rf $(PREFIX)/lib/cpoint
	mkdir $(PREFIX)/lib/cpoint
	mkdir $(PREFIX)/lib/cpoint/std
	mkdir $(PREFIX)/lib/cpoint/packages
	mkdir $(PREFIX)/lib/cpoint/bdwgc
	mkdir $(PREFIX)/lib/cpoint/bdwgc_prefix
	cp -r std/* $(PREFIX)/lib/cpoint/std
	cp -r bdwgc/* $(PREFIX)/lib/cpoint/bdwgc
	chmod a=rwx -R $(PREFIX)/lib/cpoint/
	chown -R $(USERNAME):$(USERNAME) $(PREFIX)/lib/cpoint/
	rm -f $(PREFIX)/lib/cpoint/bdwgc/Makefile



run:
	./cpoint -std ./std tests/test2.cpoint

clean: clean-build
	make -C std/c_api clean
	make -C std clean
	make -C tests clean
ifeq ($(OS),Windows_NT)
	Remove-Item -Recurse -Force -Path cpoint.*
	Remove-Item -Recurse -Force -Path out.o
else
	rm -rf cpoint out.ll out.ll.* cpoint.* a.out out.o
endif
	make -C bdwgc clean
	rm bdwgc/Makefile

test:
	make -C tests python
#	make -C tests run
