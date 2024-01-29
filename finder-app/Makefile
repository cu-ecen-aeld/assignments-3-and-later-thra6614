CC = $(CROSS_COMPILE)gcc

FLAGS = -g -Wall

writer: writer.o
	$(CC) $(FLAGS) writer.o -o writer

writer.o: writer.c
	$(CC) $(FLAGS) -c writer.c -o writer.o

.PHONY : clean
clean:
	rm -f *.o writer

