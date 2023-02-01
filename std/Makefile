TARGET ?= $(shell gcc -dumpmachine)

ifneq (,$(findstring android,$(TARGET)))
# have if which works in termux
ifneq (,$(wildcard ../cpoint))
    CCPOINT=../cpoint
else
    CCPOINT=cpoint
endif
else
ifneq (,$(wildcard ../cpoint))
    CCPOINT=../cpoint
else
    CCPOINT=cpoint
endif
endif

SRC := $(wildcard *.cpoint)
OBJECTS := $(patsubst %.cpoint, %.o, $(SRC))
SRC_C := $(wildcard c_api/*.c)
OBJECTS_C := $(patsubst c_api/%.c, c_api/%.o, $(SRC_C))
CPOINTFLAGS=-c -nostd -target-triplet $(TARGET)

all: libstd.a

libstd.a: $(OBJECTS) $(OBJECTS_C)
	ar -rc $@ $^

c_api/%.o:
	+TARGET=$(TARGET) make -C c_api

%.o: %.cpoint
	$(CCPOINT) $(CPOINTFLAGS) $< -o $@

clean:
	make -C c_api clean
	rm -f ./*.o ./*.a ./*.ll ./*.log
