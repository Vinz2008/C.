CCPOINT=../cpoint
SRC := $(wildcard *.cpoint)
OBJECTS := $(patsubst %.cpoint, %.o, $(SRC))

all: libstd.a

libstd.a: $(OBJECTS)
	ar -rc $@ $^

%.o: %.cpoint
	$(CCPOINT) $< -o $@

clean:
	rm -f ./*.o ./*.a