#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "jrb.h"
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
  // char ***argvs;        /* argcv[i] is the argv array for the i-th command*/ 
  Dllist list;          /* I use this to incrementally read the commands.*/ 
} Command;


/* Free strings in c->list */
void free_list(Command *c) {
  Dllist tmp;
  char **argv_tmp;
  int argc_tmp;
  int index = 0;

  /* Free char** and char* for argvs */ 
  dll_traverse(tmp, c->list) {
    argv_tmp = (char**)tmp->val.v;
    argc_tmp = c->argcs[index++];
    for(int i = 0; i < argc_tmp; i++) free(argv_tmp[i]);
    free(argv_tmp);
  }
}


/*  Reset pointers and values to reset command before READY
    NOTE: argcs & list are not deallocated at until the end */
void reset_command(Command *c) {
  free_list(c);
  // if(c->argvs) { free(c->argvs); c->argvs = NULL; }
  free(c->stdinp);  c->stdinp = NULL;
  free(c->stdoutp); c->stdoutp = NULL;

  /* Remove all nodes in dllist */
  while(!dll_empty(c->list))
    dll_delete_node(dll_first(c->list));

  c->append = NOAPPEND;
  c->wait = WAIT;
  c->n_commands = 0;
}


void free_all(Command *c, IS is) {
  jettison_inputstruct(is);
  /* free memory in dllist first, then free c->argvs */
  free_list(c);
  free_dllist(c->list);
  // if(c->argvs != NULL) free(c->argvs);
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
  // c->argvs = NULL;
  c->argcs = (int*)malloc(BUFSIZ);
  c->list = new_dllist();
  return c;
}


void move_argvs(Command *c) {
  Dllist tmp;
  int index = 0;

  /* Allocate space for argvs and copy over from list */
  // c->argvs = (char***)malloc(sizeof(char**) * c->n_commands);
  dll_traverse(tmp, c->list) {
    // c->argvs[index++] = (char**)tmp->val.v;
  }
}


void add_command(Command *c, IS is) {
  /* Update each argc_tmp with number of fields in argv_tmp */
  c->argcs[c->n_commands++] = is->NF;

  /* Allocate memory for argvs and store them in dllist */
  char **tmp = (char**)malloc(sizeof(char*) * (is->NF + 1));
  for(int i = 0; i < is->NF; i++) 
    tmp[i] = strdup(is->fields[i]);

  /* Add to list and add extra index for NULL terminating */
  tmp[is->NF] = NULL;
  dll_append(c->list, new_jval_v(tmp));
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
  dll_traverse(tmp, c->list) {
    argv_tmp = (char**)(tmp->val.v);
    argc_tmp = c->argcs[index++];

    printf("  %d: argc_tmp: %d    argv_tmp: ", index, argc_tmp);
    for(int i = 0; i < argc_tmp; i++) printf("%s ", argv_tmp[i]);
    printf("\n");
  }
  printf("\n");
}


void redirect_stdin(const char *input) {
  int fd = open(input, O_RDONLY);
  if(fd < 0) {
    perror("fd for input failed");
    exit(1);
  }
  if(dup2(fd, STDIN_FILENO) < 0) {
    perror("dup2 for stdin failed:");
    exit(1);
  } 
  close(fd);
}


void redirect_stdout(const char *output, int append) {
  int fd;
  if(append == APPEND) 
    fd = open(output, O_WRONLY | O_CREAT| O_APPEND, 0644);
  else  
    fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if(fd < 0) {
    perror("fd for output failed");
    exit(1);
  }
  if(dup2(fd, STDOUT_FILENO) < 0) {
    perror("dup2 for stdout failed:");
    exit(1);
  } 
  close(fd);
}


