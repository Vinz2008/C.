TARGET ?= $(shell gcc -dumpmachine)
CFLAGS=-Wall -c -g -target $(TARGET)
ifneq (,$(findstring wasi,$(TARGET)))
WASI_SYSROOT ?= /usr/share/wasi-sysroot
CFLAGS+=-D__WASM__ --sysroot $(WASI_SYSROOT)
endif

all: test.o

test.o: test.c
	clang -o $@ $^ $(CFLAGS)

clean:
	rm ./*.o