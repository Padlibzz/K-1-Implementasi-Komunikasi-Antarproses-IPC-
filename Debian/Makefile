CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -pthread
BIN      = stok_socket_server stok_xmlrpc_server pesanan

all: $(BIN)

stok_socket_server: stok_socket_server.cpp net.hpp stok.hpp
	$(CXX) $(CXXFLAGS) -o $@ $<

stok_xmlrpc_server: stok_xmlrpc_server.cpp net.hpp stok.hpp
	$(CXX) $(CXXFLAGS) -o $@ $<

pesanan: pesanan.cpp net.hpp stok.hpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test: all
	bash ./test.sh

clean:
	rm -f $(BIN) hasil_pengujian.txt
