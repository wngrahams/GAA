CC  = gcc
CXX = g++

INCLUDES =

CFLAGS   = -O0 -g -Wall -std=c99   $(INCLUDES)
CXXFLAGS = -O0 -g -Wall -std=c++11 $(INCLUDES)

LDFLAGS = -g  
LDLIBS  = 

executables = GAA
objects = GAA.o graph-parser.o

.PHONY: default
default: $(executables)

$(executables): graph-parser.o

$(objects): ga-params.h ga-utils.h bitarray.h graph.h selection.h crossover.h

.PHONY: clean
clean:
	rm -f *~ a.out core $(objects) $(executables)

.PHONY: all
all: clean default

