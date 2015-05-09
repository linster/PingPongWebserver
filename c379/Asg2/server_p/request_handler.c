/* CMPUT 379: Assignment 2
 * Stefan Martynkiw	1296154
 * Feb 18, 2014
 *
 * Webserver core request handler 
 */

 /* This file contains the logic that is called within a fork(), p_thread(),
  * or in server_s.
  */

#include "request_handler.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>


#define MAX_REQ_FILENAME        1024 /* Max number of chars in filename reqd*/
#define MAX_REQ_PATH            8192 /* Max number of characters in path */
#define DEBUG                   0    /*Set to 1 for debugging */

#define LEN_ERR_RESPONSE        16384 /* Maximum length of response on
                                       * HTTP error. Implies that error
                                       * pages can be roughly 14KiB in size */

#define MAX_REQ_PATH            8192 /* Max number of characters in path */

char document_root[MAX_REQ_PATH] = {'\0'};







/* Document root for the webserver: Must have a trailing slash */
//char document_root[MAX_REQ_PATH] = "/cshome/smartynk/379/Asg2/docroot/";
//char document_root[MAX_REQ_PATH] = {'\0'};


int logout(char* client_addr, char* req_line, char* logfilename, int clientsd, 
	int response_code, int l200written, int l200total) {

	char msg200[] = "200 OK";
        char msg400[] = "400 Bad Request";
        char msg403[] = "403 Forbidden";
        char msg404[] = "404 Not Found";
        char msg500[] = "500 Internal Server Error";

        char httpmessagebuf[64] = {'\0'};
        char* httpmsg = &(httpmessagebuf[0]);
        switch (response_code) {
                case 200:
                        strcpy(httpmsg, msg200);
                        break;
                case 400:
                        strcpy(httpmsg, msg400);
                        break;
                case 403:
                        strcpy(httpmsg, msg403);
                        break;
                case 404:
                        strcpy(httpmsg, msg404);
                        break;
                case 500:
                        strcpy(httpmsg, msg500);
                        break;
        }

	printf("\n\nhttpmessage buf: %s\n\n", httpmsg);

	//char client_addr[] = "0.0.0.0";
	printf("client addr %s", client_addr);
	
	 /*Build the time string */
        /* Get the GMT time */
        time_t local_time = time(NULL);
	struct tm tmGMT = {0};
        gmtime_r(&local_time, &tmGMT);
        /* Prepare time string */
        char timestr[320] = {'\0'};
        char* tstr = &(timestr[0]);

        int lentime = strftime(tstr, 320, "%c GMT", &tmGMT);
	printf("\ntimestr %s\n", tstr);
        char loglinearr[16384] = {'\0'};
	char* logline = &(loglinearr[0]);

        int append200 = snprintf(logline, 16384, "%s\t%s\t%s\t%s", tstr,
                client_addr,  req_line,  httpmsg);

        if (response_code == 200){
                sprintf(logline+append200," %d/%d \n", l200written, l200total);
        } else {
                strcat(logline, "\n");
        }


        /* Need to get the log file in as a parameter,
         * open it, then write to it
         */
        printf("Logline %s", logline);
        printf("Logfile path %s", logfilename);

	 if (access(logfilename, F_OK) < 0) {
                /* Create logfile */
		//printf("creating logfile");
                creat(logfilename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }
	//printf("after creat");


        /* File exists but cannot be written or read from */
        if (access(logfilename, R_OK|W_OK) < 0 && (access(logfilename, F_OK) == 0)){
                printf("Error accessing logfile");
                err(1, "Logfile cannot be written to");
                return 0;
        }



        int fdlog = open(logfilename, O_WRONLY|O_APPEND);
        printf("Fdlog %d:", fdlog);

        if (fdlog < 0) {
                printf("Error opening logfile");
                err(1, "Error opening logfile");
		close(fdlog);
                return 0;
        }

        //int writecode = write_to_sd(logline, 300, fdlog);

	int writecode = write(fdlog, logline, strlen(logline));
	
        if (writecode <= 0){
                printf("Error in write_to_sd while logging");
                err(1, "Error in write_to_sd while logging");
		close(fdlog);
                return 0;
        }

	close(fdlog);
        return writecode; /* Return number of bytes written to logfile */


	return 0;

}



int request_valid(char* request_str, int request_len, 
			char* requested_filename) {
	/* Parses the HTTP request. Returns 0 if invalid, length of 
	 * path requested if valid
	 */

	/* The request must be of the form:
	 * GET filename HTTP/1.1
	 * <any number of lines>
	 * \n
	 */

	const char get[] = "GET";
	if (strncmp(request_str, &(get[0]), 3)) {
		/* GET not at beginning of line */
		return 0;
	}

	int i = 4; /* first letter of filename in GET */
	int j = 0; /* cursor in requested_filename */
	while (request_str[i] != ' ' && i <= request_len) {
		requested_filename[j] = request_str[i];
		j++;
		i++;
	}

	if (j > MAX_REQ_FILENAME) {
		/* Requested filename too long */
		printf("Requested filename too long \n");
		return 0;
	}

	/* Need to check if HTTP/1.1 is at the end of the GET line */

	int index_http = i+1; /* At index_http + 0, we should have the 'H' */
	const char http_head[] = "HTTP/1.1";

	/* Strncmp returns 0 if strings match */
	if (!strncmp(&(request_str[index_http]), &(http_head[0]), 8) ){
		 /* Line contains HTTP/1.1 at the end */

		//BUG
		// CHECK HERE IF REQUEST CONTAINS \n\0 at the end
                if (! strstr(request_str, "\n\n\0")){
                        return 0;
                }




		/* Return a convinient index for future use */
		return j+1;
	} else {
		if (DEBUG == 2) {
			printf("Returning at second strncmp! \n");
			printf("Debugging output \n");
			printf("request_str[index_http]:|%s|\n", &(request_str[index_http]));
			printf("&http_head:|%s|\n", &(http_head[0]));
			printf("request_str[j]:|%s| \n", &(request_str[j]));
			printf("request_filename:|%s|", &(requested_filename[0]));
		}
		return 0;
	}

}


int resp_error(int number, char *r) {
	
	/* This function prepares the error message seen by the client
	 * when there's a 400, 403, 404, 500
	 */

	char e400[] = "400 Bad Request\n";
	char e403[] = "403 Forbidden\n";
	char e404[] = "404 Not Found\n";
	char e500[] = "500 Internal Server Error\n";

	char m400[] = 	"<html><body>\n<h2>HTTP: 400</h2>"
			"Bad request.</body></html>";

	char m403[] = 	"<html><body>\n<h2>HTTP: 403: Forbidden</h2>\n"
			"Access Denied. Reporting to <s>NSA</s> AICT. "
			"\n</body></html>";

	char m404[] = 	"<html><body>\n<h2>HTTP: 404: File Not Found</h2>"
			"\n</body></html>";

	char m500[] = 	"<html><body>\n"
			"<h2>HTTP: 500: Internal Server Error</h2>\n"
			"Exits are North, South, East, and Dennis.\n </body>"
			"</html>";




	char http_header[] = "HTTP/1.1 ";
	strcat(r, &(http_header[0]));

	switch(number) {
		case 400:
			strcat(r, &(e400[0])); break;
		case 403:
			strcat(r, &(e403[0])); break;
		case 404:
			strcat(r, &(e404[0])); break;
		case 500:
			strcat(r, &(e500[0])); break;
		default: /* Should reject other inputs */
			printf("Unsupported error type. Type entered is %d", number);
			return 0;
	}
	/* Get the GMT time */
	time_t local_time = time(NULL);
	struct tm tmGMT = {0};
	gmtime_r(&local_time, &tmGMT);
	/* Prepare time string */
	char timestr[80] = {'\0'};
	strftime(&(timestr[0]), 80, "Date: %c GMT \n", &tmGMT);
	strcat(r, &(timestr[0]));
	/* Content type of error output */
	strcat(r, "Content-Type: text/html\n");

	/* Content Length of response */
	strcat(r, "Content-Length: @               \n\n");

	/* Find @, replace with strlen(mXXX) */
	char* length_field = strchr(r, '@');
	char length_num[10] = {'\0'}; 

	/* Append required error page and prep Content-Length */
	int ll;
	switch(number) {
		case 400:
		strcat(r, &(m400[0])); 
	ll = snprintf(&(length_num[0]) ,8, "%d", (int)strlen(&(m400[0])));
		break;

		case 403:
		strcat(r, &(m403[0]));
	ll = snprintf(&(length_num[0]) ,8, "%d", (int)strlen(&(m403[0])));
		break;

		case 404:
		strcat(r, &(m404[0]));
	ll = snprintf(&(length_num[0]) ,8, "%d", (int)strlen(&(m404[0])));
		break;
		case 500:
		strcat(r, &(m500[0])); 
	ll = snprintf(&(length_num[0]) ,8, "%d", (int)strlen(&(m500[0])));
		break;
	}


	//BUG: Content-Length is of the included page only, not the total
	//	response!! FIXED

	/* Crappy strcpy into the r buffer. Doesn't copy over the
	 * \0 on purpose so the string doesn't end unexpectedly */
	int i = 0;
	while(i<ll){
		length_field[i] = length_num[i];
		i++;
	}

	if (1) {
		printf("Error Response: \n\n%s\n", r);
	}

	return (int)strlen(r); /* Return length of response */
}

int build_200response(char* response, int filesize, char* filepointer) {
/* Builds the HTTP 200 reponse header with correct 
 * GMT time, Content-Size, and so on
 */

	strcat(response, "HTTP/1.1 200 OK \n");

	/* Get the GMT time */
	time_t local_time = time(NULL);
	struct tm tmGMT = {0};
	gmtime_r(&local_time, &tmGMT);
	/* Prepare time string */
	char timestr[80] = {'\0'};
	strftime(&(timestr[0]), 80, "Date: %c GMT \n", &tmGMT);
	strcat(response, &(timestr[0]));
	/* Content type of output */
	strcat(response, "Content-Type: text/html\n@");

	char length_line[40] = {'\0'};
	int ll = snprintf( &(length_line[0]), 40, "Content-Length: %d\n\n",
			   filesize);

	char* length_insertion_pt = strchr(response, '@');

	/* Crappy Strcat into response buffer, not copying the trailing \0 */
	int i = 0;
	while (i < ll){
		length_insertion_pt[i] = length_line[i];
		i++;
	}

	strcat(response, "\n");

	/* Scan the contents of the file pointer and append to response*/
	strcat(response, filepointer);

	return 1;

}


int write_to_sd(char* buffer, unsigned int len, int clientsd) {
/** Writes the contents of buffer to the socket described by clientsd
  * returns: number of bytes written to socket
  * returns 0 if no bytes written to socket
  * returns -1 if error
  */

	if (0) {
		/* Only print out what was to be written */
		printf("write_to_sd debug print \n\n %s", buffer);
		//return 1;
	}

	//return 0;


	ssize_t written, w;
	/*
	 * write the message to the client, being sure to
	 * handle a short write, or being interrupted by
	 * a signal before we could write anything.
	 */
	w = 0;
	written = 0;
	while (written < strlen(buffer)) {
		w = write(clientsd, buffer + written,
 	        strlen(buffer) - written);
		if (w == -1) {
			if (errno != EINTR)
			err(1, "write failed");
			return 0;
			}
			else
			written += w;
			}
	return written;

}

int write_to_log(char* client_addr, int response_code, int clientsd, 
	char* request_line, int l200written, int l200total) {
/* Prints the log output to stdout.
 * FORMAT:
 * date \t ClientIP addr \t request_line \t response code
 *
 * The request is sucessful only if the file is sucessfully written 
 * IN ITS ENTIRETY to the socket
 *
 */

//if code != 200, ignore the 200_written, 200_total_bytes flags

	char logfile = "/cshome/smartynk/379/Asg2/server_f/log.txt";

	char msg200[] = "200 OK";
	char msg400[] = "400 Bad Request";
	char msg403[] = "403 Forbidden";
	char msg404[] = "404 Not Found";
	char msg500[] = "500 Internal Server Error";

	char httpmessagebuf[64] = {'\0'};
	char* httpmsg = &(httpmessagebuf[0]);
	switch (response_code) {
		case 200:
			strcpy(httpmsg, msg200);
			break;
		case 400: 
			strcpy(httpmsg, msg400);
			break;
		case 403:
			strcpy(httpmsg, msg403);
			break;
		case 404:
			strcpy(httpmsg, msg404);
			break;
		case 500:
			strcpy(httpmsg, msg500);
			break;
	}


	/*Build the time string */
	/* Get the GMT time */
	time_t local_time = time(NULL);
	struct tm tmGMT = {0};
	gmtime_r(&local_time, &tmGMT);
	/* Prepare time string */
	char timestr[320] = {'\0'};
	char* tstr = &(timestr[0]);

	int lentime = strftime(tstr, 320, "%c GMT", &tmGMT);
	//printf("Lentime %d", lentime);

	char loglinearr[16384] = {'\0'};
	char* logline = &(loglinearr[0]);

	int append200 = sprintf("%s\t%s\t%s\t%s", tstr,  client_addr,  request_line,  httpmsg);

	if (response_code == 200){
		sprintf(logline+append200," %d/%d \n", l200written, l200total);
	} else {
		strcat(logline, "\n");
	}
	
	/* Need to get the log file in as a parameter,
	 * open it, then write to it 
	 */
	printf("Logline %s", logline);
	printf("Logfile path %s", logfile);
	//return 0;

	if (access(logfile, F_OK) < 0) {
		/* Create logfile */
		creat(logfile, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	}

	/* File exists but cannot be written or read from */
	if (access(logfile, R_OK|W_OK) < 0 && (access(logfile, F_OK) == 0)){
		printf("Error accessing logfile");
		err(1, "Logfile cannot be written to");
		return 0;
	}



	int fdlog = open(logfile, O_APPEND);
	printf("Fdlog %d:", fdlog);
	if (fdlog < 0) {
		printf("Error opening logfile");
		err(1, "Error opening logfile");
		return 0;
	}
	

	int writecode = write_to_sd(logline, 300, fdlog); 
	if (writecode <= 0){
		printf("Error in write_to_sd while logging");
		err(1, "Error in write_to_sd while logging");
		return 0;
	}
	return writecode; /* Return number of bytes written to logfile */
}

int handle_request(char* clientaddr, char* request_str, int request_len,
			int clientsd, char* logfile, char* docroot) {

	strcpy(&(document_root[0]), docroot);
	printf("handle_request *logfile: %s", logfile);

	/* Main request handler.
	 * clientsd is the socket descriptor for the client. Modelled after
	 * server.c (line 138)
	 * 
	 * Parses input, gets file, generates log output, writes response
	 * to socket
	 */
	int response_code = 200; /* 200 for now. Will change if errors */

	/* Safe to assume that the maximum requested filename length is 
	 * 8192 bytes */
	char requested_filename[MAX_REQ_FILENAME + 16384] = {'\0'};
	int filename_length = request_valid(request_str, request_len,
						&(requested_filename[0]) );



	/* Store just the first line of request_str for logging */
	char GET_line[MAX_REQ_PATH + 30] = {'\0'};
	strncpy(&(GET_line[0]), request_str, MAX_REQ_PATH + 30);
	/* Get_line has the entire request atm. Now take the first \n 
	 * and turn it into a \0. Should be good for logging.
 	 */
	char* firstnl = strchr(&(GET_line[0]), '\n');
	*(firstnl+1) = '\0';

	//printf("GET_line: %s", &(GET_line[0]));
	//int i = 0;
	//while (request_str[i] != '\n') {
	//	GET_line[i] = request_str[i];
	//	i++;
	//}


	if (filename_length == 0) {
		response_code = 400; /* Bad request */
		char response[LEN_ERR_RESPONSE] = {'\0'};
		char* r = &(response[0]);
		int len = 0;
		resp_error(response_code, r);
		write_to_sd(r, len, clientsd);
		logout(clientaddr, &(GET_line[0]), logfile, clientsd, 
			response_code, 0,0);

		return response_code;
	}

	/* Create absolute file path */
	char absfilepath[MAX_REQ_PATH] = {'\0'};
	char* absfp = &(absfilepath[0]);
	strcat(absfp, &(document_root[0]));
	strncat(absfp, &(requested_filename[0]), MAX_REQ_FILENAME);

	if (0){
		printf("Requested filename %s \n", &(requested_filename[0]));
		printf("Absolute file path: %s \n\n", absfp);
	}
	/* Check if requested file exists */
	if ( access(absfp, F_OK) < 0 ){
		/* File requested does not exist */
		response_code = 404;
		char response[LEN_ERR_RESPONSE] = {'\0'};
		char* r = &(response[0]);
		int len = resp_error(404, response);
		write_to_sd(r, len, clientsd);
		logout(clientaddr, &(GET_line[0]), logfile, clientsd, 
			response_code, 0,0);
		return response_code;
	}
	/* Check if requested file can be read */
	if ( access(absfp, R_OK) < 0 ) {
		/* File cannot be read */
		response_code = 403;
		char response[LEN_ERR_RESPONSE] = {'\0'};
		char* r = &(response[0]);
		int len = resp_error(404, r);
		write_to_sd(r, len, clientsd);

		logout(clientaddr, &(GET_line[0]), logfile, clientsd, 
			response_code, 0,0);
		return response_code;
	}

	/* Open a file descriptor for the requested file */
	if ( access(absfp, R_OK|F_OK) == 0 ) {
	}

	int srvfilefd = open(absfp, O_RDONLY);
	if (srvfilefd < 0) {
		/* There was a problem opening the file */
		/* Throw an error 500 */
		response_code = 500;
		char response[LEN_ERR_RESPONSE] = {'\0'};
		char* r = &(response[0]);
		int len = resp_error(500, r);
		write_to_sd(r, len, clientsd);

		logout(clientaddr,&(GET_line[0]), logfile, clientsd, 
			response_code, 0,0);
		return response_code;
	}

	/* Get the file size */
	struct stat fileinfo = {0};
	if (fstat(srvfilefd, &fileinfo) < 0) {err(1, "Error fstat file info");}
	off_t filesize = fileinfo.st_size;


	/* Assume file is a pile of chars */
	char* filebuf = (char*) calloc((size_t)filesize, sizeof(char));
	char* readcursor = filebuf; /* Copy pointer incase read modifies it */

	/* Read in the file */
	int n;
	while ( (n = read(srvfilefd, readcursor, filesize)) > 0) {
		if (n < 0) { 
			err(1, "File Read Error in request_handler");
		}
	}

	/* Close the file */
	if (close(srvfilefd) < 0) {
		/* Error closing the file */
		err(1, "Error closing the file");
	}


	/* Build the HTTP 200 response */
	char* responsebuf  = (char *) calloc((int)filesize + 300, sizeof(char));
	build_200response(responsebuf, (int)filesize, filebuf);

	/* Write through to socket descriptor */
	/* write_to_sd(buffer, len, clientsd); */
	int written = write_to_sd(responsebuf, filesize+300 , clientsd);

	if (response_code == 200) {
		logout(clientaddr, &(GET_line[0]), logfile, clientsd, 
			response_code, written, (int)strlen(responsebuf));

	} else {
		logout(clientaddr, &(GET_line[0]), logfile, clientsd, 
			response_code, 0, 0);
	}

	return response_code;

}
