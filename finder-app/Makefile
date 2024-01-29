CC = $(CROSS_COMPILE)gcc

FLAGS = -g -Wall -c

writer: writer.o
	$(CC) writer.o -o writer

writer.o: writer.c
	$(CC) $(FLAGS) writer.c -o writer.o

.PHONY : clean
clean:
	rm -f *.o writer

