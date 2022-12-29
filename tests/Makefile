CC=gcc

OBJS=\
tests.o \

all: clean test

test: $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c -g $^ -o $@

clean:
	rm -f ./*.o ./*.ll ./*.log test

run: all
	./test