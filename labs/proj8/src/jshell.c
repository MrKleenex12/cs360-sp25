#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fields.h"
#include "dllist.h"
#include "jval.h"


/* Copied from Dr. Jantz lab 8 writeup */
typedef struct{
  char *stdinp;         /* Filename from which to redirect stdin.  NULL if empty.*/ 
  char *stdoutp;        /* Filename to which to redirect stdout.  NULL if empty.*/ 
  int append_stdout;    /* Boolean for appending.*/ 
  int wait;             /* Boolean for whether I should wait.*/ 
  int n_commands;       /* The number of commands that I have to execute*/ 
  int *argcs;           /* argcs[i] is argc for the i-th command*/ 
  char ***argvs;        /* argcv[i] is the argv array for the i-th command*/ 
  Dllist comlist;       /* I use this to incrementally read the commands.*/ 
} Command;

void free_argvs(Command *c) {
  for(int i = 0; i < c->n_commands; i++) {
    for(int j = 0; j < c->argcs[i]; j++)      /* Iterate through each argv */
      free(c->argvs[i][j]);
    free(c->argvs[i]);                        /* Free argv */
  } 
  free(c->argvs);                             /* Free argvs */

}

void free_command(Command *c) {
  if(c->stdinp != NULL) free(c->stdinp);
  if(c->stdoutp != NULL) free(c->stdoutp);
  if(c->argvs != NULL) free_argvs(c);
  if(c->argcs != NULL) free(c->argcs);
  free_dllist(c->comlist);
  free(c);
}


Command* make_command() {
  /* Allocate command struct and */
  Command *c = (Command*)malloc(sizeof(Command));
  c->append_stdout = 0;
  c->wait = 1;
  c->n_commands = 0;
  c->stdinp = NULL;
  c->stdoutp = NULL;
  c->argvs = NULL;
  c->argcs = (int*)malloc(BUFSIZ);
  c->comlist = new_dllist();
  return c;
}

void move_argvs(Command *c) {
  Dllist tmp;
  int index = 0;

  /* Allocate space for argvs and copy over from comlist */
  c->argvs = (char***)malloc(sizeof(char*) * c->n_commands);
  dll_traverse(tmp, c->comlist) {
    c->argvs[index++] = (char**)tmp->val.v;
  }
}


void add_command(Command *c, IS is) {
  /* Update each argc with number of fields in argv */
  c->argcs[c->n_commands++] = is->NF;

  /* Allocate memory for argvs and store them in dllist */
  char **tmp = (char**)malloc(sizeof(char*) * is->NF);
  for(int i = 0; i < is->NF; i++) { tmp[i] = strdup(is->fields[i]); }
  /* add extra index for NULL terminating */
  // tmp[is->NF] = NULL;
  dll_append(c->comlist, new_jval_v(tmp));
}


int main(int argc, char *argv[]) {
  IS is = new_inputstruct(NULL);                  /* input proccessing */ 
  Command *com = make_command();                  /* structure for storing commands */
  Dllist tmp;

  /*  Use char array for first commmand line argument
      set index to 1 if letter was found */
  char letters[] = {0, 0, 0};
  if (argc == 2) {
    letters[0] = strchr(argv[1], 'r') != NULL;
    letters[1] = strchr(argv[1], 'p') != NULL;
    letters[2] = strchr(argv[1], 'n') != NULL;
  }


  /* TODO program one command execution */  
  while(get_line(is) > 0) {
    /*  Break input and process commands */
    if(strcmp(is->fields[0], "END") == 0) break;
    /*  If nothing else, it is a command that must be 
        added into the list */
    else {  add_command(com, is); }
  }


  move_argvs(com);
  

  jettison_inputstruct(is);
  free_command(com);
  return 0;
}