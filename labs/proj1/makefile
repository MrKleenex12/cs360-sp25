CC = clang 
CFLAGS = -Wall -Wextra -g

bin/chain_heal: obj/chain_heal.o
	$(CC) $(CFLAGS) $^ -o $@ -lm -fsanitize=address,undefined
	
obj/chain_heal.o: src/chain_heal.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f bin/* obj/*