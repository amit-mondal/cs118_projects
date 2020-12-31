#include "rdt_server.h"

rdt_server::rdt_server(int portno) : seq_num(0), pw_size_pkts(0), pw_size_bytes(0) {
    this->pw_head = new spw_node();
    this->pw_tail = pw_head;
    this->file_index = 0;
    this->file_buf = nullptr;

    // create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
	rdt_const::print_sc_error("could not open socket");
    }

    // zero-out address
    memset((char *)&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // bind to the port
    if (::bind(sockfd, ((struct sockaddr *) &serv_addr), sizeof(serv_addr)) < 0) {
	rdt_const::print_sc_error("could not bind");
    }

    // set up poll data structures
    fds[0].fd = sockfd;
    fds[0].events = POLLIN | POLLPRI;

    // initialize clilen before calling recv for the first time - prevents invalid args error
    // on later sendto call.
    clilen = sizeof(rem_addr);       
}

rdt_server::~rdt_server() {
    if (file_buf) {
	delete file_buf;
    }
    // Delete packet window.
    spw_node* temp;
    spw_node* curr = pw_head;
    while (curr) {
	temp = curr->next;
	delete curr;
	curr = temp;
    }
}

void rdt_server::log_pkt(packet* pkt, bool retransmission) {
    printf("Sending packet %d %d", pkt->seq_num, rdt_const::MAX_WINDOW_SIZE);
    if (pkt->is_syn()) {
	printf(" SYN");
    }
    else if (pkt->is_fin()) {
	printf(" FIN");
    }
    if (retransmission) {
	printf(" Retransmission");
    }
    printf("\n");
}


void rdt_server::add_pkt(packet* pkt) {
    spw_node* node = new spw_node(pkt);
    // update list info
    if (pw_head == pw_tail) {
	// handle edge case when head = tail (win size 0).
	pw_head->next = node;
    }
    pw_tail->next = node;
    pw_tail = node;
    pw_size_pkts++;
    pw_size_bytes += pkt->pkt_len;
}

void rdt_server::add_pkt(uint16_t seq_num, char* data, size_t data_len) {
    // All packets in the server window are non-ACK packets.
    add_pkt(new packet(seq_num, 0, false, false, false, data, data_len));
}

void rdt_server::dump_win() {
    printf("window dump - packets: %zu, bytes: %zu\n", pw_size_pkts, pw_size_bytes);
    printf("seq num: %d\n[", seq_num);
    spw_node* curr = pw_head->next;
    while (curr) {
	printf("(seq: %d, len: %d), ", curr->pkt->seq_num, curr->pkt->pkt_len);
	curr = curr->next;
    }
    printf("]");
    if (pw_head == pw_tail) {
	printf("tail equals head\n");
    }
    else {
	printf("tail seq num: %d\n", pw_tail->pkt->seq_num);
    }
}

void rdt_server::recv_pkt() {
    recvlen = recvfrom(sockfd, buf, rdt_const::MAX_PKT_LEN, 0, (struct sockaddr *) &rem_addr, &clilen);

    if (recvlen < 0) {
	rdt_const::print_sc_error("could not receive client data");
    }
}

void rdt_server::send_pkt(packet* pkt) {
    size_t pkt_len = pkt->make_raw_pkt(buf);
    if (sendto(sockfd, buf, pkt_len, 0, (struct sockaddr*) &rem_addr, clilen) < 0) {
	rdt_const::print_sc_error("could not send data packet");
    }
}

void rdt_server::process_ack(packet* pkt) {

    printf("Receiving packet %d\n", pkt->ack_num);

    // Check if ACK corresponds to packet in window.
    spw_node* curr = pw_head->next;

    // If the ACK num is too small, just return;
    if (!curr ||
	(rdt_const::cmp_seqnum(
			       rdt_const::inc_seqnum(curr->pkt->seq_num, curr->pkt->pkt_len),
			       pkt->ack_num
			       ) > 0)
	) {
	return;
    }
    
    while (curr) {
	if (pkt->ack_num == rdt_const::inc_seqnum(curr->pkt->seq_num, curr->pkt->pkt_len)) {
	    curr->is_acked = true;
	    break;
	}
	curr = curr->next;
    }

    // Now shrink the window.
    curr = pw_head->next;
    while (curr && curr->is_acked) {
	pw_head->next = curr->next;
	pw_size_pkts--;
	pw_size_bytes -= curr->pkt->pkt_len;	
	delete curr;
	curr = pw_head->next;
    }
    if (pw_size_pkts == 0) {
	pw_tail = pw_head;
    }
    
}

// Returns false once end of file is reached.
bool rdt_server::fill_window() {
    
    size_t data_len = 0;
    
    // Fill up window size until there's no more room.
    while (pw_size_bytes < rdt_const::MAX_WINDOW_SIZE && file_index < filesize) {
	
	data_len = std::min((long) packet::PKT_MAX_DATA_SIZE, (long) (filesize - file_index));
	this->add_pkt(seq_num, &file_buf[file_index], data_len);
	file_index += data_len;
	seq_num = rdt_const::inc_seqnum(seq_num, (data_len + packet::PKT_HEADER_SIZE));
    }
    return file_index < filesize;
}

