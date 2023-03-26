CXX = g++

ifeq ($(OS),Windows_NT)
OUTPUTBIN = cpoint-build.exe
else
OUTPUTBIN = cpoint-build
endif

SRCDIR=src

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SRCS))

CXXFLAGS = -c -g -Wall -Itomlplusplus/include -std=c++17

all: clean $(OUTPUTBIN)

$(OUTPUTBIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o:$(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(OUTPUTBIN) src/*.o
