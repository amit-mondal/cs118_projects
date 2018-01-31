#include "response.h"
using namespace std;

// Simple convenience function for printing syscall errors
void print_sc_error(string error_description) {
    int error_num = errno;
    fprintf(stderr, "Error: %s: %s", error_description.c_str(),
	    strerror(error_num));
    fflush(stderr);
    exit(1);
}

enum file_ext { html, jpeg, gif, none };

struct http_response {
    string status;
    long content_size;
    string content_type;
    string to_string() {
	if (status == NOT_FOUND) {
	    return status + "\r\n";
	}
	return status + "\r\nConnection: close\r\n" +
	    "Content-Length: " + std::to_string(content_size) +
	    "\r\nContent-Type: " + content_type + "\r\n\r\n";
    }
};

long get_filesize(string filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : 0;
}

file_ext get_file_extension(string filename) {
    string extension = filename.substr(filename.find_last_of(".") + 1);
    if (extension == "html" || extension == "htm") {
	return html;
    }
    if (extension == "gif") {
	return gif;
    }
    if (extension == "jpeg" || extension == "jpg") {
	return jpeg;
    }
    return none;
}

void send_data(int sockfd, char* data, long data_len) {
    int bytes_sent;
    while (data_len > 0) {
	bytes_sent = send(sockfd, data, data_len, 0);
	if (bytes_sent < 0) {
	    print_sc_error("could not send data");	    
	}
	data += bytes_sent;
	data_len -= bytes_sent;
    }    
}

void send_file(int sockfd, string filename) {
    http_response response;
    char* buf = nullptr;
    long filesize = get_filesize(filename);
    if (filesize < 0) {
	response.status = NOT_FOUND;
    }
    else {
	response.status = OK;
    }
    file_ext extension = get_file_extension(filename);
    if (extension == html) {
	response.content_type = "text/html";
    }
    else if (extension == jpeg) {
	response.content_type = "image/jpeg";
    }
    else if (extension == gif) {
	response.content_type = "image/gif";
    }
    else {
	response.content_type = "application/octet-stream";
    }

    ifstream file(filename, ios::in|ios::binary|ios::ate);
    if (file.is_open()) {
	buf = new char[filesize];
	file.seekg(0, ios::beg);
	file.read(buf, filesize);	
	file.close();	
    }
    else {
	response.status = NOT_FOUND;
    }
    response.content_size = filesize;
    string response_string = response.to_string();
    char* cstr = new char[response_string.length() + 1];
    strcpy(cstr, response_string.c_str());
    send_data(sockfd, cstr, response_string.length());
    if (buf) {
	send_data(sockfd, buf, filesize);
	delete [] buf;
    }
    delete [] cstr;
}
