#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <jrb.h>
#include <jval.h>
#include <fields.h>

#define BUF_SIZE 1012

typedef struct char_buffers {
  char buffer[BUF_SIZE];
  char fname[BUF_SIZE];
  char last_dir[BUF_SIZE];
} CBufs;

typedef struct byte_sizes {
  long in, mtime, fsize, last_mtime;
  u_int32_t itmp, last_mode; 
} BSizes;

typedef struct timeval timeval;

int slash(const char *fname) {
  for(int i = strlen(fname)-1; i >= 0; i--) {
    if(fname[i] == '/') { return i+1; }
  }
  return 0;
}

void set_time(char *fname, timeval *times, long *mtime) {
  times[0].tv_usec = 0;
  times[0].tv_sec = time(0);
  times[1].tv_usec = 0;
  times[1].tv_sec = *mtime;
  utimes(fname, times);
}

void all_info(char *fname, u_int32_t *itmp, long *in) {
  fread(fname, *itmp, 1, stdin);                /* Size of filename */
  fread(in, 8, 1, stdin);                       /* Name of file */
  fname[*itmp] = '\0';
}

void create_file(CBufs *cb, BSizes *bs) {
  FILE *file = fopen(cb->fname, "w+");
  fwrite(cb->buffer, bs->fsize, 1, file);
  chmod(cb->fname, bs->itmp);
  fclose(file);
}

void first_read(CBufs *cb, BSizes *bs, timeval *times) {
  /* Read mode and Mtime */
  fread(&(bs->itmp), 4, 1, stdin);              /* Mode */
  fread(&(bs->mtime), 8, 1, stdin);             /* Mtime */

  /* IF File */
  if(((bs->itmp) >> 15 & 1) == 1) {
    fread(&(bs->fsize), 8, 1, stdin);           /* Size of file */
    fread(cb->buffer, (bs->fsize), 1, stdin);   /* File's contents */
    create_file(cb, bs);                        /* Create file & contents */
  } 
  else {
    /*
    if(cb->last_dir[0] != '\0') {
      chmod(cb->last_dir, bs->last_mode);
      set_time(cb->last_dir, times, &(bs->last_mtime));
    }
    */

    mkdir(cb->fname, 16877);
    /*
    strcmp(cb->last_dir, cb->fname);
    bs->last_mode = bs->itmp;
    bs->last_mtime = bs->mtime;
    */
  }
  set_time(cb->fname, times, &(bs->mtime));
}


void read_tar() {
  CBufs cb;
  BSizes bs;
  JRB inodes, tmp;
  inodes = make_jrb();
  timeval times[2];

  /* Continously read through all of stdin */
  while(fread(&bs.itmp, 4, 1, stdin) > 0) {
    all_info(cb.fname, &bs.itmp, &bs.in);

    tmp = jrb_find_dbl(inodes, bs.in);
    if(tmp == NULL) {
      first_read(&cb, &bs, times);
      /* Add into list after */
      Jval j = new_jval_s(strdup(cb.fname));
      jrb_insert_dbl(inodes, bs.in, j);                    
    }
    else { link(tmp->val.s, cb.fname); }
  }
  /* Set all directory modes */
  /* Free JRB */
  jrb_traverse(tmp, inodes) { free(tmp->val.s); }
  jrb_free_tree(inodes);
}

int main() {
  read_tar();
  /* TODO Find a way to change working directories */
  return 0;
}