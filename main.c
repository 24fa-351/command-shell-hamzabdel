#include "cmdshell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    char line[MAX_LINE];

    while (1) {
        printf("xsh#");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;
        if (strcmp(line, "exit\n") == 0 || strcmp(line, "quit\n") == 0)
            break;

        line[strcspn(line, "\n")] = 0;
        parse_and_execute(line);
    }
    return 0;
}
