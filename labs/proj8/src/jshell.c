#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "fields.h"
#include "dllist.h"
#include "jval.h"


#define APPEND 1
#define NOAPPEND 0
#define WAIT 1
#define NOWAIT 0


/* Copied from Dr. Jantz lab 8 writeup */
typedef struct{
  char *stdinp;         /* Filename from which to redirect stdin.  NULL if empty.*/ 
  char *stdoutp;        /* Filename to which to redirect stdout.  NULL if empty.*/ 
  int append;           /* Boolean for appending.*/ 
  int wait;             /* Boolean for whether I should wait.*/ 
  int n_commands;       /* The number of commands that I have to execute*/ 
  int *argcs;           /* argcs[i] is argc_tmp for the i-th command*/ 
  char ***argvs;        /* argcv[i] is the argv array for the i-th command*/ 
  Dllist comlist;       /* I use this to incrementally read the commands.*/ 
} Command;


void free_argvs(Command *c) {
  /* Free Dllist and then free argvs needed */
  Dllist tmp;
  char **argv_tmp;
  int argc_tmp;
  int index = 0;

  dll_traverse(tmp, c->comlist) {
    argv_tmp = (char**)tmp->val.v;
    argc_tmp = c->argcs[index++];
    for(int i = 0; i < argc_tmp; i++) free(argv_tmp[i]);
    free(argv_tmp);
  }
  free_dllist(c->comlist);
  if(c->argvs != NULL) free(c->argvs);
}


void reset_command(Command *c) {
  /* Free and reset all pointers */
  free_argvs(c);    c->argvs = NULL;
  free(c->stdinp);  c->stdinp = NULL;
  free(c->stdoutp); c->stdoutp = NULL;
  c->comlist = new_dllist();
  /* Reset all int values */
  c->append = NOAPPEND;
  c->wait = WAIT;
  c->n_commands = 0;
}


void free_all(Command *c, IS is) {
  jettison_inputstruct(is);
  /* free memory in dllist first, then free c->argvs */
  free_argvs(c);
  if(c->argvs != NULL) free(c->argvs);
  if(c->argcs != NULL) free(c->argcs);
  if(c->stdinp != NULL) free(c->stdinp);
  if(c->stdoutp != NULL) free(c->stdoutp);
  free(c);
}


Command* make_command() {
  /* Allocate command struct and default values */
  Command *c = (Command*)malloc(sizeof(Command));
  c->append = NOAPPEND;
  c->wait = WAIT;
  c->n_commands = 0;
  c->stdinp = NULL;
  c->stdoutp = NULL;
  c->argvs = NULL;
  c->argcs = (int*)malloc(BUFSIZ);
  c->comlist = new_dllist();
  return c;
}


void move_argvs(Command *c) {
  Dllist tmp;
  int index = 0;

  /* Allocate space for argvs and copy over from comlist */
  c->argvs = (char***)malloc(sizeof(char**) * c->n_commands);
  dll_traverse(tmp, c->comlist) {
    c->argvs[index++] = (char**)tmp->val.v;
  }
}


void add_command(Command *c, IS is) {
  /* Update each argc_tmp with number of fields in argv_tmp */
  c->argcs[c->n_commands++] = is->NF;

  /* Allocate memory for argvs and store them in dllist */
  char **tmp = (char**)malloc(sizeof(char*) * (is->NF + 1));
  for(int i = 0; i < is->NF; i++) 
    tmp[i] = strdup(is->fields[i]);

  /* Add to comlist and add extra index for NULL terminating */
  tmp[is->NF] = NULL;
  dll_append(c->comlist, new_jval_v(tmp));
}


