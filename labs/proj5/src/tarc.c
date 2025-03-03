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
#define PATH_SIZE 512 
#define INIT_ARR_SIZE 4 

typedef struct string_vector {
  int length;   /* Total Length Allocated */
  int size;     /* Size of items in vector */
  char **vector;
} SV;

void free_SV(SV *sv) {
  for(int i = 0; i < sv->size; i++) { free(sv->vector[i]); }
  free(sv->vector);
  free(sv);
}

SV* new_SV() {
  SV *sv = (SV*)malloc(sizeof(SV));
  sv->vector = (char**)malloc(INIT_ARR_SIZE * sizeof(char*));
  sv->length = INIT_ARR_SIZE;
  sv->size = 0;

  return sv;
}

void SV_append(SV *sv, char *str) {
  /* Resize if vector full */
  if(sv->size == sv->length) {
    /* 1.5 times larger */
    sv->length += sv->length/2;
    /* Reallocate space for bigger vector */
    char **temp = (char**)realloc(sv->vector, sv->length * sizeof(char*));
    if(temp == NULL) {
      fprintf(stderr, "Realloc Failed\n");
      exit(1);
    }
    sv->vector = temp;
  } 
  /* Append string to vector */
  sv->vector[sv->size++] = strdup(str);
}

void print(struct stat *buf, char *name, const char is_file, JRB list) {
  /* Print info for all files */
  long len = strlen(name);
  // printf("size of file name: 0x%08lx\n", strlen(name));
  // fwrite(&len, 4, 1, stdout);
  // printf("name: %s\n", name);
  // fwrite(name, strlen(name), 1, stdout);
  // printf("inode: 0x%016llx\n", buf->st_ino);
  // fwrite(&(buf->st_ino), 8, 1, stdout);

  /* Check if inode has been printed*/
  JRB tmp = jrb_find_dbl(list, buf->st_ino);
  if(tmp == NULL) {
    /* Print mode and modification */
    // printf("Mode: 0x%08hx\n", buf->st_mode);
    // fwrite(&(buf->st_mode), 4, 1, stdout);
    // printf("Modification time: 0x%016lx\n", buf->st_mtime);
    // fwrite(&(buf->st_mtime), 8, 1, stdout);
    /* Print size and bytes if file */
    if(is_file == 1) {
      FILE *file = fopen(name, "r"); 
      if(file == NULL) {
        perror(name);
        exit(1);
      }

      // printf("file size: 0x%016llx\n", buf->st_size);
      // fwrite(&(buf->st_size), 8, 1, stdout);
      /* Print out characters of file */
      size_t bytes_read;
      while((bytes_read = fread(name, 1, PATH_SIZE, file) > 0)) {
        printf("bytes read: %zu\n", bytes_read);
        fwrite(name, 1, bytes_read, stdout);
      }
      fclose(file);
    }
    /* Add inode into jrb */
    jrb_insert_dbl(list, buf->st_ino, new_jval_i(0));
  }
  printf("\n");
}

void read_dir(DIR *d, SV *sv, char *dir_name, char *path, JRB list) {
  struct dirent *file;
  struct stat buf;

  while((file = readdir(d)) != NULL) {
    char *f = file->d_name;
    /* skip if . or .. directory */
    if(strcmp(f, ".") == 0 || strcmp(f, "..") == 0) continue; 
    /* Add file to path name */
    snprintf(path, PATH_SIZE, "%s/%s", dir_name, f);
    /* read in filename */
    if(stat(path, &buf) < 0) {
      perror(path);
      exit(1);
    }
    /* If directory, add to vector to check later*/
    if(S_ISDIR(buf.st_mode)) { SV_append(sv, path); }
    else { print(&buf, path, 1, list); }
  }
}

void open_dir(char *dir_name, char *path, JRB list) {
  /* Open and error check directory */
  DIR *d = opendir(dir_name);
  if(d == NULL) {
    perror(dir_name);
    exit(1);
  }
  /* Open details of directory */
  struct stat buf;
  if(stat(dir_name, &buf) < 0) {
    perror(dir_name);
    exit(1);
  }
  /* Hold names of directories */
  SV *str_vec = new_SV();
  print(&buf, dir_name, 0, list);
  /* Print out files in directory */
  read_dir(d, str_vec, dir_name, path, list);
  /* Close current directory */
  closedir(d);

  /* Recursively print files in other directories */
  for(int i = 0; i < str_vec->size; i++) {
    open_dir(str_vec->vector[i], path, list);
  }
  free_SV(str_vec);
}

int main(int argc, char **argv) {
  char *dir = (argc > 1) ? argv[1] : ".";
  char path[PATH_SIZE];
  JRB inodes;
  inodes = make_jrb();

  open_dir(dir, path, inodes);
  jrb_free_tree(inodes);

  return 0;
}