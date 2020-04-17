CC = gcc
CXX = g++

INCDIR = 
INCDEP = 

CFLAGS = -g -Wall 
CXXFLAGS = -g -Wall 

LDFLAGS = -g

LDLIBS =

objs = GAA.o

.PHONY: default
default: GAA

GAA: $(objs) $(INCDEP)
	$(CC) $(CFLAGS) $(INCDIR) $(LDLIBS) -o GAA $(objs) $(LDFLAGS)

GAA.o: ./GAA.c ./GAA.h
	$(CC) $(CFLAGS) $(INCDIR) -c ./GAA.c

.PHONY: clean
clean:
	rm -rf *.o a.out core GAA *.dSYM

.PHONY: all
all: clean default

