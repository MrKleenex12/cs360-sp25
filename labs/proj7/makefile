CC=clang

ALL: test

clean:
	rm -f *.o test

test: test.o mymalloc.o
	$(CC) -o $@ $^

mymalloc.o: mymalloc.h mymalloc.c
	$(CC) -c mymalloc.c

test.o: test.c
	$(CC) -c $^