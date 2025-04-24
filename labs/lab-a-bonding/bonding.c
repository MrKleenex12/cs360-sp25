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

typedef struct thread_mole {
  pthread_cond_t *condition;
  int id;
  int h1, h2;
  int o;
} thread_mole;

void *initialize_v(char *verbosity) {
  global_info *g = (global_info *)malloc(sizeof(global_info));
  g->lock = new_mutex();
  g->h_wait = new_dllist();
  g->o_wait = new_dllist();

  return (void *)g;
}

thread_mole *make_tm() {
  thread_mole *tm = (thread_mole *)malloc(sizeof(thread_mole));
  tm->condition = new_cond();
  tm->id = -1;
  tm->h1 = -1;
  tm->h2 = -1;
  tm->o = -1;

  return tm;
}

thread_mole *get_thread(Dllist l) {
  Dllist temp = dll_first(l);
  thread_mole* tm = (thread_mole*)temp->val.v;
  dll_delete_node(temp);
  return tm;
}

void *hydrogen(void *arg) {
  struct bonding_arg *a;
  global_info *g;
  thread_mole *tm;

  a = (struct bonding_arg *)arg;
  g = (global_info *)a->v;
  tm = make_tm();              /* Create a thread_mole data structure */
  pthread_mutex_lock(g->lock); /* Lock the global mutex */

  /* Look at waiting lists and see if molecule can be created with two other
   * threads */
  if(!dll_empty(g->h_wait) && !dll_empty(g->o_wait)) {
    thread_mole *other_h, *other_o;

    /* Find waiting h & waiting o, then remove from list */
    other_h = get_thread(g->h_wait); 
    other_o = get_thread(g->o_wait); 

    printf("Found other two: %d & %d\n", other_h->id, other_o->id);
    /* TODO Set ID's in all three thread_mole structs */

    /* TODO Signal the other two threads */

    /* TODO Unlock mutex */
  } else {
    /* Add hydrogen molecule to h_wait and call pthread_cond_wait() */
    tm->id = a->id;
    dll_append(g->h_wait, new_jval_v((void *)tm));
    printf("added %d to h_wait\n", tm->id);

    /* Wait until signaled */
    if(pthread_cond_wait(tm->condition, g->lock) != 0) {
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
  thread_mole *tm;

  a = (struct bonding_arg *)arg;
  g = (global_info *)a->v;
  tm = make_tm();              /* Create a thread_mole data structure */
  pthread_mutex_lock(g->lock); /* Lock the global mutex */

  /* Checks if there are at least two waiting hydrogens in h_wait */
  if(dll_first(g->h_wait)->flink != g->h_wait) {
    thread_mole *h1, *h2;

    /* Find two waiting hydrogens and remove from list */
    h1 = get_thread(g->h_wait); 
    h2 = get_thread(g->h_wait); 

    printf("Found other two: %d & %d\n", h1->id, h2->id);
    /* TODO Set ID's in all three thread_mole structs */

    /* TODO Signal the other two threads */

    /* TODO Unlock mutex */
  } else {
    /* Add oxygen molecule to o_wait and call pthread_cond_wait() */
    tm->id = a->id;
    dll_append(g->o_wait, new_jval_v((void *)tm));
    printf("added %d to o_wait\n", tm->id);

    /* Wait until signaled */
    if(pthread_cond_wait(tm->condition, g->lock) != 0) {
      perror("wait failed");
      exit(1);
    }
    pthread_mutex_unlock(g->lock);
  }
  return NULL;
}
