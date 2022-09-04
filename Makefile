CC=gcc
buildFolder=build
ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint.exe
else
OUTPUTBIN = cpoint
endif
CFLAGS = -c -g -Wall

all: setup $(buildFolder)/interpret.o $(buildFolder)/parser.o $(buildFolder)/ast.o $(buildFolder)/lexer.o $(buildFolder)/removeCharFromString.o $(buildFolder)/startswith.o $(buildFolder)/types.o $(buildFolder)/utils.o $(buildFolder)/isCharContainedInStr.o $(buildFolder)/main.o linking clean-build

setup:
ifeq ($(OS),Windows_NT)
	rmdir .\$(buildFolder)\ /s /q
else
	rm -rf $(buildFolder)
endif
	mkdir $(buildFolder)

$(buildFolder)/interpret.o:
	$(CC) $(CFLAGS) interpret.c -o $(buildFolder)/interpret.o

$(buildFolder)/parser.o:
	$(CC) $(CFLAGS) parser.c -o $(buildFolder)/parser.o

$(buildFolder)/lexer.o:
	$(CC) $(CFLAGS) lexer.c -o $(buildFolder)/lexer.o

$(buildFolder)/ast.o:
	$(CC) $(CFLAGS) ast.c -o $(buildFolder)/ast.o

$(buildFolder)/removeCharFromString.o:
	$(CC) $(CFLAGS) libs/removeCharFromString.c -o $(buildFolder)/removeCharFromString.o

$(buildFolder)/startswith.o:	
	$(CC) $(CFLAGS) libs/startswith.c -o $(buildFolder)/startswith.o

$(buildFolder)/types.o:
	$(CC) $(CFLAGS) types.c -o $(buildFolder)/types.o

$(buildFolder)/utils.o:
	$(CC) $(CFLAGS) utils.c -o $(buildFolder)/utils.o

$(buildFolder)/isCharContainedInStr.o:
	$(CC) $(CFLAGS) libs/isCharContainedInStr.c -o $(buildFolder)/isCharContainedInStr.o

$(buildFolder)/main.o:
	$(CC) $(CFLAGS) main.c -o $(buildFolder)/main.o

linking:
	$(CC) -o $(OUTPUTBIN) $(buildFolder)/main.o $(buildFolder)/interpret.o $(buildFolder)/removeCharFromString.o $(buildFolder)/startswith.o $(buildFolder)/parser.o $(buildFolder)/types.o $(buildFolder)/isCharContainedInStr.o $(buildFolder)/utils.o $(buildFolder)/ast.o $(buildFolder)/lexer.o

clean-build:
ifeq ($(OS),Windows_NT)
	rmdir .\$(buildFolder)\ /s /q
else
	rm -rf $(buildFolder)
endif

install:
	rm -rf /opt/cpoint/
	mkdir /opt/cpoint
	cp -r std/ /opt/cpoint/
	cp cpoint /opt/cpoint/
ifeq ($(origin SHELL_USED),undefined)
	@#exit 1
else
	@#echo `export PATH=$PATH:/opt/cpoint` >> /home/${LOGNAME}/.${SHELL_USED}rc
endif

run:
	./cpoint test.cpoint -d --llvm
clean:
	rm -rf $(buildFolder) cpoint out.ll out.ll.* cpoint.*
