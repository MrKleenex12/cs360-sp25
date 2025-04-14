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
  int append_stdout;    /* Boolean for appending.*/ 
  int wait;             /* Boolean for whether I should wait.*/ 
  int n_commands;       /* The number of commands that I have to execute*/ 
  int *argcs;           /* argcs[i] is argc for the i-th command*/ 
  char ***argvs;        /* argcv[i] is the argv array for the i-th command*/ 
  Dllist comlist;       /* I use this to incrementally read the commands.*/ 
} Command;


void free_argv(char **argv, size_t size) {
  for(size_t i = 0; i < size; i++) free(argv[i]);
  free(argv);
}

void free_argvs(Command *c) {
  int i = 0;

  if(c->argvs != NULL) {
    /* Iterate through argvs and free each argv with free_argv */
    for(i = 0; i < c->n_commands; i++)
      free_argv(c->argvs[i], c->argcs[i]);
    free(c->argvs);
  }
  else {
    /* Free dllist argvs if choose to use dllist instead of c->argvs */
    Dllist tmp;
    dll_traverse(tmp, c->comlist)
      free_argv((char**)tmp->val.v, c->argcs[i++]);
  }
  free_dllist(c->comlist);
}

void reset_command(Command *c) {
  c->append_stdout = NOAPPEND;
  c->wait = WAIT;
  c->n_commands = 0;
  free(c->stdinp);  c->stdinp = NULL;
  free(c->stdoutp); c->stdoutp = NULL;
  free_argvs(c);
  c->comlist = new_dllist();
}


void free_command(Command *c) {
  free_argvs(c);
  if(c->argcs != NULL) free(c->argcs);
  if(c->stdinp != NULL) free(c->stdinp);
  if(c->stdoutp != NULL) free(c->stdoutp);
  free(c);
}


Command* make_command() {
  /* Allocate command struct and default values */
  Command *c = (Command*)malloc(sizeof(Command));
  c->append_stdout = NOAPPEND;
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
  /* Update each argc with number of fields in argv */
  c->argcs[c->n_commands++] = is->NF;

  /* Allocate memory for argvs and store them in dllist */
  char **tmp = (char**)malloc(sizeof(char*) * (is->NF + 1));
  for(int i = 0; i < is->NF; i++) { tmp[i] = strdup(is->fields[i]); }
  /* add extra index for NULL terminating */
  tmp[is->NF] = NULL;
  dll_append(c->comlist, new_jval_v(tmp));
}


void print_command(Command *c) {
  printf("stdin:    %s\n", c->stdinp);
  printf("stdout:   %s (Append=%d)\n", c->stdoutp, c->append_stdout);
  printf("N_Commands:  %d\n", c->n_commands);
  printf("Wait:        %d\n", c->wait);

  Dllist tmp;
  int index = 0;
  int argc_tmp;
  char **argv_tmp;

  dll_traverse(tmp, c->comlist) {
    argv_tmp = (char**)(tmp->val.v);
    argc_tmp = c->argcs[index];

    printf("  %d: argc: %d    argv: ", index, argc_tmp);
    for(int i = 0; i < argc_tmp; i++) printf("%s ", argv_tmp[i]);
    printf("\n");
    index++;
  }
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
  if(append == 1) 
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
    if(c->stdoutp != NULL) redirect_output(c->stdoutp, c->append_stdout);

    (void) execvp(c->argvs[0][0], c->argvs[0]);
    perror("execvp failed in execute_command:");
    exit(1);
  } else {
    if(c->wait == WAIT) wait(&status);
  }
}


void read_is(Command *c, IS is, int *letters) {
  /* Reading stdin for jshell commands */
  while(get_line(is) > -1) {
    if(is->fields[0][0] == '#' || is->NF == 0)    /* IGNORE */
      continue;
    else if(is->fields[0][0] == '<')              /* STDIN */
      c->stdinp = strdup(is->fields[1]);
    else if(is->fields[0][0] == '>') {            /* STDOUT */
      c->stdoutp = strdup(is->fields[1]);
      if(strcmp(is->fields[0], ">>") == 0)        /* Append */
        c->append_stdout = 1;
    } 
    else if(strcmp(is->fields[0], "NOWAIT") == 0) /* WAIT */
      c->wait = NOWAIT;
    else if(strcmp(is->fields[0], "END") == 0) {  /* END */
      if(letters[1]== 1) print_command(c); 
      return;
    } 
    else add_command(c, is);                      /* COMMAND */
  }
}


int main(int argc, char *argv[]) {
  IS is = new_inputstruct(NULL);                  /* input proccessing */ 
  Command *com = make_command();                  /* structure for storing commands */
  Dllist tmp;

  /*  Use char array for first commmand line argument
      set index to 1 if letter was found */
  int letters[] = {0, 0, 0};
  if (argc == 2) {
    letters[0] = strchr(argv[1], 'r') != NULL;
    letters[1] = strchr(argv[1], 'p') != NULL;
    letters[2] = strchr(argv[1], 'n') != NULL;
  }
  
  while(1) {
    if(letters[0] == 1) printf("READY\n\n");
    read_is(com, is, letters);
    move_argvs(com);

    if(!letters[2])
      execute_command(com);
    
    reset_command(com);
  }
  
  jettison_inputstruct(is);
  free_command(com);
  return 0;
}