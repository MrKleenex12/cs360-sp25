#include <stdio.h>
#include <stdlib.h>

/*  Order of bytes:

  - size of filename  - 4B int
  - file's name without NULL char
  - file's inode      - 8B long

  IF FIRST TIME READING
  - file's mode       - 4B int
  - file's mod time   - 8B long

    IF FILE & NOT DIR
    - file's size     - 8B long
    - file's bytes (contents)
*/

int main() {
  uint32_t fn_size;
  size_t nobjects;
  long inode;
  
  nobjects = fread(&fn_size, 4, 1, stdin);
  if(nobjects == 0) {
    fprintf(stderr, "Error reading bytes\n");
    exit(1);
  }
  char fname[fn_size+1];
  nobjects = fread(fname, fn_size, 1, stdin);
  nobjects = fread(&inode, 8, 1, stdin);

  printf("%u - %s\n", fn_size, fname);
  printf("%ld\n", inode);
  return 0;
}