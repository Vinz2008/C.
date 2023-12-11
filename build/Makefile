CXX ?= g++

OUTPUTBIN = cpoint-build

ifeq ($(OS),Windows_NT)
OUTPUTBIN += .exe
endif

SRCDIR=src

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SRCS))
OBJS += ../src/cli.o

CXXFLAGS = -c -g -Wall -Wno-sign-compare -Itomlplusplus/include -std=c++17

all: $(OUTPUTBIN)

release: set_release all

set_release: 
	$(eval CXXFLAGS += -O3) 
	$(eval LDFLAGS += -s) 

$(OUTPUTBIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o:$(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

clean-build:
	rm -f src/*.o

clean: clean-build
	rm -f $(OUTPUTBIN)


run:
	../$(OUTPUTBIN) -C example/