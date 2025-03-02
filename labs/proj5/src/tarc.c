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

int main(int argc, char **argv) {
  DIR *d;
  d = opendir(".");
  if(d == NULL) {
    fprintf(stderr, "Couldn't open \".\"\n");
    exit(1);
  }

  struct stat buf;
  struct dirent *de;
  JRB files, tmp;
  int exists;
  int maxlen = 0;

  files = make_jrb();

  for(de = readdir(d); de != NULL; de = readdir(d)) {
    exists = stat(de->d_name, &buf);

    if(exists < 0) { fprintf(stderr, "%s not found\n", de->d_name); }
    else {
      jrb_insert_str(files, strdup(de->d_name), new_jval_l(buf.st_size));
      if(strlen(de->d_name) > maxlen) { maxlen = strlen(de->d_name); }
    }
  }
  closedir(d);

  jrb_traverse(tmp, files) {
    printf("%*s -  %ld\n", maxlen, tmp->key.s, tmp->val.l);
    free(tmp->key.s);
  }

  jrb_free_tree(files); 
  return 0;
}