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

/* Error check usage and CL arguments */
void usage(int argc, char **argv, int *port) {
  if(argc < 3 || (*port = atoi(argv[1])) < 8000) {
    fprintf(
        stderr,
        "usage: ./chat_server port_num(at least 8000) Chat-Room-Names ...\n");
    exit(1);
  }
}

int main(int argc, char **argv) {
  char *hn;
  void *rv;
  int fd, port, sock;
  // FILE *fin, *fout;
  // FILE *stdin_to_socket[2];
  // FILE *socket_to_stdin[2];
  pthread_t tid;

  /* Error check usage and CL arguments */
  usage(argc, argv, &port);
  sock = serve_socket(port);

  /*
  if(pthread_create(&tid, NULL, main_thread, (void *)sock) != 0) {
    perror("main pthread create: ");
    exit(1);
  }

  if(pthread_join(tid, &rv) != 0) {
    perror("main pthread join: ");
    exit(1);
  }
  */
  return 0;
}