#include "shell.h"
#include <fcntl.h>

char* history[HISTORY_SIZE];

int history_count = 0;

ShellVar variables[MAX_VARS];
int var_count = 0;

void set_var(const char* name, const char* value) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            strncpy(variables[i].value, value, 255);
            variables[i].value[255] = '\0';
            return;
        }
    }
    if (var_count < MAX_VARS) {
        strncpy(variables[var_count].name, name, 63);
        variables[var_count].name[63] = '\0';
        strncpy(variables[var_count].value, value, 255);
        variables[var_count].value[255] = '\0';
        var_count++;
    }
}

char* get_var(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }
    return NULL;
}

void expand_vars(char** arglist) {
    for (int i = 0; arglist[i] != NULL; i++) {
        if (arglist[i][0] == '$') {
            char* var_name = arglist[i] + 1; // skip the $
            char* val = get_var(var_name);
            if (val) {
                // Replace the argument with the variable's value
                strncpy(arglist[i], val, ARGLEN - 1);
                arglist[i][ARGLEN - 1] = '\0';
            }
        }
    }
}
Job jobs[MAX_JOBS];
int job_count = 0;

/* ---------- Job Control ---------- */
void add_job(pid_t pid, const char* cmd) {
    if (job_count >= MAX_JOBS) return;
    jobs[job_count].pid = pid;
    strncpy(jobs[job_count].cmd, cmd, 255);
    jobs[job_count].cmd[255] = '\0';
    jobs[job_count].active = 1;
    job_count++;
}

void reap_zombies(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].pid == pid && jobs[i].active) {
                printf("\n[Background job %d done] %s\n", pid, jobs[i].cmd);
                jobs[i].active = 0;
                break;
            }
        }
    }
}

/* ---------- Readline completion ---------- */
char *command_generator(const char *text, int state) {
    static int i, len;
    static char *cmds[] = {"cd", "exit", "help", "jobs", "history", NULL};
    if (!state) { i = 0; len = strlen(text); }
    while (cmds[i]) {
        char *c = cmds[i++];
        if (strncmp(c, text, len) == 0) return strdup(c);
    }
    return NULL;
}

char **my_completion(const char *text, int start, int end) {
    (void)end;
    if (start == 0) return rl_completion_matches(text, command_generator);
    return NULL;
}

void initialize_readline(void) {
    rl_readline_name = "myshell";
    rl_attempted_completion_function = my_completion;
}

/* ---------- Input reading ---------- */
char* read_cmd_readline(char* prompt) {
    char* line = readline(prompt);
    if (line == NULL) return NULL;
    if (line[0] != '\0') add_history(line);
    return line;
}

/* ---------- Tokenizer (Fixed for quotes) ---------- */
char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') return NULL;

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;
        if (*cp == '\0') break;

        int len = 0;
        int in_quotes = 0;

        while (*cp != '\0' && (in_quotes || (*cp != ' ' && *cp != '\t'))) {
            if (*cp == '"') {
                in_quotes = !in_quotes;
                cp++;
            } else {
                arglist[argnum][len++] = *cp;
                cp++;
            }
        }
        arglist[argnum][len] = '\0';
        if (len > 0) argnum++;
    }

    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

/* ---------- Built-ins ---------- */
int handle_builtin(char** arglist) {
    if (arglist[0] == NULL) return 0;

    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) chdir(getenv("HOME"));
        else if (chdir(arglist[1]) != 0) perror("cd failed");
        return 1;
    }

    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <dir>  - Change directory\n");
        printf("  exit      - Exit the shell\n");
        printf("  help      - Show this help message\n");
        printf("  jobs      - List background jobs\n");
        printf("  history   - Show command history\n");
	printf("  set       - Show shell variables\n");
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        int found = 0;
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].active) {
                printf("[%d] %d  %s\n", i + 1, jobs[i].pid, jobs[i].cmd);
                found = 1;
            }
        }
        if (!found) printf("No active background jobs.\n");
        return 1;
    }

    if (strcmp(arglist[0], "history") == 0) {
        for (int i = 0; i < history_count; i++) {
            printf("%d  %s\n", i + 1, history[i]);
        }
        return 1;
    }
	    if (strcmp(arglist[0], "set") == 0 || strcmp(arglist[0], "export") == 0) {
        for (int i = 0; i < var_count; i++) {
            printf("%s=%s\n", variables[i].name, variables[i].value);
        }
        return 1;
    }

    return 0;
}

