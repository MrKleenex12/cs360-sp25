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
  FILE *fsock;
} Client;

/* Error check usage and CL arguments */
void usage(int argc, char **argv, int *port) {
  if(argc < 3 || (*port = atoi(argv[1])) < 8000) {
    fprintf(
        stderr,
        "usage: ./chat_server port_num(at least 8000) Chat-Room-Names ...\n");
    exit(1);
  }
}

void *one_person(void *arg) {
  Client *c = (Client *)arg;
  char buf[BUFSIZ];

  while(fgets(buf, BUFSIZ, c->fsock) != NULL) {
    fputs(buf, c->fsock);
    fflush(c->fsock);
  }

  return NULL;
}

/* Main Thread spins off 1 thread to connect to one person and keep talking */
void *main_thread(void *arg) {
  Client *c = (Client *)malloc(sizeof(Client));
  pthread_t tid;
  FILE *fsock;
  int fd;
  void *rv;

  fd = accept_connection(*((int *)arg));
  fsock = fdopen(fd, "r");
  c->fsock = fsock;

  if(pthread_create(&tid, NULL, one_person, (void *)c) != 0) {
    perror("one person create\n");
    exit(1);
  }

  if(pthread_join(tid, &rv) != 0) {
    perror("one person join\n");
    exit(1);
  }
  exit(0);
}

void *process_connection(void *c) {
  FILE **connection;
  char buf[BUFSIZ];

  connection = (FILE**)c;

  while(fgets(buf, BUFSIZ, connection[0]) != NULL) {
    fputs(buf, connection[1]);
    fflush(connection[1]);
  }
  printf("Exiting now\n");
  exit(0);
}

int main(int argc, char **argv) {
  char buf[BUFSIZ];
  char *hn;
  int fd, port, sock;
  FILE *fin, *fout;
  FILE *stdin_to_socket[2];
  FILE *socket_to_stdin[2];
  pthread_t tid;

  /* Error check usage and CL arguments */
  usage(argc, argv, &port);
  sock = serve_socket(port);
  fd = accept_connection(sock);
  hn = getenv("USER");

  /* Print out to terminal to start receiving input from Client */
  printf("Connection established: Server '%s'\nRecieving\n", hn);
  /* Send to Client's terminal */
  sprintf(buf, "Recieving from Server: %s\n", hn);
  write(fd, buf, strlen(buf));

  /* Open FILES to connect between client and Room */
  fin = fdopen(fd, "r");
  fout = fdopen(fd, "w");

  /* Set up array of FILE *'s for process_connection */
  stdin_to_socket[0] = stdin;
  stdin_to_socket[1] = fout;
  socket_to_stdin[0] = fin;
  socket_to_stdin[1] = stdout;

  if(pthread_create(&tid, NULL, process_connection, socket_to_stdin) != 0) {
    perror("main: pthread_create:");
    exit(1);
  }
  (void) process_connection(stdin_to_socket);

  /*
  if(pthread_create(&tid, NULL, main_thread, (void *)&sock) != 0) {
    perror("main pthread create: ");
    exit(1);
  }

  if(pthread_join(tid, &rv) != 0) {
    perror("main pthread join: ");
    exit(1);
  }*/
  return 0;
}