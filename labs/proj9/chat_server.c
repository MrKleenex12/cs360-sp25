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


int main(int argc, char **argv) {
  char *hn;
  void *rv;
  int fd, port, *sock;
  // FILE *fin, *fout;
  // FILE *stdin_to_socket[2];
  // FILE *socket_to_stdin[2];
  pthread_t tid;

  if(argc < 3) {
    fprintf(stderr, "usage: ./chat_server port Chat-Room-Names ...\n");
    exit(1);
  }
  if((port = atoi(argv[1])) < 8000) {
    fprintf(stderr, "port must be at least 8000\n");
    exit(1);
  }

  return 0;
}