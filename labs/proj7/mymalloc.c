#include "mymalloc.h"

#include <unistd.h>

/* 
  Overview:

*/

/*
  my_malloc()

  - align s bytes of memory on an 8-byte quantity
  - reserve 8 bytes before the pointer
    - first 4 of 8 bytes stores the size of the chunk allocated
      - users memory,
      - bookkeeping bytes, and
      - any padding
  
  Example:
    my_malloc(9990) will pad 9990 to a multiple of eight (9992) and 
    add 8 bytes for bookkeeping for a total of 10000 bytes. Since that
    is bigger than 8192, call sbrk(10000). Put the number 10000 at 
    address 0x10800 and return 0x10808 to the user.
*/

/* Singular global variable */
void *head = NULL;

typedef struct flist {
  int size;
  struct flist *flink;
  struct flist *blink;
} *Flist;

void *my_malloc(size_t size) {
  if(head == NULL) {
    if(size <= 8192) { size = 8192; }
    head = sbrk(size);
    printf("head: 0x%lx\n", (unsigned long) head);
  }
}

void my_free(void *ptr) {

}

void *free_list_begin() {

}

void *free_list_next(void *node) {

}

void coalesce_free_list() {

}

