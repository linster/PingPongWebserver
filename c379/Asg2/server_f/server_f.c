#include "request_handler.h"

/* Server_f.c
 * CMPUT 379: Asg 2
 * Stefan Martynkiw
 * 1296154
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


static void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s portnumber docroot logfile\n", __progname);
	exit(1);
}

static void kidhandler(int signum) {
	/* signal handler for SIGCHLD */
	waitpid(WAIT_ANY, NULL, WNOHANG);
}


int main(int argc,  char *argv[])
{

	struct sockaddr_in sockname, client;
	char buffer[80], *ep;
	struct sigaction sa;
	socklen_t clientlen;
        int sd;
	u_short port;
	pid_t pid;
	u_long p;

	/*
	 * first, figure out what port we will listen on - it should
	 * be our first parameter.
	 */

	if (argc != 4)
		usage();
	errno = 0;
        p = strtoul(argv[1], &ep, 10);
        if (*argv[1] == '\0' || *ep != '\0') {
		/* parameter wasn't a number, or was empty */
		fprintf(stderr, "%s - not a number\n", argv[1]);
		usage();
	}
        if ((errno == ERANGE && p == ULONG_MAX) || (p > USHRT_MAX)) {
		/* It's a number, but it either can't fit in an unsigned
		 * long, or is too big for an unsigned short
		 */
		fprintf(stderr, "%s - value out of range\n", argv[1]);
		usage();
	}
	/* now safe to do this */
	port = p;

	#ifndef DEBUG
	if (daemon(1,0) == -1)
		err(1, "daemon() failed");
	#endif


	/* Grab the document root off the commandline */

	/* Copy into param as handle_request */
	/* Grab the logfile path off the commandline */
	char logfile[16384] = {'\0'};
	strncpy(&(logfile[0]), argv[3], 16384);

	memset(&sockname, 0, sizeof(sockname));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(port);
	sockname.sin_addr.s_addr = htonl(INADDR_ANY);
	sd=socket(AF_INET,SOCK_STREAM,0);
	if ( sd == -1)
		err(1, "socket failed");

	if (bind(sd, (struct sockaddr *) &sockname, sizeof(sockname)) == -1)
		err(1, "bind failed");

	if (listen(sd,3) == -1)
		err(1, "listen failed");

	/*
	 * we're now bound, and listening for connections on "sd" -
	 * each call to "accept" will return us a descriptor talking to
	 * a connected client
	 */


	/*
	 * first, let's make sure we can have children without leaving
	 * zombies around when they die - we can do this by catching
	 * SIGCHLD.
	 */
	sa.sa_handler = kidhandler;
        sigemptyset(&sa.sa_mask);
	/*
	 * we want to allow system calls like accept to be restarted if they
	 * get interrupted by a SIGCHLD
	 */
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1)
                err(1, "sigaction failed");

	/*
	 * finally - the main loop.  accept connections and deal with 'em
	 */
	printf("Server up and listening for connections on port %u\n", port);
	for(;;) {
		int clientsd;
		clientlen = sizeof(&client);
		clientsd = accept(sd, (struct sockaddr *)&client, &clientlen);
		if (clientsd == -1)
			err(1, "accept failed");
		else
			printf("Accepted Connectoion\n");


		/* Get the client addr from the client sockaddr */
		char* clientaddr = inet_ntoa(client.sin_addr);


		/*
		 * We fork child to deal with each connection, this way more
		 * than one client can connect to us and get served at any one
		 * time.
		 */

		pid = fork();
		if (pid == -1)
		     err(1, "fork failed");

		if(pid == 0) {
		/* We are in the child process */

		/* Read the request string from the client */
		//Max request size = 8192 bytes


		char requestbuf[8192] = {'\0'};
		char* buffer = &(requestbuf[0]);
		ssize_t readbts = 0; /* Number of bytes read */

		int r = -1;
		int rc = 0;
		int maxread = sizeof(buffer) - 1; /* leave room for a 0 byte */
		while ((r != 0) && rc < maxread) {
			r = read(clientsd, buffer + rc, 8192);

			if (r == -1) {
				if (errno != EINTR)
					err(1, "Read failed xx");
			} else{ 
				rc += r;

				//if (!strcmp(&(buffer[rc - 3]), "\n\n") ){
				/* Check if last read bytes are a newline
				 * and an empty line
				 * If so, then break */
				// break;
				//}

			}
		}
		/*
		 * we must make absolutely sure buffer has a terminating 0 byte
		 * if we are to use it as a C string 
		 */
		/*buffer[rc] = '\0';*/

		/*Need to declare logfile outside of the child */
		printf("\n\nserver.c logfile %s\n\n", logfile);
		int resp = handle_request(clientaddr, buffer, 8192, clientsd, logfile, argv[2]);
		exit(0);
		

		}
		close(clientsd);
	}
}

