#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fields.h"

int main(int argc, char *argv[]) {
  IS is = new_inputstruct(NULL);  // input proccessing

  /*  Use char array for first commmand line argument
      set index to 1 if letter was found */
  char letters[] = {0, 0, 0};
  if(argc == 2) {
    for(size_t i = 0; i < strlen(argv[1]); i++) {
      if(argv[1][i] == 'r') letters[0] = 1;
      else if(argv[1][i] == 'p') letters[1] = 1;
      else if(argv[1][i] == 'n') letters[2] = 1;
    }
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