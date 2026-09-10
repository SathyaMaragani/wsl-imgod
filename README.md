# wsl-imgod

Operating Systems and Systems Programming coursework, developed on Ubuntu (WSL2).

This repository holds two separate bodies of work:

| Part | Location | What it is |
|------|----------|------------|
| **ShellForge** - weekly project | repository root (`src/`, `include/`, `Makefile`) | Project-Based Learning: a Unix-like shell, built up week by week |
| **Practical sessions** | [`practicals/`](practicals/) | Standalone lab practicals 1-4, each self-contained |

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

### Build and run

```bash
make
make run
```

---

## Part 2 - Practical sessions 1-4

Lab practicals, kept separate from the weekly project. Full details in
[`practicals/README.md`](practicals/README.md).

| Practical | Program | Topic |
|-----------|---------|-------|
| 1 | `practicals/src/prog1.c` | Command execution with `fork()`, `execvp()` and `wait()` |
| 2 | `practicals/src/prog2.c` | File copy using `open()`, `read()`, `write()`, `close()` |
| 3 | `practicals/src/prog3.c` | Process IDs and process states across `fork()` |
| 4 | `practicals/src/wait_waitpid_demo.c` | `wait()` vs `waitpid()` for synchronising multiple children |
| 4 | `practicals/src/zombie_process.c` | Creating and eliminating a zombie process |

Written reports for the analysis parts of practicals 1 and 2 are in
[`practicals/docs/`](practicals/docs/).

### Build and run

```bash
cd practicals/src
make
```
