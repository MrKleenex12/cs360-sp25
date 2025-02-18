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

LL get_fsize(const char* file_name) {
  struct stat st;
  if(stat(file_name, &st) >= 0) { return st.st_size; }
  else { return -1; }
  return 0;
}

/* last 4 bytes indicate how many bits should be read */
u_int32_t four_bits(const char* file_name, const LL fsize) {
  FILE* f = fopen(file_name, "r");
  if(f == NULL) { return -1; }       /* Fail if can't read file */ 

  u_int32_t nbits; 
  fseek(f, fsize-4, SEEK_SET);
  /* Return 1 if can't read 4 bytes into one int*/
  if(fread(&nbits, 4, 1, f) != 1) { return -1; }

  fclose(f);
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
  if (fd == -1) { return NULL; }

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

HN* binary(HN* hn, HN* head, unsigned char c, const int bits) {
  for(int i = 0; i < bits; i++) {
    /* Calculate bit */
    u_int8_t bit = (((c >> i) & 1) ? 1 : 0);

    /* Check for matching sequence */
    if(hn->strings[bit] == NULL) {
      /* Makes sure child exists and valid string */
      if(hn->ptrs[bit] != NULL) {
        hn = hn->ptrs[bit];
        continue;
      }
      /* Error Message */
      fprintf(stderr, "Unrecognized bits\n");
      delete_tree(head);
      exit(1);
    }
    /* print & reset once found an empty string */
    printf("%s", hn->strings[bit]);
    hn = head;
  }

  return hn;
}

void decrypt_file(const char* file_name, HN* head, const LL fsize, const LL nbits) {
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
    /* Print out strings based off bits */
    for(i = 0; i < times; i++) {
      index = binary(index, head, buff[i], 8);
    }
    if(nbits % 8 != 0) { 
      index = binary(index, head, buff[i], nbits%8);
    }
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
  LL file_size =  get_fsize(argv[1]);
  if(file_size == -1) {
    perror(argv[1]);
    return 1;
  }

  /* Read in code definition file */
  HN* head = open_code_file(argv[1], file_size);
  if(head == NULL) {
    perror("Error opening file");
    return 1;
  }

  /* File size of input file */
  file_size =  get_fsize(argv[2]);
  if(file_size < 4) {
    fprintf(stderr, "Error: file is not the correct size.\n");
    delete_tree(head);
    return 1;
  }

  LL nbits = four_bits(argv[2], file_size);
  if(nbits == -1 || nbits == 0) { 
    delete_tree(head);
    return 1;
  }

  decrypt_file(argv[2], head, file_size, nbits);

  delete_tree(head);
  return 0;
}