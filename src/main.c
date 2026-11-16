#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        // Skip empty lines
        if (cmdline[0] == '\0') {
            free(cmdline);
            continue;
        }

        // Handle !n (re-execution)
        if (cmdline[0] == '!') {
            int n = atoi(&cmdline[1]);
            if (n > 0 && n <= history_count) {
                free(cmdline);
                cmdline = strdup(history[n - 1]);
                printf("%s\n", cmdline);
            } else {
                printf("Error: No such command in history.\n");
                free(cmdline);
                continue;
            }
        } else {
            // Add to history (shift if full)
            if (history_count == HISTORY_SIZE) {
                free(history[0]);
                for (int i = 1; i < HISTORY_SIZE; i++) {
                    history[i - 1] = history[i];
                }
                history_count--;
            }
            history[history_count++] = strdup(cmdline);
        }

        // Tokenize and execute
        if ((arglist = tokenize(cmdline)) != NULL) {
            if (handle_builtin(arglist) == 0) {
                execute(arglist);
            }
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
