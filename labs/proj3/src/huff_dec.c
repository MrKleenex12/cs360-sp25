#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

typedef long long LL;
typedef size_t ST;

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

LL get_fsize(const int fd) {
  struct stat st;
  if(fstat(fd, &st) >= 0) { return st.st_size; }
  else { return -1; }
  return 0;
}

/* last 4 bytes indicate how many bits should be read */
u_int32_t four_bits(int fd, const LL fsize) {
  // int fd = open(file_name, O_RDONLY);
  // if(fd == -1) { return 1; }

  u_int32_t nbits = 0; 
  lseek(fd, fsize-4, SEEK_SET);
  /* Return 1 if can't read 4 bytes into one int*/
  if(read(fd, &nbits, 4) != 4) { return 1; }

  close(fd);
  return nbits;
}

void delete_tree(HN* n) {
  /* BASE CASE */
  if(n == NULL) { return; }
  /* Post-order Traversal */
  delete_tree(n->ptrs[0]);
  delete_tree(n->ptrs[1]);
  /* Free all memory */
  if(n->strings[0] != NULL) { free(n->strings[0]); }
  if(n->strings[1] != NULL) { free(n->strings[1]); }
  free(n);
}

void print(HN* n) {
  /* Print Children */
  if(n->strings[0] != NULL) { printf("0:%8s\n", n->strings[0]); }
  else { printf("0:    NULL\n"); }
  if(n->strings[1] != NULL) { printf("1:%8s\n", n->strings[1]); }
  else { printf("1:    NULL\n"); }
  printf("\n");

  /* Pre-Order Traversal */
  if(n->ptrs[0] != NULL) { print(n->ptrs[0]); }
  if(n->ptrs[1] != NULL) { print(n->ptrs[1]); }
}

char* read_string(const char* buff, ST *curr, ST *last) {
  /* Continue until end of string */
  while(buff[*curr] != 0) {
    (*curr)++;
  }
  /* Copy string over to str */
  ST len = *curr - *last + 1;
  char *str = (char*)malloc(len * sizeof(char));
  snprintf(str, len, "%s", buff + *last);

  /* Update index in buff */
  *last = ++(*curr);
  return str;
}

void add_to_tree(HN* head, char* str, const char* buff, ST* curr, ST* last) {
  HN* prev_HN = head;
  u_int8_t old_bit = buff[*curr]-48;       /* Read first bit and move to next bit */
  (*curr)++;

  while(buff[*curr] != 0) {           /* Read subsequential bits into tree after first bit */
    u_int8_t new_bit = buff[*curr]-48;
    /* Create HN child based on last bit if NULL */
    if(prev_HN->ptrs[old_bit] == NULL) {
      prev_HN->ptrs[old_bit] = create_hn();
    }
    /* Move HN down tree and update new bit */
    prev_HN = prev_HN->ptrs[old_bit];
    old_bit = new_bit;
    (*curr)++;
  }
  /* Update index in buff */
  *last = ++(*curr);
  /* Set string of last bit to be str; */
  prev_HN->strings[old_bit] = str;
}

HN* open_code_file(const char* file_name, const LL fsize) {
  /* Error Check: Code definition file */
  int fd = open(file_name, O_RDONLY);
  if (fd == -1) {
    close(fd);
    exit(1);
  }

  HN* head = create_hn();
  char buff[fsize];   /* Read in file into char array */
  ST nobjects;
  
  if ((nobjects = read(fd, buff, sizeof(buff))) > 0) {
    ST curr_index = 0;
    ST last_index = 0;
    /* Read in whole file as a string */
    while(curr_index < nobjects) {
      char* str = read_string(buff, &curr_index, &last_index);
      /* Read Sequence of bits */
      add_to_tree(head, str, buff, &curr_index, &last_index);
    }
  }
  close(fd);
  return head;
}

void binary(HN** hn, HN* head, unsigned char c, const int bits) {
  /* Index through specified number of bits */
  for(int i = 0; i < bits; i++) {
    u_int8_t bit = (((c >> i) & 1) ? 1 : 0); /* Calculate bit */

    /* Check for matching sequence */
    if((*hn)->strings[bit]) {
      /* print & reset once found an empty string */
      printf("%s", (*hn)->strings[bit]);
      (*hn) = head;
    } else if ((*hn)->ptrs[bit]) {
      (*hn) = (*hn)->ptrs[bit];
    } else {
      /* Error Message */
      fprintf(stderr, "Unrecognized bits\n");
      delete_tree(head);
      exit(1);
    }
  }
}

void output(const char* file_name, HN* head, const LL fsize, const LL nbits) {
  /* Check if open File*/
  int fd = open(file_name, O_RDONLY);
  if (fd == -1) {
    perror("Error opening file");
    delete_tree(head);
    exit(1);
  }
  /* Error check that number of bits is in file */
  if((nbits+7)/8 + 4 > fsize) {
    fprintf(stderr, "Error: Total bits = %lld, but file's size is %lld\n", nbits, fsize);
    delete_tree(head);
    exit(1);
  }
  /* Read in file into buffer */
  char buff[fsize];  
  int nobjects;
  /* Print out strings */
  if ((nobjects = read(fd, buff, sizeof(buff))) > 0) {
    HN* index = head;
    ST times = nbits / 8;
    ST i;
    /* Print out strings by reading 8 bits at a time */
    for(i = 0; i < times; i++) { binary(&index, head, buff[i], 8); }
    /* Print out remainder of bits if needed */
    if(nbits % 8 != 0) {  binary(&index, head, buff[i], nbits%8); }
  }

  close(fd);
}

int main(int argc, char** argv) {
  /* Error Check: Usage */
  if(argc != 3) {
    fprintf(stderr, "Usage: ./bin/huff_def code_def_file intput\n");
    return 1;
  }  

  /* File size of code definition file */
  int fd = open(argv[1], O_RDONLY);
  LL code_size =  get_fsize(fd);
  if(code_size == -1) {
    perror(argv[1]);
    close(fd);
    return 1;
  }

  /* Read in code definition file */
  HN* head = open_code_file(argv[1], code_size);

  /* File size of input file */
  int fd2 = open(argv[2], O_RDONLY);
  LL input_size =  get_fsize(fd2);
  if(input_size < 4) {
    fprintf(stderr, "Error: file is not the correct size.\n");
    delete_tree(head);
    return 1;
  }

  u_int32_t nbits = four_bits(fd2, input_size);
  if(nbits == 1 || nbits == 0) { 
    delete_tree(head);
    close(fd2);
    return 1;
  }

  output(argv[2], head, input_size, nbits);
  
  close(fd);
  close(fd2);
  delete_tree(head);
  return 0;
}