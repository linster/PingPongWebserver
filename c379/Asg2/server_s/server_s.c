/* Portions (okay, Most of it) adapted from:
 * Copyright (c) 2008 Bob Beck <beck@obtuse.com>
 */

/*
 * select_server.c - an example of using select to implement a non-forking
 * server. In this case this is an "echo" server - it simply reads
 * input from clients, and echoes it back again to them, one line at
 * a time.
 *
 * to use, cc -DDEBUG -o select_server select_server.c
 * or cc -o select_server select_server.c after you read the code :)
 *
 * then run with select_server PORT
 * where PORT is some numeric port you want to listen on.
 *
 * to connect to it, then use telnet or nc
 * i.e.
 * telnet localhost PORT
 * or
 * nc localhost PORT
 * 
 */

#include "request_handler.h"
#include <sys/param.h>
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

#include <fcntl.h>
#include <sys/stat.h>



/* we use this structure to keep track of each connection to us */
struct con {

	/* Socket information */
	int sd; 		/* the socket for this connection */
	int state; 		/* the state of the connection */
	struct sockaddr_in sa; /* the sockaddr of the connection */
	size_t  slen;   	/* the sockaddr length of the connection */
	/* buf stores the HTTP request read in */
	char *buf;	/* a buffer to store the characters read in */
	char *bp;	/* where we are in the buffer */
	size_t bs;	/* total size of the buffer */
	size_t bl;	/* how much we have left to read/write */
	/* Stores the HTTP response out for writing */
	char* HTTP_buf; /* HTTP buffer ptr */
	char* HTTP_bp;	/* Where we are in the HTTP buffer */
	size_t HTTP_sz; /* HTTP buffer size */
	size_t HTTP_bl; /* How much left to read/write */
	/* Stores the logfile response out for writing */
	char* LOG_buf; 	/* Log file buffer ptr*/
	char* LOG_bp;  	/* Where we are in the Log buffer */
	size_t LOG_sz; 	/* Log buffer size */
	size_t LOG_bl;	/* How much left to read/write */
};

#define MAXCONN 256
struct con connections[MAXCONN];

#define BUF_ASIZE 256 /* how much buf will we allocate at a time. */

/* states used in struct con. */
#define STATE_UNUSED 0
#define STATE_READING 1
#define STATE_WRITING_HTTP 2
#define STATE_WRITING_LOG 3


/* Global variables for Log file descriptor, logfile path, docroot path */
int logfilefd;
char* logfilepath;
char* docrootpath;

static void usage(){
	extern char * __progname;
	fprintf(stderr, "usage: %s portnumber docroot logfile\n", __progname);
	exit(1);
}


/*
 * get a free connection structure, to save a new connection in
 */
struct con * get_free_conn()
{
	int i;
	for (i = 0; i < MAXCONN; i++) {
		if (connections[i].state == STATE_UNUSED)
			return(&connections[i]);
	}
	return(NULL); /* we're all full - indicate this to our caller */
}



/*
 * close or initialize a connection - resets a connection to the default,
 * unused state.
 */
void closecon (struct con *cp, int initflag)
{
	if (!initflag) {
		if (cp->sd != -1)
			close(cp->sd); /* close the socket */
		free(cp->buf); /* free up our buffer */
	}
	memset(cp, 0, sizeof(struct con)); /* zero out the con struct */
	cp->HTTP_buf = (char*)malloc(2000);
	cp->LOG_buf  = (char*)malloc(2000);
	cp->buf = NULL; /* unnecessary because of memset above, but put here
			 * to remind you NULL is 0.
			 */
	cp->sd = -1;
}

/*
 * handlewrite - deal with a connection that we want to write stuff
 * to. assumes the caller has checked that cp->sd is writeable
 * by using select(). once we write everything out, change the
 * state of the connection to the reading state.
 */
