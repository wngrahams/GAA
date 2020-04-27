CC  = gcc
CXX = g++

INCLUDES =
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = -g
LDLIBS = 

.PHONY: default
default: libllist.a

libllist.a: llist/llist.o
	ar rc libllist.a llist/llist.o
	ranlib libllist.a

# header dependency
llist/mylist.o: llist/llist.h

.PHONY: clean
clean:
	rm -f ./*/*.o *~ a.out core libllist.a

.PHONY: all
all: clean libllist.a

