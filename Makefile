CXX ?= g++
# CC set to export it for other Makefiles
CC ?= gcc
DESTDIR ?= /
BINDIR ?= $(DESTDIR)/usr/bin
PREFIX ?= $(DESTDIR)/usr/local
# TODO : should no_opti be true or false by default ?
NO_OPTI ?= TRUE
NO_STACK_PROTECTOR ?= FALSE
TARGET ?= $(shell $(CC) -dumpmachine)
export CC
export CXX

NO_MOLD ?= FALSE

STATIC_LLVM ?= FALSE

#LLVM_CONFIG ?= llvm-config
LLVM_PREFIX ?= /usr


STATIC_LLVM_DEPENDENCIES ?= FALSE

STATIC_LLVM_DEPENDENCIES_PREFIX ?= $(LLVM_PREFIX)

export LLVM_PREFIX
export STATIC_LLVM

ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint.exe
else
OUTPUTBIN = cpoint
endif

CXXFLAGS =  -MMD -c -g -Wall -Wno-sign-compare -DTARGET="\"$(TARGET)\""

# CXXFLAGS += -MMD

# change it when it is changed with the llvm version
WINDOWS_CXXFLAGS = -std=c++17 -fno-exceptions -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS

#ADDITIONAL_LDFLAGS ?=

LLVM_TOOLS_EMBEDDED_COMPILER = FALSE

ifneq ($(shell $(CC) -dM -E src/config.h | grep ENABLE_LLVM_TOOLS_EMBEDDED_COMPILER),)
LLVM_TOOLS_EMBEDDED_COMPILER = TRUE
endif

PROFILING ?= FALSE

# ifeq ($(OS),Windows_NT)
# CXXFLAGS += $(WINDOWS_CXXFLAGS)
# else
ifneq (,$(findstring mingw,$(CXX)))
CXXFLAGS += $(WINDOWS_CXXFLAGS)
else
CXXFLAGS += $(shell $(LLVM_PREFIX)/bin/llvm-config --cxxflags)
endif
#endif

ifeq ($(NO_OPTI),TRUE)
CXXFLAGS += -O0 -ggdb3
else
CXXFLAGS += -O2
endif

ifeq ($(NO_STACK_PROTECTOR),TRUE)
CXXFLAGS += -fno-stack-protector
endif

ifneq ($(PREFIX),$(DESTDIR)/usr/local)
CXXFLAGS += -DDEFAULT_PREFIX_PATH=\"$(PREFIX)\"
endif

LDFLAGS = $(ADDITIONAL_LDFLAGS)

ifeq ($(STATIC_LLVM_DEPENDENCIES), TRUE)
LDFLAGS += -Wl,-Bstatic
#LDFLAGS += -L$(STATIC_LLVM_DEPENDENCIES_PREFIX) -Wl,-Bstatic -lncursesw 
endif

ifeq ($(OS), Windows_NT)
LDFLAGS += -L/usr/x86_64-w64-mingw32/lib/lib -lLLVM-17 -lstdc++ 
# LDFLAGS += -lintl # for now not needed (TODO ?)
else
ifneq (,$(findstring mingw,$(CXX)))
LDFLAGS += -L/usr/x86_64-w64-mingw32/lib/ -lLLVM-17 -lstdc++ 
#LDFLAGS += -lintl
else

ifeq ($(STATIC_LLVM), TRUE)
CLANG_STATIC_LIBS=$(shell ls $(LLVM_PREFIX)/lib/libclang*.a)
CLANG_STATIC_LDFLAGS = $(addprefix -l,$(basename $(notdir $(subst /lib,/,$(CLANG_STATIC_LIBS)))))
LLD_STATIC_LIBS=$(shell ls $(LLVM_PREFIX)/lib/liblld*.a)
LLD_STATIC_LDFLAGS = $(addprefix -l,$(basename $(notdir $(subst /lib,/,$(LLD_STATIC_LIBS)))))
LDFLAGS +=  -Wl,--start-group $(CLANG_STATIC_LDFLAGS) -Wl,--end-group  $(LLD_STATIC_LDFLAGS) $(shell $(LLVM_PREFIX)/bin/llvm-config --ldflags --system-libs --libs all)
else
LDFLAGS += -lclang-cpp -llldCommon -llldELF -llldMachO -llldCOFF -llldWasm -llldMinGW $(shell $(LLVM_PREFIX)/bin/llvm-config --ldflags --system-libs --libs core)
endif

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

ifneq (, $(shell which mold))
ifeq ($(NO_MOLD), FALSE)
LDFLAGS += -fuse-ld=mold
endif
endif

endif

SRCDIR=src

