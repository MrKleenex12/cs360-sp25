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
  Dllist clients;
  Dllist messages;
  pthread_mutex_t *mutex;
  pthread_cond_t *cond;
} Room;

typedef struct Client {
  FILE *fin;
  FILE *fout;
  Room *r;
  char *name;
} Client;

typedef struct Global_Vars {
  Room *room_arr;
  JRB rooms;
  Client *c;
  int sock, size;
} Global_Vars;

Global_Vars *make_gv(int argc, int port) {
  Global_Vars *g = (Global_Vars *)malloc(sizeof(Global_Vars));

  g->size = argc - 2;
  g->room_arr = (Room *)malloc(sizeof(Room) * g->size);
  g->rooms = make_jrb();
  g->sock = serve_socket(port);
  g->c = NULL;
  return g;
}

Client *make_client(int fd) {
  Client *c = (Client *)malloc(sizeof(Client));

  c->fin = fdopen(fd, "r");
  c->fout = fdopen(fd, "w");
  c->r = NULL;
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

void *room_thread(void *room) {
  // Room *r = (Room *)room;
  return NULL;
}

void user_prompt(Global_Vars *g, char *buf) {
  Client *c = (Client *)g->c;
  int n_objects;
  int fd = fileno(c->fin);

  // clang-format off
  if(write(fd, "\nEnter your chat name (no spaces):\n", 35) < 0) {
    perror("write:");
    exit(1);
  }

  if((n_objects = read(fd, buf, BUFSIZ)) < 0) { perror("read:"); exit(1); }
  buf[n_objects-1] = '\0'; // 0 termminate name
  c->name = strdup(buf);

  if(write(fd, "Enter chat room:\n", 17) < 0) { perror("write:"); exit(1); }
  if((n_objects = read(fd, buf, BUFSIZ)) < 0) { perror("read:"); exit(1); }
  buf[n_objects-1] = '\0'; // 0 termminate Room name

  // clang-format on

  JRB tmp = jrb_find_str(g->rooms, buf);
  if(tmp != NULL) {
    c->r = (Room *)tmp->val.v;
  } else {
    fprintf(stderr, "Incorrect Room name\n");
    exit(1);
  }
}

/* Called by main_thread to set up communication */
void *client_thread(void *arg) {
  Global_Vars *g = (Global_Vars *)arg;
  Client *c = (Client *)g->c;
  Room *r;
  JRB jtmp;
  Dllist dtmp;
  char buf[BUFSIZ];
  int len;

  // Print rooms and clients in it
  fputs("Chat Rooms:\n\n", c->fout);
  jrb_traverse(jtmp, g->rooms) {
    r = (Room *)jtmp->val.v;
    sprintf(buf, "%s:", jtmp->key.s);
    len = strlen(buf);
    // Print Clients
    dll_traverse(dtmp, r->clients) {
      buf[len] = ' ';
      strcat(buf + len + 1, dtmp->val.s);
      len += strlen(dtmp->val.s);
    }
    buf[len] = '\n';
    fputs(buf, c->fout);
    fflush(c->fout);
  }

  user_prompt(g, buf);
  return NULL;
}

/* Loop indefinitely to accept & spawn client threads */
void *main_thread(void *arg) {
  Global_Vars *g = (Global_Vars *)arg;
  Client *c;
  pthread_t tid;
  int fd;

  /* While loop to accept clients */
  while(1) {
    fd = accept_connection(g->sock);
    g->c = (c = make_client(fd));
    if(pthread_create(&tid, NULL, client_thread, g) != 0) {
      perror("main p_thread create:");
      exit(1);
    }
  }

  exit(0);
}

int main(int argc, char **argv) {
  Global_Vars *g;
  pthread_t main_t, room_t;
  char *name;
  int i, port;
  void *rv;

  usage(argc, argv, &port);  // Error check usage and set port num
  g = make_gv(argc, port);   // Allocate memory for global variables

  // Insert each Room into JRB and set up dllists
  for(i = 0; i < g->size; i++) {
    name = strdup(argv[i + 2]);

    g->room_arr[i].clients = new_dllist();
    g->room_arr[i].messages = new_dllist();
    jrb_insert_str(g->rooms, name, new_jval_v((void *)g->room_arr + i));

    if(pthread_create(&room_t, NULL, room_thread, g->room_arr + i) != 0) {
      perror("room pthread create:");
      exit(1);
    }
  }

  /* Call main thread to start accepting clients */
  if(pthread_create(&main_t, NULL, main_thread, (void *)g) != 0) {
    perror("main pthread create: ");
    exit(1);
  }

  if(pthread_join(main_t, &rv) != 0) {
    perror("main pthread join: ");
    exit(1);
  }

  /* Freeing everything */
  for(i = 0; i < g->size; i++) {
    free_dllist((g->room_arr + i)->clients);
    free_dllist((g->room_arr + i)->messages);
  }
  free(g->room_arr);
  jrb_free_tree(g->rooms);
  free(g);
  return 0;
}