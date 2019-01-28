CC = gcc 
CFLAGS = -Wall -g -fstack-protector-all 

.PHONY: all clean 

all: drone_pi

drone_pi: main.o utils.o
		$(CC) -o drone_pi main.o utils.o

utils.o: utils.c utils.h
		$(CC) $(CFLAGS) -c utils.c

main.o: main.c 
		$(CC) $(CFLAGS) -c main.c

clean: 
	rm -f main.o utils.o drone_pi
