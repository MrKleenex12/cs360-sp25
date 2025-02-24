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
  printf("executable: %s\n", m->exectuable);
  Dllist tmp;
  for(size_t i = 0; i < 4; i++) {
    dll_traverse(tmp, m->list[i]) { printf("%s ", tmp->val.s); }
    printf("\n");
  }
}

MF* new_make() {
  /* Malloc and set default values for MF */
  MF *m = (MF*)malloc(sizeof(MF));  
  m->exectuable = NULL;
  for(size_t i = 0; i < 4; i++) { m->list[i] = new_dllist(); }
  m->recompiled = new_dllist();
  m->ctoo = make_jrb();
  return m;
}

/* Free everything in a MF */
void delete_make(MF *m, Dllist tmp, JRB j) {
  /* Free executable */
  if(m->exectuable != NULL) { free(m->exectuable); }
  /* Free all file lists */
  for(size_t i = 0; i < 4; i++) {
    dll_traverse(tmp, m->list[i]) { free(tmp->val.s); }
    free_dllist(m->list[i]);
  }
  jrb_traverse(j, m->ctoo) { free(j->val.s); }

  /* Free other stuff */
  free_dllist(m->recompiled);
  jrb_free_tree(m->ctoo);
  free(m);
}

void delete_everything(MF *m, Dllist tmp, JRB j, IS is) {
  delete_make(m, tmp, j);
  jettison_inputstruct(is);
}

JRB make_map(MF *m) {
  JRB make_map = make_jrb();
  jrb_insert_str(make_map, "C", new_jval_v((void*)m->list[0]));
  jrb_insert_str(make_map, "H", new_jval_v((void*)m->list[1]));
  jrb_insert_str(make_map, "F", new_jval_v((void*)m->list[2]));
  jrb_insert_str(make_map, "L", new_jval_v((void*)m->list[3]));
  return make_map;
}

int read_file(IS is, MF **m) {
  *m = new_make();
  JRB file_map = make_map(*m);
  Dllist list;
  int foundE = 0;

  while(get_line(is) >= 0) {
    /* Skip if blank line */
    if(is->NF == 0) { continue; }
    char *letter = is->fields[0];
    if(strlen(letter) > 1) {
      jrb_free_tree(file_map);
      return -1;
    }
    /* If not E, add list into dllist */
    if(strcmp(letter, "E") != 0) {
      list = (Dllist)jrb_find_str(file_map, letter)->val.v;
      for(int i = 1; i < is->NF; i++) {
        dll_append(list, new_jval_s(strdup(is->fields[i])));
      }
    } else { /* If E */
      foundE = 1;
      (*m)->exectuable = strdup(is->fields[1]);
    }
  }
  jrb_free_tree(file_map);
  return foundE;
}

UL H_check(Dllist d, Dllist tmp) {
  struct stat buf;
  UL max_time = 0;
  int exists;
  /* Find time of most recently modified header file */
  dll_traverse(tmp, d) {
    exists = stat(tmp->val.s, &buf);
    if(exists < 0) {
      fprintf(stderr, "%s not available,\n", tmp->val.s);
      return -1;
    }
    /* Check if max time */
    if(buf.st_mtime > max_time) { max_time = buf.st_mtime; }
  }
  return max_time;
}

char* to_obj(char *cfile) {
  char *ofile = strdup(cfile);
  ofile[strlen(ofile)-1] = 'o'; 
  return ofile;
}

int S_check(MF *m, Dllist tmp, const UL *htime) {
  struct stat buf;
  UL ctime, otime;
  int exists;
  int nfiles = 0;
  char *cfile, *ofile;

  /* Check C files*/
  dll_traverse(tmp, m->list[0]) {
    cfile = tmp->val.s;
    
    /* Find time of C file */
    exists = stat(cfile, &buf);
    if(exists >= 0) { ctime = buf.st_mtime;}
    else {
      fprintf(stderr, "%s not available,\n", cfile);
      return -1;
    }
    /* Stat .o file corresponding with C file */
    ofile = to_obj(cfile);
    exists = stat(ofile, &buf);
    otime = buf.st_mtime;
    jrb_insert_str(m->ctoo, cfile, new_jval_s(ofile));
    /* Check if C file needs to be recompiled*/
    if(exists < 0 || otime < ctime || otime < *htime) {
      dll_append(m->recompiled, new_jval_s(cfile));
      nfiles++; 
    }
  }

  return nfiles;
}

char* base_call(Dllist flist, Dllist tmp, u_int32_t *len, const char *base) {
  *len = strlen(base);
  /* Add strlen of flags */
  dll_traverse(tmp, flist) { *len += strlen(tmp->val.s)+1; }
  char *call = (char*)malloc((*len+1) * sizeof(char));
  strcpy(call, base);
  *len = 6;
  /* Copy all flags into call */
  dll_traverse(tmp, flist) {
    call[*len] = ' ';
    strcpy(call+*len+1, tmp->val.s);
    *len += strlen(call+*len);
  }
  return call;
}

void O_compile(MF *m, Dllist tmp) {
  u_int32_t len = 0;
  char *call = base_call(m->list[2], tmp, &len, "gcc -c");
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

UL O_check(MF *m, JRB tmp) {
  struct stat buf;
  int exists;
  UL max_time = 0;

  /* Find Most recent obj file */
  jrb_traverse(tmp, m->ctoo) {
    exists = stat(tmp->val.s, &buf);
    if(exists < 0) {
      fprintf(stderr, "%s doesn't exist\n", tmp->val.s);
      return 1;
    }
    if(buf.st_mtime > max_time) { max_time = buf.st_mtime; }
  }
  return max_time;
}

void E_compile() {

}

void E_check(MF *m, const UL obj_time) {
  struct stat buf;
  UL exe_time;
  int exists;

  exists = stat(m->exectuable, &buf);
  exe_time = buf.st_mtime;
  if(exists < 0 || exe_time < obj_time) {
    printf("Need to compile %s\n", m->exectuable);
  }
}

int main(int argc, char **argv) {
  /* Reading in through argv or stdin */
  IS is = (argc == 2) ? new_inputstruct(argv[1]) : new_inputstruct(NULL);
  if(!is && argc == 2) {
    perror(argv[1]);
    return 1;
  }
  
  MF *m;
  Dllist tmp;
  JRB j;

  int result = read_file(is, &m);         /* Check if executable was in file*/
  /* Error Check */
  if(result == -1 || result == 0) {
    char *str = (result == 0) ? "No executable Found" : "Bad File";
    fprintf(stderr, "%s\n", str);
    delete_everything(m, tmp, j, is);
    return 1;
  }

  UL htime = H_check(m->list[1], tmp);    /* Find time header was updated */
  int ncfiles = S_check(m, tmp, &htime);  /* Number of C files need to be recompiled */

  /* Error Check */
  if(ncfiles == -1) {
    fprintf(stderr, "Error reading source files\n");
    delete_everything(m, tmp, j, is);
    return 1;
  } else if(ncfiles != 0) { O_compile(m, tmp); }

  UL max_obj_time = O_check(m, j);        /* Find time most recent obj was updated*/

  /* Error Check */
  if(max_obj_time == 1) { return 1; }

  E_check(m, max_obj_time);
   
  delete_everything(m, tmp, j, is);
  return 0;
}