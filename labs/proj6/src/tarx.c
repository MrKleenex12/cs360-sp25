#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
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
  char buffer[BUF_SIZE];
  char name[BUF_SIZE];
  long ltmp, in, mtime;
  u_int32_t itmp; 

  JRB inodes, tmp;
  inodes = make_jrb();


  while(fread(&itmp, 4, 1, stdin) > 0) {
    /* All files */
    fread(name, itmp, 1, stdin);
    name[itmp] = '\0';
    printf("Name: %s\n", name);
    fread(&in, 8, 1, stdin);
    printf("Inode %ld ", in);
    /* IF first time seeing inode */
    tmp = jrb_find_dbl(inodes, in);

    if(tmp == NULL) {
      fread(&itmp, 4, 1, stdin);
      printf("Mode %o ", itmp);
      fread(&mtime, 8, 1, stdin);
      printf("Mtime %ld", mtime);
      /* IF File and not Directory */
      if((itmp >> 15 & 1) == 1) {
        fread(&ltmp, 8, 1, stdin);
        fread(buffer, ltmp, 1, stdin);
      } else {
        mkdir(name, itmp);
      }

      /* Add into list after */
      jrb_insert_dbl(inodes, in, new_jval_i(0));
    }
    printf("\n\n");
  }
  jrb_free_tree(inodes);
}

int main() {
  read_tar();
  return 0;
}