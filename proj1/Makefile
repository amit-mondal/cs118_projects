CXX = g++
CXXFLAGS = -g -Wall -Wextra -Wno-unused-parameter

sources = main.cpp request.cpp response.cpp
headers = request.h response.h
objfiles = main.o response.o request.o

all: server

server: $(sources) $(headers) $(objfiles)
	$(CXX) $(CXXFLAGS) $(objfiles) -o server 

main.o: $(headers)
request.o: request.h
response.o: response.h

clean:
	rm -f *.o *~ server