/* ---------- Feature 7: if-then-else-fi ---------- */
int run_command_and_get_status(char* cmdline) {
    char* copy = strdup(cmdline);
    char** arglist = tokenize(copy);

    if (arglist == NULL) {
        free(copy);
        return 1;
    }

    if (handle_builtin(arglist) == 1) {
        for (int i = 0; arglist[i] != NULL; i++) free(arglist[i]);
        free(arglist);
        free(copy);
        return 0;
    }

    char* infile = NULL;
    char* outfile = NULL;
    char* clean_args[MAXARGS];
    int ci = 0;
    
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], ">") == 0 && arglist[i+1]) { outfile = arglist[++i]; }
        else if (strcmp(arglist[i], "<") == 0 && arglist[i+1]) { infile = arglist[++i]; }
        else { clean_args[ci++] = arglist[i]; }
    }
    clean_args[ci] = NULL;

    int status;
    pid_t pid = fork();
    
    if (pid == 0) {
        if (infile) {
            int fd = open(infile, O_RDONLY);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDIN_FILENO); close(fd);
        }
        if (outfile) {
            int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDOUT_FILENO); close(fd);
        }
        execvp(clean_args[0], clean_args);
        perror("if-command not found");
        exit(127);
    }
    
    waitpid(pid, &status, 0);

    for (int i = 0; arglist[i] != NULL; i++) free(arglist[i]);
    free(arglist);
    free(copy);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status); 
    }
    return 1;
}

static void run_block(char* block) {
    if (block == NULL || block[0] == '\0') return;
    char* saveptr;
    char* cmd = strtok_r(block, "\n;", &saveptr);
    while (cmd != NULL) {
        while (*cmd == ' ' || *cmd == '\t') cmd++;
        if (*cmd != '\0') {
            char* copy = strdup(cmd);
            char** arglist = tokenize(copy);
            if (arglist) {
                if (handle_builtin(arglist) == 0) {
                    execute_with_redirection(arglist);
                }
                for (int i = 0; arglist[i] != NULL; i++) free(arglist[i]);
                free(arglist);
            }
            free(copy);
        }
        cmd = strtok_r(NULL, "\n;", &saveptr);
    }
}

int execute_if_block(char* first_line) {
    char condition[512];
    char then_block[2048] = "";
    char else_block[2048] = "";
    int in_then = 0, in_else = 0;
    int has_else = 0;
    int found_fi = 0;

    char* cond = first_line + 2;
    while (*cond == ' ') cond++;
    strncpy(condition, cond, sizeof(condition) - 1);
    condition[sizeof(condition) - 1] = '\0';

    char* then_ptr = strstr(condition, " then");
    if (then_ptr) {
        *then_ptr = '\0';
        char* after_then = then_ptr + 5;
        while (*after_then == ' ') after_then++;
        if (*after_then) {
            strncpy(then_block, after_then, sizeof(then_block) - 1);
        }
        in_then = 1;
    }

    char* line;
    while ((line = readline("> ")) != NULL) {
        add_history(line);

        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

        if (strncmp(trimmed, "fi", 2) == 0 && (trimmed[2] == '\0' || trimmed[2] == ' ')) {
            found_fi = 1;
            free(line);
            break;
        }

        if (strncmp(trimmed, "then", 4) == 0 && (trimmed[4] == '\0' || trimmed[4] == ' ')) {
            in_then = 1;
            in_else = 0;
            char* rest = trimmed + 4;
            while (*rest == ' ') rest++;
            if (*rest) {
                strncat(then_block, rest, sizeof(then_block) - strlen(then_block) - 1);
                strncat(then_block, "\n", sizeof(then_block) - strlen(then_block) - 1);
            }
            free(line);
            continue;
        }

        if (strncmp(trimmed, "else", 4) == 0 && (trimmed[4] == '\0' || trimmed[4] == ' ')) {
            in_then = 0;
            in_else = 1;
            has_else = 1;
            free(line);
            continue;
        }

        if (in_then) {
            strncat(then_block, trimmed, sizeof(then_block) - strlen(then_block) - 1);
            strncat(then_block, "\n", sizeof(then_block) - strlen(then_block) - 1);
        } else if (in_else) {
            strncat(else_block, trimmed, sizeof(else_block) - strlen(else_block) - 1);
            strncat(else_block, "\n", sizeof(else_block) - strlen(else_block) - 1);
        }
        free(line);
    }

    if (!found_fi) {
        printf("Error: 'fi' not found.\n");
        return 1;
    }

    // --- DEBUG PRINT ---
    
    int status = run_command_and_get_status(condition);
    
    // --- DEBUG PRINT ---

    if (status == 0) {
        run_block(then_block);
    } else if (has_else) {
        run_block(else_block);
    }
    return 0;
}
