#include "request_handler.h"

/* Server_f.c
 * CMPUT 379: Asg 2
 * Stefan Martynkiw
 * 1296154
 */

#include <pthread.h>

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

/* Number of simultaneous requests */
#define NUM_SIMULT_REQ	64



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


struct handler_data { //Move this outside of any functions, near BOF
	char* request_str;
	int request_len;
	int clientsd;
	char* logfile;
	char* docroot;
	int* inUseFlag;
	char* clientaddr; /*String for the client address */
	pthread_mutex_t* resplock;
};

	/* Array of thread_data structs for passing */
	struct handler_data handler_data_array[NUM_SIMULT_REQ];

	/* Mutex for locking out of handle_request */
	pthread_mutex_t gresplock = PTHREAD_MUTEX_INITIALIZER;



void* THandleRequest(void* threadarg) {

	struct handler_data *Targs;
	sleep(1);

	Targs = (struct handler_data *) threadarg;

	/* CRITICAL SECTION */
	pthread_mutex_lock( (Targs->resplock) );
	int resp = handle_request(Targs->clientaddr,
				  Targs->request_str,
				  Targs->request_len,
				  Targs->clientsd,
				  Targs->logfile,
				  Targs->docroot);

	/* Close the client connection before exiting */
	close(Targs->clientsd);
	pthread_mutex_unlock( (Targs->resplock) );
	/* END CRITICAL SECTION */


	/* Relinquish tnum so that other threads can use the slot */
	*(Targs->inUseFlag) = 0;
	return 0;
}



int main(int argc,  char *argv[])
{



	struct sockaddr_in sockname, client;
	char *ep;
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
        	/* don't daemonize if we compile with -DDEBUG */
        	if (daemon(1, 0) == -1)
                	err(1, "daemon() failed");
	#endif





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



	/* Set up threads */
	pthread_t threads[NUM_SIMULT_REQ];
	
	/* Stores the current thread index. When we thread off a req, 
	 * use this as the index into the threads[] array */
	int tnum = 0; /*Stores the current thread index */

	/*Need an inUse[tnum] array. set to 0 when thread relinquished.
	 *set to 1 when thread using the tnum index. */
	int inUse[NUM_SIMULT_REQ] = {0};







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

		/* Get the data from the client before making a 
		 * handler thread */


		/* Get the client addr from the client sockaddr */
                char* clientaddr = inet_ntoa(client.sin_addr);


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
			}
		}
		/*
		 * we must make absolutely sure buffer has a terminating 0 byte
		 * if we are to use it as a C string 
		 */
		buffer[rc] = '\0';


		/* Need to find a tnum that is not in use */

		int i;
		for (i = 0; i < NUM_SIMULT_REQ; i++) {
			if (inUse[i] == 0) {
				tnum = i;
				inUse[tnum] = 1;
			}
		}

		/* Create a new thread for each connection */
		handler_data_array[tnum].request_str = buffer;
		handler_data_array[tnum].request_len = 8192;
		handler_data_array[tnum].clientsd = clientsd;
		handler_data_array[tnum].logfile = logfile;
		handler_data_array[tnum].docroot = argv[2];
		handler_data_array[tnum].inUseFlag = &(inUse[tnum]);
		handler_data_array[tnum].resplock = &gresplock;
		/* This.clientaddr an allocated buffer made by inet_ntoa */
		handler_data_array[tnum].clientaddr = clientaddr;

		rc = pthread_create(&threads[tnum], NULL, THandleRequest, 
				(void*) &handler_data_array[tnum]);
		if (rc) {
                	printf("ERROR; ret from pthread_create() is %d\n", rc);
                	exit(-1);
            	}

	}
}


