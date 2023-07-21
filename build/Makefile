CXX = g++

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

$(OUTPUTBIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o:$(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

clean-build:
	rm -f src/*.o

clean: clean-build
	rm -f $(OUTPUTBIN)


run:
	cd example && ../$(OUTPUTBIN)