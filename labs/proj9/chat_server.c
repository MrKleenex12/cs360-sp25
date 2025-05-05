#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dllist.h"
#include "fields.h"
#include "jrb.h"
#include "jval.h"
#include "sockettome.h"

typedef struct Client {
  FILE *stdin_to_socket[2];
  FILE *socket_to_stdout[2];
} Client;

Client *make_client(int fd) {
  Client *c = (Client *)malloc(sizeof(Client));
  FILE *fin = fdopen(fd, "r");
  FILE *fout = fdopen(fd, "w");

  c->stdin_to_socket[0] = stdin;
  c->stdin_to_socket[1] = fout;
  c->socket_to_stdout[0] = fin;
  c->socket_to_stdout[1] = stdout;

  return c;
}

/* Error check usage and CL arguments */
void usage(int argc, char **argv, int *port) {
  if(argc < 3 || (*port = atoi(argv[1])) < 8000) {
    fprintf(
        stderr,
        "usage: ./chat_server port_num(at least 8000) Chat-Room-Names ...\n");
    exit(1);
  }
}

void *process_connection(void *c) {
  FILE **connection;
  char buf[BUFSIZ];

  connection = (FILE **)c;

  while(fgets(buf, BUFSIZ, connection[0]) != NULL) {
    fputs(buf, connection[1]);
    fflush(connection[1]);
  }
  printf("Exiting now\n");
  exit(0);
}

void *one_person(void *arg) { return NULL; }

/* Main Thread spins off 1 thread to connect to one person and keep talking */
void *main_thread(void *s) {
  Client *c;
  pthread_t tid;
  char buf[BUFSIZ], *hn;
  int sock, fd;
  
  sock = *((int *)s);
  fd = accept_connection(sock);
  c = make_client(fd);

  hn = getenv("USER");
  /* Print out to terminal to start receiving input from Client */
  printf("Connection established: Server '%s'\nRecieving\n", hn);
  /* Send to Client's terminal */
  sprintf(buf, "Recieving from Server: %s\n", hn);
  write(fd, buf, strlen(buf));

  if(pthread_create(&tid, NULL, process_connection, (void*) c->socket_to_stdout) != 0) {
    perror("main_thread pcreate:");
    exit(1);
  } 

  (void) process_connection(c->stdin_to_socket); 
  exit(0);
}

int main(int argc, char **argv) {
  int port, sock;
  pthread_t tid;
  void *rv;

  /* Error check usage and CL arguments */
  usage(argc, argv, &port);
  sock = serve_socket(port);

  if(pthread_create(&tid, NULL, main_thread, (void *)&sock) != 0) {
    perror("main pthread create: ");
    exit(1);
  }

  if(pthread_join(tid, &rv) != 0) {
    perror("main pthread join: ");
    exit(1);
  }
  return 0;
}