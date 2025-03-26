#include "mymalloc.h"
#include <unistd.h>

typedef unsigned long UL;

void *head = NULL;    /* Singular global variable */

typedef struct flist {
  int size;
  struct flist *flink;
  struct flist *blink;
} *Flist;

void Print(Flist f, char *name) {
  printf("%s: 0x%08lx s: %d f: 0x%08lx b: 0x%08lx\n", 
    name, (UL)f, f->size, (UL)f->flink, (UL)f->blink);
}

void set_flist(Flist *f, const int size, Flist forw, Flist backw) {
  (*f)->size = size;
  (*f)->flink = forw;
  (*f)->blink = backw;
}

/* Searches through list for chunk of memory that is big enough */
void *find_chunk(Flist h, size_t s) {
  Flist f = h;
  while(f != NULL) {
    if(s <= f->size) { return f; }
    f = f->flink;
  }
  return NULL;  /* return if no chunks found */
}

void *split(void *ptr, size_t s) {
  Flist f, rem;
  f = (Flist)ptr;

  /* Set up Flist after split */
  rem = (Flist)(ptr + s);
  rem->size = f->size - s;
  rem->flink = f->flink;
  rem->blink = f->blink;
  if(ptr == head) { head = (void*) rem; }
  
  return ptr + 8;
}

void *my_malloc(size_t s) {
  Flist f;
  void *ret = NULL;
  int tmp;

  /* Create heap if heap is null */
  if(head == NULL) {
    head = sbrk(8192);
    int *h = (int*) head;
    *h = 8192;
  }
  f = (Flist)head;
  Print(f, "head");
  
  /* Pad s to 8 bytes and add 8 for bookkeeping*/
  if((tmp = s % 8) != 0) { s += (8-tmp) + 8; }

  /* Find empty chunk of memory big enough for what is requested */
  f = find_chunk(f, s);
  if(f == NULL) { fprintf(stderr, "no chunks found\n"); exit(1); }
  // Print(f, "found chunk");

  /* TODO Split memory chunk in two if bigger than requested */
  ret = split(f, s);

  f = head;
  Print(f, "new head");
  printf("\n");

  return ret;
}

void my_free(void *ptr) {
  /* new and old head nodes of Flist */
  Flist new, old;

  ptr -= 8;
  printf("freeing: 0x%08lx\n", (UL)ptr);
  old = (Flist)head;
  head = ptr;
  new = (Flist)head;

  /* Adjust forward and backward pointers */
  new->flink = old;
  new->blink = NULL;
  old->blink = new;

  Print(new, "new head");
  Print(old, "old head");
}

void *free_list_begin() {
  return NULL;
}

void *free_list_next(void *node) {
  return NULL;
}

void coalesce_free_list() {

}

