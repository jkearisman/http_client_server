/*
	 client.c
	 Written by: Jesse Earisman
Date: 18-9-2014

A basic http server
Given a port, this program will set up an http server on that port. It supports 
HTTP GET requests.
 */

#include "get_socket.h"
#include "handle_connection.h"
#include <signal.h>
#include <pthread.h>

#define MAX_CLIENTS 20



//Global root socket, when we catch a SIGINT, we can close this, and the main loop will end
int root_socket;
pthread_t threads[MAX_CLIENTS];
struct server_thread_attr thread_attrs[MAX_CLIENTS];
int was_active[MAX_CLIENTS];


void sighandler(int signum);

/*
	 When the program receives a sigint, this function is called, it closes the root
	 socket, and the program shuts down gracefully
 */
void sighandler(int signum) {
	if(signum == SIGINT) {
		printf("Caught a SIGINT\n");
	} else {
		printf("Caught a non-SIGINT signal\n");
	}
	printf("Closing root socket\n");
	close(root_socket);
}
/*
	 this is the main function for the HTTP server.

 */
int main(int argc, char* argv[]) {
	if(argc != 2) {
		printf("Usage: server PORT\n");
		return -1;
	}
	memset(&thread_attrs, 0, sizeof(thread_attrs));
	memset(&was_active, 0, sizeof(was_active));

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

	//Basically, it binds the root_socket to ::1
	int result = get_socket(NULL, argv[1], &root_socket);
	if(result != 0) {
		return -1;
	}

	int listen_res = listen(root_socket, BACKLOG);
	if(listen_res == -1) {
		printf("Listening failed\n");
		return -1;
	}

	int con_sock = accept(root_socket, NULL, NULL);
	while(con_sock != -1) {
		int i;
		int no_open_threads = 1;
		for(i = 0; i < MAX_CLIENTS; i++) {
			if(!thread_attrs[i].is_running) {
				printf("Opening thread %d\n", i);
				if(was_active[i]) {
					pthread_join(threads[i], NULL);
				} else {
					was_active[i] = 1;
				}
				thread_attrs[i].is_running = 1;
				thread_attrs[i].socket = con_sock;
				pthread_create(&threads[i], NULL, handle_connection, (void*)(&thread_attrs[i]));
				no_open_threads = 0;
				break;
			}
		}
		if(no_open_threads) {
			close(con_sock);
			printf("No open threads were found, a client was dumped. Sorry.\n");
		}
		con_sock = accept(root_socket, NULL, NULL);
	}
	printf("Closing all threads\n");
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(was_active[i]) {
			printf("\tClosing thread %d\n", i);
			pthread_join(threads[i], NULL);
		}

	}
	free(act);
	return 0;
}
