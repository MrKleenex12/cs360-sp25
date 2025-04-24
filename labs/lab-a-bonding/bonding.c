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

typedef struct thread_info {
  pthread_cond_t *condition;
  int id;
  int h[2];
  int o;
} thread_info;

void *initialize_v(char *verbosity) { return NULL; }

void *hydrogen(void *arg) { return NULL; }

void *oxygen(void *arg) { return NULL; }
