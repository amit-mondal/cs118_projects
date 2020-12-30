#include "rdt_server.h"

int main(int argc, char** argv) {
    
    // check args
    if (argc < 2) {
	fprintf(stderr, "ERROR: no port provided\n");
	exit(1);
    }

    rdt_server server = rdt_server(atoi(argv[1]));
    server.connect();
    server.send_file();
}
