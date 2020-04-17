CC = gcc
CXX = g++

INCDIR = 
INCDEP = 

CFLAGS = -g -Wall 
CXXFLAGS = -g -Wall 

LDFLAGS = -g

LDLIBS = -lm -lpthread

objs = tsp.o tsp-random.o tsp-sahc.o tsp-rmhc.o tsp-ga.o

.PHONY: default
default: tsp

tsp: $(objs) $(INCDEP)
	$(CC) $(CFLAGS) $(INCDIR) $(LDLIBS) -o tsp $(objs) $(LDFLAGS)

tsp.o: ./src/tsp.c ./src/tsp.h
	$(CC) $(CFLAGS) $(INCDIR) -c ./src/tsp.c

tsp-random.o: ./src/tsp-random.c ./src/tsp-random.h ./src/tsp.h
	$(CC) $(CFLAGS) $(INCDIR) -c ./src/tsp-random.c

tsp-sahc.o: ./src/tsp-sahc.c ./src/tsp-sahc.h ./src/tsp.h
	$(CC) $(CFLAGS) $(INCDIR) -c ./src/tsp-sahc.c

tsp-rmhc.o: ./src/tsp-rmhc.c ./src/tsp-rmhc.h ./src/tsp.h
	$(CC) $(CFLAGS) $(INCDIR) -c ./src/tsp-rmhc.c

tsp-ga.o: ./src/tsp-ga.c ./src/tsp-ga.h ./src/tsp.h
	$(CC) $(CFLAGS) $(INCDIR) -c ./src/tsp-ga.c

.PHONY: clean
clean:
	rm -rf *.o a.out core tsp *.dSYM

.PHONY: all
all: clean default

