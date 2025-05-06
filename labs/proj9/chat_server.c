/*  Larry Wang - lab - 9
    Creates a server where people can join Rooms using a port number given by
    the socket*/
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
  Room **room_arr;
  JRB rooms;
  Client *c;
  int sock, size;
} Global_Vars;

Global_Vars *make_gv(int argc, int port) {
  Global_Vars *g = (Global_Vars *)malloc(sizeof(Global_Vars));
  g->size = argc - 2;
  g->room_arr = (Room **)malloc(sizeof(Room *) * g->size);
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
  c->name = NULL;
  return c;
}

void free_client(Client *c) {
  fclose(c->fin);
  fclose(c->fout);
  if(c->name != NULL) free(c->name);
  free(c);
}

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
  Dllist dtmp, to_remove;
  char *msg;

  while(1) {
    // Waits until messages actually appear
    pthread_mutex_lock(&r->mutex);
    while(dll_empty(r->messages)) pthread_cond_wait(&r->cond, &r->mutex);

    // Prints messages to all Clients and clears message list
    while(!dll_empty(r->messages)) {  // Each message
      msg = dll_first(r->messages)->val.s;
      dll_traverse(dtmp, r->clients) {  // Each Client
        c = (Client *)dtmp->val.v;
        if(fputs(msg, c->fout) < 0 || fflush(c->fout) < 0) {
          to_remove = dtmp;
          dtmp = dtmp->blink;
          dll_delete_node(to_remove);
          free_client(c);
        }
      }
      dll_delete_node(r->messages->flink);
    }
    pthread_mutex_unlock(&r->mutex);
  }
  return NULL;
}

int user_prompt(Global_Vars *g, char *buf) {
  Client *c = g->c;
  int fd = fileno(c->fin);

  // Asking for Name
  if(write(fd, "\nEnter your chat name (no spaces):\n", 35) < 0) return -1;
  if(fgets(buf, BUFSIZ, c->fin) == NULL) return -1;
  buf[strlen(buf) - 1] = '\0';
  c->name = strdup(buf);

  // Asking for Chat Room
  if(write(fd, "Enter chat room:\n", 17) < 0) return -1;
  if(fgets(buf, BUFSIZ, c->fin) == NULL) return -1;
  buf[strlen(buf) - 1] = '\0';

  // Find Room to add Client into
  JRB tmp = jrb_find_str(g->rooms, buf);
  if(tmp == NULL) {
    fprintf(stderr, "Incorrect Room name\n");
    exit(1);
  } else
    c->r = (Room *)tmp->val.v;

  return 0;
}

int print_rooms(Global_Vars *g, char *buf) {
  Room *r;
  Client *c;
  JRB jtmp1;
  Dllist dtmp;
  int len;

  if(fputs("Chat Rooms:\n\n", g->c->fout) < 0) return -1;

  // Traverse through rooms
  jrb_traverse(jtmp1, g->rooms) {
    r = (Room *)jtmp1->val.v;
    sprintf(buf, "%s:", jtmp1->key.s);
    len = strlen(buf);
    // Traaverse through clients
    dll_traverse(dtmp, r->clients) {
      c = (Client *)dtmp->val.v;
      buf[len] = ' ';
      strcpy(buf + len + 1, c->name);
      len += strlen(c->name) + 1;
    }
    strcpy(buf + len, "\n");
    if(fputs(buf, g->c->fout) < 0) return -1;
    if(fflush(g->c->fout) < 0) return -1;
  }
  return 0;
}

void *client_thread(void *arg) {
  Global_Vars *g = (Global_Vars *)arg;
  Client *c = g->c;
  char buf[BUFSIZ];

  // if(print_rooms(g, buf) < 0 || user_prompt(g, buf) < 0) {
  //   free_client(c);
  //   return NULL;
  // }

  // Print rooms and check user name and room to join
  if(print_rooms(g, buf) != 0) {
    free_client(c);
    return NULL;
  }
  if(user_prompt(g, buf) != 0) {
    free_client(c);
    return NULL;
  }

  // Wake up room thread and send join message
  sprintf(buf, "%s has joined\n", c->name);
  pthread_mutex_lock(&c->r->mutex);
  dll_append(c->r->clients, new_jval_v((void *)c));
  dll_append(c->r->messages, new_jval_s(strdup(buf)));
  pthread_cond_signal(&c->r->cond);
  pthread_mutex_unlock(&c->r->mutex);

  // Run while valid input is send to the room message list
  char msg[BUFSIZ + strlen(c->name) + 50];
  while(fgets(buf, BUFSIZ, c->fin) != NULL) {
    sprintf(msg, "%s: %s", c->name, buf);
    pthread_mutex_lock(&c->r->mutex);
    dll_append(c->r->messages, new_jval_s(strdup(msg)));
    pthread_cond_signal(&c->r->cond);
    pthread_mutex_unlock(&c->r->mutex);
  }

  // Find Client in r->client and remove whenever client disconnects
  pthread_mutex_lock(&c->r->mutex);
  Dllist dtmp;
  dll_traverse(dtmp, c->r->clients) {
    if((Client *)dtmp->val.v == c) {
      dll_delete_node(dtmp);
      break;
    }
  }
  
  // Sending client message
  sprintf(msg, "%s has left\n", c->name);
  dll_append(c->r->messages, new_jval_s(msg));
  pthread_cond_signal(&c->r->cond);
  pthread_mutex_unlock(&c->r->mutex);

  free_client(c);
  return NULL;
}

void *main_thread(void *arg) {
  Global_Vars *g = (Global_Vars *)arg;
  Client *c;
  pthread_t tid;

  // Spins off client threads whenever connection
  while(1) {
    g->c = (c = make_client(accept_connection(g->sock)));
    if(pthread_create(&tid, NULL, client_thread, g) != 0) {
      perror("main pthread create:");
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

  usage(argc, argv, &port);
  g = make_gv(argc, port);

  for(i = 0; i < g->size; i++) {
    name = strdup(argv[i + 2]);
    g->room_arr[i] = (Room *)malloc(sizeof(Room));
    g->room_arr[i]->clients = new_dllist();
    g->room_arr[i]->messages = new_dllist();
    pthread_mutex_init(&g->room_arr[i]->mutex, NULL);
    pthread_cond_init(&g->room_arr[i]->cond, NULL);
    jrb_insert_str(g->rooms, name, new_jval_v((void *)(g->room_arr[i])));

    if(pthread_create(&room_t, NULL, room_thread, g->room_arr[i]) != 0) {
      perror("room pthread create:");
      exit(1);
    }
  }

  if(pthread_create(&main_t, NULL, main_thread, (void *)g) != 0) {
    perror("main pthread create: ");
    exit(1);
  }

  if(pthread_join(main_t, &rv) != 0) {
    perror("main pthread join: ");
    exit(1);
  }

  for(i = 0; i < g->size; i++) {
    free_dllist(g->room_arr[i]->clients);
    free_dllist(g->room_arr[i]->messages);
    pthread_mutex_destroy(&g->room_arr[i]->mutex);
    pthread_cond_destroy(&g->room_arr[i]->cond);
    free(g->room_arr[i]);
  }
  free(g->room_arr);
  jrb_free_tree(g->rooms);
  free(g);
  return 0;
}
