#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "fields.h"
#include "jval.h"
#include "jrb.h"
#include "dllist.h"
#include "sys/stat.h"

#define BUF_SIZE 8192

int main(int argc, char **argv) {

  char buffer[BUF_SIZE];
  int fd = open(argv[1], O_RDONLY);
  if(fd == -1) {
    close(fd);
    return 1;
  }
  
  long nobjects;
  if((nobjects = read(fd, (void*)buffer, sizeof(buffer))) > 0) {
    for(int i = 0; i < nobjects; i++) {
      printf("%c", buffer[i]);
    }
  }

  close(fd);
  return 0;
}