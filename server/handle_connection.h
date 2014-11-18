
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

struct server_thread_attr {
	int socket;
	int is_running;
};

int handle_connection(void* con_attrs);
