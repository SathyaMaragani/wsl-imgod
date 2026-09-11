#!/bin/bash
# Checks that parse_line() builds argv[] correctly.
# Run from the repository root:  ./tests/test_parser.sh   (or: make test)
set -u
SHELL_BIN=bin/shellforge
[ -x "$SHELL_BIN" ] || { echo "build first: make"; exit 1; }

fail=0
check() {
    desc="$1"; input="$2"; expected="$3"
    actual=$(printf '%s\nexit\n' "$input" | ./$SHELL_BIN | grep '^argv\[' | tr '\n' '|')
    if [ "$actual" = "$expected" ]; then
        echo "  ok    $desc"
    else
        echo "  FAIL  $desc"
        echo "        expected: $expected"
        echo "        actual  : $actual"
        fail=1
    fi
}

echo "parse_line() tests:"
check "simple command"        "ls"                  "argv[0] = ls|"
check "command with flags"    "ls -l /home"         "argv[0] = ls|argv[1] = -l|argv[2] = /home|"
check "collapses extra spaces" "gcc    main.c"      "argv[0] = gcc|argv[1] = main.c|"
check "tab separated"         "$(printf 'cat\tsample.txt')" "argv[0] = cat|argv[1] = sample.txt|"
check "empty line"            ""                    ""
check "whitespace only"       "   "                 ""

[ $fail -eq 0 ] && echo "all parser tests passed" || echo "some tests failed"
exit $fail
