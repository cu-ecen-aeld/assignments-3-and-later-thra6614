CC = $(CROSS_COMPILE)gcc

FLAGS = -g -Wall

writer.o: writer.c
	$(CC) $(FLAGS) writer.c -o writer

.PHONY : clean
clean:
	rm -f *.o writer

