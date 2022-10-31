CC=gcc

ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint.exe
else
OUTPUTBIN = cpoint
endif
CFLAGS = -c -g -Wall -O2

OBJS=\
libs/startswith.o \
libs/removeCharFromString.o \
libs/isCharContainedInStr.o \
ast.o \
codegen.o \
lexer.o \
parser.o \
interpret.o \
utils.o \
types.o \
main.o \

all: cpoint 

cpoint: $(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $^

%.o:%.c
	$(CC) $(CFLAGS) -o $@ $^

clean-build:
ifeq ($(OS),Windows_NT)
	rmdir .\*.o .\libs\*.o /s /q
else
	rm -f ./*.o ./libs/*.o
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
clean: clean-build
	rm -rf cpoint out.ll out.ll.* cpoint.*