void handlewrite_http(struct con *cp)
{

/* Here I can assume that handleread() has completed. 
 * Now I can call handle_request and get it to write into
 * the appropriate connection struct's HTTP_buf and LOG_buf
 * members.
 *
 * Then I will use Bob's logic to ensure the writes are complete.
 * the writes are not complete  till both LOG_bl and HTTP_bl
 * are zero. Once they are, change state to Readable and free 
 * the buffers pointed to.
 */

/*80085 is the magic socket number to make handle_request 
 * behave for non-blocking IO. Sorry if this reminds you of PHP2 */
//	handle_request(cp->buf, cp->bs, 80085, logfilepath, docrootpath,
//		 cp->HTTP_buf, cp->LOG_buf);

	if (!cp->HTTP_sz){
		handle_request(cp->buf, cp->bs, 
				80085, logfilepath, docrootpath,
		 		(cp->HTTP_buf), (cp->LOG_buf));
		cp->HTTP_sz = strlen(cp->HTTP_buf); 
		cp->HTTP_bl = cp->HTTP_sz;
		cp->HTTP_bp = cp->HTTP_buf;
	}
	printf("size of HTTP_buf: %d", cp->HTTP_sz);


	printf("\n\n ******* HTTP BUFFER ***** \n\n %s", cp->HTTP_buf);


	ssize_t i;

	/*
	 * assuming before we are called, cp->sd was put into an fd_set
	 * and checked for writeability by select, we know that we can
	 * do one write() and write something. We are *NOT* guaranteed
	 * how much we can write. So while we will be able to write bytes
	 * we don't know if we will get a whole line, or even how much
	 * we will get - so we do *exactly* one write. and keep track
	 * of where we are. If we don't want to block, we can't do
	 * multiple writes to write everything out without calling
	 * select() again between writes.
	 */

	i = write(cp->sd, cp->HTTP_bp, cp->HTTP_bl);
	//i = write(cp->sd, cp->HTTP_bp, cp->HTTP_sz);
	printf("i %d", i);

	if (i == -1) {
		if (errno != EAGAIN) {
			/* the write failed */
			closecon(cp, 0);
		}
		/*
		 * note if EAGAIN, we just return, and let our caller
		 * decide to call us again when socket is writable
		 */
		return;
	}
	/* otherwise, something ok happened */
	cp->HTTP_bp += i; /* move where we are */
	cp->HTTP_bl -= i; /* decrease how much we have left to write */
	if (cp->HTTP_bl == 0) {
		/* we wrote it all out, hooray, so go back to reading */
		cp->state = STATE_WRITING_LOG;
		cp->bl = cp->bs; /* we can read up to this much */
		cp->bp = cp->buf;	    /* we'll start at the beginning */
		cp->HTTP_bl = cp->HTTP_sz;
		cp->LOG_bp = cp->LOG_buf;


		cp->HTTP_sz;
		free(cp->HTTP_buf);
	}
}
void handlewrite_log(struct con *cp)
{
printf("In handlewrite_log");
/* Here I can assume that handleread() has completed. 
 * Now I can call handle_request and get it to write into
 * the appropriate connection struct's HTTP_buf and LOG_buf
 * members.
 *
 * Then I will use Bob's logic to ensure the writes are complete.
 * the writes are not complete  till both LOG_bl and HTTP_bl
 * are zero. Once they are, change state to Readable and free 
 * the buffers pointed to.
 */

/*80085 is the magic socket number to make handle_request 
 * behave for non-blocking IO. Sorry if this reminds you of PHP2 */
	if (!cp->LOG_sz){
	handle_request(cp->buf, cp->bs, 80085, 
			logfilepath, docrootpath,
			cp->HTTP_buf, cp->LOG_buf);
	cp->LOG_sz = strlen(cp->LOG_buf);
	}



	ssize_t i;

	/*
	 * assuming before we are called, cp->sd was put into an fd_set
	 * and checked for writeability by select, we know that we can
	 * do one write() and write something. We are *NOT* guaranteed
	 * how much we can write. So while we will be able to write bytes
	 * we don't know if we will get a whole line, or even how much
	 * we will get - so we do *exactly* one write. and keep track
	 * of where we are. If we don't want to block, we can't do
	 * multiple writes to write everything out without calling
	 * select() again between writes.
	 */

	i = write(logfilefd, cp->LOG_bp, cp->LOG_bl);
	printf("log i: %d", i);
	if (i == -1) {
		if (errno != EAGAIN) {
			/* the write failed */
			closecon(cp, 0);
		}
		/*
		 * note if EAGAIN, we just return, and let our caller
		 * decide to call us again when socket is writable
		 */
		return;
	}
	/* otherwise, something ok happened */
	cp->LOG_bp += i; /* move where we are */
	cp->LOG_bl -= i; /* decrease how much we have left to write */
	if (cp->LOG_bl == 0 && cp->LOG_bl == 0) {
		/* we wrote it all out, hooray, so go back to reading */
		cp->state = STATE_READING;
		cp->bl = cp->bs; /* we can read up to this much */
		cp->bp = cp->buf;	    /* we'll start at the beginning */
		
		cp->LOG_sz = 0;
		free(cp->LOG_buf);

	}
}

/*
 * handleread - deal with a connection that we want to read stuff
 * from. assumes the caller has checked that cp->sd is writeable
 * by using select(). If a newline is seen at the end of what we
 * are reading, change the state of this connection to the writing
 * state.
 */
