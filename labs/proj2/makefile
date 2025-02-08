CC = gcc 
INCLUDES = -I ~/VScode/school/cs360-sp25/include 
CFLAGS = $(INCLUDES) -fsanitize=address,undefined -g
LIBDIR = ~/VScode/school/cs360-sp25/lib
LIBS = $(LIBDIR)/libfdr.a 

EXECUTABLES = bin/famtree

all: $(EXECUTABLES)

bin/famtree: src/famtree.c
	$(CC) $(CFLAGS) -o bin/famtree src/famtree.c $(LIBS)

clean:
	rm $(EXECUTABLES)
