# wsl-imgod

OSSP coursework, developed on Ubuntu 26.04 (WSL2).

Two separate parts: **ShellForge**, the weekly shell project in the repository
root, and the **practicals**, standalone labs in [`practicals/`](practicals/).

## ShellForge - weekly project

| Week | Status | Delivered |
|------|--------|-----------|
| 1 | Done | REPL loop, Makefile build, repository setup |
| 2 | Done | Dynamic input using `malloc()`, `realloc()` and `free()` |
| 3 | Done | Command parser using `strtok()`, builds `argv[]` ready for `execvp()` |
| 4 | Done | Command execution with `fork()`, `execvp()` and `waitpid()` |
| 5 | Done | Built-in commands (`cd`, `pwd`, `env`, `help`, `clear`, `exit`) |
| 6 | Done | Signal handling: survives Ctrl+C, reaps zombies via SIGCHLD |
| 7 | Not started | - |

```bash
make && make run
make test          # parser unit tests + shell integration tests
```

Notes: [`docs/week3_parser.md`](docs/week3_parser.md),
[`docs/week4_6_processes_builtins_signals.md`](docs/week4_6_processes_builtins_signals.md)

## Practical sessions

| # | Status | Topic |
|---|--------|-------|
| 1 | Done | Run a command with `fork()` + `execvp()` + `wait()`; how the OS abstracts hardware |
| 2 | Done | File copy using `open/read/write/close`; `strace` and user/kernel transitions |
| 3 | Done | PID, PPID and process states across `fork()` |
| 4 | Done | `wait()` vs `waitpid()`; creating and reaping a zombie process |
| 5 | Done | Producer-consumer over an anonymous pipe; `ls -l \| grep ".c"` using `dup2()` |
| 6 | Done | Client-server over named pipes (FIFOs); `sigaction()` signal handling |
| 7 | Done | Process address space: code, data, BSS, heap, stack vs `/proc/<PID>/maps` |
| 8 | Done | `malloc`/`calloc`/`realloc`/`free`, Valgrind leak detection, copy-on-write |

```bash
cd practicals/src && make
```

Programs, reports and recorded output: [`practicals/README.md`](practicals/README.md)
