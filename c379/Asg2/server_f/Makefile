CC = gcc
CFLAGS = -g -Wall
LIBS = -lpthread

server_f.o: server_f.c
	$(CC) $(CFLAGS) -c server_f.c
request_handler.o: request_handler.c
	$(CC) $(CFLAGS) -c request_handler.c
server_f: request_handler.o server_f.o
	$(CC) $(CFLAGS) -o server_f request_handler.o server_f.o

clean: 
	rm -f *.o tester *~ core


