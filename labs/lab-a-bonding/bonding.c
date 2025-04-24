#include "bonding.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dllist.h"
#include "jval.h"

typedef struct global_info {
  pthread_mutex_t *lock;
  Dllist h_wait;
  Dllist o_wait;
} global_info;

typedef struct molecule {
  pthread_cond_t *condition;
  int id;
  int h[2];
  int o;
} molecule;

void *initialize_v(char *verbosity) {
  global_info *g = (global_info *)malloc(sizeof(global_info));
  g->lock = new_mutex();
  g->h_wait = new_dllist();
  g->o_wait = new_dllist();

  return (void *)g;
}

molecule *make_molecule(int id) {
  molecule *m = (molecule *)malloc(sizeof(molecule));
  m->condition = new_cond();
  m->id = id;
  m->h[0] = -1;
  m->h[1] = -1;
  m->o = -1;

  return m;
}

molecule *get_atom(Dllist l) {
  Dllist temp = dll_first(l);
  molecule *m = (molecule *)temp->val.v;
  dll_delete_node(temp);
  return m;
}

void print(molecule *h1, molecule *h2, molecule *o) {
  printf("h1: %d %d %d \n", h1->h[0], h1->h[1], h1->o);
  printf("h2: %d %d %d \n", h2->h[0], h2->h[1], h2->o);
  printf(" o: %d %d %d \n", o->h[0], o->h[1], o->o);
}

void set_ids(molecule *h1, molecule *h2, molecule *o) {
  // clang-format off
  h1->h[0] = h1->id; h1->h[1] = h2->id; h1->o = o->id;
  h2->h[0] = h1->id; h2->h[1] = h2->id; h2->o = o->id;
  o->h[0] = h1->id; o->h[1] = h2->id; o->o = o->id;
  // clang-format on
}

void *hydrogen(void *arg) {
  struct bonding_arg *a;
  struct global_info *g;
  struct molecule *m;
  char *result;

  a = (struct bonding_arg *)arg;
  g = (global_info *)a->v;
  m = make_molecule(a->id);    /* Create a molecule data structure with id */
  pthread_mutex_lock(g->lock); /* Lock the global mutex */

  /* Look at waiting lists and see if molecule can be created with two other
   * threads */
  if(!dll_empty(g->h_wait) && !dll_empty(g->o_wait)) {
    molecule *H, *O;

    /* Find waiting h & waiting o, then remove from list */
    H = get_atom(g->h_wait);
    O = get_atom(g->o_wait);
    printf("Found other two: %d & %d\n", H->id, O->id);

    set_ids(m, H, O);
    print(m, H, O);

    if(pthread_cond_signal(H->condition) != 0) {
      perror("H signal error:");
      exit(1);
    }
    if(pthread_cond_signal(O->condition) != 0) {
      perror("H signal error:");
      exit(1);
    }

    pthread_mutex_unlock(g->lock);
  } else {
    /* Add hydrogen molecule to h_wait and call pthread_cond_wait() */
    dll_append(g->h_wait, new_jval_v((void *)m));
    printf("added %d to h_wait\n", m->id);

    /* Wait until signaled */
    if(pthread_cond_wait(m->condition, g->lock) != 0) {
      perror("wait failed");
      exit(1);
    }
    pthread_mutex_unlock(g->lock);
  }

  /* TODO call Bond and return result */
  return NULL;
}

void *oxygen(void *arg) {
  struct bonding_arg *a;
  struct global_info *g;
  struct molecule *m;
  char *result;

  a = (struct bonding_arg *)arg;
  g = (global_info *)a->v;
  m = make_molecule(a->id);    /* Create a molecule data structure with id */
  pthread_mutex_lock(g->lock); /* Lock the global mutex */

  /* Checks if there are at least two waiting hydrogens in h_wait */
  if(dll_first(g->h_wait)->flink != g->h_wait) {
    molecule *H1, *H2;

    /* Find two waiting hydrogens and remove from list */
    H1 = get_atom(g->h_wait);
    H2 = get_atom(g->h_wait);
    printf("Found other two: %d & %d\n", H1->id, H2->id);

    set_ids(H1, H2, m);
    print(H1, H2, m);

    if(pthread_cond_signal(H1->condition) != 0) {
      perror("H signal error:");
      exit(1);
    }
    if(pthread_cond_signal(H2->condition) != 0) {
      perror("H signal error:");
      exit(1);
    }
    pthread_mutex_unlock(g->lock);
  } else {
    /* Add oxygen molecule to o_wait and call pthread_cond_wait() */
    dll_append(g->o_wait, new_jval_v((void *)m));
    printf("added %d to o_wait\n", m->id);

    /* Wait until signaled */
    if(pthread_cond_wait(m->condition, g->lock) != 0) {
      perror("wait failed");
      exit(1);
    }
    pthread_mutex_unlock(g->lock);
  }
  /* TODO call Bond and return result */

  return NULL;
}
