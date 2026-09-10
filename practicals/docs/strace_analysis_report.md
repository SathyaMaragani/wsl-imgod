# Practical 2 - Part B
# strace Analysis and User Space / Kernel Space Transitions

Captured traces: `strace_cat.txt` (`strace cat sample.txt`) and
`strace_prog2.txt` (`strace ./prog2 input.txt output2.txt`).

## 1. `strace cat sample.txt` - observed sequence

```
execve("/usr/bin/cat", ["cat", "sample.txt"], 0x7ffc4fb6cd48) = 0
brk(NULL)                                                     = 0x5b5c839e5000
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = ...
access("/etc/ld.so.preload", R_OK)                            = -1 ENOENT
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC)      = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=21347, ...})          = 0
mmap(NULL, 21347, PROT_READ, MAP_PRIVATE, 3, 0)               = ...
close(3)                                                      = 0
openat(AT_FDCWD, "/usr/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\0...", 832)                            = 832
mmap(...)                                                     = ...
close(3)                                                      = 0
statx(AT_FDCWD, "sample.txt", ...)                            = 0
openat(AT_FDCWD, "sample.txt", O_RDONLY|O_CLOEXEC)            = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=18, ...})             = 0
fstat(1, {st_mode=S_IFIFO|0600, ...})                         = 0
splice(3, NULL, 1, NULL, 1048576, 0)                          = 18
splice(3, NULL, 1, NULL, 1048576, 0)                          = 0
close(3)                                                      = 0
exit_group(0)                                                 = ?
+++ exited with 0 +++
```

Three phases are visible:

1. **Program load** - `execve` replaces the image; `brk`/`mmap` build the address space.
2. **Dynamic linking** - `ld.so.cache` and `libc.so.6` are opened and mapped.
3. **Actual work** - `sample.txt` is opened, its bytes are moved to fd 1, then closed.

Note: modern GNU `cat` uses `splice()` instead of a `read()`/`write()` pair when
the destination allows it, because `splice()` moves the data inside the kernel
page cache and avoids two copies across the user/kernel boundary. When stdout is
a plain terminal the classic `read()` + `write()` pair appears instead.

## 2. Kernel services behind each call

| System call   | Purpose                                     | Kernel service         |
|---------------|---------------------------------------------|------------------------|
| `execve()`    | Loads and starts the `cat` program image    | Process management     |
| `brk()`       | Grows the heap                              | Memory management      |
| `mmap()`      | Maps libraries / anonymous memory           | Virtual memory manager |
| `access()`    | Checks accessibility of a path              | File system service    |
| `openat()`    | Opens libraries and `sample.txt`            | File management (VFS)  |
| `fstat()`/`statx()` | Retrieves file metadata (size, mode)  | File system service    |
| `read()`      | Reads bytes from a descriptor               | File I/O service       |
| `write()`     | Writes bytes to a descriptor                | I/O management         |
| `splice()`    | Moves data between descriptors in-kernel    | File I/O / page cache  |
| `close()`     | Releases the descriptor                     | File management        |
| `exit_group()`| Terminates all threads of the process       | Process management     |

## 3. `prog2` trace - the four calls the practical asks about

```
openat(AT_FDCWD, "input.txt", O_RDONLY)                     = 3
openat(AT_FDCWD, "output2.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 4
read(3, "Operating Systems Lab Program\n", 1024)            = 30
write(4, "Operating Systems Lab Program\n", 30)             = 30
read(3, "", 1024)                                           = 0
close(3)                                                    = 0
close(4)                                                    = 0
write(1, "File copied successfully.\n", 26)                 = 26
```

This maps one-to-one onto the C source:

- the two `open()` calls return descriptors **3** and **4** (0,1,2 are already taken
  by stdin/stdout/stderr);
- the loop performs `read()` then `write()` until `read()` returns **0**, which is
  end-of-file and ends the `while` loop;
- both descriptors are released by `close()`;
- the final `printf` becomes a `write()` on descriptor 1 when the stdio buffer is
  flushed at exit - one kernel entry for the whole string, not one per character.

## 4. Control transfer between user space and kernel space

### 4.1 The two modes

The CPU runs in one of two privilege levels:

- **User mode (ring 3)** - the process may execute ordinary instructions and touch
  only its own mapped pages. It cannot address a device, a page table or another
  process.
- **Kernel mode (ring 0)** - full access to the hardware and to all of memory.

A user process therefore cannot copy a file by itself; it must **request** the
operation from the kernel.

### 4.2 What happens on one `read()`

```
user space                     |  kernel space
-------------------------------+-------------------------------------------
1. prog2 calls read(3, buf, 1024)
2. libc wrapper puts the call
   number (0) in %rax and the
   arguments in %rdi/%rsi/%rdx
3. executes the SYSCALL instr. -->
                               4. CPU switches to ring 0, jumps to the
                                  kernel syscall entry point, switches to
                                  the kernel stack of this task
                               5. sys_read() looks up fd 3 in the process
                                  file-descriptor table -> struct file -> inode
                               6. VFS calls the ext4 read handler; if the data
                                  is not in the page cache the block layer
                                  issues disk I/O and the process is put to
                                  SLEEP until the interrupt arrives
                               7. bytes are copied from the page cache into
                                  the user buffer (copy_to_user)
                               8. return value (30) placed in %rax, SYSRET
                            <--
9. read() returns 30; prog2
   resumes in user mode
```

Each of `open`, `read`, `write` and `close` performs this **mode switch** exactly
once per call. Nothing else in `prog2` - the `while` condition, the comparison
`bytes_written != bytes_read`, the loop arithmetic - enters the kernel at all;
that code runs purely in user mode.

### 4.3 Why the trap is necessary

- **Protection** - the descriptor number 3 is meaningless outside the kernel; only
  the kernel holds the table that turns it into an inode, and it can check the
  permission bits before acting.
- **Arbitration** - many processes share one disk. The kernel serialises access
  through the block layer and the page cache.
- **Abstraction** - the same `read()` works for a file, a pipe or a terminal, as
  the `fstat(1, {st_mode=S_IFIFO})` line in the `cat` trace shows.

### 4.4 Cost, and why the buffer size matters

A mode switch is not free (register save/restore, stack switch, pipeline and TLB
effects). `prog2` uses `BUFFER_SIZE 1024`, so a 1 MB file needs about 1024
`read()` + 1024 `write()` pairs, i.e. ~2048 transitions. Enlarging the buffer to
64 KB would cut that to ~32 pairs. This is the same reason stdio buffers `printf`
output in user space and flushes it with a single `write()`.

## 5. Conclusion

`strace` makes the user/kernel boundary visible: every line it prints is one
crossing of that boundary. The rest of the program - all the C logic between
those lines - never leaves user mode. The kernel services involved fall into four
groups: process management (`execve`, `exit_group`), memory management (`brk`,
`mmap`), file management (`openat`, `close`, `statx`) and I/O (`read`, `write`,
`splice`).
