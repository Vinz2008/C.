CC=gcc
SRC := $(wildcard *.c)
OBJECTS := $(patsubst %.c, %.o, $(SRC))

all: libc_api.a 

libc_api.a: $(OBJECTS)
	ar -rc $@ $^

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -f ./*.o ./*.a