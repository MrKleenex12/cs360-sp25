#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fields.h"
#include "dllist.h"

/* Copied from Jantz lab 8 writeup */
typedef struct {
  char *stdin;          /* Filename from which to redirect stdin.  NULL if empty.*/ 
  char *stdout;         /* Filename to which to redirect stdout.  NULL if empty.*/ 
  int append_stdout;    /* Boolean for appending.*/ 
  int wait;             /* Boolean for whether I should wait.*/ 
  int n_commands;       /* The number of commands that I have to execute*/ 
  int *argcs;           /* argcs[i] is argc for the i-th command*/ 
  char ***argvs;        /* argcv[i] is the argv array for the i-th command*/ 
  Dllist comlist;       /* I use this to incrementally read the commands.*/ 
} Command;

int main(int argc, char *argv[]) {
  IS is = new_inputstruct(NULL);  // input proccessing

  /*  Use char array for first commmand line argument
      set index to 1 if letter was found */
  char letters[] = {0, 0, 0};
  if (argc == 2) {
    letters[0] = strchr(argv[1], 'r') != NULL;
    letters[1] = strchr(argv[1], 'p') != NULL;
    letters[2] = strchr(argv[1], 'n') != NULL;
  }

  /* TODO program one command execution */  

  // while(get_line(is) > 0) {
  //   /* Ignore if blank line or line begins with a '#' */
  //   if(is->text1[0] == '#') continue;
  //   else if(strcmp(is->text1, "END") == 0) {
  //     /*  TODO command is over should execute */ 
  //   }
  //   else if(is->text1[0] == '<') {
  //     /*  TODO redirect stdin from that file to the first
  //         CPC (child process in the command) */
  //   }
  //   else if(is->text1[0] == '>') {
  //     /*  TODO redirect stdout of the last CPC to that file */ 
  //   }
  //   else if(strcmp(is->fields[0], ">>") == 0) {
  //     /*  TODO redirect stdout of the last CPC and append to that file */ 
  //   }
  //   else if(strcmp(is->text1, "NOWAIT") == 0) {
  //     /*  TODO you will not wait for any of the child processes to exit */
  //   }
  //   else {
  //     /*  TODO any other line is interpreted as an argv array for a CPC
  //         when executing, each child process should be connected to the 
  //         next via pipes -- stdout of child i -> stdin of child i+1 */
  //   }
  // }

  jettison_inputstruct(is);
  return 0;
}