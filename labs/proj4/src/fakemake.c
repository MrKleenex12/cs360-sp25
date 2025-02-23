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
  Dllist recompiled;  /* list of c files that need to be recompiled */
  JRB ctoo;           /* c to object files JRB */
  char* exectuable;
} MF;

void print(MF *m) {
  printf("Executable: %s\n", m->exectuable);
  Dllist tmp;
  for(size_t i = 0; i < 4; i++) {
    dll_traverse(tmp, m->list[i]) { printf("%s ", tmp->val.s); }
    printf("\n");
  }
}

MF* create_make() {
  /* Malloc and set default values for MF */
  MF *m = (MF*)malloc(sizeof(MF));  
  for(size_t i = 0; i < 4; i++) { m->list[i] = new_dllist(); }
  m->recompiled = new_dllist();
  m->ctoo = make_jrb();
  return m;
}

/* Free everything in a MF */
void rm_make(MF *m, Dllist tmp) {
  /* Free executable */
  if(m->exectuable != NULL) { free(m->exectuable); }
  /* Free all file lists */
  for(size_t i = 0; i < 4; i++) {
    dll_traverse(tmp, m->list[i]) { free(tmp->val.s); }
    free_dllist(m->list[i]);
  }
  /* Free other stuff */
  free_dllist(m->recompiled);
  jrb_free_tree(m->ctoo);
  free(m);
}

void delete_everything(MF *m, Dllist tmp, IS is) {
  rm_make(m, tmp);
  jettison_inputstruct(is);
}

JRB map(MF *m) {
  JRB map = make_jrb();
  jrb_insert_str(map, "C", new_jval_v((void*)m->list[0]));
  jrb_insert_str(map, "H", new_jval_v((void*)m->list[1]));
  jrb_insert_str(map, "F", new_jval_v((void*)m->list[2]));
  jrb_insert_str(map, "L", new_jval_v((void*)m->list[3]));
  return map;
}

MF* read_file(IS is, char *foundE) {
  MF *m = create_make();
  JRB file_map = map(m);
  Dllist list;

  while(get_line(is) >= 0) {
    /* Skip if blank line */
    if(is->NF == 0) { continue; }
    char *letter = is->fields[0];
    /* If not E, add list into dllist */
    if(strcmp(letter, "E") != 0) {
      list = (Dllist)jrb_find_str(file_map, letter)->val.v;
      for(int i = 1; i < is->NF; i++) {
        dll_append(list, new_jval_s(strdup(is->fields[i])));
      }
    } else {
      /* Set executable */
      *foundE = 1;
      m->exectuable = strdup(is->fields[1]);
    }
  }
  jrb_free_tree(file_map);
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

char* ofile_copy(char *cfile) {
  char *ofile = strdup(cfile);
  ofile[strlen(ofile)-1] = 'o'; 
  return ofile;
}

int sources(MF *m, Dllist tmp, const UL *htime) {
  struct stat buf;
  UL ctime, otime;
  int exists;
  char *cfile, *ofile;

  /* Check C files*/
  dll_traverse(tmp, m->list[0]) {
    cfile = tmp->val.s;
    
    /* Find time of C file */
    exists = stat(cfile, &buf);
    if(exists >= 0) { ctime = buf.st_mtime;}
    else {
      fprintf(stderr, "%s not available,\n", cfile);
      return 1;
    }
    /* Stat .o file corresponding with C file */
    ofile = ofile_copy(cfile);
    exists = stat(ofile, &buf);
    otime = buf.st_mtime;
    jrb_insert_str(m->ctoo, cfile, new_jval_s(ofile));
    /* Check if C file needs to be recompiled*/
    if(exists < 0 || otime < ctime || otime < *htime) {
      // printf("%s needs to be recompiled.\n", tmp->val.s);
      dll_append(m->recompiled, new_jval_s(cfile));
    }
  }

  return 0;
}

char* base_call(Dllist flist, Dllist tmp, u_int32_t *len) {
  *len = 6;
  /* Add strlen of flags */
  dll_traverse(tmp, flist) { *len += strlen(tmp->val.s)+1; }
  char *call = (char*)malloc((*len+1) * sizeof(char));
  strcpy(call, "gcc -c");
  *len = 6;
  /* Copy all flags into call */
  dll_traverse(tmp, flist) {
    call[*len] = ' ';
    strcpy(call+*len+1, tmp->val.s);
    *len += strlen(call+*len);
  }
  return call;
}

void compile_objs(MF *m, Dllist tmp) {
  u_int32_t len = 0;
  char *call = base_call(m->list[2], tmp, &len);
  /* Call all files that need to be recompiled */
  dll_traverse(tmp, m->recompiled) {
    /* Realloc space for call and create string */
    call = realloc(call, len+2+strlen(tmp->val.s));
    call[len] = ' ';
    strcpy(call+len+1, tmp->val.s);
    /* System call to make files */
    system(call);
    printf("%s\n", call);
  }
  free(call);
}

int main(int argc, char **argv) {
  /* Reading in through argv or stdin */
  IS is = (argc == 2) ? new_inputstruct(argv[1]) : new_inputstruct(NULL);
  if(!is && argc == 2) {
    perror(argv[1]);
    return 1;
  }
  
  char foundE = 0;
  MF *m = read_file(is, &foundE);
  Dllist tmp;
  /* Check if executable was in file*/
  if(foundE == 0) {
    fprintf(stderr, "No Executable Found.\n");
    delete_everything(m, tmp, is);
  }
  
  UL htime = headers(m->list[1], tmp);
  int result = sources(m, tmp, &htime);
  if(result) {
    fprintf(stderr, "Error reading source files\n");
    delete_everything(m, tmp, is);
    return 1;
  }

  compile_objs(m, tmp);
  
  JRB j;
  jrb_traverse(j, m->ctoo) {
    free(j->val.s);
  }
  delete_everything(m, tmp, is);
  return 0;
}