/* Print state of Command */
void print_command(Command *c) {
  Dllist tmp;
  int index = 0;
  int argc_tmp;
  char **argv_tmp;

  printf("stdin:    %s\n", c->stdinp);
  printf("stdout:   %s (Append=%d)\n", c->stdoutp, c->append);
  printf("N_Commands:  %d\n", c->n_commands);
  printf("Wait:        %d\n", c->wait);
  /* Print what is in argvs */
  dll_traverse(tmp, c->comlist) {
    argv_tmp = (char**)(tmp->val.v);
    argc_tmp = c->argcs[index++];

    printf("  %d: argc_tmp: %d    argv_tmp: ", index, argc_tmp);
    for(int i = 0; i < argc_tmp; i++) printf("%s ", argv_tmp[i]);
    printf("\n");
  }
  if(c->argvs != NULL) printf("argvs not empty\n");
  printf("\n");
}


/* Redirects stdin from input */
void redirect_input(const char *input) {
  int fd = open(input, O_RDONLY);
  if(fd < 0) {
    perror("fd for stdin failed");
    exit(1);
  }
  dup2(fd, 0);
  close(fd);
}


/* Redirects stdout to output and appends/truncates */
void redirect_output(const char *output, int append) {
  int fd;
  if(append == APPEND) 
    fd = open(output, O_WRONLY | O_CREAT| O_APPEND, 0644);
  else  
    fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if(fd < 0) {
    perror("fd for stdout failed");
    exit(1);
  }
  dup2(fd, 1);
  close(fd);
}


void execute_command(Command *c) {
  int pid, status;
  int command_count = 0;
  
  /* FLUSH STDIN, STDOUT, AND STDERR BEFORE ANY FORKS */ 
  fflush(stdin);
  fflush(stdout);
  fflush(stderr);
  pid = fork(); 

  if(pid == 0) {
    if(c->stdinp != NULL) redirect_input(c->stdinp);
    if(c->stdoutp != NULL) redirect_output(c->stdoutp, c->append);
    /* Call execvp to execute command on null terminated argv */
    char **argv_tmp = (char**)dll_first(c->comlist)->val.v;
    (void) execvp(argv_tmp[0], argv_tmp);
    perror("execvp failed in execute_command:");
    exit(1);
  } else {
    if(c->wait == WAIT) wait(&status);
  }
}


int read_is(Command *c, IS is, int *letters) {
  /* Reading stdin for jshell commands */
  while(get_line(is) > -1) {
    if(is->fields[0][0] == '#' || is->NF == 0)    /* IGNORE */
      continue;
    else if(is->fields[0][0] == '<')              /* STDIN */
      c->stdinp = strdup(is->fields[1]);
    else if(is->fields[0][0] == '>') {            /* STDOUT */
      c->stdoutp = strdup(is->fields[1]);
      if(strcmp(is->fields[0], ">>") == 0)        /* Append */
        c->append = 1;
    } 
    else if(strcmp(is->fields[0], "NOWAIT") == 0) /* WAIT */
      c->wait = NOWAIT;
    else if(strcmp(is->fields[0], "END") == 0) {  /* END */
      if(letters[1]== 1) print_command(c); 
      return 0;
    } 
    else if(strcmp(is->fields[0], "BREAK") == 0)  /* BREAK */
      return -1;
    else add_command(c, is);                      /* COMMAND */
  }
  return 0;
}


int main(int argc_tmp, char *argv[]) {
  IS is = new_inputstruct(NULL);                  /* Input proccessing */ 
  Command *com = make_command();                  /* Structure for storing commands */
  Dllist tmp;

  /*  Use char array for first commmand line argument
      set index to 1 if letter was found */
  int letters[] = {0, 0, 0};
  if (argc_tmp == 2) {
    letters[0] = strchr(argv[1], 'r') != NULL;
    letters[1] = strchr(argv[1], 'p') != NULL;
    letters[2] = strchr(argv[1], 'n') != NULL;
  }
  
  while(1) {
    if(letters[0] == 1) printf("READY\n\n");
    int result = read_is(com, is, letters);
    if(result == -1) break;
    move_argvs(com);

    if(!letters[2]) execute_command(com);
    
    reset_command(com);
  }

  free_all(com, is);
  return 0;
}