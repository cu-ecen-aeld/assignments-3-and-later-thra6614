CC = $(CROSS_COMPILE)gcc

FLAGS = -g -Wall


all: writer.c
	$(CC) $(FLAGS) writer.c -o writer

.PHONY : clean
clean:
	rm -f *.o writer

