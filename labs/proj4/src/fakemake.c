#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fields.h"
#include "dllist.h"

typedef struct makefile {
  int C, F, H, L;
  char* exectuable;
} makefile;

makefile* create_make(makefile *def) {
  /* Take struct with default values and set makefile pointer to them */
  makefile *m = (makefile*)malloc(sizeof(makefile));  
  *m = *def;
  return m;
}

void rm_make(makefile *m) {
  if(m->exectuable != NULL) {
    free(m->exectuable);
  }
  free(m);
}

makefile* read_file(makefile *def, IS is, unsigned char *foundE) {
  makefile *m = create_make(def);

  /* Read files */
  while(get_line(is) >= 0) {
    /* Skip if blank line */
    if(is->NF == 0) {
      continue;
    }

    /* Reading in Lines based off first character */
    if(strcmp(is->fields[0], "C") == 0) {         /* Source files */
      (m->C)++;
    } else if(strcmp(is->fields[0], "H") == 0) {  /* Header files */
      (m->H)++;
    } else if(strcmp(is->fields[0], "F") == 0) {  /* Flags */
      (m->F)++;
    } else if(strcmp(is->fields[0], "L") == 0) {  /* Libraries */
      (m->L)++;
    } else if(strcmp(is->fields[0], "E") == 0) {  /* Executable name */
      *foundE = 1;
      m->exectuable = strdup(is->fields[1]);
    }
  }
  return m;
}

int main(int argc, char **argv) {
  /* Reading in through argv or stdin */
  IS is = (argc == 2) ? new_inputstruct(argv[1]) : new_inputstruct(NULL);
  if(!is && argc == 2) {
    perror(argv[1]);
    return 1;
  }
  
  unsigned char foundE = 0;
  makefile def = {0, 0, 0, 0, NULL};
  makefile *m = read_file(&def, is, &foundE);

  /* Check if executable was in file*/
  if(foundE == 0) {
    fprintf(stderr, "No Executable Found.\n");
    rm_make(m);
    jettison_inputstruct(is);
    return 1;
  } else {
    printf("%s\n", m->exectuable);
    printf("C:%d F:%d H:%d L:%d\n", m->C, m->F, m->H, m->L);
  }

  rm_make(m);
  jettison_inputstruct(is);
}