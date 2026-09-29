#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "pipes.h"

/*
 * Runs  cmd1 | cmd2  by connecting cmd1's stdout to cmd2's stdin through an
 * anonymous pipe.
 */
void execute_pipe(char **cmd1, char **cmd2)
{
    int pipefd[2];
    pid_t pid1, pid2;

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        return;
    }

    /* First child: execute cmd1, writing into the pipe */
    pid1 = fork();
    if (pid1 == -1)
    {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    if (pid1 == 0)
    {
        /* Child 1 does not need the read end */
        close(pipefd[0]);

        /* Redirect stdout to pipe */
        if (dup2(pipefd[1], STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);

        execvp(cmd1[0], cmd1);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    /* Second child: execute cmd2, reading from the pipe */
    pid2 = fork();
    if (pid2 == -1)
    {
        perror("fork");
        /*
         * Close both ends before giving up, otherwise child 1 blocks forever
         * writing into a pipe nobody reads, and reap it so it is not left as
         * a zombie.
         */
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(pid1, NULL, 0);
        return;
    }
    if (pid2 == 0)
    {
        /* Child 2 does not need the write end */
        close(pipefd[1]);

        /* Redirect stdin from pipe */
        if (dup2(pipefd[0], STDIN_FILENO) == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(pipefd[0]);

        execvp(cmd2[0], cmd2);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    /*
     * The parent uses neither end and MUST close both. If it kept the write
     * end open, cmd2 would never see end-of-file and the shell would hang.
     */
    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}
