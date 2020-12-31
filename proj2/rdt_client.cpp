#include "rdt_client.h"

rdt_client::rdt_client(char* hostname, int portno, char* filename) :
    filename(filename) {

    pw_size_pkts = 0;
    pw_size_bytes = 0;
    // No needed to use inc_seqnum for this case
    expected_seqnum = rdt_const::INIT_SEQ_NUM + packet::PKT_HEADER_SIZE;
    pw_head = new cpw_node();
    pw_tail = pw_head;

    // Set up all the necessary socket info here.
    
    // Create socket.
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
	rdt_const::print_sc_error("could not open socket");
    }

    // Fill in the server's address and data.
    memset((char*)&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);

    // Look up the address of the server given its name.
    server = gethostbyname(hostname);
    if (!server) {
	rdt_const::print_sc_error("could not obtain host address");
    }

    // Put the host's address into the server address structure.
    memcpy((void *) &serv_addr.sin_addr, server->h_addr_list[0], server->h_length);

    serv_addr_len = sizeof(serv_addr);

    // Set up poll data structures.
    fds[0].fd = sockfd;
    fds[0].events = POLLIN | POLLPRI;    
}

rdt_client::~rdt_client() {
    // Delete packet window.
    cpw_node* temp;
    cpw_node* curr = pw_head;
    while (curr) {
	temp = curr->next;
	delete curr;
	curr = temp;
    }
}

void rdt_client::log_pkt(packet* pkt) {
    printf("Receiving packet %d", pkt->seq_num);
    if (pkt->is_syn()) {
	printf(" SYN");
    }
    else if (pkt->is_fin()) {
	printf(" FIN");
    }
    if (pkt->is_ack()) {
	printf("ACK");
    }
    printf("\n");
}

void rdt_client::recv_pkt() {
    recvlen = recvfrom(sockfd, buf, rdt_const::MAX_PKT_LEN, 0, (struct sockaddr*) &serv_addr, &serv_addr_len);

    if (recvlen < 0) {
	rdt_const::print_sc_error("could not receive client data");
    }    
}

rdt_client::pkt_status rdt_client::add_packet(char* raw_pkt, size_t len) {

    cpw_node* curr = pw_head;
    packet* pkt = new packet(raw_pkt, len);
    this->log_pkt(pkt);

    if (pkt->is_syn() && pkt->is_ack()) {
	delete pkt;
	return PKT_SYNACK;
    }

    if (pkt->is_ack() || pkt->is_syn()) {
	delete pkt;
	return PKT_ERR;
    }

    if (pkt->is_fin() && !pkt->is_ack() && !pkt->is_syn()) {
	delete pkt;
	return PKT_FIN;
    }

    // We always ACK received packets.
    this->send_ack(pkt);

    if (rdt_const::cmp_seqnum(pkt->seq_num, expected_seqnum) < 0) {
	printf(" Retransmission\n");
	delete pkt;
	return PKT_DUP;
    }

    // If the packet is the expected one, start sending ACKs and shrinking the window.
    if (pkt->seq_num == expected_seqnum) {
	expected_seqnum = rdt_const::inc_seqnum(expected_seqnum, pkt->pkt_len);
	this->append_data(pkt);
	delete pkt;

	curr =  pw_head->next;
	while (curr && curr->pkt->seq_num == expected_seqnum) {
	    expected_seqnum = rdt_const::inc_seqnum(expected_seqnum, curr->pkt->pkt_len);
	    //curr = curr->next;
	    //this->send_ack(curr->pkt);
	    pw_head->next = curr->next;
	    this->append_data(curr->pkt);
	    delete curr;
	    curr = pw_head->next;
	    pw_size_pkts--;
	}
	if (pw_size_pkts == 0) {
	    pw_tail = pw_head;
	}
	printf("\n");	
	return PKT_RCVD;
    }

    // Otherwise place it in appropriate place in the window.
    while (curr->next && rdt_const::cmp_seqnum(curr->next->pkt->seq_num, pkt->seq_num) <= 0) {
	if (curr->next->pkt->seq_num == pkt->seq_num) {
	    delete pkt;
	    printf(" Retransmission\n");	    
	    return PKT_DUP;
	}
	curr = curr->next;
    }

    cpw_node* temp = curr->next;
    curr->next = new cpw_node(pkt);
    if (curr == pw_tail) {
	pw_tail = curr->next;
    }
    curr->next->next = temp;
    pw_size_pkts++;
    printf("\n");
    return PKT_BUFFERED;
}

