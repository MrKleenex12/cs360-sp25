#include <stdio.h>
#include <stdlib.h>
#include <jrb.h>
#include <jval.h>

#define BUF_SIZE 8192

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

void read_tar() {
  long ltmp, in;
  u_int32_t itmp; 
  char buffer[BUF_SIZE];

  JRB inodes, tmp;
  inodes = make_jrb();
  // size_t nobjects;

  while(fread(&itmp, 4, 1, stdin) > 0) {
    /* All files */
    fread(buffer, itmp, 1, stdin);
    buffer[itmp] = '\0';
    printf("Name: %s\n", buffer);
    fread(&in, 8, 1, stdin);
    printf("Inode %ld ", in);
    /* IF first time seeing inode */
    tmp = jrb_find_dbl(inodes, in);
    if(tmp == NULL) {
      fread(&itmp, 4, 1, stdin);
      printf("Mode %u ", itmp);
      fread(&ltmp, 8, 1, stdin);
      printf("Mtime %ld\n", ltmp);

      if((itmp >> 15 & 1) == 1) {
        fread(&ltmp, 8, 1, stdin);
        fread(buffer, ltmp, 1, stdin);
      }

      /* Add into list after */
      jrb_insert_dbl(inodes, in, new_jval_i(0));
    }
    printf("\n");
  }
  jrb_free_tree(inodes);
}

int main() {
  read_tar();
  return 0;
}