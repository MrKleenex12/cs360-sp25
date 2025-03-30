#include <unistd.h>
#include "mymalloc.h"

typedef unsigned long UL;

void *malloc_head = NULL;

typedef struct free_list_node {
  int size;
  struct free_list_node *next;
  struct free_list_node *back;
} *FLN;

void Print(void *ptr, char *name) {
  if(!ptr) {
    printf("%s NULL\n", name);
    return;
  }

  FLN f = (FLN)ptr; 
  printf("%s: 0x%08lx s: %d f: 0x%08lx b: 0x%08lx\n", 
    name, (UL)f, f->size, (UL)f->next, (UL)f->back);
}

void set_ptrs(FLN ptr, FLN forward, FLN backward) {
  ptr->next = forward;
  ptr->back = backward;
}

// Searches through list for ret of memory that is big enough
void *find_chunk(void *ptr, size_t s) {
  FLN f = (FLN)ptr;
  while(f != NULL) {
    if(s <= f->size) { return f; }
    f = f->next;
  }
  return NULL;                                  // If no chunks found 
}

void *call_sbrk(size_t s) {
  int size = (s < 8192) ? 8192 : s;             // Must sbrk with 8192 or bigger
  void *start = sbrk(size); 
  int *h = (int*) start;                        // set size of FLN node
  *h = size;
  return start;
}

void *split(void *chunk, size_t size) {
  FLN f = (FLN)chunk;
  FLN ret = NULL;

  // Set size and store onto old size of memory chunk
  if(size+8 < f->size) {
    int old = f->size;
    f->size -= size;
    ret = (FLN)(chunk + f->size);
    ret->size = size;
    set_ptrs(ret, NULL, NULL);
  }
  else { 
    ret = f;
    if(f->back) f->back->next = f->next;
    if(f->next) f->next->back = f->back;
  }

  return (void*)ret;
}

void *my_malloc(size_t size) {
  FLN head = (FLN)free_list_begin();            // Start of FLN
  FLN chunk = NULL;                             // Chunk found to return to user
  void *ret;

  size = (size+7+8) & -8;                       // Pad to 8 bytes and +8 for bookkeeping
  if(!head) {                            // Create heap if heap is null
    head = (FLN)call_sbrk(size);
    malloc_head = head;
  }
  // Print(head, "head");

  // If valid chunk not found, sbrk more to the end of FLN
  if(!(chunk = find_chunk(head, size))) {
    chunk = (FLN)call_sbrk(size);
    FLN f = head;
    while(f->next != NULL) f = f->next;
    f->next = chunk;
  }
  // Print(chunk, "found chunk");

  // Split memory chunk if enough space in free list
  if(size+8 >= chunk->size && !(chunk->next)) {
    malloc_head = NULL;
    ret = chunk;
  }
  else { ret = split(chunk, size); }
  /*
  Print(ret, "ret");
  Print((FLN)free_list_begin(), "new malloc_head");
  printf("\n");
  */
  return ret + 8;
}

void my_free(void *ptr) {
  ptr -= 8;
  FLN head = (FLN)free_list_begin(); 
  FLN f = (FLN)ptr;
  // printf("freeing: 0x%08lx\n", (UL)ptr);

  if(!head) {
    malloc_head = f;
    set_ptrs(f, NULL, NULL);
  }
  else if(f > head) {
    malloc_head = f;
    set_ptrs(f, head, NULL);
  }
  else {
    if(head->next) head->next->back = f;
    set_ptrs(f, head->next, head);
    head->next = f;
  }
}

void *free_list_begin() {
  return malloc_head;
}

void *free_list_next(void *node) {
  FLN f = (FLN)node;
  return f->next;
}

FLN insert(FLN new, FLN sorted) {
  if(new < sorted || sorted == NULL) {
    new->next = sorted; 
    if(sorted) sorted->back = new;
    sorted = new;
  }
  else {
    FLN curr = sorted;
    while(new > curr->next && curr->next != NULL) {
      curr = curr->next;
    }
    if(curr->next) curr->next->back = new;
    new->next = curr->next;
    new->back = curr;
    curr->next = new;
  }
  return sorted;
}

// Insertion sort
FLN sort(void *head) {
  FLN sorted = NULL;
  FLN curr = (FLN)head;
  FLN next = NULL;

  /*
  printf("Before sorted\n");
  FLN f = curr;
  while(f) {
    Print(f, "f");
    f = f->next;
  } 
  */

  while(curr != NULL) {
    next = curr->next;
    sorted = insert(curr, sorted);
    curr = next;
  }

  malloc_head = sorted;
  sorted->back = NULL;

  /*
  printf("After sorted\n");
  f = sorted;
  while(f) {
    Print(f, "f");
    f = f->next;
  } 
  */
  return sorted;
}

void coalesce_free_list() {
  FLN f = (FLN)free_list_begin();
  f = sort(f);

  while(f != NULL) {
    // If current chunk addres + size = next chunk address
    while((void*)f + f->size == f->next) {
      if(f->next) {
        f->size += f->next->size;
        if(f->next->next) f->next->next->back = f;
        f->next = f->next->next;
      } else { f->next = NULL; }
    }

    f = f->next;
  }
  // Print(free_list_begin(), "head");
}