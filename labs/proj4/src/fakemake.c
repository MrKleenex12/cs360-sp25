#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "fields.h"
#include "dllist.h"
#include "jval.h"

typedef unsigned long UL;

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
  /* Read descriptor file */
  while(get_line(is) >= 0) {
    if(is->NF == 0) { continue; }                 /* Skip if blank line */

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

UL headers(Dllist d, Dllist tmp) {
  struct stat buf;
  UL htime = 0;
  int exists;
  /* Find time of most recently modified header file */
  dll_traverse(tmp, d) {
    exists = stat(tmp->val.s, &buf);
    if(exists < 0) {
      fprintf(stderr, "%s not available,\n", tmp->val.s);
      return -1;
    }
    /* Check if max time */
    if(buf.st_mtime > htime) { htime = buf.st_mtime; }
  }
  return htime;
}

char* ocopy(char *cfile) {
  char *ofile = strdup(cfile);
  ofile[strlen(ofile)-1] = 'o'; 
  return ofile;
}
/* TODO Process all C files
    - Print out C file needs to be recompiled if:
      - corresponding .o doesn't exist
      - existing .o file is less than C file or htime 
*/
void sources(Dllist d, Dllist tmp, const UL *htime) {
  struct stat buf;
  UL ctime, otime;
  int exists;
  char *cfile, *ofile;

  /* Check C files*/
  dll_traverse(tmp, d) {
    cfile = tmp->val.s;
    
    /* Find time of C file */
    exists = stat(cfile, &buf);
    if(exists < 0) {
      fprintf(stderr, "%s not available,\n", cfile);
      return;
    } else { ctime = buf.st_mtime; }
    /* Stat .o file corresponding with C file */
    ofile = ocopy(cfile);
    exists = stat(ofile, &buf);
    otime = buf.st_mtime;

    /* Check if C file needs to be recompiled*/
    if(exists < 0 || otime < ctime || otime < *htime) {
      printf("%s needs to be recompiled.\n", tmp->val.s);
    }
    free(ofile);
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
  Dllist tmp;
  /* Check if executable was in file*/
  if(foundE == 0) {
    fprintf(stderr, "No Executable Found.\n");
    rm_make(m);
    jettison_inputstruct(is);
    return 1;
  }
  
  // print(m);
  UL htime = headers(m->H, tmp);
  printf("max header time: %ld\n", htime);
  sources(m->C, tmp, &htime);


  rm_make(m);
  jettison_inputstruct(is);
}