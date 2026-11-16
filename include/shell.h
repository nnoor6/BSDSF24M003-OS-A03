#include <readline/readline.h>
#include <readline/history.h>


#ifndef SHELL_H
#define SHELL_H
#define HISTORY_SIZE 20
extern char* history[HISTORY_SIZE];
extern int history_count;
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "


// Function prototypes

#define MAX_JOBS 20

typedef struct {
    pid_t pid;
    char cmd[256];
    int active;
} Job;

extern Job jobs[MAX_JOBS];
extern int job_count;

void add_job(pid_t pid, const char* cmd);
void reap_zombies(void);
void initialize_readline(void);
int execute_with_redirection(char** arglist);
char* read_cmd_readline(char* prompt);
int handle_builtin(char** arglist);
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist);

#endif // SHELL_H
