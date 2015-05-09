#include <stdio.h>
#include "../request_handler.c"


/* Tests the request_handler */
#include <time.h>
#include <stdlib.h>


int main() {
//	test_request_valid();
//	time_test();
//	test_resp_error(500, 0);
//	test_build200_response();
//	test_handle_request();
//	test_write_log();

int l = logout("bob", "log.txt", 0, 200, 20, 20);

}

int test_write_log(){

//write_to_log(int resp_code, int clientsd, char* request_line,
//		int l200written, int l200total, char* logfile);

//	char  req_line[] = "GET /test.html HTTP/1.1";
	char req_line[] = "";
//	char  logline[] = "/cshome/smartynk/379/Asg2/server_f/log.txt";
	char logline[] = "";
	int result = write_to_log(403, 0, &(req_line[0]), 0, 0);//, &(logline[0]));
	printf("result %d", result);

}


int test_handle_request() {

	char inputstr[2000] = {'\0'};
	gets(&(inputstr[0]));

//	handle_request( &(inputstr[0]), 2000, 0);
}

int test_build200_response(){

	char* response = (char *) malloc(20);
	int filesize = 500;

	int ret = build_200response(response, filesize, 0);
	printf("ret %d \n\n", ret);
}

int test_resp_error(int eo, int clientsd){

	char* r = (char*) malloc(40000);
	
	resp_error(500, r);

}

int time_test() {
	time_t current_time;
	char* c_time_string;
	current_time = time(NULL);
	c_time_string = ctime(&current_time);
	printf("Current time %s", c_time_string);
}


int test_request_valid() {

	char inputstr[2000] = {'\0'};
	gets(&(inputstr[0]));

	char req_filename[2000] = {'\0'};

	int valid = request_valid(&(inputstr[0]), 2000, req_filename);

	printf("Valid: %d \n", valid);
	printf("Request filename: %s \n", req_filename);
	

}


