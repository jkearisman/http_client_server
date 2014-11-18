Simple HTTP client/server

Jesse Earisman

18-9-2014

To Build Client:
	cd client && make

To Build Server:
	cd server && make

executables are created in the same directory as makefiles

### Server ###
To run server:
server PORT

	Server will start up on the given port number, or fail if it cannot bind with that port.
	Unless you are running with elevated privileges, all ports under 1024 should be off limits
	Recommended port is 8080 for basic users, the backup http port, although the server can theoretically 
		run anything up to the maximum port number of 65536

	In the server.c file, there is a constant WEBROOT. This is the root of your web server.
	When someone requests /file, the server looks in WEBROOT/file
	By default, the webroot is "./srv", the srv directory next to the executable
		this can be modified and recompiled with any value desired, ex /var/www
	It is not possible to obtain anything higher up in the directory tree than
		the WEBROOT, but anything in WEBROOT or its subdirectories can be gotten by
		sending a request to the server.
	There is another constant called MAX_MSG_LEN. This is the most data that the server will
		ever put on the wire in one send call. This value also has a limit at the os level, 
		the program will work fine even if the OS limit is lower than the MAX_MSG_LEN
	The server is run in an infinite loop, to close it, send it a SIGINT with ctrl-c, it
		will shutdown gracefully
	The Server uses the pthread library to enable multiple simultaneous threads. It maintains a number of
		threads up to the defined MAX_CLIENT_NUM (default 20)

### Client ###
To run client:
client [options] URL PORT

	If the url is given in the form "example.coom/aoeu.html", the client
		will request "aoeu.html" from example.com
	If the url is simply given in the form "example.com", the client will request "/"
		this should do for most websites, as they will automatically direct you to something
		like index.html

	The client can get normal html pages, images, or anything else. By default, it
		prints everything it gets to stdout, but using unix file direction, it can pipe
		the output to arbitrary files, like so
		$ client website.com/images/picture.png 80 > my_copy.png
		then view the image with your favorite program
		NOTE: the same can be done with regular html files, pipe to file, and display

	There is only one additional option. If the -p flag is given, the client will time
		the request, and print out the round trip time (RTT) after the webpage. This is
		timed using clock_gettime(), and is precise to the nanosecond, although only
		microseconds are printed



### More Notes ###
Both Programs have been verified with valgrind to have no memory leaks
Both programs are relatively secure, but no though audit has been done, if you
	put sensitive data in here, i'm not responsible for what happens
