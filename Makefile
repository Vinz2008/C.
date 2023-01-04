CC=g++
DESTDIR ?= /usr/bin
PREFIX ?= /usr/local


ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint.exe
else
OUTPUTBIN = cpoint
endif
CFLAGS = -c -g -Wall -O2 $(shell llvm-config --cxxflags)
LDFLAGS = $(shell llvm-config --ldflags --system-libs --libs core)

OBJS=\
lexer.o \
ast.o \
codegen.o \
debuginfo.o \
types.o \
linker.o \
main.o \

all: cpoint std_lib

cpoint: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

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
	./cpoint test2.cpoint -d

clean: clean-build
	make -C std/c_api clean
	make -C std clean
	make -C tests clean
	rm -rf cpoint out.ll out.ll.* cpoint.*

std_lib:
	make -C std

test: all
	make -C tests run
