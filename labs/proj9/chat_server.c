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

typedef struct Room {
  char *name;
  Dllist clients;
  Dllist messages;
} Room;

typedef struct Client {
  FILE *stdin_to_socket[2];
  FILE *socket_to_stdout[2];
  Room *r;
} Client;

typedef struct Global_Vars {
  Room *room_arr;
  JRB rooms;
  int sock;
} Global_Vars;

Global_Vars *make_gv(int argc, int port) {
  Global_Vars *g = (Global_Vars *)malloc(sizeof(Global_Vars));

  g->room_arr = (Room *)malloc(sizeof(Room) * (argc - 2));
  g->rooms = make_jrb();
  g->sock = serve_socket(port);
  return g;
}

Client *make_client(int fd) {
  Client *c = (Client *)malloc(sizeof(Client));
  FILE *fin = fdopen(fd, "r");
  FILE *fout = fdopen(fd, "w");

  c->r = NULL;
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

/* Called by main_thread to set up communication */
void *client_thread() { return NULL; }

/* Loop indefinitely to accept & spawn client threads */
void *main_thread(void *arg) {
  Global_Vars *g;
  FILE *stdin_to_socket[2];
  FILE *socket_to_stdout[2];
  pthread_t tid;
  char buf[BUFSIZ], *hn;
  int fd;

  g = (Global_Vars *)arg;
  fd = accept_connection(g->sock);

  FILE *fin = fdopen(fd, "r");
  FILE *fout = fdopen(fd, "w");
  stdin_to_socket[0] = stdin;
  stdin_to_socket[1] = fout;
  socket_to_stdout[0] = fin;
  socket_to_stdout[1] = stdout;

  /* Send starting messages on both sides */
  hn = getenv("USER");
  printf("Connection established: Server '%s'\nRecieving\n", hn);
  sprintf(buf, "Recieving from Server: %s\n", hn);
  write(fd, buf, strlen(buf));

  if(pthread_create(&tid, NULL, process_connection, (void *)socket_to_stdout) !=
     0) {
    perror("main_thread pcreate:");
    exit(1);
  }

  (void)process_connection(stdin_to_socket);
  exit(0);
}

int main(int argc, char **argv) {
  Global_Vars *g;
  pthread_t tid;
  char *name;
  int i, port;
  void *rv;

  usage(argc, argv, &port);  // Error check usage and set port num
  g = make_gv(argc, port);   // Allocate memory for global variables

  // Insert each Room into JRB and set up dllists
  for(i = 2; i < argc; i++) {
    name = strdup(argv[i]);

    (g->room_arr + (i - 2))->clients = new_dllist();
    (g->room_arr + (i - 2))->messages = new_dllist();
    (g->room_arr + (i - 2))->name = name;
    jrb_insert_str(g->rooms, name, new_jval_v((void *)g->room_arr + (i - 2)));
  }

  /* Call main thread to start accepting clients */
  if(pthread_create(&tid, NULL, main_thread, (void *)g) != 0) {
    perror("main pthread create: ");
    exit(1);
  }

  if(pthread_join(tid, &rv) != 0) {
    perror("main pthread join: ");
    exit(1);
  }

  /* Freeing everything */
  for(i = 0; i < argc - 2; i++) {
    free_dllist((g->room_arr + i)->clients);
    free_dllist((g->room_arr + i)->messages);
    free((g->room_arr + i)->name);
  }
  free(g->room_arr);
  jrb_free_tree(g->rooms);
  free(g);
  return 0;
}