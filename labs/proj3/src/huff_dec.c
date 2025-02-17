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

off_t get_fsize(const char* file_name) {
  struct stat st;
  if(stat(file_name, &st) >= 0) { return st.st_size; }
  else {
    perror(file_name);
    return -1;
  }
  return 0;
}

/* last 4 bytes indicate how many bits should be read */
u_int32_t four_bits(const char* file_name, const off_t fsize) {
  FILE* f = fopen(file_name, "r");
  if(f == NULL) { return -1; }       /* Fail if can't read file */ 

  u_int32_t nbits; 
  fseek(f, fsize-4, SEEK_SET);
  if(fread(&nbits, 4, 1, f) != 1) { /* Return 1 if can't read 4 bytes into one int*/
    fprintf(stderr, "Error: file is not the correct size.\n");
    return -1;
  }
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

char* read_string(const char* buff, int *curr, int *last) {
  /* Continue until end of string */
  while(buff[*curr] != 0) {   (*curr)++;    }
  /* Copy string over to str */
  int len = *curr - *last + 1;
  char *str = (char*)malloc(len * sizeof(char));
  snprintf(str, len, "%s", buff + *last);

  return str;
}

void add_to_tree(HN* head, char* str, const char* buff, int* curr) {
  HN* prev_HN = head;
  int old_bit = buff[*curr]-48;       /* Read first bit and move to next bit */
  (*curr)++;

  while(buff[*curr] != 0) {           /* Read subsequential bits into tree after first bit */
    int new_bit = buff[*curr]-48;
    /* Create HN child based on last bit if NULL */
    if(prev_HN->ptrs[old_bit] == NULL) {
      prev_HN->ptrs[old_bit] = create_hn();
    }
    /* Move HN down tree and update new bit */
    prev_HN = prev_HN->ptrs[old_bit];
    old_bit = new_bit;
    (*curr)++;
  }

  prev_HN->strings[old_bit] = str;    /* Set string to be str; */
}

HN* open_code_file(const char* file_name, const off_t fsize) {
  /* Error Check: Code definition file */
  int fd = open(file_name, O_RDONLY);
  if (fd == -1) {
    perror("Error opening file");
    exit(1);
  }

  HN* head = create_hn();           /* Head of tree pointer */
  char buff[fsize];                 /* Read in file into char array */
  int nobjects;                     /* Helper variable to read in file */
  
  if ((nobjects = read(fd, buff, sizeof(buff))) > 0) {
    int curr_index = 0;
    int last_index = 0;
    /* Read in whole file as a string */
    while(curr_index < nobjects) {
      /* Read string */
      char* str = read_string(buff, &curr_index, &last_index);
      last_index = ++curr_index;    /* Update index */
      /* Read Sequence of bits */
      add_to_tree(head, str, buff, &curr_index);
      last_index = ++curr_index;    /* Update index */
    }
  }
  close(fd);
  return head;
}

void binary(unsigned char c, char* str, const int bits) {
  for(int i = 0; i < bits; i++) {
    str[i] = (((c >> i) & 1) ? '1' : '0');
  }

  str[bits] = '\0';
}

char* decrypt_file(const char* file_name, const off_t fsize, const int nbits) {
  int fd = open(file_name, O_RDONLY);
  if (fd == -1) {
    perror("Error opening file");
    return NULL;
  }

  char buff[fsize];  // Read in chunks
  int nobjects;
  /* Alloc memory for string of bit stream */
  char* bit_str = (char*)malloc((nbits+1) * sizeof(char));

  if ((nobjects = read(fd, buff, sizeof(buff))) > 0) {
    /* Error check that number of bits is in file */
    if((nobjects-4) * 8 < nbits) {
      fprintf(stderr, "error reading bits\n");
      exit(1);
    }

    int times = nbits / 8;
    int i;
    /* Add in bits by sets of 8 */
    for(i = 0; i < times; i++) { binary(buff[i], bit_str+(8*i), 8); }
    /* Add in remaning bits */
    if(nbits % 8 != 0) { binary(buff[i], bit_str+(8*times), nbits%8); }
  }
  close(fd);
  return bit_str; 
}

void output(HN* head, const char* bit_stream) {
  HN* curr = head;  
  for(int i = 0; i < (int)strlen(bit_stream); i++) {
    int bit = bit_stream[i]-48;
    if(curr->strings[bit] == NULL) {
      curr = curr->ptrs[bit];
      continue;
    }

    printf("%s", curr->strings[bit]);
    curr = head;
  }
}

int main(int argc, char** argv) {
  /* Error Check: Usage */
  if(argc != 3) {
    fprintf(stderr, "Usage: ./bin/huff_def code_def_file encrypted_file\n");
    return 1;
  }  

  /* File size of encrypted file */
  off_t file_size =  get_fsize(argv[2]);            /* File size of encrypted file */
  if(file_size == -1) { return 1; }
  int nbits = four_bits(argv[2], file_size);
  if(nbits == -1) { return 1; }

  file_size =  get_fsize(argv[1]);                  /* File size of code definition file */
  HN* head = open_code_file(argv[1], file_size);    /* Read in code definition file */

  file_size =  get_fsize(argv[2]);
  if(file_size <= 4) {
    delete_tree(head);
    return 1;
  }

  char* bit_stream = decrypt_file(argv[2], file_size, nbits);
  if(bit_stream == NULL) {
    delete_tree(head);
    return 1;
  }

  output(head, bit_stream);

  free(bit_stream);
  delete_tree(head);
  return 0;
}