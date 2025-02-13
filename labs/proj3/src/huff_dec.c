#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

off_t get_file_size(const char* file_name) {
  struct stat st;
  if(stat(file_name, &st) >= 0) { return st.st_size; }
  else {
    fprintf(stderr, "Failed to read %s\n", file_name);
    return -1;
  }
}

/* Last 4 bytes indicate how many bits should be read */
uint32_t read_last_four(const char* file_name, const off_t fsize) {
  FILE* f = fopen(file_name, "r");
  if(f == NULL) { return 1; }       /* Fail if can't read file */ 

  fseek(f, fsize-4, SEEK_SET);      /* Move file position to last 4 bytes of the file*/
  uint32_t nbits;
  if(fread(&nbits, 4, 1, f) != 1) { /* Return 1 if can't read 4 bytes into one int*/
    fprintf(stderr, "Failed to read 4 bytes\n");
    return 1;
  }
  fclose(f);
  return nbits;
}

int main(int argc, char** argv) {
  
  off_t file_size = get_file_size(argv[1]);

  if(file_size == -1) {            /* Return 1 if error reading file */
    perror(argv[1]);
    return 1;
  } else {
    printf("%10lld - %s\n", file_size, argv[1]);
    int nbits = read_last_four(argv[1], file_size);
    printf("number of bits:  %d\n", nbits);
  }

  return 0;
}