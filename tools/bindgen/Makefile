CC = gcc

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

all: $(OUTPUTBIN)

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