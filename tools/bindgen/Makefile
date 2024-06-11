CC ?= gcc

ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint-bindgen.exe
else
OUTPUTBIN = cpoint-bindgen
endif

SRCDIR=src

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

CFLAGS = -c -g -Wall
LDFLAGS = -lclang

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
