/*
server.c
Written by: Jesse Earisman
Date: 18-9-2014

A basic http client
Given a url and a port, this program will send an http GET request to that
address, and print the webpage it returns to stdout

if the -p flag is given, this program also calculates the round trip time
*/
#include "get_socket.h"
#include <time.h>


//this is the maximum size that will ever be read in from a call to recv
//adjusting it may affect the speed of the program
#define BUFFER_SIZE 0x100

int handle_content_length_encoding(int s, int content_length);
int handle_chunked_encoding(int s);
int get_content_len(char* buffer, int* length);


/*
	 for webpages where the content length is given as a header field, this method will
	 receive the page, and display the contents.

	 @param s: the socket to read from
	 @param content_length: the length of the webpage, obtained from get_content_length()

	 @return: 0 on success, -1 on failure
 */
int handle_content_length_encoding(int s, int content_length) {
	int chars_read;
	char buffer[BUFFER_SIZE];
	int stdout_fd = fileno(stdout);
	while(content_length > 0) {
		int to_read = content_length < BUFFER_SIZE ? content_length:BUFFER_SIZE;
		chars_read = recv(s, buffer, to_read, 0);
		if(chars_read <= 0) {
			printf("error\n");
			return -1;
		}
		content_length -= chars_read;
		write(stdout_fd, buffer, chars_read);
	}
	return 0;
}

/*
	 for webpages that use chunked encoding, this method will parse the encoding
	 print out the important bits

	 @param s: the receiving socket

	 @return: 0 on success, -1 on failure
 */

int handle_chunked_encoding(int s) {
	int chunk_size;
	//this is the buffer that holds the chunk header and trailing /r/n, when it is full,
	//we will use sscanf to read in the number, then get that many bytes of the page
	char length_bytes[10];
	int chars_read;
	char byte_buffer;
	int index;
	int stdout_fd = fileno(stdout);
	while(1) {
		memset(length_bytes, 0, 10);
		index = 0;
		//The \r\n we are looking for is the one after the hexadecimal number that represents chunk length
		while(strstr(length_bytes, "\r\n") == NULL) {
			chars_read = recv(s, &byte_buffer, 1, 0);
			length_bytes[index++] = byte_buffer;
		}
		length_bytes[index] = '\0';
		//If it's not a hex number, something is wrong, exit
		if(sscanf(length_bytes, "%x", &chunk_size) == 0) {
			printf("Chunked webpage was malformed\n");
			return -1;
		}
		//if the chunk size is 0, we're done. Return a success code
		if(chunk_size == 0) {
			return 0;
		}

		//Read in the whole chunk, with a max read of at most BUFFER_SIZE
		char buffer[BUFFER_SIZE];
		while(chunk_size > 0) {
			int to_read = chunk_size > BUFFER_SIZE ? BUFFER_SIZE: chunk_size;
			chars_read = recv(s, buffer, to_read, 0);
			chunk_size -= chars_read;
			write(stdout_fd, buffer, chars_read);
		}
		//takes care of trailing \r\n before the next hex value
		recv(s, length_bytes, 2, 0);
	}

	return 0;
}

/*
	 Given a header, determines whether the content length of the http body is given, if the
	 webpage is using chunked encoding, or if neither.

	 If content length is given, it fills the length variable with the length, and returns 1

	 @param buffer: a pointer to the http header string
	 @param length: a pointer to the variable where content length will be stored

	 @return: -1 on failure, 0 on chunked encoding, 1 on content-length
 */
int get_content_len(char* buffer, int* length) {
	char *saveptr, *token;
	for(token = strtok_r(buffer, "\r\n", &saveptr); token; token = strtok_r(NULL, "\r\n", &saveptr)) {
		if(strstr(token, "Transfer-Encoding: chunked")) return 0;
		if(sscanf(token, "Content-Length: %d", length)) return 1;
	}
	return -1;
}

/*
	 Prints a brief message telling people how to call the program
 */
void usage() {
	printf("Usage: client [options] URL PORT\n");
}

/*
	 this is the client for the simple http client/server
	 a lot ended up in main(), so i'll comment as i go
 */
int main(int argc, char* argv[]) {
	int print;
	struct timespec t_start;
	struct timespec t_end;

	//four args means flags must have been given
	if(argc == 4) {
		if(strcmp(argv[1], "-p") == 0) {
			//-p flag, we need to time this run
			//set the variable and start the clock
			print = 1;
			clock_gettime(CLOCK_MONOTONIC, &t_start);
		} else {
			usage();
			return -1;
		}
	} else if (argc == 3) {
		print = 0;
	} else {
		usage();
		return -1;
	}
	//Retrieve hostname and what file we want, if nothing is present after the hostname, assume we wanted /
	char* host = argv[argc-2];
	char* file = strchr(host, '/');
	if(file) {
		*file = '\0';
		++file;
	} else {
		file = "";
	}

	//put the entire request in one string, and store it for later
	char input[100 + strlen(host) + strlen(file)];
	input[0] = '\0';
	sprintf(input, "GET /%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: curl/1.0\r\nConnection: close\r\n\r\n", file, host);
	char* port = argv[argc-1];

	int s;
	if(get_socket(host, port, &s, 0) != 0) {
		return -1;
	}

	//send the request we made earlier, it should resemple GET /[filename] HTTP/1.1
	// with some extra header options like hostname and a basic user agent
	send(s, input, strlen(input), 0);

	//Prepare for the server to give us a webpage
	char* header  = malloc(BUFFER_SIZE);
	char byte_buffer;
	memset(header, 0, BUFFER_SIZE);
	int header_len = BUFFER_SIZE;
	int index = 0;
	header[0] = '\0';
	int content_length;
	int chars_read;
	//We get the header one character at a time, until we find the \r\n\r\n
	// if we get more than one, we risk receiving some of the body, which makes our lives 
	// harder later on
	while(strstr(header, "\r\n\r\n") == NULL) {
		//If the size we have alloc'ed for header is not big enough, double it
		if (index+1 >= header_len) {
			header_len *= 2;
			header = realloc(header, header_len);
		}
		chars_read = recv(s, &byte_buffer, 1, 0);
		header[index++] = byte_buffer;
		if(chars_read == 0) {
			printf("Server closed connection\n");
			free(header);
			return -1;
		}
		if(chars_read == -1) {
			printf("Socket error\n");
			free(header);
			return -1;
		}
	}

	//analyze that header, for more info, see get_content_len()
	int header_analysis = get_content_len(header, &content_length);
	free(header);
	if(header_analysis == -1) {
		printf("Content length not given\nChunked encoding is not being used, exiting\n");
	} else if (header_analysis == 0) {
		handle_chunked_encoding(s);
	} else {
		handle_content_length_encoding(s, content_length);
	}
	//Should be all done, close the socket and go home
	close(s);

	//If the -p flag was given at the start, get the current time, and subtract it from the start time
	if(print) {
		clock_gettime(CLOCK_MONOTONIC, &t_end);
		printf("\nRTT: %ld microseconds\n", 1000000*(t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec)/10000);

	}
	return 0;
}
