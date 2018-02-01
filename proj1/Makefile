CXX = g++
CXXFLAGS = -g -Wall -Wextra -Wno-unused-parameter -std=c++11

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
	rm -f *.o *~ ./server

deliverables = $(sources) $(headers) Makefile README

dist: 804746916.tar.gz
804746916.tar.gz: $(deliverables)
	tar -czf 804746916.tar.gz $(deliverables)
