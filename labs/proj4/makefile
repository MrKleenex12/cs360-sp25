CC = gcc
INCLUDES = -I ~/VScode/school/cs360-sp25/include
CFLAGS = $(INCLUDES) -g -fsanitize=address, undefined
LIB = ~/VScode/school/cs360-sp25/lib/libfdr.a

EXECUTABLES = bin/fakemake

all: $(EXECUTABLES) 

bin/fakemake: src/fakemake.c
	gcc $(INCLUDES) -o $@ $^ $(LIB)

clean:
	rm $(EXECUTABLES) 