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

void read_dir(DIR *d, SV *sv, const char *dir_name, char *path) {
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
    else { printf("FILE: %s\n", path); }
  }
}

void open_dir(const char *dir_name, char *path) {
  printf("DIR: %s\n", dir_name);
  DIR *d = opendir(dir_name);
  if(d == NULL) {
    perror(dir_name);
    exit(1);
  }

  SV *str_vec = new_SV();           /* Hold names of directories */
  read_dir(d, str_vec, dir_name, path);   /* Print out files in directory */
  closedir(d);                      /* Close current directory */

  /* Recursively print files in other directories */
  for(int i = 0; i < str_vec->size; i++) {
    open_dir(str_vec->vector[i], path);
  }
  free_SV(str_vec);
}

int main(int argc, char **argv) {
  char *dir = (argc > 1) ? argv[1] : ".";
  char path[PATH_SIZE];
  open_dir(dir, path);
  return 0;
}