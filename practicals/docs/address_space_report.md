# Practical 7
# Linux Process Address Space

Run on Ubuntu 26.04 (WSL2), x86-64, page size 4096 bytes.

## Part A - Printing the segment addresses

`prog7_linuxaddr.c` output:

```
===== Linux Process Address Space =====

Code address       : 0x58aca87511c9
Global address     : 0x58aca8754010
Static address     : 0x58aca8754014
BSS address        : 0x58aca875401c
Heap address       : 0x58accd6c2010
Stack address      : 0x7fff2d1832fc
```

### What the ordering shows

| Region | Address | Holds | Grows |
|--------|---------|-------|-------|
| Text (code) | `0x58aca87511c9` | machine instructions, read-only | fixed |
| Data | `0x58aca8754010` | initialised globals (`global_var = 10`) | fixed |
| Static | `0x58aca8754014` | `static int static_var = 20` | fixed |
| BSS | `0x58aca875401c` | uninitialised globals (`global_uninit`) | fixed |
| Heap | `0x58accd6c2010` | `malloc()` | upward |
| Stack | `0x7fff2d1832fc` | locals, return addresses | downward |

Three things are visible straight away:

1. **Code sits lowest**, then data, then BSS - the order the linker lays them out.
2. **`static_var` lands beside `global_var`** (4 bytes apart). A `static` local
   has function scope but *static storage*: it lives in the data segment, not on
   the stack, which is why it keeps its value between calls.
3. **The gap between heap and stack is enormous** - heap at `0x58ac...`, stack at
   `0x7fff...`, roughly 43 TB apart. The two grow toward each other, and that
   gap is the room they have to expand into.

### Initialised data vs BSS

`global_var = 10` must be stored in the executable, because the value 10 has to
come from somewhere. `global_uninit` is guaranteed to start at zero, so the file
only records *how many bytes* to zero - it costs no file space. That is why they
live in different segments even though both are globals.

### ASLR

Re-running the program gives different addresses every time. Linux applies
Address Space Layout Randomisation, relocating the segments at each exec so
that an attacker cannot predict where code or the stack will be. Only the
*relative* layout is stable.

## Part B - `/proc/<PID>/maps` of a live process

`memory_demo.c` prints its addresses and then sleeps so the process can be
inspected from another terminal.

```
Process ID (PID): 504
Address of code   : 0x57e62d6de229
Address of global : 0x57e62d6e1010
Address of static : 0x57e62d6e1014
Address of BSS    : 0x57e62d6e1024
Address of heap   : 0x57e63517a010
Address of stack  : 0x7fff4904f63c

Process is running...
```

`cat /proc/504/maps` (filtered):

```
57e62d6dd000-57e62d6de000 r--p 00000000 08:30 35300   .../memory_demo
57e62d6de000-57e62d6df000 r-xp 00001000 08:30 35300   .../memory_demo
57e62d6df000-57e62d6e0000 r--p 00002000 08:30 35300   .../memory_demo
57e62d6e0000-57e62d6e1000 r--p 00002000 08:30 35300   .../memory_demo
57e62d6e1000-57e62d6e2000 rw-p 00003000 08:30 35300   .../memory_demo
57e63517a000-57e63519b000 rw-p 00000000 00:00 0       [heap]
7e7224c00000-7e7224c28000 r--p 00000000 08:30 13643   /usr/lib/x86_64-linux-gnu/libc.so.6
7e7224c28000-7e7224dc0000 r-xp 00028000 08:30 13643   /usr/lib/x86_64-linux-gnu/libc.so.6
7e7224dc0000-7e7224e0e000 r--p 001c0000 08:30 13643   /usr/lib/x86_64-linux-gnu/libc.so.6
7e7224e0e000-7e7224e12000 r--p 0020d000 08:30 13643   /usr/lib/x86_64-linux-gnu/libc.so.6
7e7224e12000-7e7224e14000 rw-p 00211000 08:30 13643   /usr/lib/x86_64-linux-gnu/libc.so.6
7fff49031000-7fff49052000 rw-p 00000000 00:00 0       [stack]
```

### Reading a line

```
57e62d6de000-57e62d6df000 r-xp 00001000 08:30 35300 /path/memory_demo
|_______________________| |__| |______| |___| |___| |_______________|
    address range         perms  offset  dev   inode   backing file
```

Permissions are `r` read, `w` write, `x` execute, and `p` private (copy-on-write)
or `s` shared.

### Matching the printed addresses to the regions

| Printed address | Falls inside | Permissions | Meaning |
|---|---|---|---|
| code `0x57e62d6de229` | `57e62d6de000-57e62d6df000` | `r-xp` | executable, **not writable** - code cannot be overwritten |
| global `0x57e62d6e1010` | `57e62d6e1000-57e62d6e2000` | `rw-p` | writable data |
| static `0x57e62d6e1014` | same region | `rw-p` | confirms static lives with the globals |
| BSS `0x57e62d6e1024` | same region | `rw-p` | BSS sits at the end of the data mapping |
| heap `0x57e63517a010` | `57e63517a000-57e63519b000` | `rw-p` `[heap]` | 132 KB reserved for a 4-byte `malloc` |
| stack `0x7fff4904f63c` | `7fff49031000-7fff49052000` | `rw-p` `[stack]` | 132 KB currently mapped |

The executable appears as **five separate mappings** because the linker groups
sections by permission: read-only headers, executable code, read-only data,
relocations, then writable data. Each gets its own page-aligned mapping so the
kernel can enforce the narrowest possible rights - code pages are never
writable, data pages are never executable (W^X).

`libc.so.6` is mapped the same way and is shared: every process on the system
maps the same physical copy of its read-only and executable pages.

### `/proc/<PID>/status`

```
VmSize:   2760 kB      total virtual address space
VmRSS:    1792 kB      resident in physical RAM
VmData:    224 kB      data + heap
VmStk:     132 kB      stack
VmExe:       4 kB      executable code
```

`VmSize` (2760 kB) exceeds `VmRSS` (1792 kB): part of the address space is
mapped but not currently backed by physical memory. Note how small `VmExe` is -
4 kB of actual program code, while the process still maps ~2.7 MB, most of it
shared library.

## Conclusion

A process does not see memory as one flat block. The kernel builds an address
space out of separately mapped regions, each with its own permissions and
backing store, and `/proc/<PID>/maps` exposes that table directly. The addresses
printed by the C program fall exactly inside the regions the kernel reports,
which confirms that language-level concepts - global, static, heap, stack - map
onto concrete, permission-controlled virtual memory areas.
