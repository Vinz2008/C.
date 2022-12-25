CCPOINT=../cpoint
SRC := $(wildcard *.cpoint)
OBJECTS := $(patsubst %.cpoint, %.o, $(SRC))
SRC_C := $(wildcard c_api/*.c)
OBJECTS_C := $(patsubst c_api/%.c, c_api/%.o, $(SRC_C))

all: libstd.a

libstd.a: $(OBJECTS) $(OBJECTS_C)
	ar -rc $@ $^

c_api/%.o:
	make -C c_api

%.o: %.cpoint
	$(CCPOINT) -c -nostd $< -o $@

clean:
	rm -f ./*.o ./*.a ./*.ll ./*.log