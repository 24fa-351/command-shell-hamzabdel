#include "cmdshell.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
    char* name;
    char* value;
} EnvVar;

static EnvVar xsh_env[MAX_VARS];
static int env_count = 0;

void set_environment(const char* name, const char* value)
{
    for (int i = 0; i < env_count; i++) {
        if (strcmp(xsh_env[i].name, name) == 0) {
            free(xsh_env[i].value);
            xsh_env[i].value = strdup(value);
            return;
        }
    }
    if (env_count < MAX_VARS) {
        xsh_env[env_count].name = strdup(name);
        xsh_env[env_count].value = strdup(value);
        env_count++;
    }
}

void delete_environment(const char* name)
{
    for (int i = 0; i < env_count; i++) {
        if (strcmp(xsh_env[i].name, name) == 0) {
            free(xsh_env[i].name);
            free(xsh_env[i].value);
            for (int j = i; j < env_count - 1; j++) {
                xsh_env[j] = xsh_env[j + 1];
            }
            env_count--;
            return;
        }
    }
}

char* get_environment(const char* name)
{
    for (int i = 0; i < env_count; i++) {
        if (strcmp(xsh_env[i].name, name) == 0) {
            return xsh_env[i].value;
        }
    }
    return NULL;
}

char* replace_vars(char* line)
{
    static char buffer[MAX_LINE];
    char* src = line;
    char* dst = buffer;
    while (*src) {
        if (*src == '$') {
            src++;
            char varname[128];
            int len = 0;
            while (*src && *src != ' ' && len < 127) {
                varname[len++] = *src++;
            }
            varname[len] = '\0';
            char* value = get_environment(varname);
            if (value) {
                strcpy(dst, value);
                dst += strlen(value);
            }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
    return buffer;
}

void execute_command(char* command)
{
    char* args[MAX_ARGS];
    int arg_count = 0;
    int input_fd = -1;
    int output_fd = -1;

    if (strncmp(command, "cd", 2) == 0) {
        char* path = strtok(command + 2, " \t");
        if (path == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else if (chdir(path) != 0) {
            perror("cd");
        }
        return;
    }

    if (strcmp(args[0], "set") == 0) {
        if (args[1] != NULL && args[2] != NULL) {
            set_environment(args[1], args[2]);
        } else {
            fprintf(stderr, "set: missing arguments\n");
        }

        return;
    } else if (strcmp(args[0], "unset") == 0) {
        if (args[1] != NULL) {
            delete_environment(args[1]);
        } else {
            fprintf(stderr, "unset: missing argument\n");
        }

        return;
    }

    char* input_file = strstr(command, "<");
    if (input_file) {
        *input_file = '\0';
        input_file = strtok(input_file + 1, " \t");
        input_fd = open(input_file, O_RDONLY);
        if (input_fd < 0) {
            perror("Failed to input file");

            return;
        }
    }

    char* output_file = strstr(command, ">");
    if (output_file) {
        *output_file = '\0';
        output_file = strtok(output_file + 1, " \t");
        output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_fd < 0) {
            perror("Failed to output file");

            return;
        }
    }

    args[arg_count++] = strtok(command, " ");
    while (arg_count < MAX_ARGS && (args[arg_count] = strtok(NULL, " "))) {

        arg_count++;
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (input_fd != -1) {
            dup2(input_fd, STDIN_FILENO);

            close(input_fd);
        }
        if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO);

            close(output_fd);
        }

        execvp(args[0], args);
        perror("exec");

        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, NULL, 0);
        if (input_fd != -1)
            close(input_fd);
        if (output_fd != -1)
            close(output_fd);
    }
}

void parse_and_execute(char* line)
{
    line = replace_vars(line);

    char* commands[MAX_ARGS];
    int cmd_count = 0;

    commands[cmd_count++] = strtok(line, "|");
    while (cmd_count < MAX_ARGS && (commands[cmd_count] = strtok(NULL, "|"))) {
        cmd_count++;
    }

    int in_fd = 0;
    for (int i = 0; i < cmd_count; i++) {
        int pipe_fd[2];
        pipe(pipe_fd);

        if (fork() == 0) {
            if (in_fd != 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i != cmd_count - 1) {
                dup2(pipe_fd[1], STDOUT_FILENO);
            }
            close(pipe_fd[0]);
            execute_command(commands[i]);

            exit(0);
        } else {
            wait(NULL);
            close(pipe_fd[1]);
            in_fd = pipe_fd[0];
        }
    }
}
