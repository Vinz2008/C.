TARGET ?= $(shell gcc -dumpmachine)
CFLAGS=-Wall -target $(TARGET)
ifneq (,$(findstring wasm,$(TARGET)))
WASI_SYSROOT ?= /usr/share/wasi-sysroot
CFLAGS+=-D__WASM__ --sysroot $(WASI_SYSROOT)
endif

all: test.o

test.o: test.c
	clang -o $@ $^ -Wall -c -g

clean:
	rm ./*.o