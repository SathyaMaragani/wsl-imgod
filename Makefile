CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
SRC = src/main.c \
      src/input.c \
      src/parser.c \
      src/process.c \
      src/builtin.c \
      src/signals.c \
      src/pipes.c
TARGET = bin/shellforge

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run:
	./$(TARGET)

# Week 8: AddressSanitizer build - catches overflows and use-after-free at runtime
asan:
	mkdir -p bin
	$(CC) $(CFLAGS) -fsanitize=address -fno-omit-frame-pointer $(SRC) -o bin/shellforge_asan

# Week 8: leak check over a scripted session
memcheck: $(TARGET)
	printf 'echo hi\npwd\nls | wc -l\nno_such_cmd\nexit\n' | \
	valgrind --leak-check=full --child-silent-after-fork=yes --error-exitcode=9 ./$(TARGET)

test: $(TARGET) bin/test_parser
	./bin/test_parser
	./tests/test_shell.sh

bin/test_parser: tests/test_parser.c src/parser.c
	mkdir -p bin
	$(CC) $(CFLAGS) tests/test_parser.c src/parser.c -o bin/test_parser

clean:
	rm -rf bin/*
