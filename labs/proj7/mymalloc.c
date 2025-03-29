#include "mymalloc.h"
#include <unistd.h>

typedef unsigned long UL;

void *malloc_head = NULL;    // Singular global variable

typedef struct flist {
  int size;
  struct flist *flink;
} *Flist;

void Print(void *ptr, char *name) {
  if(ptr == NULL) {
    printf("%s NULL\n", name);
    return;
  }

  Flist f = (Flist)ptr; 
  printf("%s: 0x%08lx s: %d f: 0x%08lx\n", 
    name, (UL)f, f->size, (UL)f->flink);
}

// Searches through list for ret of memory that is big enough
void *find_chunk(void *ptr, size_t s) {
  Flist f = (Flist)ptr;
  // returns one flist node before found chunk to help with removal
  while(f != NULL) {
    if(s <= f->size) { return f; }
    f = f->flink;
  }
  return NULL;  // return if no chunks found
}

void *find_before(void *ptr) {
  Flist f = (Flist)free_list_begin();
  if((void*)f == ptr) return NULL;

  while(f->flink != NULL && (void*)f < ptr) {
    if((void*) f->flink == ptr) { return f; }
  }
  return NULL;
}

void split(void *ptr, void *before, size_t s) {
  Flist f = (Flist)ptr;
  Flist rem = NULL;
  int tmp;
  
  tmp = f->size;
  f->size = s;

  if(s + 8 < f->size) {
    rem = (Flist)(ptr + s);
    rem->size = tmp - s;
  }
  else if(f->flink != NULL) {
    f->size += 8;
    rem = f->flink;
  }
  else {
    malloc_head = NULL;
    return;
  }
 
  // Correctly adjust pointers and malloc_head
  if(ptr != free_list_begin()) {  
    Flist b = (Flist)before;
    if(b != NULL) b->flink = rem;
  }
  else { malloc_head = (void*) rem; }

  if(rem != NULL) Print(rem, "rem");
}

void *my_malloc(size_t s) {
  Flist head = (Flist)free_list_begin();  // Start of fflist
  Flist before;                           // helps with adding into list
  Flist ret;                              // memory to be returned to user

  // Create heap if heap is null
  s = (s + 7 + 8) & -8;                   // Pad to 8 bytes and +8 for bookkeeping
  // printf("s: %zu\n", s);
  if(head == NULL) {
    // Must at least call sbrk with 8192 or bigger
    int call = (s < 8192) ? 8192 : s;
    malloc_head = sbrk(call); 
    // set size of flist node
    int *h = (int*) malloc_head;
    *h = call;
  }

  head = (Flist)malloc_head;
  Print(head, "malloc_head");

  // Find flist node one before chunk to return to user
  ret = find_chunk(head, s);
  if(ret == NULL) {
    fprintf(stderr, "no chunks found\n");
    exit(1);
  }
  Print(ret, "found chunk");

  // If no node before ret, set it as before
  before = find_before(ret);
  if(before != NULL) {
    Print(before , "before");
  }

  split(ret, before, s);                  // Adjust free list and split if needed 

  head = (Flist)free_list_begin();
  Print(head, "new malloc_head");
  printf("\n");

  return ((void*)ret + 8);
}

void my_free(void *ptr) {
  ptr -= 8;
  Flist add_in = (Flist)(ptr);
  Flist begin = (Flist)free_list_begin();
  printf("freeing: 0x%08lx\n", (UL)ptr);

  if(begin == NULL) {
    malloc_head = add_in;
    add_in->flink = NULL;
    return;
  }

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
  Print(add_in->flink, "add_in next");
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
  Flist f = (Flist)free_list_begin();
  while(f != NULL) {
    void *v1 = (void*)f;
    void *v2 = (void*)f->flink;
    
    // If current chunk addres + size = next chunk address
    if(v1 + f->size == v2) {
      if(f->flink != NULL) {
        f->size += f->flink->size;
        f->flink = f->flink->flink;
      } else { f->flink = NULL; }
    }

    f = f->flink;
  }
  
  
  Print((Flist)malloc_head, "after coal");
        
     
}