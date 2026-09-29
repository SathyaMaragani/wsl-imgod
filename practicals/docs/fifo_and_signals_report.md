# Practical 6
# Client-Server with Named Pipes (FIFOs), and POSIX Signal Handling

Run on Ubuntu 26.04 (WSL2), gcc 15.2.0.

## Part A - Client-server over named pipes

`prog6_fifo_server.c` and `prog6_fifo_client.c`.

### Design

```
  client 1 ─┐
  client 2 ─┼──write──> /tmp/server_fifo ──read──> server
  client 3 ─┘                                        │
                                                     │ writes back
  client N <──read── /tmp/client_<pid>_fifo <────────┘
```

One well-known FIFO carries requests *to* the server. Each client creates its
own FIFO named after its PID for the reply, so replies never get mixed up.

### Verified: five clients, all answered correctly

Two sequential clients, then three started at the same time:

```
Client PID: 482   Enter message: Hello Server
                  Server Response: Server processed: Hello Server
Client PID: 484   Enter message: How are you?
                  Server Response: Server processed: How are you?
Client PID: 487   Server Response: Server processed: msg from A
Client PID: 490   Server Response: Server processed: msg from B
Client PID: 493   Server Response: Server processed: msg from C
```

Server terminal:

```
Server started...
Waiting for clients...

Received from Client PID 482: Hello Server
Response sent to Client PID 482

Received from Client PID 484: How are you?
Response sent to Client PID 484

Received from Client PID 487: msg from A
Response sent to Client PID 487

Received from Client PID 490: msg from B
Response sent to Client PID 490

Received from Client PID 493: msg from C
Response sent to Client PID 493
```

Every request was matched with the right reply even with three clients running
concurrently.

### FIFO behaviour with multiple clients

- All clients write requests into the **same** server FIFO. The server reads
  them in whatever order they arrive and handles them one at a time.
- A `Request` is 260 bytes, well under `PIPE_BUF` (4096). Writes at or below
  `PIPE_BUF` are **atomic**, so two clients writing simultaneously cannot have
  their messages interleaved. This is the reason the experiment above produced
  five clean messages rather than shredded ones.
- Each reply goes to a private, PID-named FIFO, so there is no contention on
  the way back.

### Two details that make it work

**The server opens its FIFO `O_RDWR`, not `O_RDONLY`.** A FIFO read returns
EOF as soon as the last writer closes. Between clients there are no writers, so
an `O_RDONLY` server would spin on EOF. Holding a write descriptor itself means
`read()` simply blocks until the next client arrives.

**Opening a FIFO blocks until both ends are present.** The client's
`open(SERVER_FIFO, O_WRONLY)` waits until the server has the read end open, and
the server's `open(client_fifo, O_WRONLY)` waits for the client's `O_RDONLY`.
That rendezvous is what synchronises the two processes.

### Cleanup

```bash
rm -f /tmp/server_fifo /tmp/client_*_fifo
```

## Part B - Signal handling with `sigaction()`

`signal_handler.c` catches SIGINT, SIGTERM and SIGUSR1.

### Verified run

```
Signal handling program started.
Process PID = 497

$ kill -SIGUSR1 497
SIGUSR1 received!
User-defined event handled.

$ kill -SIGINT 497
SIGINT received!
Interrupt signal handled.

$ kill -SIGTERM 497
SIGTERM received!
Termination requested.
Program terminating gracefully...
```

SIGINT and SIGUSR1 were handled and the program carried on; SIGTERM caused a
clean shutdown instead of an abrupt kill.

### The important pattern: handlers only set a flag

```c
volatile sig_atomic_t sigint_received = 0;

void signal_handler(int signo) {
    if (signo == SIGINT) sigint_received = 1;   /* nothing else */
}
```

The handler does no printing, no allocation, no I/O. `main()` wakes from
`pause()` and does the real work. This matters because a signal can arrive at
*any* instruction:

- Only **async-signal-safe** functions may be called from a handler.
  `printf()` is not one: if the signal lands while the program is already
  inside `printf`, the stdio buffer and its lock are in an inconsistent state
  and the handler can deadlock or corrupt output.
- `volatile` stops the compiler caching the flag in a register, so the change
  made by the handler is actually visible to the main loop.
- `sig_atomic_t` guarantees the read/write cannot be torn by an interrupt.

### `signal()` vs `sigaction()`

| | `signal()` | `sigaction()` |
|---|---|---|
| Standard | C, historically inconsistent | POSIX |
| Semantics across platforms | vary | well defined |
| Control over mask and flags | none | `sa_mask`, `sa_flags` |
| Suitable for | quick demos | production code |

This program uses `sigaction()` and zeroes the whole `struct sigaction` with
`memset()` before filling it in, so no flag is left uninitialised.

### Asynchronous vs synchronous events

| | Synchronous | Asynchronous |
|---|---|---|
| Cause | the instruction being executed | something outside the flow |
| Examples | SIGSEGV, SIGFPE (divide by zero) | SIGINT from Ctrl+C, SIGUSR1 from `kill` |
| Timing | always at the same instruction | at an unpredictable point |

The three signals handled here are all asynchronous - nothing in the program's
own execution causes them, which is exactly why the flag-and-check pattern is
required.
