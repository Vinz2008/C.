CC ?= gcc

ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint-run.exe
else
OUTPUTBIN = cpoint-run
endif

SRCDIR=src

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

CFLAGS = -c -g -Wall

LDFLAGS = 

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
	./$(OUTPUTBIN) example.cpoint