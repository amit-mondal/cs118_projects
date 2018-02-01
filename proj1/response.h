// Amit Mondal and Michael Cade Mallett
#ifndef RESPONSE_H
#define RESPONSE_H

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

const std::string NOT_FOUND = "HTTP/1.1 404 Not Found";
const std::string OK = "HTTP/1.1 200 OK";

void print_sc_error(std::string error_description);
void send_file(int sockfd, std::string filename);


#endif
