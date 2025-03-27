#include "mymalloc.h"
#include <unistd.h>

typedef unsigned long UL;

void *malloc_head = NULL;    /* Singular global variable */

typedef struct flist {
  int size;
  struct flist *flink;
  struct flist *blink;
} *Flist;

void Print(Flist f, char *name) {
  printf("%s: 0x%08lx s: %d f: 0x%08lx b: 0x%08lx\n", 
    name, (UL)f, f->size, (UL)f->flink, (UL)f->blink);
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
  Flist f = (Flist)ptr;
  Flist rem = NULL;
  
  //  Check if chunk can be split up
  if((s + 8) < f->size) {
    rem = (Flist)(ptr + s);
    rem->size = f->size - s;
  }
  else { rem = f->flink; }

  // Correctly adjust pointers
  if(ptr != malloc_head) {
    f->blink->flink = rem;
    rem->blink = f->blink;
  }
  else {
    malloc_head = (void*) rem;
    rem->blink = NULL;
  }
  f->size = s;

  
  Print(f, "f");
  if(rem != NULL) Print(rem, "rem");
  return ptr + 8;
}

void *my_malloc(size_t s) {
  Flist f;
  void *ret = NULL;
  int tmp;

  /* Create heap if heap is null */
  if(malloc_head == NULL) {
    malloc_head = sbrk(8192);
    int *h = (int*) malloc_head;
    *h = 8192;
  }
  f = (Flist)malloc_head;
  Print(f, "malloc_head");
  
  /* Pad s to 8 bytes and add 8 for bookkeeping*/
  if((tmp = s % 8) != 0) { s += (8-tmp); }
  s += 8;
  /* Find empty chunk of memory big enough for what is requested */
  f = find_chunk(f, s);
  if(f == NULL) { fprintf(stderr, "no chunks found\n"); exit(1); }
  // Print(f, "found chunk");

  /* TODO Split memory chunk in two if bigger than requested */
  ret = split(f, s);

  f = malloc_head;
  Print(f, "new malloc_head");
  printf("\n");

  return ret;
}

void my_free(void *ptr) {   // TODO Update my_free for every call
  /* new and old malloc_head nodes of Flist */
  Flist new, old;

  ptr -= 8;
  printf("freeing: 0x%08lx\n", (UL)ptr);
  old = (Flist)malloc_head;
  malloc_head = ptr;
  new = (Flist)malloc_head;

  /* Adjust forward and backward pointers */
  new->flink = old;
  new->blink = NULL;
  old->blink = new;

  Print(new, "new malloc_head");
  Print(old, "old malloc_head");
  printf("\n");
}

void *free_list_begin() {
  return NULL;
}

void *free_list_next(void *node) {
  return NULL;
}

void coalesce_free_list() {

}