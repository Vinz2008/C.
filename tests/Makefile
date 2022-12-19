CC=gcc

OBJS=\
tests.o \

all: test

test: $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c -g $^ -o $@

clean:
		rm -f ./*.o
