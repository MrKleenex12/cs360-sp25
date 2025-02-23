#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "fields.h"
#include "dllist.h"
#include "jval.h"
#include "jrb.h"

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

void rm_make(MF *m, Dllist tmp) {
  /* Free everything in a MF */
  if(m->exectuable != NULL) { free(m->exectuable); }
  for(size_t i = 0; i < 4; i++) {
    dll_traverse(tmp, m->list[i]) { free(tmp->val.s); }
    free_dllist(m->list[i]);
  }
  free(m);
}

JRB map(MF *m) {
  JRB map = make_jrb();
  jrb_insert_str(map, "C", new_jval_v((void*)m->list[0]));
  jrb_insert_str(map, "H", new_jval_v((void*)m->list[1]));
  jrb_insert_str(map, "F", new_jval_v((void*)m->list[2]));
  jrb_insert_str(map, "L", new_jval_v((void*)m->list[3]));
  return map;
}

MF* read_file(IS is, unsigned char *foundE) {
  MF *m = create_make();
  JRB letter = map(m);
  Dllist list;
  /* Read descriptor file */
  while(get_line(is) >= 0) {
    if(is->NF == 0) { continue; }                 /* Skip if blank line */

    if(strcmp(is->fields[0], "E") == 0) {
      *foundE = 1;
      m->exectuable = strdup(is->fields[1]);
    } else {
      list = (Dllist)jrb_find_str(letter, is->fields[0])->val.v;
      /* Add files into corresponding Dllist */
      for(int i = 1; i < is->NF; i++) {
        dll_append(list, new_jval_s(strdup(is->fields[i])));
      }
    }
  }
  jrb_free_tree(letter);
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
    rm_make(m, tmp);
    jettison_inputstruct(is);
    return 1;
  }
  
  // print(m);
  UL htime = headers(m->list[1], tmp);
  sources(m->list[0], tmp, &htime);

  rm_make(m, tmp);
  jettison_inputstruct(is);
}