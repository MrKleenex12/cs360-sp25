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
  int h1_atom, h2_atom;
  int o;
} molecule;

void *initialize_v(char *verbosity) {
  global_info *g = (global_info *)malloc(sizeof(global_info));
  g->lock = new_mutex();
  g->h_wait = new_dllist();
  g->o_wait = new_dllist();

  return (void *)g;
}

molecule *make_tm() {
  molecule *m = (molecule *)malloc(sizeof(molecule));
  m->condition = new_cond();
  m->id = -1;
  m->h1_atom = -1;
  m->h2_atom = -1;
  m->o = -1;

  return m;
}

molecule *get_atom(Dllist l) {
  Dllist temp = dll_first(l);
  molecule* m = (molecule*)temp->val.v;
  dll_delete_node(temp);
  return m;
}

void *hydrogen(void *arg) {
  struct bonding_arg *a;
  global_info *g;
  molecule *m;

  a = (struct bonding_arg *)arg;
  g = (global_info *)a->v;
  m = make_tm();              /* Create a molecule data structure */
  pthread_mutex_lock(g->lock); /* Lock the global mutex */

  /* Look at waiting lists and see if molecule can be created with two other
   * threads */
  if(!dll_empty(g->h_wait) && !dll_empty(g->o_wait)) {
    molecule *h_atom, *o_atom;

    /* Find waiting h & waiting o, then remove from list */
    h_atom = get_atom(g->h_wait); 
    o_atom = get_atom(g->o_wait); 

    printf("Found other two: %d & %d\n", h_atom->id, o_atom->id);
    /* TODO Set ID's in all three molecule structs */

    /* TODO Signal the other two threads */

    /* TODO Unlock mutex */
  } else {
    /* Add hydrogen molecule to h_wait and call pthread_cond_wait() */
    m->id = a->id;
    dll_append(g->h_wait, new_jval_v((void *)m));
    printf("added %d to h_wait\n", m->id);

    /* Wait until signaled */
    if(pthread_cond_wait(m->condition, g->lock) != 0) {
      perror("wait failed");
      exit(1);
    }
    pthread_mutex_unlock(g->lock);
  }
  return NULL;
}

void *oxygen(void *arg) {
  struct bonding_arg *a;
  global_info *g;
  molecule *m;

  a = (struct bonding_arg *)arg;
  g = (global_info *)a->v;
  m = make_tm();              /* Create a molecule data structure */
  pthread_mutex_lock(g->lock); /* Lock the global mutex */

  /* Checks if there are at least two waiting hydrogens in h_wait */
  if(dll_first(g->h_wait)->flink != g->h_wait) {
    molecule *h1_atom, *h2_atom;

    /* Find two waiting hydrogens and remove from list */
    h1_atom = get_atom(g->h_wait); 
    h2_atom = get_atom(g->h_wait); 

    printf("Found other two: %d & %d\n", h1_atom->id, h2_atom->id);
    /* TODO Set ID's in all three molecule structs */

    /* TODO Signal the other two threads */

    /* TODO Unlock mutex */
  } else {
    /* Add oxygen molecule to o_wait and call pthread_cond_wait() */
    m->id = a->id;
    dll_append(g->o_wait, new_jval_v((void *)m));
    printf("added %d to o_wait\n", m->id);

    /* Wait until signaled */
    if(pthread_cond_wait(m->condition, g->lock) != 0) {
      perror("wait failed");
      exit(1);
    }
    pthread_mutex_unlock(g->lock);
  }
  return NULL;
}
