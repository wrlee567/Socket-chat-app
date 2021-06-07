CC=gcc  #compiler
TARGET= s-talk

all:
	$(CC) -pthread thread.c list.o -o $(TARGET)
valgrind:
	



clean:
	rm $(TARGET)