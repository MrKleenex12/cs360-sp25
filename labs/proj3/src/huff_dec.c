#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

typedef struct huff_node {
  struct huff_node *ptrs[2];
  char *strings[2];
} HN;

HN* create_hn() {
  HN *hn = (HN*)malloc(sizeof(HN));
  hn->ptrs[0] = NULL;
  hn->ptrs[1] = NULL;
  hn->strings[0] = NULL;
  hn->strings[1] = NULL;

  return hn;
}

off_t get_file_size(const char* file_name) {
  struct stat st;
  if(stat(file_name, &st) >= 0) { return st.st_size; }
  else {  return -1;  }
}

/* last 4 bytes indicate how many bits should be read */
u_int32_t last_four(const char* file_name, const off_t fsize) {
  FILE* f = fopen(file_name, "r");
  if(f == NULL) {   return 1;   }       /* Fail if can't read file */ 

  u_int32_t nbits; 
  fseek(f, fsize-4, SEEK_SET);
  if(fread(&nbits, 4, 1, f) != 1) { /* Return 1 if can't read 4 bytes into one int*/
    fprintf(stderr, "Failed to read 4 bytes\n");
    return 1;
  }
  fclose(f);
  return nbits;
}

void delete_tree(HN* n) {
  if(n == NULL) { return; }

  delete_tree(n->ptrs[0]);
  delete_tree(n->ptrs[1]);

  if(n->strings[0] != NULL) { free(n->strings[0]); }
  if(n->strings[1] != NULL) { free(n->strings[1]); }
  free(n);
}

void print(HN* n) {
  if(n->strings[0] != NULL) {
    printf("0:%8s\n", n->strings[0]);
  }
  if(n->strings[1] != NULL) {
    printf("0:%8s\n", n->strings[1]);
  }

  if(n->ptrs[0] != NULL) { print(n->ptrs[0]); }
  if(n->ptrs[1] != NULL) { print(n->ptrs[1]); }
}

char* read_string(const char* buff, int *curr, int *last) {
  while(buff[*curr] != 0) {   (*curr)++;    }

  int len = *curr - *last + 1;
  char *s = (char*)malloc(len * sizeof(char));
  snprintf(s, len, "%s", buff + *last);

  return s;
}

void add_to_tree(HN* head, char* str, const char* buff, int* curr) {
  // HN* last_hn = *head;
  int reading = buff[*curr]-48;
  // printf("%d ", reading);
  (*curr)++;

  while(buff[*curr] != 0) {
    /*
    HN* child = last_hn->ptrs[reading];
    if(child == NULL) { child = create_hn(); }
    last_hn = child; 
    */
    reading = buff[*curr]-48;
    // printf("%d ", reading);
    (*curr)++;
  }

  // last_hn->strings[reading] = str;
  // printf("%s\n", last_hn->strings[reading]);
  // printf("\n");

}

int open_code_file(const char* file_name, const off_t fsize, HN* head) {
  FILE* f = fopen(file_name, "rb");  // Open in binary mode to avoid issues with line endings
  if (f == NULL) {   return 1;   }
  char buff[fsize];
  int nobjects;
  
  HN *tmp = head;
  while ((nobjects = fread(buff, 1, sizeof(buff), f)) > 0) {
    int curr_index = 0;
    int last_index = 0;

    while(curr_index < nobjects) {
      char* s = read_string(buff, &curr_index, &last_index);    /* Read string */
      last_index = ++curr_index;
      // printf("string: %s \n", s);

      if(tmp->ptrs[0] == NULL) {
        tmp->ptrs[0] = create_hn();
      }
      tmp->ptrs[0]->strings[0] = s;
      tmp = tmp->ptrs[0];
      add_to_tree(head, s, buff, &curr_index);               /* Read Sequence of bits */
      last_index = ++curr_index;
    }
  }

  fclose(f);
  return 0;
}

int main(int argc, char** argv) {
  off_t file_size = get_file_size(argv[1]);
  if(file_size == -1) {                             /* Return 1 if error reading file */
    perror(argv[1]);
    return 1;
  }

  /*
  HN* test = create_hn();
  char* name = malloc(6 * sizeof(char));
  strcpy(name, "Larry");
  test->strings[0] = name;
  // print(test);
  free(test);
  free(name);
  */

  HN* head = create_hn();
  open_code_file(argv[1], file_size, head);
  print(head);

  // printf("%10lld - %s\n", file_size, argv[1]);
  // int nbits = last_four(argv[1], file_size);
  // printf("number of bits:  %d\n", nbits);

  delete_tree(head);
  return 0;
}