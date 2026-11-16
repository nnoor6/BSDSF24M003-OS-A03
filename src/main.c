#include "shell.h"

int main() {
    initialize_readline();

    char* cmdline;
    char** arglist;

    while (1) {
        // Reap zombies before showing prompt
        reap_zombies();

        cmdline = read_cmd_readline(PROMPT);
        if (cmdline == NULL) break;
        if (cmdline[0] == '\0') { free(cmdline); continue; }

        // Handle !n
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
            if (history_count == HISTORY_SIZE) {
                free(history[0]);
                for (int i = 1; i < HISTORY_SIZE; i++) history[i - 1] = history[i];
                history_count--;
            }
            history[history_count++] = strdup(cmdline);
        }

        // Split by ';' for sequential chaining
        char* saveptr1;
        char* segment = strtok_r(cmdline, ";", &saveptr1);
        while (segment != NULL) {
            // Trim leading spaces
            while (*segment == ' ') segment++;

            // Skip empty segments
            if (*segment == '\0') { segment = strtok_r(NULL, ";", &saveptr1); continue; }

            // Check for trailing '&' → background
            int background = 0;
            int len = strlen(segment);
            if (len > 0 && segment[len - 1] == '&') {
                background = 1;
                segment[len - 1] = '\0';
                while (len > 1 && segment[len - 2] == ' ') { segment[len - 2] = '\0'; len--; }
            }

            // Tokenize the segment
            char* seg_copy = strdup(segment);
            arglist = tokenize(seg_copy);

            if (arglist != NULL) {
                if (handle_builtin(arglist) == 0) {
                    if (background) {
                        // Set a global flag used by execute_with_redirection
                        extern int run_in_background;
                        run_in_background = 1;
                        execute_with_redirection(arglist);
                        run_in_background = 0;
                    } else {
                        execute_with_redirection(arglist);
                    }
                }
                for (int i = 0; arglist[i] != NULL; i++) free(arglist[i]);
                free(arglist);
            }
            free(seg_copy);
            segment = strtok_r(NULL, ";", &saveptr1);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
