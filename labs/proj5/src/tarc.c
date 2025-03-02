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
#define ARR_SIZE 10

typedef struct string_vector {
  int len, size;
  char **arr;
} SV;

void free_SV(SV *sv) {
  for(int i = 0; i < sv->size; i++) {
    if(sv->arr[i] != NULL) free(sv->arr[i]);
  }
  free(sv->arr);
  free(sv);
}

SV* new_SV() {
  SV *sv = (SV*)malloc(sizeof(SV));
  sv->arr = (char**)malloc(ARR_SIZE * sizeof(char*));
  sv->len = ARR_SIZE;
  sv->size = 0;

  return sv;
}

void SV_append(SV *sv, char *str) {
  if(sv->size == sv->len) {
    sv->len += sv->len/2;
    char **temp = (char**)realloc(sv->arr, sv->len * sizeof(char*));
    if(temp == NULL) {
      fprintf(stderr, "Realloc Failed\n");
      exit(1);
    }
    sv->arr = temp;
  } 

  sv->arr[sv->size++] = str;
}

void dir_dfs(const char *dir_name) {
  DIR *d = opendir(dir_name);
  if(d == NULL) {
    perror(dir_name);
    exit(1);
  }

  struct dirent *de;
  struct stat buf;
  int exists;
  SV *sv = new_SV();  /* string vector */

  while((de = readdir(d)) != NULL) {
    /* n is name of current item in directory */
    char *n = de->d_name;
    /* skip if . or .. directory */
    if(strcmp(n, ".") == 0 || strcmp(n, "..") == 0) continue; 

    /* read in file/directory name */
    if(stat(n, &buf) < 0) {
      perror(n);
      fprintf(stderr, "Error reading in %s\n", n);
      exit(1);
    }

    if(S_ISDIR(buf.st_mode)) { SV_append(sv, strdup(n)); }  /* IF dir, add to vector to check later*/
    else { printf("FILE: %s\n", n); }               /* Print out file */

  }
  
  closedir(d);
  /* Index through directories*/
  for(int i = 0; i < sv->size; i++) {
    printf("%s\n", sv->arr[i]);
    dir_dfs(sv->arr[i]);
  }

  free_SV(sv);
}

int main(int argc, char **argv) {
  dir_dfs(".");
  return 0;
}