CC=gcc


all:
	mkdir build
	$(CC) -c -g interpret.c -o build/interpret.o
	$(CC) -c -g libs/removeCharFromString.c -o build/removeCharFromString.o
	$(CC) -c -g libs/startswith.c -o build/startswith.o
	$(CC) -c -g main.c -o build/main.o
	$(CC) -o vlang build/main.o build/interpret.o build/removeCharFromString.o build/startswith.o
	rm -rf build

windows:
	mkdir build
	$(CC) -c -g intepret.c -o build/intepret.o
	$(CC) -c -g libs/removeCharFromString.c -o build/removeCharFromString.o
	$(CC) -c -g libs/startswith.c -o build/startswith.o
	$(CC) -c -g main.c -o build/main.o
	$(CC) -o vlang.exe build/main.o build/interpret.o build/removeCharFromString.o build/startswith.o
	rmdir .\build\ /s /q


install:
	cp sasm /usr/bin/sasm
run:
	./vlang test.vlang