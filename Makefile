CXX      := /c/Users/arvidjoa/RubyDevKit/mingw/bin/g++
CC       := $(CXX)
CXXFLAGS := -W -Wall --static -O2

all: dupfind

dupfind: dupfind.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm *.o *.exe
