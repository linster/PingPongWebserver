
/* Request Handler header file */

#ifndef REQHANDLER_H
#define REQHANDLER_H


//#include <err.h>
//#include <errno.h>
//#include <fcntl.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <string.h>
//#include <time.h>
//#include <stdlib.h>


/* Document root for the webserver: Must have a trailing slash */
//char document_root[MAX_REQ_PATH] = "/cshome/smartynk/379/Asg2/docroot/";

//#define MAX_REQ_PATH            8192 /* Max number of characters in path */

//char document_root[MAX_REQ_PATH] = {'\0'};




/* Function prototypes */
int handle_request(char* request_str, int request_len, int clientsd, char*, char*, char*, char*);
int request_valid(char* request_str, int request_len, char* requested_filename);
int resp_error(int errno, char *r);
int build_200response(char* response, int filesize, char* filepointer);
int write_to_sd(char*, char* buffer, unsigned int len, int clientsd);

//int write_to_log(char*, int, int, char*, int, int);

#endif /* REQHANDLER_H */

