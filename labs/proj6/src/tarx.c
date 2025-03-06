#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <jrb.h>
#include <jval.h>

#define BUF_SIZE 8192

typedef struct char_buffers {
  char buffer[BUF_SIZE];
  char name[BUF_SIZE];
} Cbufs;

typedef struct byte_sizes {
  long in, mtime, fsize;
  u_int32_t itmp; 
} BSizes;

typedef struct timeval timeval;

/*  Order of bytes:

  - size of filename  - 4B int
  - file's name without NULL char
  - file's inode      - 8B long

  IF FIRST TIME READING
  - file's mode       - 4B int
  - file's mod time   - 8B long

    IF FILE & NOT DIR
    - file's size     - 8B long
    - file's bytes (contents)
*/

void set_time(char *name, timeval *times, long *mtime) {
  times[0].tv_usec = 0;
  times[0].tv_sec = time(0);
  times[1].tv_usec = 0;
  times[1].tv_sec = *mtime;
}

void general_info(char *name, u_int32_t *itmp, long *in) {
  fread(name, *itmp, 1, stdin);
  name[*itmp] = '\0';
  printf("Name: %s\n", name);
  fread(in, 8, 1, stdin);
  printf("Inode %ld ", *in);
}

void first_read(Cbufs *cb, BSizes *bs, timeval *times) {
  fread(&(bs->itmp), 4, 1, stdin);
  printf("Mode %o ", bs->itmp);
  fread(&(bs->mtime), 8, 1, stdin);
  printf("Mtime %ld", bs->mtime);
  /* IF File and not Directory */
  if(((bs->itmp) >> 15 & 1) == 1) {
    fread(&(bs->fsize), 8, 1, stdin);
    fread(cb->buffer, (bs->fsize), 1, stdin);
  } else {
    mkdir(cb->name, (bs->itmp));
    set_time(cb->name, times, &(bs->mtime));
  }
}

void read_tar() {
  Cbufs cb;
  BSizes bs;
  JRB inodes, tmp;
  inodes = make_jrb();
  timeval times[2];

  while(fread(&bs.itmp, 4, 1, stdin) > 0) {
    /* All files */
    general_info(cb.name, &bs.itmp, &bs.in);

    /* IF first time seeing inode */
    tmp = jrb_find_dbl(inodes, bs.in);
    if(tmp == NULL) {
      first_read(&cb, &bs, times);                         /* Read in mode and mtime */
      jrb_insert_dbl(inodes, bs.in, new_jval_i(0)); /* Add into list after */
    }

    printf("\n\n");
  }

  jrb_free_tree(inodes);
}

int main() {
  read_tar();
  return 0;
}