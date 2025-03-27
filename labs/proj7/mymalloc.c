#include "mymalloc.h"
#include <unistd.h>

typedef unsigned long UL;

void *malloc_head = NULL;    // Singular global variable

typedef struct flist {
  int size;
  struct flist *flink;
} *Flist;

void Print(void *ptr, char *name) {
  Flist f = (Flist)ptr; 
  printf("%s: 0x%08lx s: %d f: 0x%08lx\n", 
    name, (UL)f, f->size, (UL)f->flink);
}

// Searches through list for ret of memory that is big enough
void *find_chunk(void *ptr, size_t s) {
  Flist f = (Flist)ptr;
  Flist before = f;

  // returns one flist node before found chunk to help with removal
  while(f != NULL) {
    if(s <= f->size) { return before; }
    before = f;
    f = f->flink;
  }
  return NULL;  // return if no chunks found
}

void split(void *ptr, void *before, size_t s) {
  Flist f = (Flist)ptr;
  Flist rem = NULL;
  
  //  Check whether to split up chunk or just remove it
  if((s + 8) < f->size) {
    rem = (Flist)(ptr + s);
    rem->size = f->size - s;
  } else { rem = f->flink; }
  f->size = s;

  // Correctly adjust pointers and malloc_head
  if(ptr != free_list_begin()) {  
    Flist b = (Flist)before;
    if(b != NULL) b->flink = rem;
  }
  else { malloc_head = (void*) rem; }

  Print(f, "return");
  if(rem != NULL) Print(rem, "rem");
}

void *my_malloc(size_t s) {
  Flist head = (Flist)free_list_begin(); // Start of fflist
  Flist before;                          // helps with adding into list
  Flist ret;                             // memory to be returned to user

  // Create heap if heap is null
  if(head == NULL) {
    malloc_head = sbrk(8192);
    int *h = (int*) malloc_head;
    *h = 8192;
  }

  head = (Flist)malloc_head;
  Print(head, "malloc_head");
  s = (s + 7 + 8) & -8;             // Pad s to 8 bytes and add 8 for bookkeeping

  // Find flist node one before chunk to return to user
  before = find_chunk(head, s);
  if(before == NULL) { fprintf(stderr, "no chunks found\n"); exit(1); }
  // If no node before ret, set it as before
  if(before != head || before->flink != NULL) ret = before->flink;
  else { ret = before; before = NULL; }

  // Print(before, "before");
  // Print(ret, "ret");

  split(ret, before, s);          // Split memory ret in two if bigger than requested

  head = (Flist)free_list_begin();
  Print(head, "new malloc_head");
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
    malloc_head = add_in;  // Update flist head
  } 
  else {
    // Find flist node just before where ptr is
    Flist f = begin;
    while(f->flink < add_in) { f = f->flink; }
    // Adjust pointers to insert in
    Flist after = free_list_next(f);
    add_in->flink = after;
    f->flink = add_in;
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