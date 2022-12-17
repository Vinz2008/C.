SRC := $(wildcard *.c)
OBJECTS := $(patsubst %.c, %.o, $(SRC))

all: c_api.a 

c_api.a: $(OBJECTS)
	ar -rc $@ $<

%.o: %.c
	$(CC) $< -o $@
