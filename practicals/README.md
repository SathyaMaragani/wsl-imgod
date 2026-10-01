# Practical Sessions 1-9

Lab practicals for Operating Systems and Systems Programming.
Separate from the weekly ShellForge project in the repository root.

Built and run on Ubuntu 26.04 (WSL2), gcc 15.2.0.

## Layout

```
practicals/
|-- src/       C sources, Makefile, test data
|-- include/   headers (kept for the required directory structure)
|-- tests/     test inputs and expected output
`-- docs/      written reports and raw captures
```

## Programs

| Practical | File | Demonstrates |
|-----------|------|--------------|
| 1 | `src/prog1.c` | Runs a user-entered command: `fork()` + `execvp()` + `wait()` |
| 2 | `src/prog2.c` | File copy using only `open()`, `read()`, `write()`, `close()` |
| 3 | `src/prog3.c` | PID, PPID and process state at each stage of `fork()` |
| 4 | `src/wait_waitpid_demo.c` | Three children synchronised with `wait()` vs `waitpid()` |
| 4 | `src/zombie_process.c` | Creates a zombie process, then reaps it with `wait()` |
| 5 | `src/prog5.c` | Producer-consumer over an anonymous pipe, with throughput measurement |
| 5 | `src/ls_grep_pipe.c` | `ls -l \| grep ".c"` from `pipe()`, `fork()`, `dup2()`, `execlp()` |
| 6 | `src/prog6_fifo_server.c` | Named-pipe (FIFO) server, handles multiple clients |
| 6 | `src/prog6_fifo_client.c` | FIFO client with its own PID-named reply FIFO |
| 6 | `src/signal_handler.c` | `sigaction()` for SIGINT, SIGTERM, SIGUSR1 |
| 7 | `src/prog7_linuxaddr.c` | Addresses of code, data, BSS, static, heap and stack |
| 7 | `src/memory_demo.c` | Long-running process for `/proc/<PID>/maps` inspection |
| 8 | `src/dynamic_memory.c` | `malloc()`, `calloc()`, `realloc()`, `free()` |
| 8 | `src/dynamic_memory_leak.c` | Same program with a deliberate leak, for Valgrind |
| 8 | `src/cow_demo.c` | Copy-on-write after `fork()` over a 100 MB region |
| 9 | `src/copy_lowlevel.c` | File copy with `open`/`read`/`write`/`lseek`/`close` |
| 9 | `src/copy_stdio.c` | The same copy with `fopen`/`fread`/`fwrite`/`fseek`/`fclose` |
| 9 | `src/redirect_output.c` | Redirects stdout to a file with `dup2()` |
| 9 | `src/redirect_input.c` | Redirects stdin from a file with `dup2()` |

## Written reports

| File | Covers |
|------|--------|
| `docs/hardware_abstraction_report.md` | P1 - `uname`, `lscpu`, `lsblk`, `ps`, `top`; how the OS abstracts CPU, memory, storage and I/O |
| `docs/strace_analysis_report.md` | P2 - `strace` of `cat` and of `prog2`; user/kernel space transitions |
| `docs/pipe_communication_report.md` | P5 - pipe throughput, flow control, and `dup2()` pipelines |
| `docs/fifo_and_signals_report.md` | P6 - FIFO behaviour with concurrent clients; `sigaction()` and async-signal safety |
| `docs/address_space_report.md` | P7 - segment addresses matched against `/proc/<PID>/maps` |
| `docs/memory_management_report.md` | P8 - Valgrind leak detection and measured copy-on-write |
| `docs/file_io_report.md` | P9 - low-level vs stdio copy benchmarked, and `dup2()` redirection |
| `docs/practical_outputs.txt` | Recorded output of every program |
| `docs/strace_cat.txt`, `docs/strace_prog2.txt` | Raw strace captures |
| `docs/hardware_report_raw.txt` | Raw hardware command output |

## Build and run

```bash
cd practicals/src
make                          # builds all nineteen programs

echo "ls -l" | ./prog1
./prog2 input.txt output.txt
./prog3
./wait_waitpid_demo
./zombie_process &            # then within 20 s: ps -el
./prog5
./ls_grep_pipe                # same output as: ls -l | grep ".c"

./prog6_fifo_server &         # terminal 1
./prog6_fifo_client           # terminals 2, 3, ...
rm -f /tmp/server_fifo /tmp/client_*_fifo    # cleanup

./signal_handler              # then: kill -SIGUSR1 <pid>
./prog7_linuxaddr
./memory_demo &               # then: cat /proc/<pid>/maps
./dynamic_memory
valgrind --leak-check=full ./dynamic_memory_leak
./cow_demo                    # then: cat /proc/<pid>/smaps_rollup

# practical 9 - run from a scratch dir: these hard-code input.txt / output.txt
mkdir -p ~/p9bench && cd ~/p9bench
dd if=/dev/urandom of=input.dat bs=1M count=100
</path/to>/copy_lowlevel input.dat output_low.dat
</path/to>/copy_stdio    input.dat output_stdio.dat
cmp input.dat output_low.dat && cmp input.dat output_stdio.dat
</path/to>/redirect_output && cat output.txt
printf 'Operating Systems\nLinux File I/O\ndup2 system call\n' > input.txt
</path/to>/redirect_input

make clean
```
