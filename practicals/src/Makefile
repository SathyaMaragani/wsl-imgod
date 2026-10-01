CC = gcc
CFLAGS = -Wall -g
XFLAGS = -Wall -Wextra -g
OFLAGS = -Wall -Wextra -O2

all: prog1 prog2 prog3 wait_waitpid_demo zombie_process prog5 ls_grep_pipe \
     prog6_fifo_server prog6_fifo_client signal_handler \
     prog7_linuxaddr memory_demo dynamic_memory dynamic_memory_leak cow_demo \
     copy_lowlevel copy_stdio redirect_output redirect_input

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

prog5: prog5.c
	$(CC) $(CFLAGS) -o prog5 prog5.c

ls_grep_pipe: ls_grep_pipe.c
	$(CC) $(CFLAGS) -o ls_grep_pipe ls_grep_pipe.c

prog6_fifo_server: prog6_fifo_server.c
	$(CC) $(XFLAGS) -o prog6_fifo_server prog6_fifo_server.c

prog6_fifo_client: prog6_fifo_client.c
	$(CC) $(XFLAGS) -o prog6_fifo_client prog6_fifo_client.c

signal_handler: signal_handler.c
	$(CC) $(XFLAGS) -o signal_handler signal_handler.c

prog7_linuxaddr: prog7_linuxaddr.c
	$(CC) $(XFLAGS) -o prog7_linuxaddr prog7_linuxaddr.c

memory_demo: memory_demo.c
	$(CC) $(XFLAGS) -o memory_demo memory_demo.c

dynamic_memory: dynamic_memory.c
	$(CC) $(XFLAGS) -o dynamic_memory dynamic_memory.c

dynamic_memory_leak: dynamic_memory_leak.c
	$(CC) $(XFLAGS) -o dynamic_memory_leak dynamic_memory_leak.c

cow_demo: cow_demo.c
	$(CC) $(XFLAGS) -o cow_demo cow_demo.c

copy_lowlevel: copy_lowlevel.c
	$(CC) $(OFLAGS) -o copy_lowlevel copy_lowlevel.c

copy_stdio: copy_stdio.c
	$(CC) $(OFLAGS) -o copy_stdio copy_stdio.c

redirect_output: redirect_output.c
	$(CC) $(XFLAGS) -o redirect_output redirect_output.c

redirect_input: redirect_input.c
	$(CC) $(XFLAGS) -o redirect_input redirect_input.c

clean:
	rm -f prog1 prog2 prog3 wait_waitpid_demo zombie_process prog5 ls_grep_pipe \
	      prog6_fifo_server prog6_fifo_client signal_handler \
	      prog7_linuxaddr memory_demo dynamic_memory dynamic_memory_leak cow_demo \
	      copy_lowlevel copy_stdio redirect_output redirect_input \
     copy_lowlevel copy_stdio redirect_output redirect_input