void piping(Command *c) {
  pid_t pid;
  int prev_read_fd = 0;
  int pipefd[2];
  int status;
  int N = c->n_commands;
  JRB tmp;
  JRB pids = make_jrb();
  Dllist l = dll_first(c->list);

  /* INSIDE FORK LOOP*/
  for(int i = 0; i < N; i++) {
    /* Create pipe if i < N-1 (aka N-1 pipes) */
    if(i < N-1) {
      if(pipe(pipefd) < 0) {
        perror("piping() - error creating pipes:");
        exit(1);
      }
    }

    /* FLUSH STDIN, STDOUT, AND STDERR BEFORE ANY FORKS */ 
    fflush(stdin);
    fflush(stdout);
    fflush(stderr);
    pid = fork(); 


    /* CHILD */
    if(pid == 0) {
      /* Very first stdin and very final stdout */
      if(i == 0 && c->stdinp != NULL)     redirect_stdin(c->stdinp);
      if(i == N-1 && c->stdoutp != NULL)  redirect_stdout(c->stdoutp, c->append);

      /* Middle process stdin & and N > 1 */
      if(i > 0) {
        if(dup2(prev_read_fd, STDIN_FILENO) == -1) {
          perror("Mid Proc STDIN:");
          exit(1);
        }
        close(prev_read_fd);
      }

      /* Middle process stdout & N > 1 */
      if(i < N-1) {
        if(dup2(pipefd[1], STDOUT_FILENO) == -1) {
          perror("Mid Proc STDOUT:");
          exit(1);
        }
        close(pipefd[0]);
        close(pipefd[1]);
      }

      /*
      char **argv_tmp = (char**)dll_first(c->list)->val.v;
      (void) execvp(argv_tmp[0], argv_tmp);
      perror("execvp failed in piping():");
      exit(1);
      */
      // Call execvp to execute command on null terminated argv
      char **argv_tmp = (char**)l->val.v;
      l = l->flink;
      execvp(argv_tmp[0], argv_tmp);
      perror(argv_tmp[0]);
      exit(1);
    }
    /* PARENT */
    else if(pid > 0) {
      /* Don't wait inside fork loop */
      if(c->wait == WAIT) jrb_insert_int(pids, pid, new_jval_i(0));
      /* Skip if only one command */
      if(N == 1) continue;
      /* THe rest of the body is if more than one command */
      if(prev_read_fd != 0) close(prev_read_fd);
      if(i < N-1) {
        close(pipefd[1]);
        prev_read_fd = pipefd[0];
      }
    }
    /* ERROR */
    else {                              
      perror("piping() - fork:");
      exit(1);
    }
  }
  if(c->wait == WAIT) {
    /* OUTSIDE FORK LOOP */
    while(!jrb_empty(pids)) {
      pid = wait(&status);
      tmp = jrb_find_int(pids, pid);
      if(tmp != NULL) {
        jrb_delete_node(tmp);
      }
    } 
  }
  jrb_free_tree(pids);
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
  
  if(letters[0] == 1) printf("READY\n\n");

  /* Reading stdin for jshell commands */
  while(get_line(is) >= 0) {

    if(is->fields[0][0] == '#' || is->NF == 0)    /* IGNORE */
      continue;
    else if(is->fields[0][0] == '<')              /* STDIN */
      com->stdinp = strdup(is->fields[1]);
    else if(is->fields[0][0] == '>') {            /* STDOUT */
      com->stdoutp = strdup(is->fields[1]);
      if(strcmp(is->fields[0], ">>") == 0)        /* Append */
        com->append = 1;
    } 

    else if(strcmp(is->fields[0], "NOWAIT") == 0) /* WAIT */
      com->wait = NOWAIT;
    else if(strcmp(is->fields[0], "END") == 0) {  /* END */
      if(letters[1]== 1) print_command(com); 
      // move_argvs(com);
      /* Run commands if no 'n' and actual commands given */
      if(!letters[2] && !dll_empty(com->list)) {
        piping(com);
        reset_command(com);
      }
    } 
    else add_command(com, is);                    /* COMMAND */
  }

  free_all(com, is);
  return 0;
}