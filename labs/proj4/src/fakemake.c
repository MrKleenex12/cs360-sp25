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
  Dllist objs;  /* list of o files */
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
  m->objs = new_dllist();
  return m;
}

/* Free everything in a MF */
void delete_make(MF *m, Dllist tmp) {
  /* Free executable */
  if(m->exectuable != NULL) { free(m->exectuable); }
  /* Free all file lists */
  for(size_t i = 0; i < 4; i++) {
    dll_traverse(tmp, m->list[i]) { free(tmp->val.s); }
    free_dllist(m->list[i]);
  }

  /* Free other stuff */
  dll_traverse(tmp, m->objs) { free(tmp->val.s); }
  free_dllist(m->objs);
  free(m);
}

void delete_everything(MF *m, Dllist tmp, IS is) {
  delete_make(m, tmp);
  jettison_inputstruct(is);
}

JRB make_map(MF *m) {
  JRB make_map = make_jrb();
  jrb_insert_str(make_map, "C", new_jval_i(0));
  jrb_insert_str(make_map, "H", new_jval_i(1));
  jrb_insert_str(make_map, "F", new_jval_i(2));
  jrb_insert_str(make_map, "L", new_jval_i(3));
  return make_map;
}

int read_file(IS is, MF *m) {
  JRB file_map = make_map(m);
  int index;
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
      index = (int)jrb_find_str(file_map, letter)->val.i;
      for(int i = 1; i < is->NF; i++) {
        dll_append(m->list[index], new_jval_s(strdup(is->fields[i])));
      }
    } else { /* If E */
      foundE = 1;
      m->exectuable = strdup(is->fields[1]);
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

char* new_call(char *call, u_int32_t *len, const char *str) {
  call = realloc(call, *len+2+strlen(str));
  call[*len] = ' ';
  strcpy(call+*len+1, str);
  *len += (1+strlen(str));
  // printf("%s - %d\n", call, *len);
  return call;
}

char* base_call(Dllist flist, Dllist tmp, u_int32_t *len, const char *base) {
  *len = strlen(base);
  /* Add strlen of flags */
  dll_traverse(tmp, flist) { *len += strlen(tmp->val.s)+1; }
  char *call = (char*)malloc((*len+1) * sizeof(char));
  strcpy(call, base);
  *len = strlen(call);
  /* Copy all flags into call */
  dll_traverse(tmp, flist) {
    call = new_call(call, len, tmp->val.s);
  }
  return call;
}

void O_compile(MF *m, Dllist tmp, char *cfile) {
  u_int32_t len = 0;
  char *call = base_call(m->list[2], tmp, &len, "gcc -c");

  call = new_call(call, &len, cfile);       /* Realloc space for call and create string */
  system(call);                             /* System call to make files */
  printf("%s\n", call);

  free(call);
}

int S_check(MF *m, Dllist tmp, const UL *max_htime, UL *max_obj_time) {
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
      return -1;
    }
    /* Stat .o file corresponding with C file */
    ofile = to_obj(cfile);
    exists = stat(ofile, &buf);
    otime = buf.st_mtime;
    if(otime > *max_obj_time) { *max_obj_time = otime; }
    /* Check if C file needs to be recompiled*/
    if(exists < 0 || otime < ctime || otime < *max_htime) {
      dll_append(m->objs, new_jval_s(ofile));
      O_compile(m, tmp, cfile);
    }
    else { free(ofile); }
  }

  return 0;
}

void E_compile(MF *m, Dllist tmp) {
  u_int32_t len = 0;
  char *call = base_call(m->list[2], tmp, &len, "gcc");   /* Add flags before -o */
  call = new_call(call, &len, "-o");                      /* Add -o */ 
  call = new_call(call, &len, m->exectuable);             /* Add executable */
  /* Add all obj files to call */
  dll_traverse(tmp, m->objs) { call = new_call(call, &len, tmp->val.s); } 
  /* Add any libraries */
  dll_traverse(tmp, m->list[3]) { call = new_call(call, &len, tmp->val.s); }
  printf("%s\n", call);
  system(call);                                           /* Make executable */
  free(call);
}

void E_check(MF *m, Dllist tmp, const UL obj_time) {
  struct stat buf;
  UL exe_time;
  int exists;

  exists = stat(m->exectuable, &buf);
  exe_time = buf.st_mtime;
  if(exists < 0 || exe_time < obj_time) { E_compile(m, tmp); }
}

int main(int argc, char **argv) {
  /* Reading in through argv or stdin */
  IS is = (argc == 2) ? new_inputstruct(argv[1]) : new_inputstruct(NULL);
  if(!is && argc == 2) {
    perror(argv[1]);
    return 1;
  }
  
  MF *m = new_make();
  Dllist tmp;

  int result = read_file(is, m);         /* Check if executable was in file*/
  /* Error Check */
  if(result == -1 || result == 0) {
    char *str = (result == 0) ? "No executable Found" : "Bad File";
    fprintf(stderr, "%s\n", str);
    delete_everything(m, tmp, is);
    return 1;
  }

  UL max_obj_time = 0;
  UL max_htime = H_check(m->list[1], tmp);    /* Find time header was updated */
  result = S_check(m, tmp, &max_htime, &max_obj_time);  /* Number of C files need to be recompiled */

  /* Error Check */
  if(result == -1) {
    fprintf(stderr, "Error reading source files\n");
    delete_everything(m, tmp, is);
    return 1;
  }

  E_check(m, tmp, max_obj_time);
   
  delete_everything(m, tmp, is);
  return 0;
}