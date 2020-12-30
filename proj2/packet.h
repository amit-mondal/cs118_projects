
#ifndef PACKET_H
#define PACKET_H

#include "rdt_constants.h"

struct packet {
    uint16_t seq_num;
    uint16_t ack_num;
    uint16_t padding;
    uint16_t pkt_len;
    uint16_t checksum;    
    static const int PKT_HEADER_SIZE = 10;
    static const int PKT_MAX_DATA_SIZE = rdt_const::MAX_PKT_LEN - PKT_HEADER_SIZE;
    size_t data_len;
    char* data = nullptr;
    packet();
    packet(uint16_t seq_num, uint16_t ack_num, bool ack, bool syn, bool fin, char* data = nullptr, size_t data_len = 0);
    packet(char* raw_pkt, size_t len);
    ~packet();
    void update(char* raw_pkt, size_t len, bool is_new_pkt = false);
    void update(uint16_t seq_num, uint16_t ack_num, bool ack, bool syn, bool fin, char* data = nullptr, size_t data_len = 0);
    size_t make_raw_pkt(char* buf) const;
    uint16_t get_checksum() const;
    void dump() const;
    bool is_valid() const;
    bool is_syn() const;
    bool is_ack() const;
    bool is_fin() const;
};

#endif
