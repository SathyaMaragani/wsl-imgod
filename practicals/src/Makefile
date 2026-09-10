CC = gcc
CFLAGS = -Wall -g

all: prog1 prog2 prog3 wait_waitpid_demo zombie_process

prog1: prog1.c
	$(CC) $(CFLAGS) -o prog1 prog1.c

prog2: prog2.c
	$(CC) $(CFLAGS) -o prog2 prog2.c

prog3: prog3.c
	$(CC) $(CFLAGS) -o prog3 prog3.c

wait_waitpid_demo: wait_waitpid_demo.c
	$(CC) $(CFLAGS) -o wait_waitpid_demo wait_waitpid_demo.c

zombie_process: zombie_process.c
	$(CC) $(CFLAGS) -o zombie_process zombie_process.c

clean:
	rm -f prog1 prog2 prog3 wait_waitpid_demo zombie_process
