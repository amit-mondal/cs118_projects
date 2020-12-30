
#include "packet.h"

uint16_t packet::get_checksum() const {
    uint32_t sum, i;
    uint16_t tmp;
    uint8_t high_order, low_order;

    sum = 0;
    
    for (i = 0; i < data_len; i += 2) {
	tmp = 0;
	// The pointer casting is to ensure the bit-pattern does not change.
	high_order = *((uint8_t*) &(data[i]));
	low_order = *((uint8_t*) &(data[i+1]));
	tmp |= high_order;
	tmp <<= 8;
	tmp |= low_order;
	sum += tmp;
    }

    // Check for last 8 bits (in the case of odd data_len)
    if (data_len % 2) {
	sum += *((uint8_t*) &(data[data_len - 1]));
    }
    // Now add the remaining fields.
    sum += (seq_num + ack_num + padding + pkt_len);

    // Keep only the last 16 bits and add the carries.
    while (sum >> 16) {
	sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Take the one's complement of sum
    sum = ~sum;
    return (uint16_t) sum;
}

packet::packet() : seq_num(0), ack_num(0), padding(0), pkt_len(0),
		   checksum(0), data_len(0), data(nullptr) {
}

packet::packet(uint16_t seq_num, uint16_t ack_num, bool ack, bool syn, bool fin,
	       char* data, size_t data_len) : seq_num(seq_num), ack_num(ack_num),
					      data_len(data_len) {
    
    // Set padding bits appropriately.
    padding = 0;
    if (ack) {
	padding |= 1;
    }
    
    if (syn) {
	padding |= 0x0002;
    }
    
    if (fin) {
	padding |= 0x0004;
    }

    // Copy data into packet data.
    if (data_len) {
	this->data = new char[data_len];
	memcpy(this->data, data, data_len);
    }

    // Set packet length.
    pkt_len = packet::PKT_HEADER_SIZE + data_len;

    // Set checksum.
    checksum = get_checksum();
    
}

packet::packet(char* raw_pkt, size_t len) {
    this->update(raw_pkt, len, true);
}

packet::~packet() {
    if (data) {
	delete [] data;
    }
}

void packet::update(char* raw_pkt, size_t len, bool is_new_pkt) { 
    if (len < PKT_HEADER_SIZE) {
	rdt_const::print_sc_error("packet must be at least 10 bytes long");
    }
    // Get sequence number.
    memcpy((void*) &seq_num, (void*) raw_pkt, 2);
    raw_pkt += 2;
    
    // Get ack number.
    memcpy((void*) &ack_num, (void*) raw_pkt, 2);
    raw_pkt += 2;
    
    // Get padding.
    memcpy((void*) &padding, (void*) raw_pkt, 2);
    raw_pkt += 2;
    
    // Get packet length.
    memcpy((void*) &pkt_len, (void*) raw_pkt, 2);
    raw_pkt += 2;

    // Get checksum.
    memcpy((void*) &checksum, (void*) raw_pkt, 2);
    raw_pkt += 2;

    // If packet data already exists, remove it.
    if (!is_new_pkt && data_len > 0) {
	delete [] this->data;
    }
    
    // Copy data into packet data.
    data_len = len - packet::PKT_HEADER_SIZE;
    if (data_len) {
	this->data = new char[data_len];
	memcpy((void*) this->data, (void*) raw_pkt, data_len);
    }
}

void packet::update(uint16_t seq_num, uint16_t ack_num, bool ack, bool syn, bool fin,
		    char* data, size_t data_len) {
    
    // Simply plug in values
    this->seq_num = seq_num;
    this->ack_num = ack_num;
    
    // Set padding bits appropriately.
    padding = 0;
    if (ack) {
	padding |= 1;
    }
    if (syn) {
	padding |= 0x0002;
    }

    // Copy data into packet data.
    if (data_len) {

	// Delete existing data if necessary.	
	if (this->data && this->data_len > 0) {
	    delete [] this->data;
	}
	this->data = new char[data_len];
	memcpy(this->data, data, data_len);
    }

    this->data_len = data_len;

    // Set packet length.
    pkt_len = packet::PKT_HEADER_SIZE + data_len;

    // Set checksum.
    checksum = get_checksum();    
}

size_t packet::make_raw_pkt(char* buf) const {
    // Copy sequence number.
    memcpy((void*) buf, (void*) &seq_num, 2);
    buf += 2;

    // Copy ack number.
    memcpy((void*) buf, (void*) &ack_num, 2);
    buf += 2;

    // Copy padding.
    memcpy((void*) buf, (void*) &padding, 2);
    buf += 2;

    // Copy packet length
    memcpy((void*) buf, (void*) &pkt_len, 2);
    buf += 2;

    // Copy checksum
    memcpy((void*) buf, (void*) &checksum, 2);
    buf += 2;

    // Copy data
    memcpy((void*) buf, (void*) data, data_len);
    
    return data_len + packet::PKT_HEADER_SIZE;
}

void packet::dump() const {
    printf("seq_num: %d, ack_num: %d, padding: %x, pkt_len: %d, checksum: %x\n",
	   seq_num, ack_num, padding, pkt_len, checksum);
    printf("data_len: %d, data: ", data_len);
    printf("%.*s\n", data_len, data);
}

bool packet::is_valid() const {
    return this->checksum == this->get_checksum();
}

bool packet::is_syn() const {
    return (padding >> 1) & 1;
}

bool packet::is_ack() const {
    return (padding & 1);
}

bool packet::is_fin() const {
    return (padding >> 2) & 1;
}
