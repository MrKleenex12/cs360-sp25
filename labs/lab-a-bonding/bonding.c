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

/* Return node and delete from list */
molecule *get_atom(Dllist l) {
  Dllist temp = dll_first(l);
  molecule *m = (molecule *)temp->val.v;
  dll_delete_node(temp);
  return m;
}

void print(molecule *h1, molecule *h2, molecule *o) { /* Debugger function */
  printf("h1: %d %d %d \n", h1->h[0], h1->h[1], h1->o);
  printf("h2: %d %d %d \n", h2->h[0], h2->h[1], h2->o);
  printf(" o: %d %d %d \n", o->h[0], o->h[1], o->o);
}

/* Set ids of all three atoms */
void set_ids(molecule *h1, molecule *h2, molecule *o) {
  // clang-format off
  h1->h[0] = h1->id; h1->h[1] = h2->id; h1->o = o->id;
  h2->h[0] = h1->id; h2->h[1] = h2->id; h2->o = o->id;
  o->h[0] = h1->id; o->h[1] = h2->id; o->o = o->id;
  // clang-format on
}

void send_signals(pthread_cond_t *c1, pthread_cond_t *c2) {
  if(pthread_cond_signal(c1) != 0) {
    perror("H signal error:");
    exit(1);
  }
  if(pthread_cond_signal(c2) != 0) {
    perror("O signal error:");
    exit(1);
  }
}

/* Hydrogen thread function looks to find matching h and o atom to make a
 * molecule. *arg is a bonding_arg value that holds custom structure as a
 * void*. If cannot make a molecule, add hydrogen to h_wait list and call
 * cond_wait() to wait for a match. If it can make a match, set the ids of each
 * atom structure and let each h1, h2, and o atom call the same Bond(id1, id2,
 * id3) three times */

void *hydrogen(void *arg) {
  struct bonding_arg *a = (struct bonding_arg *)arg;
  struct global_info *g = (global_info *)a->v;
  struct molecule *m = make_molecule(a->id);
  pthread_mutex_lock(g->lock);

  /* Check O and H waitlists to find available atoms. If no match, add indiviual
   * H atom to h_wait. Call cond_wait on condition variable */
  if(!dll_empty(g->h_wait) && !dll_empty(g->o_wait)) {
    molecule *H = get_atom(g->h_wait);
    molecule *O = get_atom(g->o_wait);
    // printf("Found other two: %d & %d\n", H->id, O->id);
    set_ids(m, H, O); /* Set the three thread ids of each molecule struct */
    print(m, H, O);

    // clang-format off
    if(pthread_cond_signal(H->condition) != 0) { perror("H: H signal error:"); exit(1); }
    if(pthread_cond_signal(O->condition) != 0) { perror("H: O signal error:"); exit(1); }
    // clang-format on
    pthread_mutex_unlock(g->lock);
  } else {
    dll_append(g->h_wait, new_jval_v((void *)m));
    // printf("added %d to h_wait\n", m->id);
    if(pthread_cond_wait(m->condition, g->lock) != 0) {
      perror("H: wait failed");
      exit(1);
    }
    pthread_mutex_unlock(g->lock);
  }

  /* Return Bond(), free m and m->condition*/
  char *rv = Bond(m->h[0], m->h[1], m->o);
  if(pthread_cond_destroy(m->condition) != 0) {
    perror("H: destroy condition error:");
    exit(1);
  }
  free(m);
  return rv;
}

/* Oxygen thread function does the same thing as hydrogen() except it tries to
 * find two matching hydrogen atoms. Same logic in order to call Bond() */

void *oxygen(void *arg) {
  struct bonding_arg *a = (struct bonding_arg *)arg;
  struct global_info *g = (global_info *)a->v;
  struct molecule *m = make_molecule(a->id);
  pthread_mutex_lock(g->lock);

  /* Check H waitlist to find available H atoms. If no match, add indiviual
   * O atom to o_wait. Call cond_wait on condition variable */
  if(dll_first(g->h_wait)->flink != g->h_wait) {
    molecule *H1 = get_atom(g->h_wait);
    molecule *H2 = get_atom(g->h_wait);
    // printf("Found other two: %d & %d\n", H1->id, H2->id);
    set_ids(H1, H2, m); /* Set the three thread ids of each molecule struct */
    print(H1, H2, m);

    // clang-format off
    if(pthread_cond_signal(H1->condition) != 0) { perror("O: H1 signal error:"); exit(1); }
    if(pthread_cond_signal(H2->condition) != 0) { perror("O: H2 signal error:"); exit(1); }
    // clang-format on
    pthread_mutex_unlock(g->lock);
  } else {
    dll_append(g->o_wait, new_jval_v((void *)m));
    // printf("added %d to o_wait\n", m->id);
    if(pthread_cond_wait(m->condition, g->lock) != 0) {
      perror("O: wait failed");
      exit(1);
    }
    pthread_mutex_unlock(g->lock);
  }

  /* Return Bond(), free m and m->condition*/
  char *rv = Bond(m->h[0], m->h[1], m->o);
  if(pthread_cond_destroy(m->condition) != 0) {
    perror("O: destroy condition error:");
    exit(1);
  }
  free(m);
  return rv;
}
