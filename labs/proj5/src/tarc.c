#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include "fields.h"
#include "jval.h"
#include "jrb.h"
#include "dllist.h"
#include "sys/stat.h"

#define BUF_SIZE 8192
#define PATH_SIZE 512 
#define INIT_ARR_SIZE 4 

typedef struct string_vector {
  int length;   /* Total Length Allocated */
  int size;     /* Size of items in vector */
  char **vector;
} SV;

void free_SV(SV *sv) {
  for(int i = 0; i < sv->size; i++) { free(sv->vector[i]); }
  free(sv->vector);
  free(sv);
}

SV* new_SV() {
  SV *sv = (SV*)malloc(sizeof(SV));
  sv->vector = (char**)malloc(INIT_ARR_SIZE * sizeof(char*));
  sv->length = INIT_ARR_SIZE;
  sv->size = 0;

  return sv;
}

void SV_append(SV *sv, char *str) {
  /* Resize if vector full */
  if(sv->size == sv->length) {
    /* 1.5 times larger */
    sv->length += sv->length/2;
    /* Reallocate space for bigger vector */
    char **temp = (char**)realloc(sv->vector, sv->length * sizeof(char*));
    if(temp == NULL) {
      fprintf(stderr, "Realloc Failed\n");
      exit(1);
    }
    sv->vector = temp;
  } 

  sv->vector[sv->size++] = strdup(str);  /* Append string to vector */
}

void process(struct stat *buf, const char *name, const char is_file, JRB printed) {
  printf("size of file name: %lu\n", strlen(name));
  printf("name: %s\n", name);
  printf("inode: %llu\n", buf->st_ino);

  if(printed == NULL) {
    if(is_file == 1) {
      printf("file size: %lld\n", buf->st_size);
      FILE *file = fopen(name, "r"); 
      if(file == NULL) {
        perror(name);
        exit(1);
      }
      int c;
      while((c = fgetc(file)) != EOF) {
        putchar(c);
      }
      fclose(file);
    }
  }
  printf("\n");
}

void read_dir(DIR *d, SV *sv, const char *dir_name, char *path, JRB list) {
  struct dirent *file;
  struct stat buf;

  while((file = readdir(d)) != NULL) {
    char *f = file->d_name;
    /* skip if . or .. directory */
    if(strcmp(f, ".") == 0 || strcmp(f, "..") == 0) continue; 
    /* Add file to path name */
    snprintf(path, PATH_SIZE, "%s/%s", dir_name, f);

    if(stat(path, &buf) < 0) {      /* read in filename */
      perror(path);
      exit(1);
    }

    /* If directory, add to vector to check later*/
    if(S_ISDIR(buf.st_mode)) { SV_append(sv, path); }
    else {
      JRB tmp = jrb_find_int(list, buf.st_ino);
      process(&buf, path, 1, tmp);
      if(tmp == NULL) { jrb_insert_int(list, buf.st_ino, new_jval_i(0)); }
    }
  }
}

void open_dir(const char *dir_name, char *path, JRB list) {
  // printf("DIR: %s\n", dir_name);
  DIR *d = opendir(dir_name);
  if(d == NULL) {
    perror(dir_name);
    exit(1);
  }

  struct stat buf;
  if(stat(dir_name, &buf) < 0) {
    perror(dir_name);
    exit(1);
  }

  JRB tmp = jrb_find_int(list, buf.st_ino);
  process(&buf, dir_name, 0, tmp);
  if(tmp == NULL) { jrb_insert_int(list, buf.st_ino, new_jval_i(0)); }


  SV *str_vec = new_SV();               /* Hold names of directories */
  read_dir(d, str_vec, dir_name, path, list); /* Print out files in directory */
  closedir(d);                          /* Close current directory */

  /* Recursively print files in other directories */
  for(int i = 0; i < str_vec->size; i++) {
    open_dir(str_vec->vector[i], path, list);
  }
  free_SV(str_vec);
}

int main(int argc, char **argv) {
  char *dir = (argc > 1) ? argv[1] : ".";
  char path[PATH_SIZE];
  JRB inodes;
  inodes = make_jrb();

  open_dir(dir, path, inodes);
  return 0;
}