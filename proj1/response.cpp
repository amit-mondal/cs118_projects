// Amit Mondal and Michael Cade Mallett
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

// Enum to identify file types
enum file_ext { html, jpeg, gif, none };

// Struct representing the HTTP response. to_string returns the formatted header.
struct http_response {
    string status;
    long content_size;
    string content_type;
    string to_string() {
	// Send a 404 if the status is NOT_FOUND
	if (status == NOT_FOUND) {
	    return status + "\r\n\r\n<b>404 File Not Found</b>";
	}
	return status + "\r\nConnection: close\r\n" +
	    "Content-Length: " + std::to_string(content_size) +
	    "\r\nContent-Type: " + content_type + "\r\n\r\n";
    }
};

// Uses stat() to get the file size (otherwise return 0)
long get_filesize(string filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : 0;
}

// Checks the extension of the file and returns an enum instance
file_ext get_file_extension(string filename) {
    string extension = filename.substr(filename.find_last_of(".") + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
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

// Uses the send() system call to send the file data in chunks. Data that
// is not sent in one call will be re-sent in a later one.
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

// This function checks the filename, reads in its data, and sends
// a response header as well as the file data. If the file doesn't
// exist, it sends a 404 header back.

void send_file(int sockfd, string filename) {
    // Allocate a response.
    http_response response;
    char* buf = nullptr;
    // Lookup filesize (0 if the file doesn't exist).
    long filesize = get_filesize(filename);
    if (filesize < 0) {
	response.status = NOT_FOUND;
    }
    else {
	response.status = OK;
    }
    // Check the file extension.
    file_ext extension = get_file_extension(filename);
    // Set the response's content type based on the extension.
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
	// If we don't know the type, send it as binary data.
	response.content_type = "application/octet-stream";
    }

    // Read in file data using an ifstream.
    ifstream file(filename, ios::in|ios::binary|ios::ate);
    if (file.is_open()) {
	// Allocate appropriately sized buffer.
	buf = new char[filesize];
	file.seekg(0, ios::beg);
	// Read file and then close.
	file.read(buf, filesize);	
	file.close();	
    }
    else {
	// If we can't find the open, respond with a 404.
	response.status = NOT_FOUND;
    }
    // Set the content size to the filesize.
    response.content_size = filesize;
    string response_string = response.to_string();
    char* cstr = new char[response_string.length() + 1];
    strcpy(cstr, response_string.c_str());
    // Send the response header string to the socket.
    send_data(sockfd, cstr, response_string.length());
    // If the file existed, send the file data to the socket.
    if (buf) {
	send_data(sockfd, buf, filesize);
	delete [] buf;
    }
    delete [] cstr;
}
