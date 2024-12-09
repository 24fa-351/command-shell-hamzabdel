#ifndef CMDSHELL_H
#define CMDSHELL_H

#define MAX_LINE 1024
#define MAX_ARGS 100
#define MAX_VARS 100

void delete_environment(const char* name);
char* get_environment(const char* name);
void set_environment(const char* name, const char* value);
char* replace_vars(char* line);
void execute_command(char* command);
void parse_and_execute(char* line);

#endif
