CC=g++

ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint.exe
else
OUTPUTBIN = cpoint
endif
CFLAGS = -c -g -Wall -O2 $(shell llvm-config --cxxflags)
LDFLAGS = $(shell llvm-config --ldflags --system-libs --libs core)

OBJS=\
lexer.o \
ast.o \
codegen.o \
debuginfo.o \
main.o \

all: cpoint c_api_lib std_lib

cpoint: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o:%.cpp
	$(CC) $(CFLAGS) -o $@ $^

clean-build:
ifeq ($(OS),Windows_NT)
	rmdir .\*.o /s /q
else
	rm -f ./*.o
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
	./cpoint test3.cpoint -d
clean: clean-build
	make -C c_api clean
	make -C std clean
	rm -rf cpoint out.ll out.ll.* cpoint.*

c_api_lib:
	make -C c_api

std_lib:
	make -C std

test: c_api_lib std_lib
	./cpoint test5.cpoint
	gcc -o test5 out.o c_api/libc_api.a.a