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
uint32_t last_four(const char* file_name, const off_t fsize) {
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

void delete_tree(HN* node) {
  if(node == NULL) { return; }

  delete_tree(node->ptrs[0]);
  delete_tree(node->ptrs[1]);

  if(node->strings[0] != NULL) { free(node->strings[0]); }
  if(node->strings[1] != NULL) { free(node->strings[1]); }
  free(node);
}

char* read_string(const char* buff, size_t *curr, size_t *last) {
  while(buff[*curr] != 0) {   (*curr)++;    }

  int len = *curr - *last + 1;
  char *s = (char*)malloc(len * sizeof(char));
  snprintf(s, len, "%s", buff + *last);
  *last = ++(*curr);

  return s;
}

void add_to_tree(HN* head, const char* buff, char* str, size_t* curr) {
  HN* last_hn = head;
  int reading = buff[*curr]-48;
  printf("%d ", reading);
  (*curr)++;

  while(buff[*curr] != 0) {
    HN* child = last_hn->ptrs[reading];

    if(child == NULL) { child = create_hn(); }
    last_hn = child; 
    
    reading = buff[*curr]-48;

    printf("%d ", buff[*curr]-48);
    (*curr)++;
  }

  last_hn->strings[reading] = str;
  printf("\n");

}

int open_code_file(const char* file_name, const off_t fsize, HN* head) {
  FILE* f = fopen(file_name, "rb");  // Open in binary mode to avoid issues with line endings
  if (f == NULL) {   return 1;   }
  char buff[fsize];
  size_t nobjects;
  
  while ((nobjects = fread(buff, 1, sizeof(buff), f)) > 0) {
    size_t curr = 0;
    size_t last = 0;
    while(curr < nobjects) {
      char* s = read_string(buff, &curr, &last);    /* Read string */
      // printf("string: %s \n", str);
      free(s);

      add_to_tree(head, buff, s, &curr);               /* Read Sequence of bits */
      last = ++curr;
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


  HN* head = create_hn();
  open_code_file(argv[1], file_size, head);
  // printf("%10lld - %s\n", file_size, argv[1]);
  // int nbits = last_four(argv[1], file_size);
  // printf("number of bits:  %d\n", nbits);

  delete_tree(head);
  return 0;
}