void rdt_server::send_window(std::chrono::steady_clock::time_point* now) {
    spw_node* curr = pw_head->next;

    while (curr) {
	if (!curr->is_acked && (!curr->sent ||
				(now && std::chrono::duration_cast<std::chrono::milliseconds>(*now - curr->start).count() >= rdt_const::RTO_MS))) {
	    
	    
	    this->send_pkt(curr->pkt);
	    this->log_pkt(curr->pkt, curr->sent);
	    curr->sent = true;
	    curr->start = std::chrono::steady_clock::now();
	}
	curr = curr->next;	
    }
}

void rdt_server::connect() {

    enum state { ST_WAITING, ST_SYN_RCVD };

    state curr_state = ST_WAITING;

    // initialize a packet here to prevent wasteful loop vars
    packet pkt;

    // state machine loop;
    while (1) {
	// wait for SYN packet
	if (curr_state == ST_WAITING) {

	    this->recv_pkt();
	    
	    // Fill in packet info.
	    pkt.update(buf, recvlen);

	    if (pkt.is_syn() && !pkt.is_ack()) {
		// once we receive a SYN packet, break out of receive loop.	
		printf("Receiving packet %d SYN\n", pkt.ack_num);	
		curr_state = ST_SYN_RCVD;
	    }

	}
	// send SYNACK and wait for response.
	else if (curr_state == ST_SYN_RCVD) {
	    // make pkt a SYNACK packet
	    pkt.update(0, rdt_const::INIT_SEQ_NUM, true, true, false);

	    this->send_pkt(&pkt);
	    this->log_pkt(&pkt);
	    
	    // now that we've sent a SYNACK, wait for response with poll
	    if (poll(fds, 1, rdt_const::RTO_MS) && (fds[0].revents & POLLIN)) {
		
		this->recv_pkt();
		// Now check for an ack packet
		pkt.update(buf, recvlen);	      
	       
		// Check for ACK bit and correct ack number.
		if (pkt.is_ack() && !pkt.is_syn() &&
		    pkt.ack_num == rdt_const::INIT_SEQ_NUM + packet::PKT_HEADER_SIZE) {
		    printf("Receiving packet %d\n", pkt.ack_num);
		    this->seq_num = pkt.ack_num;
		    this->filename = std::string(pkt.data);
		    // If the packet is an ACK, we can leave the loop.
		    break;
		}
		// Otherwise, we keep resending the SYNACK.
	    }
	}
    }
}

void rdt_server::send_file() {
    
    std::chrono::steady_clock::time_point now;

    filesize = rdt_const::get_filesize(filename);

    // First, do the legwork of opening the file.
    std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);

    if (filesize > 0 && file.is_open()) {
	// Allocate appropraity sized buffer.
	file_buf = new char[filesize];
	file.seekg(0, std::ios::beg);
	// Read file and then close.
	file.read(file_buf, filesize);
	file.close();
    }
    else {
	fprintf(stderr, "Error: could not find file %s", filename.c_str());
	exit(1);
    }
    
    this->fill_window();

    // Send the packets in the window.
    this->send_window();

    packet ack_pkt;

    // Initially, the poll waits for just 10 ms so we can check for packet timeout.
    int timeout = rdt_const::RTO_GRAN;

    while (1) {
	if (poll(fds, 1, timeout) && (fds[0].revents & POLLIN)) {	    
	    this->recv_pkt();
	    ack_pkt.update(buf, recvlen);
	    if (!ack_pkt.is_syn() && ack_pkt.is_ack()) {
		this->process_ack(&ack_pkt);
	    }
	    if (!this->fill_window() && this->pw_size_bytes == 0) {
		break;
	    }
	}
	// Mark current time to check for timeout.
	now = std::chrono::steady_clock::now();
	this->send_window(&now);
    }

    // Finally, close the connection.
    this->close_conn();
}

void rdt_server::close_conn() {   
    
    // Make FIN packet.
    packet fin_pkt(0, 0, false, false, true);

    // Make receiver packet.
    packet pkt;

    int timeout = rdt_const::RTO_MS;

    // Now we wait for a FINACK.
    for (int i = 0; i < rdt_const::FINACK_RETRIES; i++) {
	this->send_pkt(&fin_pkt);
	this->log_pkt(&fin_pkt);
	if (poll(fds, 1, timeout) && (fds[0].revents & POLLIN)) {
	    this->recv_pkt();
	    pkt.update(buf, recvlen);
	    // If it's a FINACK packet...
	    if (pkt.is_fin() && pkt.is_ack() && !pkt.is_syn()) {
		printf("Receiving packet %d FINACK\n", pkt.ack_num);
		break;
	    }
	}
    }   
}
