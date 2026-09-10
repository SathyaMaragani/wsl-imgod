# Practical 5
# Producer-Consumer Communication Using an Anonymous Pipe

Measured on Ubuntu 26.04 (WSL2), AMD Ryzen 7 260, gcc 15.2.0.

## Part A - `prog5.c`: producer-consumer and communication efficiency

The parent produces 10,000 fixed-size messages of 100 bytes and writes them into
an anonymous pipe; the child consumes them with `read()` and counts them.

### Measured results (three consecutive runs)

| Run | Messages | Bytes transferred | Time (s) | Throughput (MB/s) |
|-----|----------|-------------------|----------|-------------------|
| 1 | 10000 | 1,000,000 | 0.012969 | 73.53 |
| 2 | 10000 | 1,000,000 | 0.013929 | 68.47 |
| 3 | 10000 | 1,000,000 | 0.012304 | 77.51 |

Every run reported `Consumer: Received 10000 messages`, so no message was lost
or merged - the producer and consumer stayed in step.

Throughput is calculated as:

```
throughput = total bytes / elapsed time
```

Roughly 70 MB/s, i.e. about 1.3 microseconds per 100-byte message. The variation
between runs comes from scheduler placement and cache state, not from the pipe.

### Why messages arrive intact

`MESSAGE_SIZE` is 100 bytes, far below `PIPE_BUF` (4096 on Linux). Writes below
`PIPE_BUF` are guaranteed atomic, so a message is never interleaved with another.
The consumer's `read(pipefd[0], buffer, MESSAGE_SIZE)` therefore returns exactly
one message per call, which is why the count matches exactly.

### How the two processes stay synchronised

The pipe is a fixed-size kernel buffer (64 KB by default), and it provides flow
control for free:

- If the producer gets ahead and fills the buffer, its `write()` **blocks** until
  the consumer drains some of it.
- If the consumer gets ahead and the buffer is empty, its `read()` **blocks**
  until the producer supplies more.

No mutex, semaphore or shared-memory synchronisation is needed - the kernel
enforces it.

### How the consumer knows when to stop

The consumer loops `while (read(...) > 0)`. `read()` returns 0 (end of file) only
when **every** write end of the pipe has been closed. This is why both closes are
essential:

- the child closes `pipefd[1]` immediately, otherwise it would hold a write end
  open itself and its `read()` would block forever;
- the parent closes `pipefd[1]` after the loop, which is the signal that ends the
  consumer.

### What the measurement actually covers

`clock_gettime()` is called before the write loop and after `wait(NULL)`, so the
elapsed time includes the producer's writes, the consumer's reads, and the
process teardown. It is a round-trip figure for the whole exchange rather than
the cost of `write()` alone.

### Cost breakdown

Each message costs one `write()` in the producer and one `read()` in the
consumer - 20,000 system calls, i.e. 20,000 user/kernel mode switches, plus two
copies of every byte (user buffer -> kernel pipe buffer -> user buffer).
Increasing `MESSAGE_SIZE` would move the same total bytes with fewer system
calls and would raise the measured throughput; this is the same buffering
trade-off seen in Practical 2 with `prog2`.

## Part B - `ls_grep_pipe.c`: implementing `ls -l | grep ".c"`

The program reproduces the shell pipeline using `pipe()`, `fork()`, `dup2()` and
`execlp()` directly.

```
      Child 1                       Child 2
    +---------+                   +-----------+
    |  ls -l  |                   | grep ".c" |
    +----+----+                   +-----^-----+
         | stdout                       | stdin
         +---------- pipe --------------+
```

### System calls involved

| System call | Purpose |
|-------------|---------|
| `pipe()`    | Creates the communication channel |
| `fork()`    | Creates the two child processes |
| `dup2()`    | Redirects standard output / standard input onto the pipe |
| `execlp()`  | Replaces each child image with `ls` and `grep` |
| `waitpid()` | Parent waits for both children |

### The redirection

- Child 1: `dup2(pipefd[1], STDOUT_FILENO)` makes fd 1 a second reference to the
  pipe's write end, so anything `ls` prints goes into the pipe.
- Child 2: `dup2(pipefd[0], STDIN_FILENO)` makes fd 0 refer to the pipe's read
  end, so `grep` reads the pipeline instead of the keyboard.

Because `exec()` preserves open file descriptors across the image replacement,
`ls` and `grep` are ordinary unmodified programs - they never know they are
talking to a pipe rather than a terminal.

### Why the parent must close both ends

After forking, three processes hold the pipe open. The parent closes `pipefd[0]`
and `pipefd[1]` because it uses neither; if it kept the write end open, `grep`
would never see end-of-file and the pipeline would hang after `ls` finished.

### Verified against the real shell

Running `./ls_grep_pipe` and running `ls -l | grep ".c"` in the shell produced
identical output on the same directory (18 matching lines), confirming the
program is equivalent to the shell pipeline.

### Trace evidence

`strace -f` on the program shows the mechanism directly:

```
524   pipe2([3, 4], 0)                = 0        <- pipe created, read=3 write=4
524   clone(...)                      = 525      <- first child
524   clone(...)                      = 526      <- second child
525   dup2(4, 1)                      = 1        <- child 1: stdout -> pipe write
526   dup2(3, 0)                      = 0        <- child 2: stdin  <- pipe read
525   execve("/usr/bin/ls", ["ls", "-l"], ...)   <- child 1 becomes ls
524   wait4(525, ...)                            <- parent waits
```

`pipe2([3, 4])` shows the two new descriptors; 3 and 4 are the lowest free
numbers because 0, 1 and 2 are already taken by stdin, stdout and stderr. The
`execve` calls appear several times with `ENOENT` first because `execlp()`
searches each directory in `$PATH` in turn until it finds `/usr/bin/ls`.

## Conclusion

An anonymous pipe gives related processes a unidirectional byte stream with
kernel-managed flow control and no explicit synchronisation. Part A measures the
cost of that channel (about 70 MB/s for 100-byte messages, bounded by two system
calls and two copies per message); Part B shows that the same primitive, combined
with `dup2()` and `exec()`, is exactly how a shell builds `command1 | command2`.
