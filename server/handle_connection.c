#include "get_socket.h"
#include "handle_connection.h"
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
			char* header = "HTTP/1.1 404 File Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
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
		memset(&body, 0, sizeof(body));
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


	 @param con_attrs: a server_thread_attr struct containing the socket, and an running flag
	 @return: 0 on success, -1 on error
 */
void* handle_connection(void* con_attrs) {
	struct server_thread_attr* just_connected = (struct server_thread_attr*)(con_attrs);
	int con_sock = just_connected->socket;

	char* msg = malloc(MSG_MAX_LEN * 8);
	char buffer[MSG_MAX_LEN];
	int msg_size = MSG_MAX_LEN * 8;
	msg[0] = '\0';
	int chars_read;
	int len = 0;
	//Here we get the request
	//All requests are \r\n\r\n terminated, when we get that, we are done
	chars_read = recv(con_sock, buffer, MSG_MAX_LEN-1, 0);
	while(chars_read > 0) {
		//what recv puts in the buffer is not null terminated
		// we add the null byte so we can use strcat
		// TODO: replace strcat with strncat or memcpy
		buffer[chars_read] = '\0';
		len += chars_read;
			//If the space we made for the message isn't big enough, double it
		if(len >= msg_size) {
			msg_size*=2;
			msg = realloc(msg, msg_size);
		}
		strcat(msg, buffer);

		//We are in the middle of a message
		if(strstr(msg, "\r\n\r\n") == NULL) {
			buffer[chars_read] = '\0';
			len += chars_read;

		} else {
			//Got the end of a message, need to do a few things
			//1) send a reply
			//2) flush the msg buffer
			send_reply(con_sock, msg);
			len = 0;
			memset(msg, 0, msg_size);
		}
			chars_read = recv(con_sock, buffer, MSG_MAX_LEN-1, 0);
	}
	// Client closed connection or there was an error
	close(con_sock);
	free(msg);
	just_connected->is_running = 0;
	printf("Thread returning\n");
	return NULL;
}


