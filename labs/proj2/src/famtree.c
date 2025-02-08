#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dllist.h" 
#include "fields.h"
#include "jrb.h"

/*  Make Person struct
    name
    Sex
    Father
    Mother
    List of children
*/

int read_file(const char* filename, IS *input) {
  *input = new_inputstruct(filename);
  IS is = *input;
  if(NULL == is) {  return 1; }

  while(get_line(is) >= 0) {
    printf("%s", is->text1); 
  }

  return 0;
}

int main(int argc, char **argv) {
  /* Check Usage */
  if(2 != argc) {
    fprintf(stderr, "usage: bin/famtree filenames\n");
    exit(1);
  }

  /* Check valid file */
  IS is;
  if(1 == read_file(argv[1], &is)) {
    perror(argv[1]);
    return 1; 
  };

  
  jettison_inputstruct(is);
}