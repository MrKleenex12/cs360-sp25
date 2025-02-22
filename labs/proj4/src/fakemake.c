#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "fields.h"
#include "dllist.h"
#include "jval.h"

typedef unsigned long UL;

typedef struct makefile {
  Dllist list[4];
  /*  0 - C files
      1 - Header files
      2 - Flags
      3 - Libraries */
  char* exectuable;
} MF;

MF* create_make() {
  /* Take struct with default values and set MF pointer to them */
  MF *m = (MF*)malloc(sizeof(MF));  
  for(size_t i = 0; i < 4; i++) {
    m->list[i] = new_dllist();
  }
  return m;
}

void print(MF *m) {
  printf("Executable: %s\n", m->exectuable);
  Dllist tmp;
  for(size_t i = 0; i < 4; i++) {
    dll_traverse(tmp, m->list[i]) { printf("%s ", tmp->val.s); }
    printf("\n");
  }
}

void rm_make(MF *m) {
  /* Free everything in a MF */
  if(m->exectuable != NULL) { free(m->exectuable); }
  for(size_t i = 0; i < 4; i++) { free_dllist(m->list[i]); }
  free(m);
}

MF* read_file(IS is, unsigned char *foundE) {
  MF *m = create_make();
  Dllist tmp;
  /* Read descriptor file */
  while(get_line(is) >= 0) {
    if(is->NF == 0) { continue; }                 /* Skip if blank line */

    /* Source files */
    if(strcmp(is->fields[0], "C") == 0) { tmp = m->list[0];}
    /* Header files */
    else if(strcmp(is->fields[0], "H") == 0) { tmp = m->list[1];}
    /* Flags */
    else if(strcmp(is->fields[0], "F") == 0) { tmp = m->list[2]; }
    /* Libraries */
    else if(strcmp(is->fields[0], "L") == 0) { tmp = m->list[3];}
    /* Executable name */
    else if(strcmp(is->fields[0], "E") == 0) {
      *foundE = 1;
      m->exectuable = strdup(is->fields[1]);
      continue;
    }
    /* Add files into corresponding Dllist */
    for(int i = 1; i < is->NF; i++) {
      dll_append(tmp, new_jval_s(strdup(is->fields[i])));
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
  MF *m = read_file(is, &foundE);
  Dllist tmp;
  /* Check if executable was in file*/
  if(foundE == 0) {
    fprintf(stderr, "No Executable Found.\n");
    rm_make(m);
    jettison_inputstruct(is);
    return 1;
  }
  
  // print(m);
  UL htime = headers(m->list[1], tmp);
  printf("max header time: %ld\n", htime);
  sources(m->list[0], tmp, &htime);


  rm_make(m);
  jettison_inputstruct(is);
}