#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dllist.h" 
#include "fields.h"
#include "jrb.h"


typedef struct person {
  char *name, sex;
  char visited;
  struct person *dad, *mom;
  Dllist kid_list;
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

Person* create_person(char* name, JRB *tree) {
  /* Check if already created in tree*/
  JRB found = jrb_find_str(*tree, name);
  if(found != NULL) {
    free(name);
    return (Person*)found->val.v;
  } 

  Person* p = (Person*)malloc(sizeof(Person));
  p->name = name;
  p->dad = NULL;
  p->mom = NULL;
  p->sex = 'U';
  p->visited = 0;
  p->kid_list = new_dllist();
  jrb_insert_str(*tree, name, new_jval_v((void*) p));

  return p;
}

void free_person(Person* p) {
  free(p->name);
  free_dllist(p->kid_list);
  free(p);
}

void print(Person* p) {
  printf("%s\n", p->name);

  char *sex = (p->sex == 'M') ? "Male" : "Female";
  if(p->sex != 'U') { printf("  Sex: %s\n", sex); }
  else { printf("  Sex: Unknown\n"); }
  if(p->dad != NULL) { printf("  Father: %s\n", p->dad->name); }
  else { printf("  Father: Unknown\n"); }
  if(p->mom != NULL) { printf("  Mother: %s\n", p->mom->name); }
  else { printf("  Mother: Unknown\n"); }

  if(dll_empty(p->kid_list)) { printf("  Children: None\n"); }
  else {
    Dllist tmp;
    printf("  Children:\n");
    dll_traverse(tmp, p->kid_list) {
      printf("    %s\n", ((Person*)tmp->val.v)->name);
    }
  }
  printf("\n");
}

void add_parent(Person* p1, Person* p2, const char c) {
  if(c == 'M') { p1->dad = p2; }
  else {p1->mom = p2;}
}

void add_kid(Person* p1, Person* p2) {
  dll_append(p1->kid_list, new_jval_v((void*) p2));
}

void read_stdin(JRB *tree) {
  IS is = new_inputstruct(NULL);
  Person *p1, *p2;

  /* Parse through file with while loop*/
  while(get_line(is) >= 0) { 
    if(is->NF == 0) { continue; }                 /* Skip blank lines */
    char *name = (is->NF > 2) ? full_name(is) : one_name(is);
    /* Update p1 if needed*/
    if(strcmp(is->fields[0], "PERSON") == 0) {
      p1 = create_person(name, tree);
      continue;
    }
    // Update p2 if not SEX line
    if(strcmp(is->fields[0], "SEX") != 0) { p2 = create_person(name, tree); }
    /* Relational Links*/
    if(strcmp(is->fields[0], "FATHER") == 0) { add_parent(p1, p2, 'M'); }
    else if(strcmp(is->fields[0], "MOTHER") == 0) { add_parent(p1, p2, 'F'); }
    else if(strcmp(is->fields[0], "FATHER_OF") == 0) { add_kid(p1, p2); }
    else if(strcmp(is->fields[0], "MOTHER_OF") == 0) { add_kid(p1, p2); }
    else if(strcmp(is->fields[0], "SEX") == 0) {
      p1->sex = *(is->fields[1]);
      free(name);
    }
  }

  jettison_inputstruct(is);
}

int check_cycle(Person* p, Dllist tmp) {
  if(p->visited == 1) { return 1; }
  p->visited = 1;
  dll_traverse(tmp, p->kid_list) {
    Person* p = ((Person*)tmp->val.v);
    check_cycle(p, tmp);
    p->visited = 0;
  }

  return 0;
}

int main(int argc, char **argv) {
  /* Read from stdin */
  JRB tree = make_jrb();
  read_stdin(&tree);

  Dllist index;
  JRB tmp;

  jrb_traverse(tmp, tree) {
    Person* p = (Person*) tmp->val.v;
    print(p);
    /*
    if(check_cycle(p, index) == 1) {
      fprintf(stderr, "Cycle Detected\n");
      return 1;
    }
    */
  }

  /* Freeing Everything*/
  jrb_traverse(tmp, tree) {
    free_person((Person*)tmp->val.v);
  }

  jrb_free_tree(tree);
}