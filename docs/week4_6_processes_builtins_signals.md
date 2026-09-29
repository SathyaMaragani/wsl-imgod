# ShellForge - Weeks 4, 5 and 6

| Week | Milestone | Files added |
|------|-----------|-------------|
| 4 | Execute real Linux commands | `include/process.h`, `src/process.c` |
| 5 | Built-in commands and environment variables | `include/builtin.h`, `src/builtin.c` |
| 6 | Signal handling | `include/signals.h`, `src/signals.c` |

## Week 4 - processes and command execution

A program is a passive file on disk; a process is that program in execution,
with a PID, its own address space and an entry in the kernel's process table.
`execute()` turns a parsed `argv[]` into a running process:

```
fork()  -> a second process, identical apart from the return value
   |          parent gets the child's PID, child gets 0
   +-- child : execvp(tokens[0], tokens)  replaces its image with the program
   +-- parent: waitpid(...)               blocks until the child finishes
```

`execvp` searches `$PATH` for the program and only ever returns on failure -
which is why `perror("ShellForge")` sits directly after it.

Verified:

```
myshell> ls
Makefile  README.md  bin  docs  include  practicals  screenshots  src  tests
myshell> pwd
/home/lucky/wsl-imgod
myshell> whoami
lucky
myshell> date
Tue Sep 29 07:55:15 UTC 2026
myshell> no_such_cmd
ShellForge: No such file or directory
```

The unknown command reports an error and the shell keeps running.

## Week 5 - built-in commands

Some commands cannot be run as child processes. `cd` is the clearest case:

```
shell (cwd = /home/user)
  fork()
    child: chdir("/tmp")   <- only the CHILD moves
    child: exit
  parent still in /home/user   <- nothing changed
```

A child's `chdir()` dies with the child. `cd` must therefore run **in the shell
process itself**, and that is the definition of a built-in.

`execute_builtin()` returns 1 if it handled the command and 0 otherwise, so
`main()` reads:

```c
if (execute_builtin(tokens) == 0)
    execute(tokens);
```

| Built-in | Implemented with | Runs in |
|----------|------------------|---------|
| `cd` | `chdir()` | shell |
| `pwd` | `getcwd()` | shell |
| `env` | `getenv("HOME" / "USER" / "PATH")` | shell |
| `help` | `printf` | shell |
| `clear` | `system("clear")` | shell |
| `exit` | `exit()` | shell |
| everything else | `fork()` + `execvp()` | child |

Verified that `cd` changes the shell's own directory:

```
myshell> pwd
/home/lucky/wsl-imgod
myshell> cd /tmp
myshell> pwd
/tmp
```

## Week 6 - signals

A signal is a software interrupt. It can arrive between any two instructions,
which is what makes handlers delicate.

| Signal | No. | Source | Default |
|--------|-----|--------|---------|
| SIGINT | 2 | Ctrl+C | terminate |
| SIGTSTP | 20 | Ctrl+Z | suspend |
| SIGCHLD | 17 | a child exited | ignored |
| SIGKILL | 9 | `kill -9` | terminate, **cannot be caught** |

`initialize_signals()` installs two handlers:

- **SIGINT** - prints a reminder and returns, so Ctrl+C no longer kills the
  shell.
- **SIGCHLD** - `while (waitpid(-1, NULL, WNOHANG) > 0);` reaps any finished
  child so none is left as a zombie.

Verified:

```
myshell> echo before-signal
before-signal
<SIGINT>
ShellForge: Press 'exit' to quit.
myshell> echo after-signal
after-signal
```

and after running several children, `ps -o stat= --ppid <shell>` reports **0**
processes in state `Z`.

## Tests

`make test` runs both suites:

```
parse_line(): 9 checks passed
shell tests:
  ok    external command runs (execvp)
  ok    unknown command: perror, shell survives
  ok    pwd built-in
  ok    cd built-in changes shell cwd
  ok    cd error handling
  ok    cd usage message
  ok    help built-in
  ok    env built-in
  ok    SIGINT handled, shell survives Ctrl+C
  ok    SIGCHLD reaps children (no zombies)
all shell tests passed
```

Valgrind: `in use at exit: 0 bytes in 0 blocks`, `ERROR SUMMARY: 0 errors`.

## Deviations from the handouts, and why

**1. A race between Week 4's `waitpid()` and Week 6's SIGCHLD handler (bug fix).**
As written, the two chapters combine into a hang. Week 4 has:

```c
do { waitpid(pid, &status, WUNTRACED); }
while (!WIFEXITED(status) && !WIFSIGNALED(status));
```

Once Week 6 installs a SIGCHLD handler that calls `waitpid(-1, ...)`, that
handler can reap the child first. The loop's `waitpid()` then returns -1
(ECHILD) **without touching `status`**, so the condition tests an
uninitialised variable and the shell can spin forever. Fixed by checking the
return value:

```c
do { reaped = waitpid(pid, &status, WUNTRACED); }
while (reaped > 0 && !WIFEXITED(status) && !WIFSIGNALED(status));
```

**2. `write()` instead of `printf()` in the SIGINT handler.**
`printf()` is not async-signal-safe. If Ctrl+C arrives while the program is
already inside stdio, the handler re-enters it with locks held and can deadlock
or corrupt the buffer. `write()` is on the POSIX async-signal-safe list. The
text printed is unchanged. Practical 6 makes the same point from the other
direction, using flags instead.

**3. The SIGCHLD handler saves and restores `errno`.**
A handler that runs between a failed syscall and the caller's `errno` check
would otherwise overwrite the error code.

**4. Return values checked on `getcwd()` and `system()`.**
Both are declared with `warn_unused_result` in glibc, so ignoring them fails a
`-Wall -Wextra` build.

**5. The Week 3 parser test was rewritten.**
It drove the shell and checked the printed `argv[...]` lines. Week 4 replaced
that printing with real execution, so the test had nothing to read. It is now
`tests/test_parser.c`, which links `parser.c` directly and asserts on the
returned array - including a case with 100 arguments to exercise the
`realloc()` growth path.

## Known limitation

`read_line()` returns an empty string at end of input instead of signalling EOF,
so Ctrl+D at the prompt loops rather than exiting. Typing `exit` works. Carried
over from Week 2 and left for a later chapter.
