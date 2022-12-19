CC=gcc

OBJS=\
tests.o \

all: test

test: $(OBJS)
	echo $(OBJECTS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c -g $^ -o $@
