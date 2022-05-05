CC=gcc
ifeq ($(OS),Windows_NT)
OUTPUTBIN = vlang.exe
else
OUTPUTBIN = vlang
endif

LDFLAGS=`llvm-config --cflags --ldflags --libs core executionengine mcjit interpreter analysis native bitwriter --system-libs`

all:
	mkdir build
	$(CC) -c -g interpret.c -o build/interpret.o `llvm-config --cflags --libs core executionengine mcjit interpreter analysis native bitwriter --system-libs`
	$(CC) -c -g parser.c -o build/parser.o
	$(CC) -c -g libs/removeCharFromString.c -o build/removeCharFromString.o
	$(CC) -c -g libs/startswith.c -o build/startswith.o
	$(CC) -c -g main.c -o build/main.o
	$(CC) -o $(OUTPUTBIN) build/main.o build/interpret.o build/removeCharFromString.o build/startswith.o build/parser.o $(LDFLAGS)
ifeq ($(OS),Windows_NT)
	rmdir .\build\ /s /q
else
	rm -rf build
endif



install:
	cp vlang /usr/bin/vlang
run:
	./vlang test.vlang -d
clean:
	rm -rf build vlang
