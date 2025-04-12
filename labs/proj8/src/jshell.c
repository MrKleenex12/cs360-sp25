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
  /* Allocate command struct and default values */
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


void print_command(Command *c) {
  printf("stdin:    %s\n", c->stdinp);
  printf("stdout:   %s (Append=%d)\n", c->stdinp, c->append_stdout);
  printf("N_Commands:  %d\n", c->n_commands);
  printf("Wait:        %d\n", c->wait);

  Dllist tmp;
  int index = 0;
  dll_traverse(tmp, c->comlist) {
    printf("  %d: argc:%d    argv: ", index, c->argcs[index]);
    for(int i = 0; i < c->argcs[index]; i++) printf("%s ", ((char**)tmp->val.v)[i]);
    printf("\n");
    index++;
  }
  /*
  for(int i = 0; i < c->n_commands; i++) {
    printf("  %d: argc:%d    argv: ", i, c->argcs[i]);
    for(int j = 0; j < c->argcs[i]; j++) printf("%s ", c->argvs[i][j]);
    printf("\n");
  }
  */
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
  /* Reading stdin for jshell commands */
  while(get_line(is) > -1) {
    /*  IGNORE: Ingore stdin */
    if(is->fields[0][0] == '#' || is->NF == 0) continue;
    /*  STDIN: Redirect stdin from file to first child process */
    else if(is->fields[0][0] == '<') { com->stdinp = strdup(is->fields[1]); } 
    /*  STDOUT: Redirect stdout of last child process */
    else if(is->fields[0][0] == '>') {
      com->stdoutp = strdup(is->fields[1]);
      if(strcmp(is->fields[0], ">>") == 0) com->append_stdout = 1;    // Append
    } 
    /*  WAIT */
    else if(strcmp(is->fields[0], "NOWAIT") == 0) com->wait = 0;
    /*  END: Break input and process commands */
    else if(strcmp(is->fields[0], "END") == 0) {
      if(letters[1]== 1) print_command(com); 
      break;
    } 
    /*  COMMAND: Anything else is a command; add it to the list */
    else { 
      // printf("%d - %s", is->NF, is->text1);
      add_command(com, is); }
  }

  move_argvs(com);
  

  jettison_inputstruct(is);
  free_command(com);
  return 0;
}