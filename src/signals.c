#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "signals.h"

/*
 * Ctrl+C must not kill the shell. Only write() is used here: printf() is not
 * async-signal-safe, so calling it from a handler that can interrupt stdio
 * risks a deadlock or corrupted buffer.
 */
static void sigint_handler(int sig)
{
    static const char msg[] = "\nShellForge: Press 'exit' to quit.\nmyshell> ";
    (void)sig;
    if(write(STDOUT_FILENO,msg,strlen(msg)) == -1)
    {
        /* nothing useful can be done from inside a handler */
    }
}

/*
 * Reaps any child that has finished so it does not linger as a zombie.
 * WNOHANG keeps the handler from blocking.
 */
static void sigchld_handler(int sig)
{
    int saved_errno = errno;
    (void)sig;
    while(waitpid(-1,NULL,WNOHANG) > 0)
        ;
    errno = saved_errno;
}

void initialize_signals(void)
{
    signal(SIGINT,sigint_handler);
    signal(SIGCHLD,sigchld_handler);
}
