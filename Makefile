CC=gcc
buildFolder=build
ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint.exe
else
OUTPUTBIN = cpoint
endif

all:
ifeq ($(OS),Windows_NT)
	rmdir .\$(buildFolder)\ /s /q
else
	rm -rf $(buildFolder)
endif
	mkdir $(buildFolder)
	$(CC) -c -g interpret.c -o $(buildFolder)/interpret.o
	$(CC) -c -g parser.c -o $(buildFolder)/parser.o
	$(CC) -c -g libs/removeCharFromString.c -o $(buildFolder)/removeCharFromString.o
	$(CC) -c -g libs/startswith.c -o $(buildFolder)/startswith.o
	$(CC) -c -g types.c -o $(buildFolder)/types.o
	$(CC) -c -g utils.c -o $(buildFolder)/utils.o
	$(CC) -c -g libs/isCharContainedInStr.c -o $(buildFolder)/isCharContainedInStr.o
	$(CC) -c -g main.c -o $(buildFolder)/main.o
	$(CC) -o $(OUTPUTBIN) $(buildFolder)/main.o $(buildFolder)/interpret.o $(buildFolder)/removeCharFromString.o $(buildFolder)/startswith.o $(buildFolder)/parser.o $(buildFolder)/types.o $(buildFolder)/isCharContainedInStr.o $(buildFolder)/utils.o
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
