CXX ?= g++
# CC set to export it for other Makefiles
CC ?= gcc
DESTDIR ?= /
BINDIR ?= $(DESTDIR)/usr/bin
PREFIX ?= $(DESTDIR)/usr/local
NO_OPTI ?= false
NO_STACK_PROTECTOR ?= false
export CC
export CXX


ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint.exe
else
OUTPUTBIN = cpoint
endif

CXXFLAGS = -c -g -Wall -Wno-sign-compare

# CXXFLAGS += -MMD

# change it when it is changed with the llvm version
WINDOWS_CXXFLAGS = -std=c++17 -fno-exceptions -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS

ifeq ($(OS),Windows_NT)
CXXFLAGS += $(WINDOWS_CXXFLAGS)
else
ifneq (,$(findstring mingw,$(CXX)))
CXXFLAGS += $(WINDOWS_CXXFLAGS)
else
CXXFLAGS += $(shell llvm-config --cxxflags)
endif
endif

ifeq ($(NO_OPTI),true)
CXXFLAGS += -O0
else
CXXFLAGS += -O2
endif

ifeq ($(NO_STACK_PROTECTOR),true)
CXXFLAGS += -fno-stack-protector
endif

ifneq ($(PREFIX),$(DESTDIR)/usr/local)
CXXFLAGS += -DDEFAULT_PREFIX_PATH=\"$(PREFIX)\"
endif

ifeq ($(OS), Windows_NT)
LDFLAGS = -L/usr/x86_64-w64-mingw32/lib/lib -lLLVM-16 -lstdc++ -lintl
else
ifneq (,$(findstring mingw,$(CXX)))
LDFLAGS = -L/usr/x86_64-w64-mingw32/lib/ -lLLVM-16 -lstdc++ -lintl
else
LDFLAGS = $(shell llvm-config --ldflags --system-libs --libs core)
endif
endif

ifneq ($(OS), Windows_NT)
ifneq ($(shell echo | $(CXX) -dM -E - | grep clang),"")
CXXFLAGS += -frtti
endif
UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
# is MacOS
LDFLAGS += -lintl
endif
endif


SRCDIR=src

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SRCS))

#DEPENDS := $(patsubst %.cpp,%.d,$(SRCS))


all: std_lib cpoint-build cpoint-bindgen cpoint-run cpoint-from-c

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

#-include $(OBJS:%.o=%.d)

$(SRCDIR)/%.o:$(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean-build:
	rm -f ./src/*.o ./src/*.temp


ifneq ($(OS),Windows_NT)
USERNAME=$(shell logname)
endif

install: all
	cp cpoint $(BINDIR)/
	cp build/cpoint-build $(BINDIR)/
	cp tools/bindgen/cpoint-bindgen $(BINDIR)/
	cp tools/run/cpoint-run $(BINDIR)/
	cp tools/from-c/cpoint-from-c $(BINDIR)/
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
	./$(OUTPUTBIN) -std ./std tests/test2.cpoint
#	./$(OUTPUTBIN) -std ./std tests/test2.cpoint -no-gc

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
#	make -C std test
#	make -C std run-test
	./build/cpoint-build -C std test

all-tests: test std-test

onefetch:
	onefetch --exclude bdwgc/* bdwgc_prefix/* tools/vscode/* tools/vim/*