#include <string>
#include <iostream>
#include <vector>

using namespace std;

struct http_request {
    string http_method;
    string request_target;
    string http_version;

    vector<string> header_values;
    vector<string> header_types;

    string data;
};

// for debugging. just prints out all of the sections of the request struct
void dump_req(http_request req) {
    cerr << "begin dump of req" << endl;
    cerr << req.http_method << endl;
    cerr << req.request_target << endl;
    cerr << req.http_version << endl;

    vector<string>::iterator j = req.header_values.begin();
    vector<string>::iterator i = req.header_types.begin();
    for ( ; i != req.header_types.end() && j != req.header_values.end(); ++i, ++j) {
	cerr << *i << "   " << *j << endl;
    }

    cerr << req.data << endl;
    cerr << "end dump" << endl;
}

// this just creates an array of the individual lines
vector<string> separate_lines(string message) {
    vector<string> separated_lines;
    string temp = "";

    bool found_carriage_return = false;
    for (string::iterator c = message.begin(), end = message.end(); c != end; ++c) {
	if (*c == '\r') {
	    found_carriage_return = true;
	}
	else if (*c == '\n' && found_carriage_return) { 
	    separated_lines.push_back(temp);
	    temp = "";
	    found_carriage_return = false;
	}
	else {
	    temp += *c;
	}
    }
    return separated_lines;

}

// want to find HTTP method, request target, and HTTP version
int parse_start_line(string line, http_request &req) {
    //TODO: turn %20 into a space
    string temp = "";
    string arr[3];
    int count = 0;

    for (string::iterator c = line.begin(), end = line.end(); c != end; ++c) {
	if (*c == ' ') {
	    arr[count] = temp;
	    temp = "";
	    count++;
	}
	else {
	    temp += *c;
	}
    }
    arr[count] = temp;

    if (arr[0] != "GET" && arr[0] != "POST" && arr[0] != "HEAD" && arr[0] != "OPTIONS") {// note that this may not be an exhaustive list. If not, fix it
	// throw an error if the HTTP method is not valid
	return 1;
    }

    //remove the slash from the request target
    arr[1] = arr[1].substr(1, arr[1].length()-1); // now this could be an empty string, ex, if the method was post

    //now check that a valid HTTP version was requested
    if (!(arr[2].substr(0, 5) == "HTTP/")) {
	return 1;
    }
    string vers = arr[2].substr(5, arr[2].length() - 5);
    if (!(vers == "1.1" || vers == "1.0" || vers == "2.0")) {
	return 1;
    }

    req.http_method = arr[0];
    req.request_target = arr[1];
    req.http_version = arr[2];

    return 0;
}

//return an array of two strings. the first contains up to the colon. The second conatins everything after
string * split_around_colon(string line) {
    static string strs[2] = { "", "" };
    for (string::iterator c = line.begin(), end = line.end(); c != end; ++c) {
	if (*c == ':') {
	    c++;
	    while (c != line.end()) {
		strs[1] += *c;
		c++;
	    }
	    return strs; //there are two strings
	}
	else {
	    strs[0] += *c;
	}
    }
    return strs; //there is one string
}

int parse_header_lines(vector<string> header_lines, http_request req) {
    vector<string>::iterator i = header_lines.begin();
    for (; i != header_lines.end(); i++) {
	string *strs = split_around_colon(*i);
	if (*(strs+1) != "") {
	    req.header_types.push_back(*strs);
	    req.header_values.push_back(*(strs+1));
	}
	else {
	    break;
	}
    }
    return 0;
}

int parse_http_message(string message, http_request &req) {
    vector<string> message_lines = separate_lines(message);
    if (parse_start_line(message_lines[0], req) == 1) {
	return -1;
    }
    vector<string> header_lines;
    for (vector<string>::iterator i = message_lines.begin() + 1; i != message_lines.end(); i++) {
	header_lines.push_back(*i);
    }
    if (parse_header_lines(header_lines, req) == 1) {
	return -1;
    }
    return 0;
}

void replace_all(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty()) {
	return;
    }
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
	str.replace(start_pos, from.length(), to);
	start_pos += to.length();
	// In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

string get_file_name(string request_text) {
    http_request req;
    if (parse_http_message(request_text, req) < 0) {
	return "";
    }
    // handle files with spaces by replacing %20 with ' '
    replace_all(req.request_target, "%20", " ");
    return req.request_target;
}
