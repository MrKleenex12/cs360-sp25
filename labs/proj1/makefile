CC = clang 
CFLAGS = -Wall -Wextra

bin/chain_heal: obj/chain_heal.o
	$(CC) $(CFLAGS) obj/chain_heal.o -o bin/chain_heal -lm -fsanitize=address,undefined
	
obj/chain_heal.o: src/chain_heal.c
	$(CC) $(CFLAGS) -c src/chain_heal.c -o obj/chain_heal.o

clean:
	rm -f bin/* obj/*