#include "mymalloc.h"
#include <unistd.h>

typedef unsigned long UL;

void *malloc_head = NULL;    // Singular global variable

typedef struct flist {
  int size;
  struct flist *flink;
  struct flist *blink;
} *Flist;

void Print(Flist f, char *name) {
  printf("%s: 0x%08lx s: %d f: 0x%08lx b: 0x%08lx\n", 
    name, (UL)f, f->size, (UL)f->flink, (UL)f->blink);
}

// Searches through list for chunk of memory that is big enough
void *find_chunk(Flist h, size_t s) {
  Flist f = h;
  while(f != NULL) {
    if(s <= f->size) { return f; }
    f = f->flink;
  }
  return NULL;  // return if no chunks found
}

void split(void *ptr, size_t s) {
  Flist f = (Flist)ptr;
  Flist rem = NULL;
  
  //  Check if chunk can be split up
  if((s + 8) < f->size) {
    rem = (Flist)(ptr + s);
    rem->size = f->size - s;
  } else { rem = f->flink; }

  // Correctly adjust pointers
  if(ptr != free_list_begin()) { 
    f->blink->flink = rem;
    rem->blink = f->blink; }
  else {
    malloc_head = (void*) rem;
    rem->blink = NULL; }
  f->size = s;

  
  Print(f, "f");
  if(rem != NULL) Print(rem, "rem");
}

void *my_malloc(size_t s) {
  Flist start, ret;

  // Create heap if heap is null
  if(free_list_begin() == NULL) {
    malloc_head = sbrk(8192);
    int *h = (int*) malloc_head;
    *h = 8192;
  }
  start = (Flist)malloc_head;
  Print(start, "malloc_head");
  
  // Pad s to 8 bytes and add 8 for bookkeeping
  s = (s + 7 + 8) & -8;
  // Find empty chunk of memory big enough for what is requested
  ret = find_chunk(start, s);
  if(ret == NULL) { fprintf(stderr, "no chunks found\n"); exit(1); }
  // Print(f, "found chunk");

  // Split memory chunk in two if bigger than requested
  split(ret, s);

  start = (Flist)free_list_begin();
  Print(start, "new malloc_head");
  printf("\n");

  return ((void*)ret + 8);
}

void my_free(void *ptr) {
  Flist add_in = (Flist)(ptr -= 8);
  Flist begin = (Flist)free_list_begin();
  printf("freeing: 0x%08lx\n", (UL)ptr);

  // Prepend to flist if before beginning
  if(add_in < begin) {
    add_in->flink = begin;
    add_in->blink = NULL;
    begin->blink = add_in;
    // Update flist start
    malloc_head = add_in;
  } 
  else {
    // Find flist node just before where ptr is
    while(begin->flink < add_in) { begin = begin->flink; }
    // Adjust pointers to add in
    Flist after = free_list_next(begin);
    add_in->flink = after;
    add_in->blink = begin;
    begin->flink = add_in;
    after->blink = add_in;
  }

  printf("\n");
}

void *free_list_begin() {
  return malloc_head;
}

void *free_list_next(void *node) {
  Flist f = (Flist)node;
  return f->flink;
}

void coalesce_free_list() {

}