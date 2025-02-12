CC = gcc 
CFLAGS = -Wall -Wextra -lm -fsanitize=address,undefined -g

EXECUTABLES = bin/huff_dec

all: $(EXECUTABLES)

bin/huff_dec: src/huff_dec.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm $(EXECUTABLES)
