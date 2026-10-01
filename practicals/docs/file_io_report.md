# Practical 9
# Low-Level Linux I/O vs Standard Library I/O, and `dup2()` Redirection

Measured on Ubuntu 26.04 (WSL2), AMD Ryzen 7 260, gcc 15.2.0.
Both copy programs compiled with the **same** optimisation level (`-O2`) so the
comparison is fair.

## Part A - two file-copy implementations

| | `copy_lowlevel.c` | `copy_stdio.c` |
|---|---|---|
| Open | `open()` | `fopen()` |
| Read | `read()` | `fread()` |
| Write | `write()` | `fwrite()` |
| Seek | `lseek()` | `fseek()` / `ftell()` |
| Close | `close()` | `fclose()` |
| Handle type | `int` file descriptor | `FILE *` stream |
| Layer | direct system calls | buffered library on top of them |

Both use an 8192-byte application buffer.

### Correctness first

```
$ ./copy_lowlevel input.dat output_low.dat
Source file size: 104857600 bytes
File copied successfully.
$ ./copy_stdio input.dat output_stdio.dat
Source file size: 104857600 bytes
File copied successfully.

$ cmp input.dat output_low.dat       # no output = identical
$ cmp input.dat output_stdio.dat     # no output = identical

$ sha256sum input.dat output_low.dat output_stdio.dat
4e861ac53747ec4720a733d525fdb0c9c438708347eb21c41297c0cdb40360bf  input.dat
4e861ac53747ec4720a733d525fdb0c9c438708347eb21c41297c0cdb40360bf  output_low.dat
4e861ac53747ec4720a733d525fdb0c9c438708347eb21c41297c0cdb40360bf  output_stdio.dat
```

All three digests match: both implementations are byte-exact on a 100 MB file
of random data.

### Performance - 6 runs each, alternating, first discarded as warm-up

| Program | Runs (ms) | Median | Best | Throughput |
|---------|-----------|--------|------|------------|
| `copy_lowlevel` | 106, 104, 107, 103, 106 | **106 ms** | 103 ms | ~943 MB/s |
| `copy_stdio` | 122, 121, 132, 119, 120 | **121 ms** | 119 ms | ~826 MB/s |

The low-level version is about **14 % faster**. The runs were alternated so that
page-cache warmth affects both equally, and the first run of each was dropped.

### Why - the system-call counts

```
$ strace -c -e trace=read,write ./copy_lowlevel input.dat /tmp/x1.dat
 59.11    0.495933          38     12801           write
 40.89    0.343070          26     12802           read
100.00    0.839003          32     25603           total

$ strace -c -e trace=read,write ./copy_stdio input.dat /tmp/x2.dat
 71.35    0.862300          33     25600           write
 28.65    0.346306          27     12802           read
100.00    1.208606          31     38402           total
```

Reads match (12,802 each) but **stdio issues twice as many writes**: 25,600
against 12,801. Looking at the sizes on a 10 MB file:

```
copy_lowlevel :  1280 writes of 8192 bytes
copy_stdio    :  2558 writes of 4096 bytes  (+1 of 8192)
```

