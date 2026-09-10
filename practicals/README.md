# Practical Sessions 1-5

Lab practicals for Operating Systems and Systems Programming.
Separate from the weekly ShellForge project in the repository root.

Built and run on Ubuntu 26.04 (WSL2), gcc 15.2.0.

## Layout

```
practicals/
|-- src/       C sources, Makefile, test data
|-- include/   headers (kept for the required directory structure)
|-- tests/     test inputs and expected output
`-- docs/      written reports and raw strace / hardware captures
```

## Programs

| Practical | File | Demonstrates |
|-----------|------|--------------|
| 1 | `src/prog1.c` | Executes a user-entered Linux command: `fork()` + `execvp()` + `wait()`, prints parent and child PID |
| 2 | `src/prog2.c` | Copies a file using only `open()`, `read()`, `write()`, `close()` |
| 3 | `src/prog3.c` | `fork()` showing PID, PPID and process state at each stage of execution |
| 4 | `src/wait_waitpid_demo.c` | Three children synchronised with `wait()` vs `waitpid()`, compares the two |
| 4 | `src/zombie_process.c` | Creates a zombie process, then eliminates it with `wait()` |
| 5 | `src/prog5.c` | Producer-consumer over an anonymous pipe, with throughput measurement |
| 5 | `src/ls_grep_pipe.c` | `ls -l \| grep ".c"` built from `pipe()`, `fork()`, `dup2()` and `execlp()` |

## Written reports

| File | Covers |
|------|--------|
| `docs/hardware_abstraction_report.md` | Practical 1 part B - `uname`, `lscpu`, `lsblk`, `ps`, `top`; how the OS abstracts CPU, memory, storage and I/O |
| `docs/strace_analysis_report.md` | Practical 2 part B - `strace` of `cat sample.txt` and of `prog2`; user space / kernel space transitions |
| `docs/pipe_communication_report.md` | Practical 5 - pipe throughput measurements, flow control, and how `dup2()` builds a shell pipeline |
| `docs/practical_outputs.txt` | Recorded output of all seven programs, including the zombie captured in `ps -el` |
| `docs/strace_cat.txt`, `docs/strace_prog2.txt` | Raw strace captures |
| `docs/hardware_report_raw.txt` | Raw output of the hardware investigation commands |

## Build and run

```bash
cd practicals/src
make                          # builds all seven programs

echo "ls -l" | ./prog1        # or run ./prog1 and type a command
./prog2 input.txt output.txt
./prog3
./wait_waitpid_demo
./zombie_process &            # then within 20 s, in another terminal: ps -el
./prog5
./ls_grep_pipe                # same output as: ls -l | grep ".c"

make clean
```

## Debugging and tracing

```bash
gdb ./prog3
(gdb) break main
(gdb) run
(gdb) continue
(gdb) quit

strace ./prog2 input.txt output.txt    # every user/kernel boundary crossing
strace cat sample.txt
strace -f ./ls_grep_pipe               # shows pipe2, dup2 and execve
```
