#DEFAULT_CC_VAL = g++
DEFAULT_CC_VAL = gcc
ifeq ($(CC), cc)
CC = $(DEFAULT_CC_VAL)
endif

#CC ?= $(DEFAULT_CC_VAL)
ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint-bindgen.exe
else
OUTPUTBIN = cpoint-bindgen
endif

SRCDIR=src

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

CFLAGS = -c -g -Wall

CFLAGS += -std=gnu17
#CFLAGS += -std=gnu++17


STATIC_LLVM ?= FALSE

LLVM_PREFIX ?= /usr

ifeq ($(STATIC_LLVM), FALSE)
LDFLAGS = -lclang
else
CLANG_STATIC_LIBS=$(shell ls $(LLVM_PREFIX)/lib/libclang*.a)
CLANG_STATIC_LDFLAGS = $(addprefix -l,$(basename $(notdir $(subst /lib,/,$(CLANG_STATIC_LIBS)))))
LDFLAGS = $(shell $(LLVM_PREFIX)/bin/llvm-config --ldflags --system-libs --libs all) $(CLANG_STATIC_LDFLAGS)
LDFLAGS += -lclang # TODO : remove this (it links the dynamic lib)
#LDFLAGS += $(LLVM_PREFIX)/lib/libclang.a
endif

ifneq ($(OS),Windows_NT)
UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
	CFLAGS += $(shell llvm-config --cflags)
endif

ifneq (, $(shell which lsb_release))
DISTRO=$(shell lsb_release -si)
ifeq ($(DISTRO),Ubuntu)
	CFLAGS += -I/usr/lib/llvm-14/include/ 
	LDFLAGS += -L/usr/lib/llvm-14/lib/
endif
endif

endif

all: $(OUTPUTBIN)

release: set_release all

set_release: 
	$(eval CXXFLAGS += -O3) 
	$(eval LDFLAGS += -s) 

$(OUTPUTBIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o:$(SRCDIR)/%.c
	$(CC) $(CFLAGS) -o $@ $^

clean-build:
	rm -f src/*.o

clean: clean-build
	rm -f $(OUTPUTBIN)

run:
	./$(OUTPUTBIN) example.h example2.h -m