glibc gives the stream a **4096-byte** buffer (the file system's block size).
The program hands `fwrite()` 8192 bytes at a time, which is larger than that
buffer, so each call is split into two 4 KB writes.

### The result is the opposite of the usual expectation

Standard library buffering is normally the thing that *reduces* system calls -
it is what makes `putchar()` in a loop tolerable. Here it does the reverse,
because the program already does its own buffering at 8 KB:

```
copy_lowlevel:   file -> 8 KB app buffer -> write()           1 copy,  1 syscall / 8 KB
copy_stdio  :   file -> 4 KB stdio buffer -> 8 KB app buffer
                     -> 4 KB stdio buffer -> write()          2 copies, 2 syscalls / 8 KB
```

So stdio adds an extra `memcpy` through its own buffer *and*, because that
buffer is half the size of the application's, doubles the write syscalls.
Buffering only helps when the application does not already buffer.

This could be removed by calling `setvbuf()` to give the stream a buffer at
least as large as the application's - it is not a flaw in stdio, just a
mismatch between two buffer sizes.

### What each program should be used for

- **Low-level I/O** when the program manages its own buffering, needs exact
  control over every syscall, or works with non-file descriptors (sockets,
  pipes, devices).
- **Standard library I/O** for convenience and formatted I/O (`fprintf`,
  `fscanf`), and for small or character-at-a-time access, where its buffer is
  what makes the pattern efficient.

### `lseek()` / `ftell()` for the file size

After the copy loop the offset already sits at end-of-file, so

```c
off_t file_size = lseek(src_fd, 0, SEEK_END);
```

returns the size - both programs report `104857600 bytes`, which matches
`ls -l`. `lseek()` moves the kernel's per-descriptor offset without transferring
any data; `SEEK_END` with offset 0 is the standard way to ask "how big is this?".

### A note on partial writes

`write()` may transfer fewer bytes than requested, so the inner loop keeps
writing until the buffer is drained:

```c
while (remaining > 0) {
    bytes_written = write(dest_fd, ptr, remaining);
    if (bytes_written == -1) { if (errno == EINTR) continue; ... }
    ptr += bytes_written;
    remaining -= bytes_written;
}
```

Ignoring this is a classic cause of silently truncated files. `EINTR` is retried
rather than treated as an error, because a signal arriving mid-write is not a
failure.

## Part B - I/O redirection with `dup2()`

### The three standard descriptors

| fd | Name | Default |
|----|------|---------|
| 0 | standard input | keyboard |
| 1 | standard output | terminal |
| 2 | standard error | terminal |

`dup2(oldfd, newfd)` makes `newfd` refer to the same open file as `oldfd`,
closing whatever `newfd` pointed at first. Nothing in the program has to change:
`printf()` always writes to descriptor 1, so redirecting descriptor 1
redirects `printf`.

### Output redirection - `redirect_output.c`

```
$ ./redirect_output
Before redirection          <- terminal

$ cat output.txt
Hello from redirected standard output!
This message is stored in output.txt
```

`strace` shows the mechanism in three lines:

```
openat(AT_FDCWD, "output.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 3
write(1, "Before redirection\n", 19)                            = 19   <- terminal
dup2(3, 1)                                                      = 1    <- fd 1 becomes the file
write(1, "Hello from redirected standard o"..., 76)             = 76   <- the file
```

The same `write(1, ...)` call reaches two different destinations, before and
after `dup2`. `close(fd)` afterwards is safe and correct: descriptor 3 is no
longer needed because descriptor 1 now refers to the same open file, and an open
file stays alive while *any* descriptor refers to it.

### Input redirection - `redirect_input.c`

`input.txt`:

```
Operating Systems
Linux File I/O
dup2 system call
```

```
$ ./redirect_input
Reading from redirected standard input:
Operating Systems
Linux File I/O
dup2 system call
```

`fgets(..., stdin)` normally blocks on the keyboard. After
`dup2(fd, STDIN_FILENO)` it reads the file instead, and the program never knew
the difference.

### How a shell uses this

This is exactly how `>` and `<` are implemented. For `ls > out.txt`, the shell:

```
fork()
  child: fd = open("out.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644)
         dup2(fd, STDOUT_FILENO)
         close(fd)
         execvp("ls", ...)            <- ls writes to fd 1, which is the file
parent: waitpid()
```

The redirection is set up **between `fork()` and `exec()`**, which is the only
window where the child can change its own descriptors but is still running the
shell's code. Descriptors survive `exec()`, which is why the unmodified `ls`
binary ends up writing to a file.

ShellForge Week 7 uses the identical technique with a pipe instead of a file -
`dup2(pipefd[1], STDOUT_FILENO)` in one child and
`dup2(pipefd[0], STDIN_FILENO)` in the other (see
[`../../docs/week7_8_pipes_and_memory.md`](../../docs/week7_8_pipes_and_memory.md)).

## Conclusion

Measured on 100 MB, raw system calls beat the standard library by ~14 % here -
not because buffering is bad, but because the program already buffered at 8 KB
and stdio's own 4 KB buffer added a second copy and doubled the write syscalls.
`dup2()` then shows that a descriptor number is just an index into a per-process
table: change what entry 1 points at, and every `printf` in the program follows,
which is the whole mechanism behind shell redirection and pipelines.

## Reproducing

```bash
cd practicals/src
make copy_lowlevel copy_stdio redirect_output redirect_input

mkdir -p ~/p9bench && cd ~/p9bench
dd if=/dev/urandom of=input.dat bs=1M count=100
<path>/copy_lowlevel input.dat output_low.dat
<path>/copy_stdio    input.dat output_stdio.dat
cmp input.dat output_low.dat && cmp input.dat output_stdio.dat
time <path>/copy_lowlevel input.dat output_low.dat     # repeat, alternating

<path>/redirect_output && cat output.txt
printf 'Operating Systems\nLinux File I/O\ndup2 system call\n' > input.txt
<path>/redirect_input
```

The redirection demos are run from a scratch directory because they hard-code
`output.txt` and `input.txt`, which already exist in `practicals/src/` from
practical 2. The `.dat` test files are gitignored.
