#include "crdt_server.h"

int main(int argc, char** argv) {
    
    // check args
    if (argc < 2) {
	fprintf(stderr, "ERROR: no port provided\n");
	exit(1);
    }

    crdt_server server(atoi(argv[1]));
    server.connect();
    server.send_file();
}
