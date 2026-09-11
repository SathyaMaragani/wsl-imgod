# ShellForge - Week 3: Parsing Commands

Milestone: parse user input into tokens (`argv[]`) ready for process execution.

## 1. Why parsing is needed

A shell never hands the typed text to the kernel as-is. `execvp()` expects a
program name plus a NULL-terminated array of arguments, so the line has to be
split first:

```
User input:  "ls -l /home"
                  |
              tokenizer (strtok)
                  |
   +------+-------+--------+
   |  ls  |  -l   | /home  |
   +------+-------+--------+
                  |
argv[0] = "ls"
argv[1] = "-l"
argv[2] = "/home"
argv[3] = NULL      <- the terminator execvp() looks for
```

This is lexical analysis: turning a flat character stream into meaningful
tokens.

## 2. Module added

| File | Contents |
|------|----------|
| `include/parser.h` | `parse_line()` and `free_tokens()` declarations |
| `src/parser.c` | Tokenizer built on `strtok()`, with a growable token array |

`main.c` now calls the parser instead of echoing the line, and `shell.h` was
bumped to version 3.0.

## 3. How `parse_line()` works

1. Allocate an array of 64 `char *` slots.
2. `strtok(line, " \t\r\n\a")` returns the first token.
3. Each token pointer is stored and the position advances.
4. If the array fills up, `realloc()` doubles it - so a command can have any
   number of arguments.
5. `strtok(NULL, ...)` continues through the same string until it returns NULL.
6. `tokens[position] = NULL` terminates the array for `execvp()`.

### How `strtok()` behaves

`strtok()` does not copy anything. It overwrites each delimiter in the original
string with `'\0'` and returns a pointer into that same buffer:

```
before:   c  a  t  ' '  s  a  m  p  l  e  .  t  x  t  \0
after:    c  a  t  \0   s  a  m  p  l  e  .  t  x  t  \0
          ^             ^
          token 1       token 2
```

Two consequences that shape the code:

- Consecutive delimiters are skipped automatically, so `gcc    main.c` yields
  two tokens, not five.
- The tokens are *not* separate allocations. `free_tokens()` therefore frees
  only the pointer array; the strings are released when `main()` frees the line.
  Freeing them individually would be a double free.

## 4. Memory ownership

```
read_line()   -> malloc'd line buffer            freed by main()
parse_line()  -> malloc'd array of pointers      freed by free_tokens()
                 (pointers aim inside the line, not at new allocations)
```

Verified with valgrind:

```
$ printf 'ls -l /home\nexit\n' | valgrind --leak-check=full ./bin/shellforge
HEAP SUMMARY:
    in use at exit: 0 bytes in 0 blocks
  total heap usage: 5 allocs, 5 frees, 8,832 bytes allocated
All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors from 0 contexts
```

## 5. Session output

```
=================================
ShellForge Version 3.0
=================================
myshell> ls -l /home

Parsed Tokens
argv[0] = ls
argv[1] = -l
argv[2] = /home
myshell> gcc main.c

Parsed Tokens
argv[0] = gcc
argv[1] = main.c
myshell> exit
Goodbye!
```

## 6. Tests

`tests/test_parser.sh` (run with `make test`) feeds lines into the shell and
checks the argv[] it prints:

```
parse_line() tests:
  ok    simple command
  ok    command with flags
  ok    collapses extra spaces
  ok    tab separated
  ok    empty line
  ok    whitespace only
all parser tests passed
```

## 7. Ready for Week 4

The array `parse_line()` returns is already in the exact shape `execvp()` needs:

```c
tokens = parse_line(line);
execvp(tokens[0], tokens);
```

This is the same `fork()` + `execvp()` + `wait()` pattern demonstrated in
practical 1 (`practicals/src/prog1.c`); Week 4 connects it to the parser.

## 8. Notes on two deviations from the handout

- The handout prints a hardcoded `"ShellForge Version 3.0"`. The existing code
  already had `SHELL_NAME` and `VERSION` macros in `shell.h`, so the version was
  bumped there instead and the printed output is identical.
- The handout's Makefile declares `$(TARGET):` with no prerequisites, which means
  `make` reports the binary up to date even after a source file changes (hence
  its `make clean` step). `$(TARGET): $(SRC)` was added so rebuilds happen
  automatically; `make clean && make` still behaves the same.

## 9. Known limitation (not in scope this week)

`read_line()` returns an empty string at end of input rather than signalling EOF,
so pressing Ctrl+D at the prompt loops instead of exiting. Typing `exit` works
normally. This is carried over from Week 2 and is left for a later chapter.
