#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
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

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	print_sc_error("ERROR on binding");

    listen(sockfd, 5);  // 5 simultaneous connection at most

    //accept connections
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd < 0)
	print_sc_error("ERROR on accept");

    int n;
    char buffer[512];

    memset(buffer, 0, 512);  // reset memory

    //read client's message
    n = read(newsockfd, buffer, 511);
    if (n < 0) print_sc_error("ERROR reading from socket");

    printf("%s", buffer);

    string filename = get_file_name(string(buffer));

    send_file(newsockfd, filename);
        
    close(newsockfd);  // close connection
    close(sockfd);

    return 0;    
}