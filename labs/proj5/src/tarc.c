#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include "fields.h"
#include "jval.h"
#include "jrb.h"
#include "dllist.h"
#include "sys/stat.h"

#define BUF_SIZE 8192

void general_info(struct stat *buf, char *str_buf, size_t *suffix) {
  long len = strlen(str_buf+(*suffix));
  fwrite(&len, 4, 1, stdout);                   /* filename size */
  fwrite(str_buf+(*suffix), len, 1, stdout);  /* filename */
  fwrite(&(buf->st_ino), 8, 1, stdout);         /* inode */
}

void file_info(struct stat *buf, char *str_buf) {
  FILE *file;
  size_t bytes_read;

  file = fopen(str_buf, "rb"); 
  if(file == NULL) {
    perror(str_buf);
    exit(1);
  }

  fwrite(&(buf->st_size), 8, 1, stdout);        /* File's size */
  while((bytes_read = fread(str_buf, 1, BUF_SIZE, file)) > 0) {
    fwrite(str_buf, 1, bytes_read, stdout);     /* File's bytes */
  }
  fclose(file);
}

void print(struct stat *buf,char *str_buf, size_t *suffix, JRB inodes, const char is_file) {
  /* Print out details for all files and directories */
  general_info(buf, str_buf, suffix);

  /* Print extra info if first time encountering this inode */
  JRB tmp = jrb_find_dbl(inodes, buf->st_ino);
  if(tmp == NULL) {
    fwrite(&(buf->st_mode), 4, 1, stdout);
    fwrite(&(buf->st_mtime), 8, 1, stdout); 
    /* If file, print file's size and bytes*/
    if(is_file == 1) { file_info(buf, str_buf);}
    /* Add inode into jrb */
    jrb_insert_dbl(inodes, buf->st_ino, new_jval_i(0));
  }
}

void read_files(DIR *dir, Dllist queue, char *dir_name, size_t *suffix, char *path, JRB inodes) {
  struct dirent *file;
  struct stat buf;

  while((file = readdir(dir)) != NULL) {
    char *f = file->d_name;
    /* skip if . or .. directory */
    if(strcmp(f, ".") == 0 || strcmp(f, "..") == 0) { continue; }
    /* Add file to path str_buf */
    snprintf(path, BUF_SIZE, "%s/%s", dir_name, f);
    /* read in filename */
    if(stat(path, &buf) < 0) {
      perror(path);
      exit(1);
    }
    /* Print if file or Add to queue if directory */
    if(S_ISREG(buf.st_mode)) { print(&buf, path, suffix, inodes, 1); }
    else { dll_append(queue, new_jval_s(strdup(path))); }
  }
}

void open_dir(char *dir_name, size_t *suffix, char *buffer, Dllist queue, JRB inodes) {
  /* Open and error check directory */
  DIR *dir = opendir(dir_name);
  if(dir == NULL) {
    perror(dir_name);
    exit(1);
  }
  /* Open details of directory */
  struct stat buf;
  if(stat(dir_name, &buf) < 0) {
    perror(dir_name);
    exit(1);
  }

  print(&buf, dir_name, suffix, inodes, 0);                 /* Print out directory */ 
  read_files(dir, queue, dir_name, suffix, buffer, inodes); /* Print out files in directory */
  closedir(dir);                                    /* Close current DIR */
}

size_t cut_prefix(char *dir_name) {
  size_t i;
  for(i = strlen(dir_name)-1; i >= 0; i--) {
    if(dir_name[i] == '/' ) { return i+1; } 
  }
  return 0;
}

void dir_search(char *dir_name, char *buffer, JRB inodes) {
  Dllist queue, tmp;
  queue = new_dllist();
  size_t suffix = cut_prefix(dir_name);

  /* Add first */
  dll_append(queue, new_jval_s(strdup(dir_name)));
  /* Index through queue and open directories */
  while(dll_empty(queue) != 1) {
    tmp = dll_first(queue);
    open_dir(tmp->val.s, &suffix, buffer, queue, inodes);
    /* pop directory */
    free(tmp->val.s);
    dll_delete_node(tmp);
  }
  free_dllist(queue);
}

int main(int argc, char **argv) {
  char *dir_name = (argc > 1) ? argv[1] : ".";
  char buffer[BUF_SIZE];
  JRB inodes;
  inodes = make_jrb();
  
  dir_search(dir_name, buffer, inodes);
  jrb_free_tree(inodes);

  return 0;
}