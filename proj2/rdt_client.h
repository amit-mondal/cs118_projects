
#ifndef RDT_CLIENT_H
#define RDT_CLIENT_H

#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>  /* signal name macros, and the kill() prototype */
#include <thread>
#include <poll.h>
#include "rdt_constants.h"
#include "packet.h"

// Client packet window node.
struct cpw_node {
    packet* pkt;
    cpw_node* next;
    bool is_acked;
    
    cpw_node(packet* pkt = nullptr, cpw_node* next = nullptr) {
	this->pkt = pkt;
	this->next = next;
    }

    ~cpw_node() {
	delete pkt;
    }
};

class rdt_client {
 public:
    rdt_client(char* hostname, int portno, char* filename);
    ~rdt_client();
    void request_file(std::string out_filename);
 private:
    char buf[rdt_const::MAX_PKT_LEN];
    int sockfd;
    char *filename;
    struct hostent* server;
    struct sockaddr_in serv_addr;
    socklen_t serv_addr_len;
    int recvlen;

    struct pollfd fds[1];
    
    std::ofstream out_file;    

    void send_pkt(packet* pkt);
    void recv_pkt();

    // Members for dealing with packet window.

    enum pkt_status { PKT_RCVD, PKT_BUFFERED, PKT_SYNACK, PKT_DUP, PKT_ERR, PKT_FIN };
    uint16_t expected_seqnum;
    size_t pw_size_pkts;
    size_t pw_size_bytes;
    cpw_node* pw_head;
    cpw_node* pw_tail;
    pkt_status add_packet(char* raw_pkt, size_t len);
    void send_ack(packet* pkt);
    void append_data(packet* pkt);
    void dump_win();
    void close_conn();
    void log_pkt(packet* pkt);
};

#endif
