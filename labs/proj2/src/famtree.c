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

typedef struct person {
  char *name, *dad, *mom, sex;
} Person;

char* full_name(IS is) {
  int nf = is->NF;
  int ssize = strlen(is->fields[1]);
  /* Adjust size if more words in name */
  for(int i = 2; i < nf; i++) { ssize += (strlen(is->fields[i]) + 1); }

  /* Get first word */
  char* name = (char*)malloc((ssize+1) * sizeof(char)); 
  strcpy(name, is->fields[1]);

  /* Get rest of words */
  ssize = strlen(is->fields[1]);
  for(int i = 2; i < nf; i++) {
    name[ssize] = ' ';
    strcpy(name+ssize+1, is->fields[i]);
    ssize += strlen(name+ssize);
  }

  return name;
}

char* one_name(IS is) {
  char* name = (char*)malloc((strlen(is->fields[1]) + 1) * sizeof(char)); 
  strcpy(name, is->fields[1]);
  return name;
}

int read_file(const char* filename) {
  IS is = new_inputstruct(filename);
  if(NULL == is) {  return 1; }       /* Quit if reading argv went wrong */

  /* Parse through file */
  int count = 0;
  while(get_line(is) >= 0) {
    if(strcmp(is->fields[0], "PERSON") == 0) { 
      // Create Person with just name like in /testing
      Person *p = (Person*)malloc(sizeof(Person));

      p->name = (is->NF > 2) ? full_name(is) : one_name(is);

      free(p->name);
      free(p);
      count++; 
    }
  }
  printf("There are %d of People in %s\n", count, filename);

  jettison_inputstruct(is);
  return 0;
}

int main(int argc, char **argv) {
  /* Check Usage */
  if(2 != argc) {
    fprintf(stderr, "usage: bin/famtree filenames\n");
    exit(1);
  }

  /* Check valid file */
  if(1 == read_file(argv[1])) {
    perror(argv[1]);
    return 1; 
  };
}