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
| 4 | Not started | - |

```bash
make && make run
make test          # parser tests
```

Week 3 notes: [`docs/week3_parser.md`](docs/week3_parser.md)

## Practical sessions

| # | Status | Topic |
|---|--------|-------|
| 1 | Done | Run a command with `fork()` + `execvp()` + `wait()`; report on how the OS abstracts hardware |
| 2 | Done | File copy using `open/read/write/close`; `strace` and user/kernel transitions |
| 3 | Done | PID, PPID and process states across `fork()` |
| 4 | Done | `wait()` vs `waitpid()`; creating and reaping a zombie process |
| 5 | Done | Producer-consumer over an anonymous pipe; `ls -l \| grep ".c"` using `dup2()` |

```bash
cd practicals/src && make
```

Programs, reports and recorded output: [`practicals/README.md`](practicals/README.md)
