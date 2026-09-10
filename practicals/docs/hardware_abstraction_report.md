# Practical 1 - Part B
# Hardware Resources and Operating System Services

Investigated with `uname`, `lscpu`, `lsblk`, `ps` and `top` on Ubuntu 26.04 (WSL2).
Raw command output is in `hardware_report_raw.txt`.

## 1. Commands used

| Command  | Purpose                                            |
|----------|----------------------------------------------------|
| `uname -a` | Kernel name, version, machine architecture         |
| `lscpu`    | CPU architecture, cores, threads, caches           |
| `lsblk`    | Block (storage) devices and partitions             |
| `ps -ef`   | Snapshot of all processes currently in the system  |
| `top`      | Real-time CPU / memory / task utilisation          |

## 2. Observed system

```
Linux Lucky-Legion 6.6.87.2-microsoft-standard-WSL2 x86_64 GNU/Linux
CPU        : AMD Ryzen 7 260 w/ Radeon 780M, 16 logical CPUs (8 cores x 2 threads)
Caches     : L1d 256K, L1i 256K, L2 8M, L3 16M
Memory     : 15 GiB RAM + 4 GiB swap
Storage    : sdf (1 T) mounted on /, sdc (4 G) used as SWAP, plus loop devices
Tasks      : 77 total, 1 running, 76 sleeping, 0 zombie
```

## 3. How the OS abstracts each hardware resource

### 3.1 CPU - abstracted as *the process / thread*

`lscpu` reports 16 logical CPUs, yet `top` shows 77 tasks. The hardware cannot
run 77 things at once. The kernel's **scheduler** (CFS/EEVDF) multiplexes the 16
physical execution units among all runnable tasks by time-slicing, so every
process gets the illusion of owning a CPU of its own.

The abstraction the programmer sees is the **process**, created with `fork()` and
described by a `task_struct` in the kernel. `top` exposes the pieces of that
abstraction: `PR`/`NI` (scheduling priority), `%CPU` (share of CPU time granted),
and the state column `S` (sleeping), `R` (running), `Z` (zombie).

The programs written in these practicals show this directly - `prog1` and `prog3`
call `fork()` and receive a new schedulable entity without ever touching a core,
an APIC or a context-switch instruction.

### 3.2 Memory - abstracted as *the virtual address space*

`free -h` shows 15 GiB of physical RAM, but `top` reports `VIRT` values per
process that, summed, exceed it. Each process is given a private **virtual
address space**; the MMU plus the kernel's page tables translate virtual pages to
physical frames on demand.

`top` separates the layers:

- `VIRT` - virtual memory the process has mapped (may never be backed by RAM)
- `RES`  - resident set, the pages actually in physical RAM now
- `SHR`  - pages shared with other processes (e.g. one copy of libc)

Because of this abstraction, `prog1` and `prog3` can print two different values
of `getpid()` from what began as one address space: `fork()` gives the child a
copy-on-write duplicate, and the kernel only allocates real frames when one side
writes.

Swap (`sdc`, 4 GiB) extends the abstraction beyond physical RAM - a page can live
on disk and be faulted back in transparently.

### 3.3 Storage - abstracted as *the file and the file system*

`lsblk` shows raw block devices (`sda`, `sdc`, `sdf`, loop devices) with sizes and
mount points. A program never addresses cylinders, sectors or the NVMe queue.
Instead the kernel layers:

```
application  ->  file descriptor (int)
             ->  VFS  (open/read/write/close, uniform for every FS)
             ->  concrete file system (ext4)
             ->  block layer + I/O scheduler + page cache
             ->  device driver -> physical disk
```

`prog2` (Practical 2) is exactly this abstraction in use: `open()` returns a small
integer, and `read()`/`write()` move bytes without the program knowing the file
system type or the device geometry. Mounting is the same abstraction applied to
the namespace - `sdf` appears simply as `/`.

### 3.4 I/O devices - abstracted as *files and system calls*

Linux presents devices through the same file interface (`/dev/*`), so terminals,
disks and pipes are all read and written with `read()`/`write()`. In the `prog2`
trace, `write(1, "File copied successfully.\n", 26)` writes to the terminal using
the identical call used for the on-disk file - only the descriptor differs.

Device drivers hide interrupt handling, DMA setup and register programming. The
`(udev-worker)` processes visible in `ps -ef` are the userspace half of this:
they react to kernel device events and populate `/dev`.

## 4. Relationship summary

| Hardware resource | Kernel subsystem      | Abstraction seen by the program | System calls |
|-------------------|-----------------------|---------------------------------|--------------|
| CPU               | Scheduler             | Process / thread                | `fork`, `execvp`, `wait`, `exit` |
| Main memory       | Virtual memory manager| Virtual address space           | `brk`, `mmap`, `munmap` |
| Disk / storage    | VFS + block layer     | File, directory, file descriptor| `open`, `read`, `write`, `close` |
| I/O devices       | Device drivers        | Special file in `/dev`          | `read`, `write`, `ioctl` |

## 5. Conclusion

Every hardware resource is exposed to user programs through a small, uniform
software abstraction, and the only legal doorway into it is the **system call**.
The programs in these practicals never address hardware: they ask for a process,
a file descriptor or a memory mapping, and the kernel maps those requests onto
16 CPUs, 15 GiB of RAM and a 1 TB block device. That indirection is what makes
the same C source run unchanged on different machines.
