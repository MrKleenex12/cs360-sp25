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
  pthread_mutex_t mutex;
  pthread_cond_t cond;
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

void free_client(Client *c) {
  fclose(c->fin);
  fclose(c->fout);
  free(c->name);
  free(c);
}

// Error check usage and CL arguments
void usage(int argc, char **argv, int *port) {
  if(argc < 3 || (*port = atoi(argv[1])) < 8000) {
    fprintf(
        stderr,
        "usage: ./chat_server port_num(at least 8000) Chat-Room-Names ...\n");
    exit(1);
  }
}

void *room_thread(void *room) {
  Room *r = (Room *)room;
  Client *c;
  char *msg;
  Dllist tmp;

  while(1) {
    pthread_mutex_lock(&r->mutex);
    while(dll_empty(r->messages)) pthread_cond_wait(&r->cond, &r->mutex);
    while(!dll_empty(r->messages)) {
      msg = dll_first(r->messages)->val.s;
      dll_traverse(tmp, r->clients) {
        c = (Client*)tmp->val.v;
        fputs(msg, c->fout);
        fflush(c->fout);
      }
      free(msg);
      dll_delete_node(r->messages->flink);
    }
    pthread_mutex_unlock(&r->mutex);
  }
  return NULL;
}

// clang-format off
int user_prompt(Global_Vars *g, char *buf) {
  Client *c = (Client *)g->c;
  int fd = fileno(c->fin);
  int n_objects;

  if(write(fd, "\nEnter your chat name (no spaces):\n", 35) < 0) {
    perror("write:");
    return -1;
  }
  // Ask for Name
  if((n_objects = read(fd, buf, BUFSIZ)) <= 0) { perror("read:"); return -1; }
  buf[n_objects-1] = '\0'; // 0 termminate name
  c->name = strdup(buf);
  // Ask for Chat Room
  if(write(fd, "Enter chat room:\n", 17) < 0) { perror("write:"); return -1; }
  if((n_objects = read(fd, buf, BUFSIZ)) <= 0) { perror("read:"); return -1; }
  buf[n_objects-1] = '\0'; // 0 termminate Room name

  // Check for correct Room name
  JRB tmp = jrb_find_str(g->rooms, buf);
  if(tmp != NULL) {
    c->r = (Room *)tmp->val.v;
  } else {
    fprintf(stderr, "Incorrect Room name\n");
    exit(1);
  }

  return 0;
}
// clang-format on

int print_rooms(Global_Vars *g, char *buf) {
  Room *r;
  Client *c;
  JRB jtmp;
  Dllist dtmp;
  int len;

  if(fputs("Chat Rooms:\n\n", g->c->fout) < 0) {
    perror("fputs:");
    return -1;
  }
  jrb_traverse(jtmp, g->rooms) {
    r = (Room *)jtmp->val.v;
    sprintf(buf, "%s:", jtmp->key.s);
    len = strlen(buf);
    printf("length: %d\n", len);
    dll_traverse(dtmp, r->clients) {  // Print Clients
      c = (Client *)dtmp->val.v;
      buf[len] = ' ';
      strcat(buf + len + 1, c->name);
      len += strlen(c->name) + 1;
      // printf("buf: %s\n", buf);
    }
    buf[len] = '\n';
    // clang-format off
    if(fputs(buf, g->c->fout) < 0) { perror("fputs:"); return -1; }
    if(fflush(g->c->fout) < 0) { perror("fflush:"); return -1; }
    // clang-format on
  }
  return 0;
}

/* Called by main_thread to set up communication */
void *client_thread(void *arg) {
  Global_Vars *g = (Global_Vars *)arg;
  Client *c = g->c;
  char buf[BUFSIZ];

  // Print rooms and clients in it
  if(print_rooms(g, buf) != 0) {
    free_client(c);
    return NULL;
  }
  // Prompt user with username and Room
  if(user_prompt(g, buf) != 0) {
    free_client(c);
    return NULL;
  }

  dll_append(c->r->clients, new_jval_v((void *)c));

  // Read input to start sending to Room
  char msg[BUFSIZ + strlen(c->name) + 2];
  while(fgets(buf, BUFSIZ, c->fin) != NULL) {
    sprintf(msg, "%s: %s", c->name, buf);

    pthread_mutex_lock(&c->r->mutex);
    dll_append(c->r->messages, new_jval_s(strdup(msg)));
    pthread_cond_signal(&c->r->cond);
    pthread_mutex_unlock(&c->r->mutex);
  }

  // TODO Clean up when fgets reads NULL
  return NULL;
}

// Loop indefinitely to accept & spawn client threads
void *main_thread(void *arg) {
  Global_Vars *g = (Global_Vars *)arg;
  Client *c;
  pthread_t tid;
  int fd;

  // While loop to accept clients
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

  // Call main thread to start accepting clients
  if(pthread_create(&main_t, NULL, main_thread, (void *)g) != 0) {
    perror("main pthread create: ");
    exit(1);
  }

  if(pthread_join(main_t, &rv) != 0) {
    perror("main pthread join: ");
    exit(1);
  }

  // Freeing everything
  for(i = 0; i < g->size; i++) {
    free_dllist(g->room_arr[i].clients);
    free_dllist(g->room_arr[i].messages);
    pthread_mutex_destroy(&g->room_arr[i].mutex);
    pthread_cond_destroy(&g->room_arr[i].cond);
  }
  free(g->room_arr);
  jrb_free_tree(g->rooms);
  free(g);
  return 0;
}