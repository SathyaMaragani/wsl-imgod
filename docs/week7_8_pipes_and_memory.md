# ShellForge - Weeks 7 and 8

| Week | Milestone | Files |
|------|-----------|-------|
| 7 | Execute piped commands with `pipe()` and `dup2()` | `include/pipes.h`, `src/pipes.c`, `src/main.c` |
| 8 | Eliminate memory leaks, debug with Valgrind / GDB / ASan | no new modules - Makefile targets and an audit |

## Week 7 - pipes and IPC

### What a pipe is

An anonymous pipe is a unidirectional byte channel between related processes,
buffered by the kernel:

```
cmd1 ──write──> [ kernel pipe buffer ] ──read──> cmd2
```

`pipe(fd)` returns two descriptors: `fd[0]` to read, `fd[1]` to write.

### How `ls | wc -l` runs

```
parent            pipe(fd)
  |
  ├── fork ──> child 1:  close(fd[0])
  |                      dup2(fd[1], STDOUT_FILENO)   stdout -> pipe
  |                      close(fd[1])
  |                      execvp("ls", ...)
  |
  ├── fork ──> child 2:  close(fd[1])
  |                      dup2(fd[0], STDIN_FILENO)    stdin  <- pipe
  |                      close(fd[0])
  |                      execvp("wc", ...)
  |
  ├── close(fd[0]); close(fd[1])      <-- parent must close BOTH
  └── waitpid(child1); waitpid(child2)
```

`dup2(oldfd, newfd)` makes `newfd` refer to the same open file as `oldfd`,
closing whatever `newfd` pointed at first. After `dup2(fd[1], STDOUT_FILENO)`,
descriptor 1 *is* the pipe, so `ls` writes into the pipe without knowing it -
which is why `ls` and `wc` need no modification at all.

### Why every close matters

A pipe reports end-of-file only when **all** write descriptors are closed.
If the parent kept `fd[1]` open, `wc` would wait forever for input that can no
longer arrive, and the shell would hang. The same applies to child 2: it closes
the write end before reading. Each of the five `close()` calls removes one
reference.

### Parsing the `|`

The line is split at the first `|` with `strchr()`, then **each half is passed
to the Week 3 parser**:

```c
*bar = '\0';
argv1 = parse_line(line);
argv2 = parse_line(bar + 1);
```

### Verified

```
myshell> ls | wc -l
9
myshell> seq 3 | tac
3
2
1
myshell> cat /etc/hostname | sort
Lucky-Legion
myshell> ps -ef | head -3
UID  PID  PPID  C STIME TTY  TIME CMD
root   1     0 12 08:51 ?    00:00:00 /sbin/init
root   2     1  0 08:51 ?    00:00:00 /init
```

Malformed input is rejected rather than crashed on:

```
myshell> | wc
Invalid pipe command
myshell> ls |
Invalid pipe command
myshell> a | b | c
ShellForge: only two-command pipelines are supported
```

## Week 8 - memory management and debugging

No new modules. The work is verification and hardening.

### New Makefile targets

```make
asan:                 # AddressSanitizer build -> bin/shellforge_asan
memcheck:             # scripted session under valgrind, fails the build on error
```

### Valgrind

```
$ make memcheck
==488==     in use at exit: 0 bytes in 0 blocks
==488==   total heap usage: 12 allocs, 12 frees, 11,072 bytes allocated
==488== All heap blocks were freed -- no leaks are possible
==488== ERROR SUMMARY: 0 errors from 0 contexts
```

The session exercised `echo`, `pwd`, a pipeline and a failing command, so the
allocation paths of the parser, the pipeline splitter and the error path are all
covered.

**One subtlety worth knowing.** Without `--child-silent-after-fork=yes`, a
second report appears:

```
==597==     in use at exit: 576 bytes in 2 blocks
==597==    definitely lost: 0 bytes in 0 blocks
==597==    still reachable: 576 bytes in 2 blocks
```

That is the **child** produced for a command that does not exist. After
`execvp()` fails, the child calls `exit()` while still holding the copy of the
parent's line buffer (64 bytes) and token array (512 bytes) it inherited
through `fork()`. Nothing is lost - `definitely lost` is 0, the blocks are
still reachable, and the process is about to disappear. The flag stops valgrind
double-reporting the forked copies.

### AddressSanitizer

