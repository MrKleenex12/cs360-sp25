#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dllist.h" 
#include "fields.h"
#include "jrb.h"


typedef struct person {
  char *name, sex;
  char visited;
  int dependencies;
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
  /* Else make person*/
  Person* p = (Person*)malloc(sizeof(Person));
  p->name = name;
  p->dad = NULL;
  p->mom = NULL;
  p->sex = 'U';
  p->visited = 0;
  p->dependencies = 0;
  p->kid_list = new_dllist();
  jrb_insert_str(*tree, name, new_jval_v((void*) p));

  return p;
}


void free_everything(JRB tree, JRB tmp) {
  jrb_traverse(tmp, tree) {
    Person* p = ((Person*)tmp->val.v);
    free(p->name);
    free_dllist(p->kid_list);
    free(p);
  }
  jrb_free_tree(tree);

}


void print(Person* p) {
  printf("%s\n", p->name);

  /* Print all details */
  char *sex = (p->sex == 'M') ? "Male" : "Female";
  if(p->sex != 'U') { printf("  Sex: %s\n", sex); }
  else { printf("  Sex: Unknown\n"); }
  if(p->dad != NULL) { printf("  Father: %s\n", p->dad->name); }
  else { printf("  Father: Unknown\n"); }
  if(p->mom != NULL) { printf("  Mother: %s\n", p->mom->name); }
  else { printf("  Mother: Unknown\n"); }
  /* Print all Children */
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


int add_parent(Person* p1, Person* p2, const char c) {
  /* Error Check */
  if(p2->sex != c && p2->sex != 'U') { return 2; }
  p2->sex = c;

  if(c == 'M') {    /* Father */
    /* Error Check */
    if(p1->dad != NULL) { return 1; }
    p1->dad = p2; 
    dll_append(p2->kid_list, new_jval_v((void*) p1));
  }
  else {            /* Mother */
    /* Error Check */
    if(p1->mom != NULL) { return 1; }
    p1->mom = p2; 
    dll_append(p2->kid_list, new_jval_v((void*) p1));
  }
  
  return 0;
}


int add_kid(Person* p1, Person* p2, const char c) {
  /* Error Check */
  if(p1->sex != c && p1->sex != 'U') { return 1; }

  dll_append(p1->kid_list, new_jval_v((void*) p2));
  p1->sex = c;
  Person* to_change = (c == 'M') ? p2->dad : p2->mom;
  to_change = p1;

  return 0;
}

void sex_error(IS is, JRB *tree, JRB tmp) {
  fprintf(stderr, "Bad input - sex mismatch on line %d\n", is->line);
  free_everything(*tree, tmp);
  exit(1);
}

void double_parent(IS is, JRB *tree, JRB tmp, const char c) {
  if(c == 'M') { fprintf(stderr, "Bad input -- child with two fathers on line %d\n", is->line); }
  else { fprintf(stderr, "Bad input -- child with two mothers on line %d\n", is->line); }
  free_everything(*tree, tmp);
  exit(1);
}


void read_stdin(JRB *tree, JRB tmp) {
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


    /* Update p2 if not SEX line */
    if(strcmp(is->fields[0], "SEX") == 0) {
      char sex = *(is->fields[1]);
      if(p1->sex != 'U' && p1->sex != sex) { sex_error(is, tree, tmp); }
      p1->sex = sex;
      free(name);
    } 
    else { p2 = create_person(name, tree);}


    /* Relational Links */
    if(strcmp(is->fields[0], "FATHER") == 0) { 
      int error = add_parent(p1, p2, 'M');
      if(error == 1) { double_parent(is, tree, tmp, 'M'); }
      else if( error == 2) { sex_error(is, tree, tmp); }
    }

    else if(strcmp(is->fields[0], "MOTHER") == 0) {
      int error = add_parent(p1, p2, 'F');
      if(error == 1) { double_parent(is, tree, tmp, 'F'); }
      else if( error == 2) { sex_error(is, tree, tmp); }
    }
  
    else if(strcmp(is->fields[0], "FATHER_OF") == 0) {
      add_kid(p1, p2, 'M');
    }

    else if(strcmp(is->fields[0], "MOTHER_OF") == 0) {
      if(add_kid(p1, p2, 'F')) { sex_error(is, tree, tmp); }
    }
  }

  jettison_inputstruct(is);
}

int check_cycle(Person* p, Dllist tmp) {
  if(p->visited == 1) { return 1; }
  p->visited = 1;
  dll_traverse(tmp, p->kid_list) {
    Person* p = ((Person*)tmp->val.v);
    if(check_cycle(p, tmp)) { return 1; }
  }
  p->visited = 0;

  return 0;
}

void topological(JRB tree, JRB tmp, Dllist index) {
  Dllist queue = new_dllist();
  /* Calculate dependencies first */
  jrb_traverse(tmp, tree) {
    Person* p = (Person*)tmp->val.v;
    if(p->dad != NULL) { p->dependencies++; }
    if(p->mom != NULL) { p->dependencies++; }
    /* Add to queue if 0 dependencies */
    if(p->dependencies == 0) { dll_append(queue, new_jval_v((void*) p));
    }
  }

  while(!dll_empty(queue)) {
    Person* p = (Person*)dll_first(queue)->val.v;
    dll_delete_node(dll_first(queue));              /* Pop first Dllist*/
    print(p);

    /* Index through children*/
    if(dll_empty(p->kid_list)) { continue; }
    dll_traverse(index, p->kid_list) {
      Person* child = (Person*)index->val.v; 
      if(--child->dependencies == 0) { dll_append(queue, new_jval_v((void*) child));}
    }
  }

  free_dllist(queue);
}

int main(int argc, char **argv) {
  /* Read from stdin */
  JRB tree, tmp;
  tree= make_jrb();
  read_stdin(&tree, tmp);

  Dllist index;
  jrb_traverse(tmp, tree) {
    Person* p = (Person*) tmp->val.v;
    if(check_cycle(p, index) == 1) {
      fprintf(stderr, "Bad input -- cycle in specification\n");
      free_everything(tree, tmp);
      return 1;
    }
  }
  topological(tree, tmp, index);

  /* Freeing Everything*/
  free_everything(tree, tmp);
  return 0;
}