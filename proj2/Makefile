CXX = g++
CXXFLAGS = -g -Wall -Wextra -Wno-unused-parameter -std=c++11

sources = crdt_server.cpp rdt_client.cpp rdt_server.cpp packet.cpp server.cpp client.cpp cc_server.cpp

headers = crdt_server.h rdt_client.h rdt_server.h rdt_constants.h packet.h

all: cc server client $(sources)

cc: cc_server

server: rdt_server.o server.o packet.o
	$(CXX) $(CXXFLAGS) rdt_server.o server.o packet.o -o server

client: rdt_client.o client.o packet.o
	$(CXX) $(CXXFLAGS) rdt_client.o client.o packet.o -o client

cc_server: crdt_server.o cc_server.o packet.o
	$(CXX) $(CXXFLAGS) crdt_server.o cc_server.o packet.o -o cc_server

crdt_server.o: crdt_server.h packet.h rdt_constants.h

rdt_client.o: rdt_client.h packet.h rdt_constants.h
rdt_server.o: rdt_server.h packet.h rdt_constants.h

packet.o: packet.h rdt_constants.h
client.o: rdt_client.h
server.o: rdt_server.h

clean:
	rm -rf 804746916
	rm -f *.o *~ server client cc_server cc_client received.data 804746916.tar.gz

deliverables = $(sources) $(headers) Makefile README report.pdf
DESTINATION = 804746916

dist: 804746916.tar.gz
804746916.tar.gz: $(deliverables)
	mkdir -p $(DESTINATION)
	for file in $(deliverables); do \
	  cp $$file $(DESTINATION); \
	done
	tar -czf 804746916.tar.gz $(DESTINATION)