void rdt_client::append_data(packet* pkt) {
    if (out_file.is_open()) {
	out_file.write(pkt->data, pkt->data_len);
    }
    else {
	rdt_const::print_sc_error("File must be opened before writing");
	exit(1);
    }
}

void rdt_client::dump_win() {
    printf("window dump - packets: %zu, bytes: %zu\n", pw_size_pkts, pw_size_bytes);
    printf("expected seq num: %d\n[", expected_seqnum);
    cpw_node* curr = pw_head->next;
    while (curr) {
	printf("(seq: %d, len: %d), ", curr->pkt->seq_num, curr->pkt->pkt_len);
	curr = curr->next;
    }
    printf("]\n");
}

void rdt_client::send_pkt(packet* pkt) {
    size_t pkt_len = pkt->make_raw_pkt(buf);
    if (sendto(sockfd, buf, pkt_len, 0, (struct sockaddr*) &serv_addr, serv_addr_len) < 0) {
	rdt_const::print_sc_error("could not send data packet");
    }
}

void rdt_client::send_ack(packet* pkt) {
    packet ack_pkt(0, rdt_const::inc_seqnum(pkt->seq_num, pkt->pkt_len), true, false, false);    
    this->send_pkt(&ack_pkt);
    printf("Sending packet %d", ack_pkt.ack_num);
}


void rdt_client::request_file(std::string out_filename) {

    enum state { ST_WAITING, ST_SYNACK_RCVD, ST_ACK_PKTS, ST_FIN_RCVD };

    state curr_state = ST_WAITING;

    // Set up output stream -> make sure to open with trunc flag.
    out_file.open(out_filename, std::ios::out | std::ios::trunc | std::ios::binary);

    // Create syn packet.
    packet syn_pkt(0, 0, false, true, false);

    // Now get ready to receive SYNACK.
    packet pkt;    

    // State machine loop.
    while (1) {
	// Client sends syn packet and then waits for SYNACK packet.
	if (curr_state == ST_WAITING) {
	    // (Re)send the SYN packet.
	    this->send_pkt(&syn_pkt);
	    printf("Sending packet SYN\n");
	    // Now wait for SYNACK.
	    if (poll(fds, 1, rdt_const::RTO_MS) && (fds[0].revents & POLLIN)) {
		this->recv_pkt();
		pkt.update(buf, recvlen);
		this->log_pkt(&pkt);

		if (pkt.is_syn() && pkt.is_ack()) {
		    curr_state = ST_SYNACK_RCVD;
		}
	    }
	}
	else if (curr_state == ST_SYNACK_RCVD) {
	    pkt.update(0, rdt_const::INIT_SEQ_NUM + recvlen, true, false, false, filename, strlen(filename) + 1);
	    this->send_pkt(&pkt);
	    printf("Sending packet %d SYNACK\n", pkt.ack_num);
	    curr_state = ST_ACK_PKTS;

	}
	else if (curr_state == ST_ACK_PKTS) {
	    this->recv_pkt();

	    switch(this->add_packet(buf, recvlen)) {
	    case PKT_SYNACK:
		curr_state = ST_SYNACK_RCVD;
		break;
	    case PKT_ERR:
		fprintf(stderr, "Error: invalid packet received");
		pkt.update(buf, recvlen);
		pkt.dump();
		exit(1);
		break;
	    case PKT_FIN:
		curr_state = ST_FIN_RCVD;
		break;
	    default:
		break;
	    }

	}
	if (curr_state == ST_FIN_RCVD) {
	    break;
	}
    }
    this->close_conn();
}


void rdt_client::close_conn() {
    // Make FINACK packet
    packet finack_pkt(0, 0, true, false, true);
    packet pkt;

    for (int i = 0; i < rdt_const::FINACK_RETRIES; i++) {
	this->send_pkt(&finack_pkt);
	printf("Sending packet FINACK\n");
	// Poll and then do it again.
	if (poll(fds, 1, rdt_const::RTO_MS) && (fds[0].revents & POLLIN)) {
	    this->recv_pkt();
	    pkt.update(buf, recvlen);
	    this->log_pkt(&pkt);
	    // Check for FIN packet.
	    if (pkt.is_fin() && !pkt.is_ack()) {
		printf("Receiving FIN\n");
		break;
	    }
	}
    }    
}
