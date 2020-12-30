#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <fstream>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>  /* signal name macros, and the kill() prototype */
#include <poll.h>
#include <chrono>
#include <ctime>
#include "rdt_constants.h"
#include "packet.h"


class rdt_server {
 public:
    rdt_server(int portno);
    ~rdt_server();
    void connect();
    void send_file();
 private:
    int sockfd, portno, recvlen;
    socklen_t clilen;
    struct sockaddr_in serv_addr, rem_addr;
    
    static const int SERVER_BUF_SIZE = rdt_const::MAX_PKT_LEN;
    char buf[rdt_server::SERVER_BUF_SIZE];
    
    char* file_buf;
    size_t filesize;
    size_t file_index;
    
    std::string filename;
    struct pollfd fds[1];

    unsigned int seq_num;
    
    void recv_pkt();
    void send_pkt(packet* pkt);

    // Data structures and functions for maintaining packet window

    // Server packet window node.
    struct spw_node {
	packet* pkt;
	spw_node* next;
	bool sent;
	bool is_acked;
	std::chrono::steady_clock::time_point start;
    
	spw_node(packet* pkt = nullptr, spw_node* next = nullptr) {
	    this->pkt = pkt;
	    this->next = next;
	    sent = false;
	    is_acked = false;
	}

	~spw_node() {
	    delete pkt;
	}    
    };
    
    size_t pw_size_pkts;
    size_t pw_size_bytes;
    spw_node* pw_tail;
    spw_node* pw_head;

    void add_pkt(packet* pkt);
    void add_pkt(uint16_t seq_num, char* data, size_t data_len);
    void process_ack(packet* pkt);
    bool fill_window();
    void send_window(std::chrono::steady_clock::time_point* now = nullptr);
    void close_conn();
    
    void dump_win();
    void log_pkt(packet* pkt, bool retransmission = false);
};
