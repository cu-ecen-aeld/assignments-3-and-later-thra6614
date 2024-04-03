CC = $(CROSS_COMPILE)gcc

FLAGS = -g -Wall -Werror -pthread -lrt


all: socket.c
	$(CC) $(FLAGS) socket.c -o aesdsocket

.PHONY : clean
clean:
	rm -f *.o aesdsocket

