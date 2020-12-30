#include "rdt_client.h"

int main(int argc, char** argv) {

    if (argc < 4) {
	fprintf(stderr, "ERROR: must invoke with ./client <hostname> <port> <filename>");
	exit(1);
    }
    
    rdt_client client(argv[1], atoi(argv[2]), argv[3]);
    client.request_file("received.data");
}
