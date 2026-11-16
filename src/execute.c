#include "shell.h"
#include <fcntl.h>

int run_in_background = 0;

extern void add_job(pid_t pid, const char* cmd);

/* Helper: find a token in arglist, return its index or -1 */
static int find_token(char** arglist, const char* token) {
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], token) == 0) return i;
    }
    return -1;
}

/* Execute a command with possible < and > redirection */
static int execute_redirected(char** arglist, char* infile, char* outfile) {
    int status;
    int cpid = fork();

    if (cpid < 0) {
        perror("fork failed");
        return 1;
    }

    if (cpid == 0) {
        // Child: set up redirection
        if (infile != NULL) {
            int fd_in = open(infile, O_RDONLY);
            if (fd_in < 0) {
                perror("open input failed");
                exit(1);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        if (outfile != NULL) {
            int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) {
                perror("open output failed");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        execvp(arglist[0], arglist);
        perror("Command not found");
        exit(1);
    }

    // Parent
    if (run_in_background) {
        add_job(cpid, arglist[0]);
    } else {
        waitpid(cpid, &status, 0);
    }
    return 0;
}

/* Execute a pipeline: cmd1 | cmd2 */
static int execute_pipe(char** left_args, char** right_args) {
    int pipefd[2];
    int status;
    pid_t left_pid, right_pid;

    if (pipe(pipefd) < 0) {
        perror("pipe failed");
        return 1;
    }

    left_pid = fork();
    if (left_pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp(left_args[0], left_args);
        perror("Left command not found");
        exit(1);
    }

    right_pid = fork();
    if (right_pid == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execvp(right_args[0], right_args);
        perror("Right command not found");
        exit(1);
    }

    // Parent: close both ends
    close(pipefd[0]);
    close(pipefd[1]);

    if (run_in_background) {
        add_job(right_pid, "pipeline");
    } else {
        waitpid(left_pid, &status, 0);
        waitpid(right_pid, &status, 0);
    }
    return 0;
}

int execute_with_redirection(char** arglist) {
    int pipe_idx = find_token(arglist, "|");

    // --- Pipeline case ---
    if (pipe_idx != -1) {
        char* left_args[MAXARGS];
        char* right_args[MAXARGS];
        int i, j;

        for (i = 0; i < pipe_idx; i++) left_args[i] = arglist[i];
        left_args[i] = NULL;

        for (j = 0; arglist[pipe_idx + 1 + j] != NULL; j++) {
            right_args[j] = arglist[pipe_idx + 1 + j];
        }
        right_args[j] = NULL;

        return execute_pipe(left_args, right_args);
    }

    // --- Redirection case ---
    char* infile = NULL;
    char* outfile = NULL;
    char* clean_args[MAXARGS];
    int ci = 0;

    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], ">") == 0) {
            outfile = arglist[++i];
        } else if (strcmp(arglist[i], "<") == 0) {
            infile = arglist[++i];
        } else {
            clean_args[ci++] = arglist[i];
        }
    }
    clean_args[ci] = NULL;

    return execute_redirected(clean_args, infile, outfile);
}

/* Old execute() — kept for compatibility, but main.c uses execute_with_redirection */
int execute(char* arglist[]) {
    return execute_with_redirection(arglist);
}
