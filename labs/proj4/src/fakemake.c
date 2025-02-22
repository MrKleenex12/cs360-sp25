#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "fields.h"
#include "dllist.h"
#include "jval.h"

typedef struct makefile {
  Dllist C, H, F, L;
  char* exectuable;
} makefile;

makefile* create_make() {
  /* Take struct with default values and set makefile pointer to them */
  makefile *m = (makefile*)malloc(sizeof(makefile));  
  m->C = new_dllist();
  m->H = new_dllist();
  m->F = new_dllist();
  m->L = new_dllist();
  return m;
}

void print(makefile *m) {
  printf("Executable: %s\n", m->exectuable);
  Dllist tmp;
  dll_traverse(tmp, m->C) { printf("%s ", tmp->val.s); }
  printf("\n");
  dll_traverse(tmp, m->H) { printf("%s ", tmp->val.s); }
  printf("\n");
  dll_traverse(tmp, m->F) { printf("%s ", tmp->val.s); }
  printf("\n");
  dll_traverse(tmp, m->L) { printf("%s ", tmp->val.s); }
  printf("\n");
}

void rm_make(makefile *m) {
  if(m->exectuable != NULL) {
    free(m->exectuable);
  }
  free_dllist(m->C);
  free_dllist(m->H);
  free_dllist(m->F);
  free_dllist(m->L);
  free(m);
}

makefile* read_file(IS is, unsigned char *foundE) {
  makefile *m = create_make();

  /* Read files */
  while(get_line(is) >= 0) {
    /* Skip if blank line */
    if(is->NF == 0) {
      continue;
    }

    /* Reading in Lines based off first character */
    if(strcmp(is->fields[0], "C") == 0) {         /* Source files */
      for(int i = 1; i < is->NF; i++) {
        dll_append(m->C, new_jval_s(strdup(is->fields[i])));
      }
    } else if(strcmp(is->fields[0], "H") == 0) {  /* Header files */
      for(int i = 1; i < is->NF; i++) {
        dll_append(m->H, new_jval_s(strdup(is->fields[i])));
      }
    } else if(strcmp(is->fields[0], "F") == 0) {  /* Flags */
      for(int i = 1; i < is->NF; i++) {
        dll_append(m->F, new_jval_s(strdup(is->fields[i])));
      }
    } else if(strcmp(is->fields[0], "L") == 0) {  /* Libraries */
      for(int i = 1; i < is->NF; i++) {
        dll_append(m->L, new_jval_s(strdup(is->fields[i])));
      }
    } else if(strcmp(is->fields[0], "E") == 0) {  /* Executable name */
      *foundE = 1;
      m->exectuable = strdup(is->fields[1]);
    }
  }
  return m;
}

/* TODO Process Header files */
void headers(Dllist *d) {
  Dllist tmp;  
  struct stat buf;
  int exists;

  dll_traverse(tmp, (*d)) {
    exists = stat(tmp->val.s, &buf);
    if(exists < 0) {
      fprintf(stderr, "%s not available,\n", tmp->val.s);
      return;
    }
    printf("%15ld %s\n", buf.st_mtime, tmp->val.s);
  }
}

int main(int argc, char **argv) {
  /* Reading in through argv or stdin */
  IS is = (argc == 2) ? new_inputstruct(argv[1]) : new_inputstruct(NULL);
  if(!is && argc == 2) {
    perror(argv[1]);
    return 1;
  }
  
  unsigned char foundE = 0;
  makefile *m = read_file(is, &foundE);

  /* Check if executable was in file*/
  if(foundE == 0) {
    fprintf(stderr, "No Executable Found.\n");
    rm_make(m);
    jettison_inputstruct(is);
    return 1;
  }
  
  print(m);
  headers(&(m->H));

  rm_make(m);
  jettison_inputstruct(is);
}