SRCS := $(wildcard $(SRCDIR)/*.cpp) $(wildcard $(SRCDIR)/targets/*.cpp) $(wildcard $(SRCDIR)/CIR/*.cpp) $(wildcard $(SRCDIR)/backends/*.cpp) $(wildcard $(SRCDIR)/backends/*/*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SRCS))
ifeq ($(LLVM_TOOLS_EMBEDDED_COMPILER),TRUE)
LLVM_TOOLS_EXTERNAL_SRCS := $(wildcard $(SRCDIR)/external/*.cpp)
LLVM_TOOLS_EXTERNAL_OBJS = $(patsubst %.cpp,%.o,$(LLVM_TOOLS_EXTERNAL_SRCS))
OBJS += $(LLVM_TOOLS_EXTERNAL_OBJS)


# ifeq ($(STATIC_LLVM), FALSE)
# LDFLAGS += -lclang-cpp -llldCommon -llldELF -llldMachO -llldCOFF -llldWasm -llldMinGW
# endif

endif

ifeq ($(STATIC_LLVM_DEPENDENCIES), TRUE)
LDFLAGS += -Wl,-Bdynamic -Wl,--as-needed
endif

# Only use this in release mode
ifeq ($(PROFILING), TRUE)
PROFILING_EXTERNAL_SRCS := $(SRCDIR)/external/tracy/public/TracyClient.cpp
PROFILING_EXTERNAL_OBJS = $(patsubst %.cpp,%.o,$(PROFILING_EXTERNAL_SRCS))
OBJS += $(PROFILING_EXTERNAL_OBJS)
CXXFLAGS += -DPROFILING -DTRACY_ENABLE -march=native 

PROFILING_OPTIMIZATION_LEVEL ?= 0
CXXFLAGS += -O$(PROFILING_OPTIMIZATION_LEVEL)
#CXXFLAGS += -DTRACY_NO_EXIT
LDFLAGS += -lpthread -ldl
endif


DEP = $(OBJS:%.o=%.d)
#DEPENDS := $(patsubst %.cpp,%.d,$(SRCS))


all: std_lib cpoint-build cpoint-bindgen cpoint-run cpoint-from-c

release: set_release all

debug: set_debug all

set_release: 
	$(eval CXXFLAGS += -O3 -flto -DNDEBUG) 
	$(eval LDFLAGS += -s -flto) 
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


CMAKE_GC?=FALSE
CMAKE?=cmake
CMAKE_FLAGS?=

gc:
ifeq ($(CMAKE_GC),TRUE)
	mkdir -p $(shell pwd)/bdwgc_prefix
	mkdir -p bdwgc/build && cd bdwgc/build && $(CMAKE) -G "Ninja" -DCMAKE_INSTALL_PREFIX=$(shell pwd)/bdwgc_prefix -DBUILD_SHARED_LIBS=OFF $(CMAKE_FLAGS) .. && ninja  && ninja install
	rm -rf bdwgc/build
else

ifneq ($(OS),Windows_NT)
	mkdir -p $(shell pwd)/bdwgc_prefix
ifneq ($(shell test ! -f bdwgc/Makefile || echo 'yes'),yes)	
	cd bdwgc && ./autogen.sh &&  ./configure --prefix=$(shell pwd)/bdwgc_prefix --disable-threads  --enable-static  --host=$(TARGET) --with-pic
endif
else
	mkdir -p bdwgc_prefix
ifneq ($(shell test ! -f bdwgc/Makefile || echo 'yes'),yes)	
	cd bdwgc && ./autogen.sh && ./configure --prefix=$(shell pwd)/bdwgc_prefix --disable-threads  --enable-static --disable-shared
endif
endif
	+make -C bdwgc
	make -C bdwgc install

endif

std_lib: gc $(OUTPUTBIN)
	+make -C std

$(OUTPUTBIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

#-include $(OBJS:%.o=%.d)

-include $(DEP)


$(SRCDIR)/%.o:$(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<


ifneq ($(OS),Windows_NT)
USERNAME=$(shell logname)
endif

install: all
	mkdir -p $(BINDIR)
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


clean-subfolders:
	rm -f ./src/backends/*.o ./src/backends/*/*.o ./src/CIR/*.o ./src/backends/*.d ./src/backends/*/*.d ./src/CIR/*.d

clean-build: clean-subfolders
	rm -f ./src/*.o ./src/*.temp ./src/*.d

clean-cpoint: clean-build
	rm -f cpoint

clean: clean-cpoint
	make -C std/c_api clean
	make -C std clean
	make -C tests clean
	make -C build clean
	make -C tools/bindgen clean
	make -C tools/run clean
#	rm -rf cpoint out.ll out.ll.* cpoint.* a.out out.o
	rm -f out.ll out.ll.* cpoint.* a.out out.o
	make -C bdwgc clean
	rm bdwgc/Makefile

all-clean: clean
	rm -f ./src/external/*.o

test:
	make -C tests python
#	make -C tests run

std-test:
#	make -C std test
#	make -C std run-test
	./build/cpoint-build -C std test

all-tests: test std-test

onefetch:
	onefetch --exclude bdwgc/* bdwgc_prefix/* tools/vscode/* tools/vim/* src/external/*

prepare-create-release:
	mkdir -p temp_prefix
	$(eval DESTDIR = $(abspath ./temp_prefix))

create-release: prepare-create-release install
	rm -rf temp_prefix/home
	tar -czf cpoint-bin-release.tar.gz -C temp_prefix .
	
	