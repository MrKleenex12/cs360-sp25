#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFSIZE 8192

typedef struct huff_node {
  struct huff_node *ptrs[2];
  char *strings[2];
} huff_node;

off_t get_file_size(const char* file_name) {
  struct stat st;
  if(stat(file_name, &st) >= 0) { return st.st_size; }
  else {
    fprintf(stderr, "Failed to read %s\n", file_name);
    return -1;
  }
}

/* last 4 bytes indicate how many bits should be read */
uint32_t last_four(const char* file_name, const off_t fsize) {
  FILE* f = fopen(file_name, "r");
  if(f == NULL) { return 1; }       /* Fail if can't read file */ 

  u_int32_t nbits; 
  fseek(f, fsize-4, SEEK_SET);
  if(fread(&nbits, 4, 1, f) != 1) { /* Return 1 if can't read 4 bytes into one int*/
    fprintf(stderr, "Failed to read 4 bytes\n");
    return 1;
  }
  fclose(f);
  return nbits;
}

char* read_string(const char* buff, size_t *curr, size_t *last) {
  while(buff[*curr] != 0) { (*curr)++; }
  int len = *curr - *last;
  char *str = (char*)malloc((len+1) * sizeof(char));
  snprintf(str, len+1, "%s", buff + *last);
  *last = ++(*curr);
  return str;
}

void open_code_file(const char* file_name, const off_t fsize) {
  FILE* f = fopen(file_name, "rb");  // Open in binary mode to avoid issues with line endings
  if (!f) {
    perror(file_name);
    return;
  }
  char buff[fsize];
  size_t nobjects;

  while ((nobjects = fread(buff, 1, sizeof(buff), f)) > 0) {
    size_t curr = 0;
    size_t last = 0;

    while(curr < nobjects) {
      /* Read string */
      char* str = read_string(buff, &curr, &last);
      /* 
      printf("string: %s \n", str);
      */
      free(str);

      /* Read Sequence of bits */
      while(buff[curr] != 0) {
        curr++;
      }
      last = ++curr;
    }
  }
  fclose(f);
}

int main(int argc, char** argv) {
  off_t file_size = get_file_size(argv[1]);

  if(file_size == -1) {            /* Return 1 if error reading file */
    perror(argv[1]);
    return 1;
  } else {
    open_code_file(argv[1], file_size);
    // printf("%10lld - %s\n", file_size, argv[1]);
    // int nbits = last_four(argv[1], file_size);
    // printf("number of bits:  %d\n", nbits);
  }

  return 0;
}