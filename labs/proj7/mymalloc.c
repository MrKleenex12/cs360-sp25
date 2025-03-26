#include "mymalloc.h"
#include <unistd.h>

typedef unsigned long UL;

void *head = NULL;    /* Singular global variable */

typedef struct flist {
  int size;
  struct flist *flink;
  struct flist *blink;
} *Flist;

void print_Flist(Flist f) {
  printf("loc: 0x%08lx s: %d f: 0x%08lx b: 0x%08lx\n", (UL)f, f->size, (UL)f->flink, (UL)f->blink);
}

/* Searches through list for chunk of memory that is big enough */
void *find_chunk() {
  return NULL;
}

void *my_malloc(size_t s) {
  Flist f;
  void *ret;
  int tmp;

  /* Create heap if heap is null */
  if(head == NULL) {
    head = sbrk(8192);
    int *h = (int*) head;
    *h = 8192;
  }
  f = (Flist)head;
  print_Flist(f);
  
  /* Split memory chunk in two */
  
  /* Pad s to 8 bytes */
  if((tmp = s % 8) != 0) { s += tmp; }
  // printf("s: %zu\n", s);
  /* Set up bookkeeping and memory to be returned */
  tmp = f->size;
  f->size = s += 8;
  ret = head+8;
  /* Adjust head and empty memory */
  f = (Flist)(head += s);
  f->size = tmp - s; 

  return ret;
}

void my_free(void *ptr) {

}

void *free_list_begin() {
  return NULL;
}

void *free_list_next(void *node) {
  return NULL;
}

void coalesce_free_list() {

}

