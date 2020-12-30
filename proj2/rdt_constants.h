
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <sys/stat.h>
#include <stddef.h>

namespace rdt_const {

    // constants in bytes
    const int INIT_SEQ_NUM = 0;
    const int MAX_PKT_LEN = 1024;
    const int MAX_SEQ_NUM = 30720;
    const int MAX_WINDOW_SIZE = 5120;
    const int FINACK_RETRIES = 3;

    // For congestion control.
    const int INIT_SSTHRESH = 15360;
    const int INIT_CWND = 1024;
    
    // time-out in ms
    const int RTO_MS = 500;
    // time-out granularity in ms
    const int RTO_GRAN = 50;

    inline void print_sc_error(std::string error_description) {
	int error_num = errno;
	fprintf(stderr, "Error: %s: %s", error_description.c_str(),
		strerror(error_num));
	fflush(stderr);
	exit(1);
    }

    // Uses stat() to get the file size (otherwise return 0)
    inline size_t get_filesize(std::string filename) {
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : 0;
    }

    // Increment sequence numbers with safe rollover.
    inline uint16_t inc_seqnum(uint16_t seqnum, unsigned int amt) {
	unsigned int sum = seqnum + amt;
	int diff = MAX_SEQ_NUM + 1 - sum;
	// If diff is <= 0, then we account for rollover by returning the
	// absolute difference (plus 1, b/c we rollover to 0).
	return static_cast<uint16_t>( (diff <= 0) ? -diff : sum );
    }

    // Compare sequence numbers accounting for rollover. Not *that* robust, but
    // works every case we can expect and is pretty efficient.
    inline int cmp_seqnum(uint16_t lvalue, uint16_t rvalue) {
	int diff = lvalue - rvalue;
	// If the difference is too great, we account for rollover by flipping the value.
	return (std::abs(diff)  > (MAX_SEQ_NUM / 2)) ? -diff : diff;	    
    }

}

#endif
