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
	rm -rf 804746916
	rm -f *.o *~ ./server 804746916.tar.gz

deliverables = $(sources) $(headers) Makefile README report.pdf
DESTINATION = 804746916

dist: 804746916.tar.gz
804746916.tar.gz: $(deliverables)
	mkdir -p $(DESTINATION)
	for file in $(deliverables); do \
	  cp $$file $(DESTINATION); \
	done
	tar -czf 804746916.tar.gz $(DESTINATION)
