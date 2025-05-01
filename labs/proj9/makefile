CC=gcc
CFLAGS=-g -fsanitize=address,undefined
INCLUDES=sockettome.o
LDFLAGS=-lpthread

all: chat_server

clean:
	rm -f chat_server

chat_server:
	$(CC) -o chat_server $(CFLAGS) $(LDFLAGS) $(INCLUDES) chat_server.c
	