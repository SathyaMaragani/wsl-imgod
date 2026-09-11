# wsl-imgod

Operating Systems and Systems Programming coursework, developed on Ubuntu 26.04 (WSL2), gcc 15.2.0.

This repository holds two separate bodies of work:

| Part | Location | What it is |
|------|----------|------------|
| **ShellForge** - weekly project | repository root (`src/`, `include/`, `Makefile`) | Project-Based Learning: a Unix-like shell, built up week by week |
| **Practical sessions** | [`practicals/`](practicals/) | Standalone lab practicals 1-5, each self-contained |

---

## Progress so far

### ShellForge (weekly PBL project)

| Week | Status | Delivered |
|------|--------|-----------|
| Week 1 | Done | Interactive REPL loop, Makefile-based build, Git repository, Linux dev environment |
| Week 2 | Done | Dynamic command input, `malloc()` allocation, `realloc()` buffer expansion, `free()` cleanup |
| Week 3 | Done | Command parsing with `strtok()`, dynamic `argv[]` construction, modular parser, ready for `execvp()` |
| Week 4 | Not started | - |

### Practical sessions

| Practical | Status | Program(s) | Topic |
|-----------|--------|-----------|-------|
| 1 - part A | Done | `practicals/src/prog1.c` | Execute a user-entered Linux command via `fork()` + `execvp()` + `wait()`, print both PIDs |
| 1 - part B | Done | `practicals/docs/hardware_abstraction_report.md` | Report on `uname`, `lscpu`, `lsblk`, `ps`, `top` - how the OS abstracts CPU, memory, storage and I/O |
| 2 - part A | Done | `practicals/src/prog2.c` | Copy a file using only `open()`, `read()`, `write()`, `close()` |
| 2 - part B | Done | `practicals/docs/strace_analysis_report.md` | `strace` analysis of `cat sample.txt` and of `prog2`; user space / kernel space transitions |
| 3 | Done | `practicals/src/prog3.c` | PID, PPID and process states at each stage across `fork()` |
| 4 - part A | Done | `practicals/src/wait_waitpid_demo.c` | Multiple children synchronised with `wait()` vs `waitpid()`, with comparison |
| 4 - part B | Done | `practicals/src/zombie_process.c` | Create a zombie process, inspect the process table, then eliminate it with `wait()` |
| 5 - part A | Done | `practicals/src/prog5.c` | Producer-consumer over an anonymous pipe, measuring communication efficiency |
| 5 - part B | Done | `practicals/src/ls_grep_pipe.c` | Implement `ls -l \| grep ".c"` using `pipe()`, `fork()`, `dup2()` and `execlp()` |

### Environment set up

`gcc` 15.2.0, `g++`, `gdb` 17.1, `valgrind` 3.26.0, `strace` 6.19, `make` 4.4.1, `git` 2.53.0.

### Verified results

ShellForge builds with `-Wall -Wextra` with no warnings; the Week 3 parser passes
all six `tests/test_parser.sh` cases and runs clean under valgrind (5 allocs,
5 frees, 0 errors).

All seven practical programs compile with `-Wall -g` with no warnings and were run end to end:

- `prog1` forked a child that `execvp`-ed `ls -l`; the parent reported both PIDs and reaped the child.
- `prog2` copied `input.txt` to `output.txt`; the argument guard returns exit status 1 when given the wrong argument count.
- `prog3` printed PID/PPID for parent and child through sleep, wait and termination.
- `wait_waitpid_demo` recovered exit statuses 10, 20 and 30 via `WEXITSTATUS()`, showing `wait()` collecting whichever child finished first and `waitpid()` targeting a specific child.
- `zombie_process` was captured live in the process table as `1 Z ... zombie_process` / `Z+ [zombie_process] <defunct>`, and the entry disappeared after the parent called `wait()`.
- `prog5` moved 1,000,000 bytes as 10,000 messages through an anonymous pipe in ~0.013 s, about 70 MB/s; all 10,000 arrived intact across three runs.
- `ls_grep_pipe` produced byte-for-byte the same output as the real shell pipeline `ls -l | grep ".c"`.

Full recorded output, including the `ps -el` snapshots, is in
[`practicals/docs/practical_outputs.txt`](practicals/docs/practical_outputs.txt).

---

## Part 1 - ShellForge (weekly PBL project)

A Unix-like shell developed as part of the Operating Systems and Systems
Programming Project-Based Learning course.

### Week 1
- Interactive REPL loop
- Makefile-based build
- Git repository
- Linux development environment

### Week 2
- Dynamic command input
- Memory allocation using `malloc()`
- Automatic buffer expansion using `realloc()`
- Proper memory cleanup using `free()`

### Week 3 Features
- Command parsing using `strtok()`
- Dynamic `argv[]` construction
- Modular parser implementation
- Ready for process execution with `execvp()`

Details in [`docs/week3_parser.md`](docs/week3_parser.md).

### Build and run

```bash
make
make run
make test     # parser tests
```

---

## Part 2 - Practical sessions 1-5

Lab practicals, kept separate from the weekly project. Full details in
[`practicals/README.md`](practicals/README.md).

| Practical | Program | Topic |
|-----------|---------|-------|
| 1 | `practicals/src/prog1.c` | Command execution with `fork()`, `execvp()` and `wait()` |
| 2 | `practicals/src/prog2.c` | File copy using `open()`, `read()`, `write()`, `close()` |
| 3 | `practicals/src/prog3.c` | Process IDs and process states across `fork()` |
| 4 | `practicals/src/wait_waitpid_demo.c` | `wait()` vs `waitpid()` for synchronising multiple children |
| 4 | `practicals/src/zombie_process.c` | Creating and eliminating a zombie process |
| 5 | `practicals/src/prog5.c` | Producer-consumer communication over an anonymous pipe |
| 5 | `practicals/src/ls_grep_pipe.c` | Building a shell pipeline with `pipe()`, `dup2()` and `exec()` |

Written reports for the analysis parts of practicals 1, 2 and 5 are in
[`practicals/docs/`](practicals/docs/).

### Build and run

```bash
cd practicals/src
make
```
