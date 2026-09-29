#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "builtin.h"

/*
 * Runs a command inside the shell process itself.
 * Returns 1 if the command was handled here, 0 if it is an external program.
 *
 * cd must be a built-in: run through fork()/execvp() the chdir() would apply
 * to the child, which then exits, leaving the shell in its old directory.
 */
int execute_builtin(char **args)
{
    char cwd[1024];

    if(args[0] == NULL)
        return 1;

    /* exit */
    if(strcmp(args[0],"exit") == 0)
    {
        exit(EXIT_SUCCESS);
    }

    /* pwd */
    if(strcmp(args[0],"pwd") == 0)
    {
        if(getcwd(cwd,sizeof(cwd)) != NULL)
            printf("%s\n",cwd);
        else
            perror("pwd");
        return 1;
    }

    /* cd */
    if(strcmp(args[0],"cd") == 0)
    {
        if(args[1] == NULL)
        {
            printf("Usage : cd directory\n");
        }
        else
        {
            if(chdir(args[1]) != 0)
                perror("cd");
        }
        return 1;
    }

    /* clear */
    if(strcmp(args[0],"clear") == 0)
    {
        if(system("clear") == -1)
            perror("clear");
        return 1;
    }

    /* help */
    if(strcmp(args[0],"help") == 0)
    {
        printf("\nBuilt-in Commands\n");
        printf("---------------------------\n");
        printf("cd\n");
        printf("pwd\n");
        printf("clear\n");
        printf("exit\n");
        printf("help\n");
        printf("env\n");
        return 1;
    }

    /* env */
    if(strcmp(args[0],"env") == 0)
    {
        printf("HOME = %s\n",getenv("HOME"));
        printf("USER = %s\n",getenv("USER"));
        printf("PATH = %s\n",getenv("PATH"));
        return 1;
    }

    return 0;
}
