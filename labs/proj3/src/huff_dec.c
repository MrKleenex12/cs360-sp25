#include <stdio.h>
#include <stdlib.h>
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

/* Last 4 bytes indicate how many bits should be read */
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

void cat(const char* file_name) {
  FILE* f = fopen(file_name, "rb");  // Open in binary mode to avoid issues with line endings
  if (!f) {
    perror(file_name);
    return;
  }
  unsigned char buffer[BUFFSIZE];
  size_t bytesRead;

  while ((bytesRead = fread(buffer, 1, sizeof(buffer), f)) > 0) {
    for (size_t i = 0; i < bytesRead; i++) {
      // printf("%02x ", buffer[i]);  // Print each byte as hex
      printf("%c", buffer[i]);  // Print each byte as char 
    }
  }
  printf("\n");
 
  fclose(f);
}

int main(int argc, char** argv) {
  off_t file_size = get_file_size(argv[1]);

  if(file_size == -1) {            /* Return 1 if error reading file */
    perror(argv[1]);
    return 1;
  } else {
    cat(argv[1]);
    printf("%10lld - %s\n", file_size, argv[1]);
    int nbits = last_four(argv[1], file_size);
    printf("number of bits:  %d\n", nbits);
  }

  return 0;
}