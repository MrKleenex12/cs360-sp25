#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include "jrb.h"
#include "jval.h"

#define BUF_SIZE 4096
#define INIT_ARR_SIZE 4

typedef struct timeval timeval;

typedef struct global_vars {
  char buffer[BUF_SIZE];
  char fname[BUF_SIZE];
  long in, mtime, fsize;
  u_int32_t itmp; 
} Gvars;

/****************************** My own vector ******************************/
typedef struct directory_node {
  char *dir_name;
  long mtime;
  u_int32_t mode;
} Dnode; 

typedef struct DN_vector {
  int length;   /* Total Length Allocated */
  int size;     /* Size of items in vector */
  Dnode **vector;
} DNV;

void free_DNV(DNV *dnv) {
  for(int i = 0; i < dnv->size; i++) {
    free(dnv->vector[i]->dir_name);
    free(dnv->vector[i]);
  }
  free(dnv->vector);
  free(dnv);
}

DNV* make_DNV() {
  DNV *dnv = (DNV*)malloc(sizeof(DNV));
  dnv->vector = (Dnode**)malloc(INIT_ARR_SIZE * sizeof(Dnode*));
  dnv->length = INIT_ARR_SIZE;
  dnv->size = 0;
  return dnv;
}

void DN_append(DNV *dnv, Dnode *dir_node) {
  /* Resize if vector full */
  if(dnv->size == dnv->length) {
    /* 1.5 times larger */
    dnv->length += dnv->length/2;
    /* Reallocate space for bigger vector */
    Dnode **temp = (Dnode**)realloc(dnv->vector, dnv->length * sizeof(Dnode*));
    if(temp == NULL) {
      fprintf(stderr, "Realloc Failed\n");
      exit(1);
    }
    dnv->vector = temp;
  } 
  /* Append string to vector */
  dnv->vector[dnv->size++] = dir_node;
}


void set_time(char *fname, long *mtime) {
  timeval t[2] = {{0,0}, {*mtime,0}};
  utimes(fname, t);
}

void general_info(Gvars *gv) {
  size_t nobjects;

  /* Name of file */
  if((nobjects = fread(gv->fname, 1, gv->itmp, stdin)) < gv->itmp) {
    fprintf(stderr, "Erro\n");
    exit(1);
  }
  /* Inode */
  if((nobjects = fread(&(gv->in), 1, 8, stdin)) < 8) {
    fprintf(stderr, "Erro\n");
    exit(1);
  }
  gv->fname[gv->itmp] = '\0';
}

void create_file(Gvars *gv) {
  FILE *file;
  size_t nobjects;

  file = fopen(gv->fname, "wx");
  if(file == NULL) {
    perror(gv->fname);
    exit(1);
  }
  nobjects = fwrite(gv->buffer, 1, gv->fsize, file);
  if(nobjects < gv->fsize) {
    fprintf(stderr, "Cannot write to %s\n", gv->fname);
    exit(1);
  }
  chmod(gv->fname, gv->itmp);
  fclose(file);
}

Dnode* new_DN(char *dir_name, u_int32_t *mode, long *mtime) {
  Dnode *dn = (Dnode*)malloc(sizeof(Dnode));
  dn->dir_name = strdup(dir_name);
  dn->mode = *mode;
  dn->mtime = *mtime;

  return dn;
}

void first_read(Gvars *gv, DNV *dnv) {
  size_t nobjects;
  /* Read mode and Mtime */
  if((nobjects = fread(&(gv->itmp), 1, 4, stdin)) < 4) {
    fprintf(stderr, "Erro\n");
    exit(1);
  }
  if((nobjects = fread(&(gv->mtime), 1,  8, stdin)) < 8) {
    fprintf(stderr, "Erro\n");
    exit(1);
  } 

  /* IF File */
  if(((gv->itmp) >> 15 & 1) == 1) {
    /* Size of file */
    if((nobjects = fread(&(gv->fsize), 1, 8, stdin)) < 8) {
      fprintf(stderr, "Erro\n");
      exit(1);
    }
    /* File's contents */
    if((nobjects = fread(gv->buffer, 1, gv->fsize, stdin)) < gv->fsize) {
      fprintf(stderr, "Erro\n");
      exit(1);
    }

    /* Create file & contents */
    create_file(gv);
    set_time(gv->fname, &(gv->mtime));
  } 
  else {
    mkdir(gv->fname, 16877);
    Dnode *dn = new_DN(gv->fname, &(gv->itmp), &(gv->mtime));
    DN_append(dnv, dn);
  }
}

void correct_dirs(DNV *dnv) {
  for(int i = dnv->size-1; i >= 0; i--) {
    Dnode *dn = dnv->vector[i];
    chmod(dn->dir_name, dn->mode);
    set_time(dn->dir_name, &(dn->mtime));
  }
  free_DNV(dnv);
}

int main() {
  Gvars gv;
  JRB inodes, tmp;
  inodes = make_jrb();
  DNV *dnv = make_DNV();
  size_t bytes_read;

  /* Continously read through all of stdin */
  while((bytes_read = fread(&gv.itmp, 1, 4, stdin)) == 4) {
    general_info(&gv);

    tmp = jrb_find_dbl(inodes, gv.in);
    if(tmp == NULL) {
      first_read(&gv, dnv);
      /* Add into list after */
      Jval j = new_jval_s(strdup(gv.fname));
      jrb_insert_dbl(inodes, gv.in, j);                    
    }
    else { link(tmp->val.s, gv.fname); }
  }

  if(bytes_read > 0) {
    fprintf(stderr, "Error Reading bits\n");
    exit(1);
  }

  correct_dirs(dnv); 
  /* Free JRB */
  jrb_traverse(tmp, inodes) { free(tmp->val.s); }
  jrb_free_tree(inodes);
  return 0;
}