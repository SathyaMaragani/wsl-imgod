CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
SRC = src/main.c \
      src/input.c \
      src/parser.c \
      src/process.c \
      src/builtin.c \
      src/signals.c
TARGET = bin/shellforge

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run:
	./$(TARGET)

test: $(TARGET) bin/test_parser
	./bin/test_parser
	./tests/test_shell.sh

bin/test_parser: tests/test_parser.c src/parser.c
	mkdir -p bin
	$(CC) $(CFLAGS) tests/test_parser.c src/parser.c -o bin/test_parser

clean:
	rm -rf bin/*
