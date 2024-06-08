ifeq ($(CC),cc)
CC = clang
endif

SRC := $(wildcard *.c)
OBJECTS := $(patsubst %.c, %.o, $(SRC))
TARGET ?= $(shell gcc -dumpmachine)
CFLAGS=-Wall -target $(TARGET) -I../../bdwgc_prefix/include

ifneq (,$(findstring wasi,$(TARGET)))
WASI_SYSROOT ?= /usr/share/wasi-sysroot
CFLAGS += -D__WASM__ --sysroot $(WASI_SYSROOT)
endif

ifeq ($(NO_GC),TRUE)
CFLAGS += -DNO_STD
endif

all: libc_api.a 

debug: CFLAGS += -g
debug: libc_api.a

libc_api.a: $(OBJECTS)
	llvm-ar -rc $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f ./*.o ./*.a ./*.temp
