#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "process.h"

/*
 * Runs an external command: fork() a child, replace it with the requested
 * program via execvp(), and wait for it in the parent.
 */
int execute(char **tokens)
{
    pid_t pid;
    int status = 0;
    int reaped;

    pid = fork();

    if(pid == 0)
    {
        if(execvp(tokens[0],tokens) == -1)
        {
            perror("ShellForge");
        }
        exit(EXIT_FAILURE);
    }
    else if(pid < 0)
    {
        perror("fork");
    }
    else
    {
        /*
         * Loop until the child exits or is killed. The reaped > 0 guard
         * matters once the SIGCHLD handler is installed in Week 6: the
         * handler may collect this child first, in which case waitpid()
         * returns -1 (ECHILD) and never touches status. Without the guard
         * the condition would test an unset status and spin forever.
         */
        do
        {
            reaped = waitpid(pid,&status,WUNTRACED);
        }while(reaped > 0 && !WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}
