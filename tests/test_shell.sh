#!/bin/bash
# Checks command execution (Week 4), built-ins (Week 5) and signals (Week 6).
# Run from the repository root:  ./tests/test_shell.sh   (or: make test)
set -u
SHELL_BIN=bin/shellforge
[ -x "$SHELL_BIN" ] || { echo "build first: make"; exit 1; }

fail=0
pass() { echo "  ok    $1"; }
bad()  { echo "  FAIL  $1"; echo "        $2"; fail=1; }

echo "shell tests:"

# Week 4 - external command execution via fork/execvp/wait
out=$(printf 'echo hello-from-execvp\nexit\n' | ./$SHELL_BIN)
case "$out" in
  *hello-from-execvp*) pass "external command runs (execvp)" ;;
  *) bad "external command runs (execvp)" "got: $out" ;;
esac

# Week 4 - unknown command reports an error instead of dying
out=$(printf 'no_such_command_xyz\necho still-alive\nexit\n' | ./$SHELL_BIN 2>&1)
case "$out" in
  *ShellForge*[Nn]o\ such\ file*still-alive*) pass "unknown command: perror, shell survives" ;;
  *) bad "unknown command: perror, shell survives" "got: $out" ;;
esac

# Week 5 - pwd built-in runs in the shell process
out=$(printf 'pwd\nexit\n' | ./$SHELL_BIN | sed "s/myshell> //g" | grep -c "^$PWD$")
[ "$out" = "1" ] && pass "pwd built-in" || bad "pwd built-in" "expected 1 match for $PWD, got $out"

# Week 5 - cd changes the SHELL's own directory (the point of a built-in)
out=$(printf 'cd /tmp\npwd\nexit\n' | ./$SHELL_BIN | sed "s/myshell> //g" | grep -c '^/tmp$')
[ "$out" = "1" ] && pass "cd built-in changes shell cwd" || bad "cd built-in changes shell cwd" "got $out matches"

# Week 5 - cd to a bad path reports an error and keeps going
out=$(printf 'cd /no/such/dir\necho after-bad-cd\nexit\n' | ./$SHELL_BIN 2>&1)
case "$out" in
  *cd:*after-bad-cd*) pass "cd error handling" ;;
  *) bad "cd error handling" "got: $out" ;;
esac

# Week 5 - cd with no argument prints usage
out=$(printf 'cd\nexit\n' | ./$SHELL_BIN)
case "$out" in
  *"Usage : cd directory"*) pass "cd usage message" ;;
  *) bad "cd usage message" "got: $out" ;;
esac

# Week 5 - help and env
out=$(printf 'help\nexit\n' | ./$SHELL_BIN)
case "$out" in
  *"Built-in Commands"*pwd*env*) pass "help built-in" ;;
  *) bad "help built-in" "got: $out" ;;
esac
out=$(printf 'env\nexit\n' | ./$SHELL_BIN)
case "$out" in
  *HOME\ =\ /*PATH\ =\ *) pass "env built-in" ;;
  *) bad "env built-in" "got: $out" ;;
esac

# Week 6 - SIGINT does not kill the shell
fifo=$(mktemp -u /tmp/sfin.XXXXXX)
mkfifo "$fifo"
outfile=$(mktemp /tmp/sfout.XXXXXX)
./$SHELL_BIN < "$fifo" > "$outfile" 2>&1 &
shpid=$!
exec 3> "$fifo"
printf 'echo before-signal\n' >&3
sleep 0.4
kill -INT $shpid
sleep 0.4
printf 'echo after-signal\nexit\n' >&3
exec 3>&-
wait $shpid 2>/dev/null
got=$(cat "$outfile")
rm -f "$fifo" "$outfile"
case "$got" in
  *before-signal*"Press 'exit' to quit"*after-signal*)
      pass "SIGINT handled, shell survives Ctrl+C" ;;
  *)  bad "SIGINT handled, shell survives Ctrl+C" "got: $got" ;;
esac

# Week 6 - no zombies left behind after running children
fifo=$(mktemp -u /tmp/sfin.XXXXXX)
mkfifo "$fifo"
./$SHELL_BIN < "$fifo" > /dev/null 2>&1 &
shpid=$!
exec 3> "$fifo"
printf 'true\ntrue\ntrue\n' >&3
sleep 0.5
zombies=$(ps -o stat= --ppid $shpid 2>/dev/null | grep -c Z || true)
printf 'exit\n' >&3
exec 3>&-
wait $shpid 2>/dev/null
rm -f "$fifo"
[ "$zombies" = "0" ] && pass "SIGCHLD reaps children (no zombies)" \
                     || bad "SIGCHLD reaps children (no zombies)" "found $zombies zombie(s)"

[ $fail -eq 0 ] && echo "all shell tests passed" || echo "some tests failed"
exit $fail
