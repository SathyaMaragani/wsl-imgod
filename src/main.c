#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/shell.h"
#include "../include/input.h"
#include "../include/parser.h"
#include "../include/process.h"
#include "../include/builtin.h"
#include "../include/signals.h"
#include "../include/pipes.h"

/*
 * Handles a two-stage pipeline: splits the line at '|' and parses each half
 * with the Week 3 parser, so pipelines get the same tokenizing as any other
 * command. The line buffer is modified in place; the caller still frees it.
 */
static void run_pipeline(char *line, char *bar)
{
    char **argv1;
    char **argv2;

    if (strchr(bar + 1, '|') != NULL)
    {
        printf("ShellForge: only two-command pipelines are supported\n");
        return;
    }

    *bar = '\0';                 /* split the buffer into left and right */

    argv1 = parse_line(line);
    argv2 = parse_line(bar + 1);

    if (argv1[0] == NULL || argv2[0] == NULL)
        printf("Invalid pipe command\n");
    else
        execute_pipe(argv1, argv2);

    free_tokens(argv1);
    free_tokens(argv2);
}

int main()
{
    char *line;
    char **tokens;
    char *bar;

    initialize_signals();

    printf("=================================\n");
    printf("%s Version %s\n",SHELL_NAME,VERSION);
    printf("=================================\n");

    while(1)
    {
        printf("myshell> ");
        fflush(stdout);

        line = read_line();

        if(strcmp(line,"exit")==0)
        {
            free(line);
            break;
        }

        bar = strchr(line,'|');
        if(bar != NULL)
        {
            run_pipeline(line,bar);
            free(line);
            continue;
        }

        tokens = parse_line(line);

        /* built-ins run in the shell itself; everything else is forked */
        if(execute_builtin(tokens)==0)
        {
            execute(tokens);
        }

        free_tokens(tokens);
        free(line);
    }

    printf("Goodbye!\n");
    return 0;
}
