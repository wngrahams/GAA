CC  = gcc
CXX = g++

INCLUDES = -I../../lib

CFLAGS   = -O0 -g -Wall -std=c99   $(INCLUDES)
CXXFLAGS = -O0 -g -Wall -std=c++11 $(INCLUDES)

LDFLAGS = -g -L../../lib 
LDLIBS  = -lllist

executables = GAA
objects = GAA.o graph-parser.o
headers := ga-params.h ga-utils.h bitarray.h graph.h selection.h crossover.h
headers += mergesort.h

.PHONY: default
default: $(executables)

$(executables): graph-parser.o

$(objects): $(headers) 

.PHONY: clean
clean:
	rm -f *~ a.out core $(objects) $(executables)

.PHONY: all
all: clean default