void handleread(struct con *cp){

	/* Moderate hack here:
	 *
	 * request_handler.c segfaults on the realloc calls
	 * when prepping HTTP_buf and LOG_buf for writing into.
	 * Since all connections pass through this state, use this
	 * point in control flow to malloc some really small area on the 
	 * heap for LOG_buf and HTTP_buf to be resized later during 
	 * the write phases */
	//cp->HTTP_buf = (char*)calloc(2, sizeof(char));
	//cp->LOG_buf = (char*)calloc(2, sizeof(char));

	ssize_t i;

	/*
	 * first, let's make sure we have enough room to do a
	 * decent sized read.
	 */

	if (cp->bl < 10) {
		char *tmp;
		tmp = realloc(cp->buf, (cp->bs + BUF_ASIZE) * sizeof(char));
		if (tmp == NULL) {
			/* we're out of memory */
			closecon(cp, 0);
			return;
		}
		cp->buf = tmp;
		cp->bs += BUF_ASIZE;
		cp->bl += BUF_ASIZE;
		cp->bp = cp->buf + (cp->bs - cp->bl);
	}

	/*
	 * assuming before we are called, cp->sd was put into an fd_set
	 * and checked for readability by select, we know that we can
	 * do one read() and get something. We are *NOT* guaranteed
	 * how much we can get. So while we will be able to read bytes
	 * we don't know if we will get a whole line, or even how much
	 * we will get - so we do *exactly* one read. and keep track
	 * of where we are. If we don't want to block, we can't do
	 * multiple reads to read in a whole line without calling
	 * select() to check for readability between each read.
	 */
	i = read(cp->sd, cp->bp, cp->bl);
	if (i == 0) {
		/* 0 byte read means the connection got closed */
		closecon(cp, 0);
		return;
	}
	if (i == -1) {
		if (errno != EAGAIN) {
			/* read failed */
			err(1, "read failed! sd %d\n", cp->sd);
			closecon(cp, 0);
		}
		/*
		 * note if EAGAIN, we just return, and let our caller
		 * decide to call us again when socket is readable
		 */
		return;
	}
	/*
	 * ok we really got something read. chage where we're
	 * pointing
	 */
	cp->bp += i;
	cp->bl -= i;

	/*
	 * now check to see if we should change state - i.e. we got
	 * a newline on the end of the buffer
	 */
	if (*(cp->bp - 1) == '\n') {
		cp->state = STATE_WRITING_HTTP;
		cp->bl = cp->bp - cp->buf; /* how much will we write */
		cp->bp = cp->buf;	   /* and we'll start from here */
	}
}