```
$ make asan
$ ./bin/shellforge_asan
myshell> echo asan-ok
asan-ok
myshell> ls | wc -l
9
myshell> no_such_cmd
ShellForge: No such file or directory
myshell> exit
Goodbye!
```

No `ERROR: AddressSanitizer` output - no overflow, use-after-free or
double-free on any exercised path. ASan catches these at the moment they
happen, unlike valgrind which is slower but needs no rebuild.

### GDB

```
$ gdb ./bin/shellforge
(gdb) break execute_pipe
(gdb) run
myshell> ls | wc -l

Breakpoint 1, execute_pipe (cmd1=0x55555555b080, cmd2=0x55555555b290)
    at src/pipes.c:12
(gdb) print cmd1[0]
$1 = 0x55555555a020 "ls"
(gdb) print cmd2[0]
$2 = 0x55555555a025 "wc"
(gdb) bt
#0  execute_pipe (...) at src/pipes.c:12
#1  0x000055555555554f in run_pipeline (line=0x55555555a020 "ls", bar=...)
    at src/main.c:36
#2  0x0000555555555643 in main () at src/main.c:70
```

The backtrace confirms the call path `main -> run_pipeline -> execute_pipe`,
and the arguments confirm the line was split into `ls` and `wc` correctly.

### Memory-management checklist

| Check | Status |
|-------|--------|
| every `malloc`/`realloc` result tested before use | yes - `input.c`, `parser.c` both `perror` and exit on failure |
| every allocation freed on every path | yes - `free_tokens()` + `free(line)` on the normal, pipeline and `exit` paths |
| no pointer freed twice | yes - tokens point *into* the line buffer, so only the array is freed |
| no access after free | yes - valgrind and ASan both clean |
| every file descriptor closed | yes - all five pipe closes; `close()` before every `exit` path in `pipes.c` |
| every child reaped | yes - `waitpid()` in `process.c` and `pipes.c`, plus the SIGCHLD handler |
| syscall return values checked | `fork`, `pipe`, `dup2`, `execvp`, `chdir`, `getcwd`, `system`, `waitpid` |

### Tests

`make test` runs 9 parser assertions and 14 shell tests:

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
  ok    pipeline: echo | tr
  ok    pipeline: data flows through the pipe in order
  ok    shell survives a pipeline
  ok    malformed pipelines rejected
all shell tests passed
```

## Deviations from the Week 7 handout, and why

**1. The handout's `main.c` does not compile.** Its braces are unbalanced - the
`else` branch opens two blocks, `return 0;` ends up inside the `while` loop, and
a stray `}` closes the function early. Rewritten with the loop structure intact.

**2. It silently removes the Week 5 built-ins.** The handout's Week 7 `main.c`
calls `execute(tokens)` directly, dropping the `execute_builtin()` dispatch. That
would regress `cd`, `pwd`, `env`, `help` and `clear` back to not working. The
dispatch is kept.

**3. `pipes.c` starts `include <stdio.h>` without the `#`.** Typo, fixed.

**4. The handout adds a second tokenizer.** It defines a local `tokenize()`
writing into `char *argv1[64]` - a duplicate of `parse_line()` that also
overflows silently past 63 arguments. `parse_line()` is reused instead: one
tokenizer, and it grows with `realloc()`.

**5. Leaks in the handout's pipe path.** Its pipe branch never frees `line`, and
its `continue` on a malformed pipeline skips the free as well - a leak on every
piped command. Both paths free now, which is what makes `make memcheck` clean.

**6. `strchr()` rather than `strtok()` to split on `|`.** The handout calls
`strtok(line, "|")` and then `tokenize()`, which also uses `strtok` - nesting
two `strtok` walks over different strings, which does not work because `strtok`
keeps a single internal cursor.

**7. `pipes.c` cleans up if the second `fork()` fails.** As written it returns
with both pipe ends still open and child 1 unreaped, which would leave child 1
blocked forever writing into a pipe nobody will read.

## Known limitations (not in scope for chapters 1-8)

- **No quote handling.** `echo "a b"` passes the quote characters through
  literally, because the parser splits purely on whitespace. Quoting has not
  been introduced by any chapter yet.
- **Two-stage pipelines only.** `a | b | c` is reported rather than run.
- **No redirection** (`>`, `<`, `>>`) and no background jobs (`&`).
- `read_line()` returns an empty string at end of input instead of signalling
  EOF, so Ctrl+D loops at the prompt. Typing `exit` works.
