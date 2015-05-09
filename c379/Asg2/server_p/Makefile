CC = gcc
CFLAGS = -g -Wall
LIBS = -lpthread -pthread

server_p.o: server_p.c
	$(CC) $(CFLAGS) $(LIBS) -c server_p.c
request_handler.o: request_handler.c
	$(CC) $(CFLAGS) $(LIBS) -c request_handler.c
server_p: request_handler.o server_p.o
	$(CC) $(CFLAGS) $(LIBS) -o server_p request_handler.o server_p.o

clean: 
	rm -f *.o tester *~ core


