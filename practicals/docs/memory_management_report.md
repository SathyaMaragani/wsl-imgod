# Practical 8
# Dynamic Memory, Valgrind Leak Detection, and Copy-on-Write

Run on Ubuntu 26.04 (WSL2), valgrind 3.26.0, page size 4096 bytes.

## Part A - `malloc`, `calloc`, `realloc`, `free`

| Function | Purpose |
|----------|---------|
| `malloc()` | allocates a block of **uninitialised** memory |
| `calloc()` | allocates and **zeroes** it |
| `realloc()` | resizes an existing allocation, preserving contents |
| `free()` | releases a block |

`dynamic_memory.c` output:

```
1. malloc() demonstration
Memory allocated using malloc():
10 20 30 40 50

2. calloc() demonstration
Memory allocated using calloc():
0 0 0 0 0

3. realloc() demonstration
Memory after realloc():
10 20 30 40 50 60 70 80 90 100

4. free() demonstration
Allocated memory successfully released.
```

The `calloc()` row printing `0 0 0 0 0` is the whole difference from `malloc()`:
`malloc` would have shown whatever bytes happened to be there. The `realloc()`
row keeps `10..50` and adds `60..100`, showing the original contents survived
the resize.

### Why `realloc()` goes through a temporary

```c
temp = realloc(malloc_ptr, 10 * sizeof(int));
if (temp == NULL) { free(malloc_ptr); ... }
malloc_ptr = temp;
```

If `realloc()` fails it returns NULL **and leaves the original block valid**.
Writing `malloc_ptr = realloc(malloc_ptr, ...)` directly would overwrite the
only pointer to that block with NULL - the memory would still be allocated but
unreachable, i.e. leaked, exactly when the program is already short of memory.

## Part B - Valgrind

### Clean run

```
$ valgrind --leak-check=full ./dynamic_memory
==510==     in use at exit: 0 bytes in 0 blocks
==510==   total heap usage: 4 allocs, 4 frees, 4,176 bytes allocated
==510== All heap blocks were freed -- no leaks are possible
==510== ERROR SUMMARY: 0 errors from 0 contexts
```

(4 allocations rather than 3: `realloc` counts as one, and stdio allocates a
buffer for stdout.)

### Deliberate leak

`dynamic_memory_leak.c` is the same program with `free(calloc_ptr)` removed,
kept as a separate file so both runs stay reproducible.

```
$ valgrind --leak-check=full --show-leak-kinds=all ./dynamic_memory_leak
==513==     in use at exit: 20 bytes in 1 blocks
==513==   total heap usage: 4 allocs, 3 frees, 4,176 bytes allocated
==513== 20 bytes in 1 blocks are definitely lost in loss record 1 of 1
==513==    by 0x4001249: main (dynamic_memory_leak.c:25)
==513==    definitely lost: 20 bytes in 1 blocks
==513== ERROR SUMMARY: 1 errors from 1 contexts
```

20 bytes = `5 * sizeof(int)`, the exact `calloc()` block. Valgrind names the
source line that allocated it, which is what `-g` at compile time buys.

### Leak categories

| Category | Meaning |
|----------|---------|
| **definitely lost** | no pointer to the block remains - a genuine leak |
| **indirectly lost** | only reachable through a block that is itself lost (e.g. children of a leaked tree node) |
| **possibly lost** | only an interior pointer remains; valgrind cannot tell if it is deliberate |
| **still reachable** | a pointer still exists at exit - usually a global cache, often harmless |

### Other errors Valgrind catches

| Error | Example | Why it is wrong |
|-------|---------|-----------------|
| use after free | `free(p); printf("%d", *p);` | the block may already be reused |
| buffer overflow | `p = malloc(5*sizeof(int)); p[5] = 100;` | valid indices are 0..4 |
| double free | `free(p); free(p);` | corrupts the allocator's bookkeeping |
| uninitialised read | `int *p = malloc(4); if (*p) ...` | value is indeterminate |

## Part C - Copy-on-Write after `fork()`

`cow_demo.c` allocates and touches 100 MB, forks, and then has the child write
one byte in every 4 KB page. Measured with `/proc/<PID>/smaps_rollup` at each
stage.

### Measured results

| | Parent Rss | Parent Pss | Shared_Dirty | Parent Private_Dirty |
|---|---|---|---|---|
| **Stage 1** before `fork()` | 104,196 kB | 102,558 kB | 0 kB | 102,508 kB |
| **Stage 2** after `fork()`, before the write | 104,260 kB | 51,317 kB | 102,476 kB | 32 kB |
| **Stage 3** after the child writes | 104,260 kB | 102,519 kB | 72 kB | 102,436 kB |

And the child:

| | Child Rss | Child Pss | Shared_Dirty | Child Private_Dirty |
|---|---|---|---|---|
| **Stage 2** | 103,404 kB | 51,286 kB | 102,476 kB | 32 kB |
| **Stage 3** | 103,404 kB | 102,488 kB | 72 kB | 102,436 kB |

### Reading the numbers

**Stage 1 - one owner.** The parent alone holds 100 MB: `Private_Dirty` is
102,508 kB and `Shared_Dirty` is 0.

**Stage 2 - `fork()` copied nothing.** `Private_Dirty` collapses from
102,508 kB to **32 kB** and the same 102,476 kB now appears as `Shared_Dirty`
in *both* processes. `Pss` - which divides each shared page by the number of
sharers - halves to ~51,300 kB each. Two processes, one physical copy. This is
the whole point: `fork()` of a 100 MB process did not consume another 100 MB.

**Stage 3 - the writes force the copy.** After the child writes one byte per
page, `Shared_Dirty` falls to 72 kB and `Private_Dirty` climbs back to
102,436 kB **in both processes**. `Pss` returns to ~102,500 kB each. Physical
memory used by the pair went from ~100 MB to ~200 MB - the copy happened now,
on write, not at `fork()`.

### Why one byte per page is enough

```c
for (size_t i = 0; i < SIZE; i += 4096)
    data[i] = 2;
```

COW works at **page** granularity. Writing a single byte dirties the whole
4 KB page, so stepping by the page size (`getconf PAGE_SIZE` = 4096) triggers a
copy of every page while touching only 1/4096 of the bytes - 25,600 faults
instead of 104 million writes.

### The mechanism

1. `fork()` gives the child its own page tables, pointing at the **same**
   physical frames, and marks every entry **read-only** in both processes.
2. A read by either process is served straight from the shared frame.
3. A write traps: the MMU raises a page fault because the entry is read-only.
4. The kernel sees the VMA is actually writable and marked COW, allocates a
   fresh frame, copies 4 KB into it, points the faulting process's entry at the
   copy and marks it writable.
5. The write is retried and succeeds, now on private memory.

### Metrics used

| Field | Meaning |
|-------|---------|
| `Rss` | pages resident in physical RAM (counts shared pages in full, for every sharer) |
| `Pss` | proportional set size - shared pages divided by the number of sharers |
| `Shared_Dirty` | modified pages currently shared with another process |
| `Private_Dirty` | modified pages private to this process |

`Pss` is the honest measure here: summing `Rss` over both processes at stage 2
would claim ~207 MB of RAM in use when the real figure is ~100 MB. Summing
`Pss` gives ~102 MB, which is correct.

## Conclusion

Valgrind turns invisible allocator bugs into named source lines, and
`/proc/<PID>/smaps_rollup` turns copy-on-write from a diagram into measured
numbers: `fork()` of a 100 MB process cost nothing until the child wrote, at
which point exactly the pages it touched were duplicated.
