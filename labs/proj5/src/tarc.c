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

typedef struct char_vec {
  int size;
} CV;

void dir_dfs(const char *dir_name) {
  DIR *d = opendir(dir_name);
  if(d == NULL) {
    perror(dir_name);
    exit(1);
  }

  struct dirent *de;
  struct stat buf;
  int exists;
  Dllist dirs, tmp;
  dirs = new_dllist();

  while((de = readdir(d)) != NULL) {
    char *n = de->d_name;
    if(strcmp(n, ".") == 0 || strcmp(n, "..") == 0) continue; 

    if(stat(n, &buf) == -1) {
      perror(n);
      exit(1);
    }

    /* Add to directory list to check later */
    if(S_ISDIR(buf.st_mode)) { dll_append(dirs, new_jval_s(n)); }
    /* Print out file */
    else { printf("FILE: %s\n", n); }
  }

  dll_traverse(tmp, dirs) {
    printf("%s\n", tmp->val.s);
  }

  free_dllist(dirs);
  closedir(d);
}

int main(int argc, char **argv) {
  dir_dfs(".");
  return 0;
}