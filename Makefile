CXX=g++
DESTDIR ?= /usr/bin
PREFIX ?= /usr/local
NO_OPTI ?= false
NO_STACK_PROTECTOR ?= false

ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint.exe
else
OUTPUTBIN = cpoint
endif
CXXFLAGS = -c -g -Wall $(shell llvm-config --cxxflags) -Wno-sign-compare
ifeq ($(NO_OPTI),true)
CXXFLAGS += -O0
else
CXXFLAGS += -O2
endif

ifeq ($(NO_STACK_PROTECTOR),true)
CXXFLAGS += -fno-stack-protector
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


all: std_lib_plus_compiler_plus_gc cpoint-build cpoint-bindgen cpoint-run cpoint-from-c

release: set_release all

debug: set_debug all

set_release: 
	$(eval CXXFLAGS += -O3) 
	$(eval LDFLAGS += -s) 
	$(eval MAKETARGET += release)

set_debug:
	$(eval CXXFLAGS += -O0)

cpoint-build:
	+make -C build $(MAKETARGET)

cpoint-bindgen:
	+make -C tools/bindgen $(MAKETARGET)

cpoint-run:
	+make -C tools/run $(MAKETARGET)

cpoint-from-c:
	+make -C tools/from-c $(MAKETARGET)

std_lib_plus_compiler_plus_gc: std_lib


gc:
#	cd bdwgc && ./autogen.sh && ./configure --enable-cplusplus
ifneq ($(OS),Windows_NT)
	mkdir -p $(shell pwd)/bdwgc_prefix
ifneq ($(shell test ! -f bdwgc/Makefile || echo 'yes'),yes)	
	cd bdwgc && ./autogen.sh && ./configure --prefix=$(shell pwd)/bdwgc_prefix --disable-threads  --enable-static
endif
else
#	rm -rf bdwgc_prefix
#	mkdir bdwgc_prefix
	mkdir -p bdwgc_prefix
ifneq ($(shell test ! -f bdwgc/Makefile || echo 'yes'),yes)	
	cd bdwgc && ./autogen.sh && ./configure --prefix=$(shell pwd)/bdwgc_prefix --disable-threads  --enable-static --disable-shared
endif
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
	rm -f ./src/*.o ./src/*.temp


ifneq ($(OS),Windows_NT)
USERNAME=$(shell logname)
endif

install: std_lib
	cp cpoint $(DESTDIR)/
	cp build/cpoint-build $(DESTDIR)/
	cp tools/bindgen/cpoint-bindgen $(DESTDIR)/
	cp tools/run/cpoint-run $(DESTDIR)/
	cp tools/from-c/cpoint-from-c $(DESTDIR)/
	rm -rf $(PREFIX)/lib/cpoint
	mkdir -p $(PREFIX)/lib/cpoint
	mkdir -p $(PREFIX)/lib/cpoint/std
	mkdir -p $(PREFIX)/lib/cpoint/packages
	mkdir -p $(PREFIX)/lib/cpoint/bdwgc
	mkdir -p $(PREFIX)/lib/cpoint/bdwgc_prefix
	mkdir -p $(PREFIX)/lib/cpoint/build_install
	cp -r std/* $(PREFIX)/lib/cpoint/std
	cp -r bdwgc/* $(PREFIX)/lib/cpoint/bdwgc
	cp -r bdwgc_prefix/* $(PREFIX)/lib/cpoint/bdwgc_prefix
	chmod a=rwx -R $(PREFIX)/lib/cpoint/
	chown -R $(USERNAME):$(USERNAME) $(PREFIX)/lib/cpoint/
	rm -f $(PREFIX)/lib/cpoint/bdwgc/Makefile



run:
	./$(OUTPUTBIN) -std ./std tests/test2.cpoint -no-gc

clean: clean-build
	make -C std/c_api clean
	make -C std clean
	make -C tests clean
	make -C build clean
	make -C tools/bindgen clean
	make -C tools/run clean
	rm -rf cpoint out.ll out.ll.* cpoint.* a.out out.o
	make -C bdwgc clean
	rm bdwgc/Makefile

test:
	make -C tests python
#	make -C tests run

std-test:
	make -C std test
	make -C std run-test

onefetch:
	onefetch --exclude bdwgc/* bdwgc_prefix/* build/tomlplusplus/*