int main(int argc,  char *argv[])
{
	struct sockaddr_in sockname;
	int max = -1, omax;	     /* the biggest value sd. for select */
	int sd;			     /* our listen socket */
	fd_set *readable = NULL , *writable = NULL; /* fd_sets for select */
	u_short port;
	u_long p;
	char *ep;
	int i;

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

	/* Grab the docroot off the commandline */
	docrootpath = (char*)calloc(strlen(argv[2]), sizeof(char)  );
	strncpy (docrootpath, argv[2], strlen(argv[2]));

	/* Grab the logfilepath off the commandline */
	logfilepath = (char*)calloc(strlen(argv[3]), sizeof(char));
	strncpy(logfilepath, argv[3], strlen(argv[3]));

	/* Open the logfile for appending and store the socket descriptor */
	if (access(logfilepath, F_OK) < 0) {
                /* Create logfile */
                creat(logfilepath, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }
        /* File exists but cannot be written or read from */
        if (access(logfilepath, R_OK|W_OK) < 0 && (access(logfilepath, F_OK) == 0 )){
                printf("Error accessing logfile");
                err(1, "Logfile cannot be written to");
                return 0;
        }
        logfilefd = open(logfilepath, O_WRONLY|O_APPEND);
        if (logfilefd < 0) {
                printf("Error opening logfile");
                err(1, "Error opening logfile");
                close(logfilefd);
                return 0;
        }


	/* now before we get going, decide if we want to daemonize, that
	 * is, run in the background like a real system process
	 */
#ifndef DEBUG
	/* don't daemonize if we compile with -DDEBUG */
	if (daemon(1, 0) == -1)
		err(1, "daemon() failed");
#endif

	/* now off to the races - let's set up our listening socket */
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
	 * We're now bound, and listening for connections on "sd".
	 * Each call to "accept" will return us a descriptor talking to
	 * a connected client.
	 */

	/*
	 * finally - the main loop.  accept connections and deal with 'em
	 */
#ifndef DEBUG
	/*
	 * since we'll be running as a daemon if we're not compiled with
	 * -DDEBUG, we better not be using printf - since stdout will be
	 * unusable
	 */
	printf("Server up and listening for connections on port %u\n", port);
#endif	

	/* initialize all our connection structures */
	for (i = 0; i < MAXCONN; i++)
		closecon(&connections[i], 1);

	for(;;) {
		int i;
		int maxfd = -1; /* the biggest value sd we are interested in.*/

		/*
		 * first we have to initialize the fd_sets to keep
		 * track of readable and writable sockets. we have
		 * to make sure we have fd_sets that are big enough
		 * to hold our largest valued socket descriptor.
		 * so first, we find the max value by iterating through
		 * all the connections, and then we allocate fd sets
		 * that are big enough, if they aren't already.
		 */
		omax = max;
		max = sd; /* the listen socket */

		for (i = 0; i < MAXCONN; i++) {
			if (connections[i].sd > max)
				max = connections[i].sd;
		}
		if (max > omax) {
			/* we need bigger fd_sets allocated */

			/* free the old ones - does nothing if they are NULL */
			free(readable);
			free(writable);

			/*
			 * this is how to allocate fd_sets for select
			 */
			readable = (fd_set *)calloc(howmany(max + 1, NFDBITS),
			    sizeof(fd_mask));
			if (readable == NULL)
				err(1, "out of memory");
			writable = (fd_set *)calloc(howmany(max + 1, NFDBITS),
			    sizeof(fd_mask));
			if (writable == NULL)
				err(1, "out of memory");
			omax = max;
			/*
			 * note that calloc always returns 0'ed memory,
			 * (unlike malloc) so these sets are all set to 0
			 * and ready to go
			 */
		} else {
			/*
			 * our allocated sets are big enough, just make
			 * sure they are cleared to 0. 
			 */
			memset(readable, 0, howmany(max+1, NFDBITS) *
			    sizeof(fd_mask));
			memset(writable, 0, howmany(max+1, NFDBITS) *
			    sizeof(fd_mask));
		}

		/*
		 * Now, we decide which sockets we are interested
		 * in reading and writing, by setting the corresponding
		 * bit in the readable and writable fd_sets.
		 */

		/*
		 * we are always interesting in reading from the
		 * listening socket. so put it in the read set.
		 */

		FD_SET(sd, readable);
		if (maxfd < sd)
			maxfd = sd;

		/*
		 * now go through the list of connections, and if we
		 * are interested in reading from, or writing to, the
		 * connection's socket, put it in the readable, or
		 * writable fd_set - in preparation to call select
		 * to tell us which ones we can read and write to.
		 */
		for (i = 0; i<MAXCONN; i++) {
			if (connections[i].state == STATE_READING) {
				FD_SET(connections[i].sd, readable);
				if (maxfd < connections[i].sd)
					maxfd = connections[i].sd;
			}
			if (connections[i].state == STATE_WRITING_HTTP) {
				FD_SET(connections[i].sd, writable);
				if (maxfd < connections[i].sd)
					maxfd = connections[i].sd;
			}
			if (connections[i].state == STATE_WRITING_LOG) {
				FD_SET(connections[i].sd, writable);
				if (maxfd < connections[i].sd)
					maxfd = connections[i].sd;
			}
		}

		/*
		 * finally, we can call select. we have filled in "readable"
		 * and "writable" with everything we are interested in, and
		 * when select returns, it will indicate in each fd_set
		 * which sockets are readable and writable
		 */
		i = select(maxfd + 1, readable, writable, NULL,NULL);
		if (i == -1  && errno != EINTR)
			err(1, "select failed");
		if (i > 0) {

			/* something is readable or writable... */

			/*
			 * First things first.  check the listen socket.
			 * If it was readable - we have a new connection
			 * to accept.
			 */

			if (FD_ISSET(sd, readable)) {
				struct con *cp;
				int newsd;
				socklen_t slen;
				struct sockaddr_in sa;

				slen = sizeof(sa);
				newsd = accept(sd, (struct sockaddr *)&sa,
				    &slen);


				if (newsd == -1)
					err(1, "accept failed");

				cp = get_free_conn();
				if (cp == NULL) {
					/*
					 * we have no connection structures
					 * so we close connection to our
					 * client to not leave him hanging
					 * because we are too busy to
					 * service his request
					 */
					close(newsd);
				} else {
					/*
					 * ok, if this worked, we now have a
					 * new connection. set him up to be
					 * READING so we do something with him
					 */
					cp->state = STATE_READING;
					cp->sd = newsd;
					cp->slen = slen;
					memcpy(&cp->sa, &sa, sizeof(sa));
				}
			}
			/*
			 * now, iterate through all of our connections,
			 * check to see if they are readble or writable,
			 * and if so, do a read or write accordingly 
			 */
			for (i = 0; i<MAXCONN; i++) {
				if ((connections[i].state == STATE_READING) &&
				    FD_ISSET(connections[i].sd, readable))
					handleread(&connections[i]);
				if ((connections[i].state == STATE_WRITING_HTTP) &&
				    FD_ISSET(connections[i].sd, writable))
					handlewrite_http(&connections[i]);
				if ((connections[i].state == STATE_WRITING_LOG) &&
				    FD_ISSET(connections[i].sd, writable))
					handlewrite_log(&connections[i]);
			}
		}
	}
}
