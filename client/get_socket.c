/*
common.c
Written by: Jesse Earisman
Date: 18-9-2014

Methods common to client.c and server.c
right now, it is just the get_socket() function, but any future functions that are used
by both client.c and server.c should go in here
*/

#include "common.h"

/*
	 Given a host and a destination port, gets a socket

	 This function uses getaddrinfo to find an appropriate socket
	 On each result getaddrinfo returns, it creates a socket with the results, and checks
	 to see if that is a valid socket.
	 If it is a valid socket, it tries to connect.
	 If either of these steps fails, it keeps trying until it has exhausted the list.
	 If no results are good, it returns -1
	 Otherwise, retruns 0

	 I found I was duplicating a lot of code between server and client, so everything is
	 now part of get_socket()
	 servers call this function with is_server equal to 1
	 clients call it with is_server == 0

	 @param host: the hostname e.g. "www.cnn.com" or NULL
	 @param port: the port we want to connect on e.g. "80"
	 @param s: where we put the socket we successfully connect to
	 @param is_server: determines whether we use bind or connect

	 @return: -1 on failure, 0 on success
 */
int get_socket(char* host, char* port, int* s, int is_server) {
	struct addrinfo hints, *res, *temp;
	memset(&hints, 0, sizeof hints);
	//Servers need to support ipv6, so they use AF_INET6
	//but clients may want to use either, so they use AF_UNSPEC
	if(is_server) {
		hints.ai_family = AF_INET6;
	} else {
		hints.ai_family = AF_UNSPEC;
	}
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;


	if(host == NULL) {
		hints.ai_flags = AI_PASSIVE;
	} else {
		hints.ai_flags = 0;
	}

	int result = getaddrinfo(host, port, &hints, &res);
	if (result != 0) {
		printf("%s\n", gai_strerror(result));
		return result;
	}

	int bind_res;
	int yes = 1;
	for(temp = res; temp; temp = res->ai_next) {
		*s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if(*s == -1) {
			printf("Socket error\n");
			continue;
		}

		//Allows us to reuse ports as they wait for the kernel to clear them
		setsockopt(*s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		//Servers want to bind, clients want to connect
		if(is_server) {
			bind_res = bind(*s, res->ai_addr, res->ai_addrlen);
		} else {
			bind_res = connect(*s, res->ai_addr, res->ai_addrlen);
		}
		if(bind_res != 0) {
			close(*s);
			printf("Could not bind\n");
			continue;
		}
		break;
	}

	//None of the results were good
	if (temp == NULL) {
		printf("could not connect\n");
		freeaddrinfo(res);
		return -1;
	}

	freeaddrinfo(res);
	return 0;
}
