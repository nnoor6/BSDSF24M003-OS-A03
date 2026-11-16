#include "shell.h"

char* history[HISTORY_SIZE];
int history_count = 0;

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
    if (line == NULL) return NULL;   // Ctrl+D
    if (line[0] != '\0') add_history(line);
    return line;
}

char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;
    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }
    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL;
    }
    cmdline[pos] = '\0';
    return cmdline;
}

/* ---------- Tokenizer ---------- */

char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') return NULL;

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;
        if (*cp == '\0') break;
        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) len++;
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
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
        if (arglist[1] == NULL) {
            chdir(getenv("HOME"));
        } else {
            if (chdir(arglist[1]) != 0) perror("cd failed");
        }
        return 1;
    }

    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <dir>  - Change directory\n");
        printf("  exit      - Exit the shell\n");
        printf("  help      - Show this help message\n");
        printf("  jobs      - List background jobs (not yet implemented)\n");
        printf("  history   - Show command history\n");
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    if (strcmp(arglist[0], "history") == 0) {
        for (int i = 0; i < history_count; i++) {
            printf("%d  %s\n", i + 1, history[i]);
        }
        return 1;
    }

    return 0;
}
