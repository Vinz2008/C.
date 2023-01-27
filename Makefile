CC=g++
DESTDIR ?= /usr/bin
PREFIX ?= /usr/local
NO_OPTI ?= false

ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint.exe
else
OUTPUTBIN = cpoint
endif
CFLAGS = -c -g -Wall $(shell llvm-config --cxxflags)
ifeq ($(NO_OPTI),true)
CFLAGS += -O0
else
CFLAGS += -O2
endif
LDFLAGS = $(shell llvm-config --ldflags --system-libs --libs core)

OBJS=\
lexer.o \
ast.o \
codegen.o \
preprocessor.o \
errors.o \
debuginfo.o \
types.o \
linker.o \
target-triplet.o \
main.o \

all: std_lib_plus_compiler

std_lib_plus_compiler: std_lib

std_lib: $(OUTPUTBIN)
	+make -C std

$(OUTPUTBIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o:%.cpp
	$(CC) $(CFLAGS) -o $@ $^

clean-build:
ifeq ($(OS),Windows_NT)
	rmdir .\*.o /s /q
else
	rm -f ./*.o
endif

install:
	cp cpoint $(DESTDIR)/
	rm -rf $(PREFIX)/lib/cpoint
	mkdir $(PREFIX)/lib/cpoint
	cp -r std/* $(PREFIX)/lib/cpoint


run:
	./cpoint -std ./std test2.cpoint

clean: clean-build
	make -C std/c_api clean
	make -C std clean
	make -C tests clean
	rm -rf cpoint out.ll out.ll.* cpoint.*


test:
	make -C tests run
