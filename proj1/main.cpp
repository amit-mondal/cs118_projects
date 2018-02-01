// Amit Mondal and Michael Cade Mallett
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>  /* signal name macros, and the kill() prototype */
#include "response.h"
#include "request.h"
using namespace std;

// Max bytes for an incoming request.
const int REQUEST_BUFFER_SIZE = 2048;

int main(int argc, char** argv) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
	fprintf(stderr,"ERROR, no port provided\n");
	exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // create socket
    if (sockfd < 0)
	print_sc_error("ERROR opening socket");
    
    memset((char *) &serv_addr, 0, sizeof(serv_addr));   // reset memory

    // fill in address info
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (::bind(sockfd, ((struct sockaddr *) &serv_addr), sizeof(serv_addr)) < 0) {
	print_sc_error("ERROR on binding");
    }

    if (listen(sockfd, 5) < 0) { // Called with a backlog of 5, but only expect a single connection.
	print_sc_error("ERROR on call to listen()");
    }    

    int n;
    // Buffer to read in request data.
    char buffer[REQUEST_BUFFER_SIZE];    
    memset(buffer, 0, REQUEST_BUFFER_SIZE);  // reset memory

    // Read HTTP request in loop condition.
    while (1) {
	// Accept new connections in the loop.
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0) {
	    print_sc_error("ERROR on accept");
	}
	// Read in the HTTP request.
	n = read(newsockfd, buffer, REQUEST_BUFFER_SIZE);
	if (n < 0) print_sc_error("ERROR reading from socket");
	// Write it to STDOUT.
	if (write(STDOUT_FILENO, buffer, n) < 0) {
	    print_sc_error("ERROR writing to stdout");
	}
	// Extract the filename from the request text.
	string filename = get_file_name(string(buffer));
	// Now send the response header and file data.
	send_file(newsockfd, filename);
	close(newsockfd);  // close connection and wait for next request.
    }       
    close(sockfd);

    return 0;    
}
