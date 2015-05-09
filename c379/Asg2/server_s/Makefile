CC = gcc
CFLAGS = -g -Wall #-DDEBUG
LIBS = -lpthread

server_s.o: server_s.c
	$(CC) $(CFLAGS) -c server_s.c
request_handler.o: request_handler.c
	$(CC) $(CFLAGS) -c request_handler.c
server_s: request_handler.o server_s.o
	$(CC) $(CFLAGS) -o server_s request_handler.o server_s.o

clean: 
	rm -f *.o tester *~ core


