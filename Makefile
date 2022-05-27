CC=gcc
ifeq ($(OS),Windows_NT)
OUTPUTBIN = vlang.exe
else
OUTPUTBIN = vlang
endif

all:
ifeq ($(OS),Windows_NT)
	rmdir .\build\ /s /q
else
	rm -rf build
endif
	mkdir build
	$(CC) -c -g interpret.c -o build/interpret.o
	$(CC) -c -g parser.c -o build/parser.o
	$(CC) -c -g libs/removeCharFromString.c -o build/removeCharFromString.o
	$(CC) -c -g libs/startswith.c -o build/startswith.o
	$(CC) -c -g main.c -o build/main.o
	$(CC) -o $(OUTPUTBIN) build/main.o build/interpret.o build/removeCharFromString.o build/startswith.o build/parser.o
ifeq ($(OS),Windows_NT)
	rmdir .\build\ /s /q
else
	rm -rf build
endif



install:
	cp vlang /usr/bin/vlang
run:
	./vlang test.vlang -d --llvm
clean:
	rm -rf build vlang
