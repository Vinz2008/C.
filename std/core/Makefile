TARGET ?= $(shell gcc -dumpmachine)

DEBUG ?= FALSE

ifeq ($(shell test ! -f ../../cpoint || echo 'yes'),yes)
    CCPOINT=../../cpoint
else
    CCPOINT=cpoint
endif

SRC := $(wildcard *.cpoint)
OBJECTS := $(patsubst %.cpoint, %.o, $(SRC))
CPOINTFLAGS=-c -no-std -target-triplet $(TARGET) -no-gc

ifeq ($(DEBUG),TRUE)
	CPOINTFLAGS += -d
endif


all: libcore.a

debug: CPOINTFLAGS += -g
debug: libcore.a

%.o: %.cpoint
	$(CCPOINT) $(CPOINTFLAGS) $< -o $@

libcore.a: $(OBJECTS)
	llvm-ar -rc $@ $(OBJECTS)

clean-objects:
	rm -f ./*.o

clean: clean-objects
	rm -f ./*.a ./*.ll ./*.log ./*.temp ./*.test ./libcore.a