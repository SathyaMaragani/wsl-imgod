CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
SRC = src/main.c \
      src/input.c \
      src/parser.c
TARGET = bin/shellforge

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run:
	./$(TARGET)

test:
	./tests/test_parser.sh

clean:
	rm -rf bin/*
