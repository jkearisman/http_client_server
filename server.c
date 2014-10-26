/*
client.c
Written by: Jesse Earisman
Date: 18-9-2014

A basic http server
Given a port, this program will set up an http server on that port. It supports 
HTTP GET requests.
*/

#include "common.h"
#include <signal.h>

#define BACKLOG 10
#define MSG_MAX_LEN 0x100
#define WEBROOT "./srv"

//This can be further filled with more header data, if necesary
// Example, if we wanted to add a place to put the user agent string, it would be here
struct request {
	char* method;
	char* file;
	char* version;
	//char* user_agent;
};

//Global root socket, when we catch a SIGINT, we can close this, and the main loop will end
int root_socket;


void sighandler(int signum);
int parse_request(char* request, struct request *r);
int send_reply(int con_sock, char* request);
int handle_connection(int con_sock);

/*
	 When the program receives a sigint, this function is called, it closes the root
	 socket, and the program shuts down gracefully
 */
void sighandler(int signum) {
	printf("recieved a sigint\n");
	printf("closing server socket...\t");
	close(root_socket);
	printf("done.\n");
	printf("Server shutting down\n");
}

/*
	 Given a http request, this method will fill a request struct with the data contained

	 The method parses the first line to obtain the Method, requested file, and HTTP version
	 if not all of these are present, it returns an error
	 If any of these fields are malformed this method does not report the error, but relies on the calling method

	 @param request: the string representing the http request
	 @param r: an empty struct that will be filled by this method

	 @return: 0 on success, -1 of failure
 */
int parse_request(char* request, struct request *r) {
	char *saveptr_main, *saveptr_top;
	char* line = strtok_r(request, "\r\n", &saveptr_main);
	//parse first line should look like
	// "GET /[file] HTTP/1.1"
	r->method = strtok_r(line, " ", &saveptr_top);
	r->file = strtok_r(NULL, " ", &saveptr_top);
	r->version = strtok_r(NULL, " ", &saveptr_top);
	if(r->method) {
		//For now, we only understand GET, everything else is unparsable
		if(strcmp(r->method, "GET") == 0) {
			if(r->file) {
				if(r->version) {
					line = strtok_r(NULL, "\r\n", &saveptr_main);
					while(line) {
						line = strtok_r(NULL, "\r\n", &saveptr_main);
					}
					return 0;
				}
			}
		}
	}
	//Parsing has failed, return -1 to indicate an error
	return -1;
}

/*
	 given a socket, and an http request string, this method sends an appropriate reply
	 first, it parses the request with parse_request(). If the request is malformed, it
	 returns a 400 Code, if the requested file is not present, it returns 404. If the HTTP
	 version is greater than 1.1, returns 505
	 If everything is OK, it returns a 200, followed by the file requested


	 @param con_sock: the socket
	 @param request
	 @return: -1 on request, 0 on error
 */
int send_reply(int con_sock, char* request) {
	struct request r;
	int content_length;
	double version;
	//A -1 from parse request means fields are missing
	if(parse_request(request, &r) == -1 || sscanf(r.version, "HTTP/%lf", &version) == 0) {
		char* reply = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\nContent-Length: 15\r\n\r\n400 Bad Request";
		send(con_sock, reply, strlen(reply), 0);
	} else {

		if(version > 1.1) {
			char* reply = "HTTP/1.1 505 Version Not Supported\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
			send(con_sock, reply, strlen(reply), 0);
		}
		//Request is ok, lets get the file they asked for
		char file[strlen(WEBROOT) + strlen(r.file) + 10];
		file[0] = '\0';
		sprintf(file, "%s%s", WEBROOT, r.file);
		//If the last character is a /, they probably wanted /index.html
		if(r.file[strlen(r.file)-1] == '/') {
			strcat(file, "index.html");
		}


		int fd = open(file, 0);
		//Could not find the file, or they requested to go up a directory
		// which we don't want them to do, return a 404
		if(fd == -1 || strstr(file, "..")) {
			char* header = "HTTP/1.1 404 File Not Found\r\nConnection: close\r\nContent-Length: 13\r\n\r\n404 Not Found";
			send(con_sock,  header, strlen(header), 0);
			return 0;
		}

		//fstat finds out how big the file is
		struct stat buf;
		fstat(fd, &buf);
		content_length = buf.st_size+1;
		char header[200];
		sprintf(header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", content_length);
		//Send out the header, containing 200 OK, and the content length
		send(con_sock, header, strlen(header), 0);
		char body[content_length];
		//Get and send out the requested file
		read(fd, body, content_length);
		send(con_sock, body, content_length, 0);
		close(fd);
	}
	return 0;
}



/*
	 When a socket connects, this function takes the socket, receives the http request, 
	 and sends an appropriate reply, using the send_reply() method


	 @param con_sock: the new connection socket given by accept()
	 @return: 0 on success, -1 on error
 */
int handle_connection(int con_sock) {
	char* msg = malloc(MSG_MAX_LEN * 8);
	char buffer[MSG_MAX_LEN];
	int msg_size = MSG_MAX_LEN * 8;
	msg[0] = '\0';
	int chars_read;
	int len = 0;
	//Here we get the request
	//All requests are \r\n\r\n terminated, when we get that, we are done
	while (strstr(msg, "\r\n\r\n") == NULL) {
		chars_read = recv(con_sock, buffer, MSG_MAX_LEN-1, 0);

		if(chars_read == 0) {
			printf("Client closed connection\n");
			free(msg);
			return -1;
		}
		if(chars_read < 0) {
			printf("Socket error\n");
			free(msg);
			return -1;
		}
		//what we get from recv() is not null terminated, if we get 10 chars in a 20
		//  char buffer, the last 10 will be garbage, so we set this to signify the end
		//  of the string
		buffer[chars_read] = '\0';
		len += chars_read;

		//If the space we made for the message isn't big enough, double it
		if(len >= msg_size) {
			msg_size*=2;
			msg = realloc(msg, msg_size);
		}

		strcat(msg, buffer);
	}
	//give the header to send_reply, to take the next step
	send_reply(con_sock, msg);
	close(con_sock);
	free(msg);
	return 0;
}


/*
	 this is the main function for the HTTP server.

 */
int main(int argc, char* argv[]) {
	if(argc != 2) {
		printf("Usage: server PORT\n");
		return -1;
	}

	//set up the function to handle ^c
	struct sigaction* act = malloc(sizeof (struct sigaction));
	memset(act, 0, sizeof(*act));
	act->sa_handler = sighandler;
	if(sigaction(SIGINT, act, NULL) == -1) {
		//This should never fail, and if it does, you need someone smarter than me
		// to fix it
		printf("Setting sigaction failed, contact your local wizard\n");
		free(act);
		return -1;
	} 

	//See common.c for the implementation of get_socket
	//Basically, it binds the root_socket to ::1
	int result = get_socket(NULL, argv[1], &root_socket, 1);
	if(result != 0) {
		return -1;
	}

	int listen_res = listen(root_socket, BACKLOG);
	if(listen_res == -1) {
		printf("Listening failed\n");
		return -1;
	}

	for(;;) {
		//Stops here until it gets a connection or the root_socket is closed
		int con_sock = accept(root_socket,  NULL, NULL);
		if(con_sock == -1) {
			break;
		}
		handle_connection(con_sock);
	}
	free(act);
	return 0;
}
