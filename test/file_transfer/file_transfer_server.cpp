#include <iostream>
#include <fstream>

#include "file_transfer.hpp"

#include "bluegrass/service.hpp"
#include "bluegrass/socket.hpp"

using namespace std;
using namespace bluegrass;

// this routine sends "zimmermann.txt" to the client Bluetooth device
void transfer(bluegrass::socket& conn) {	
	scoped_socket us(std::move(conn));
	
	bdaddr_t peer {0};
	struct packet_t packet {0, 0};
	uint8_t count {1};
	
	// find out who the client is
	cout << "Receiving address of client\n" << flush;
	us >> &peer;
	cout << "Connection received from " << peer << endl << flush;

	// change this file path to where ever your zimmermann.txt file is
	ifstream file("../../test_files/zimmermann.txt", ios::binary);
	cout << "Transfering file \"zimmermann.txt\" to client\n" << flush;
	
	// print status of each packet sent
	while (file.good()) {
		file.read((char*) packet.data, sizeof packet.data);
		packet.size = file.gcount();
		cout << '[' << peer << ']' << " sending packet " << (int) count++;
		
		if (us.send(&packet)) {
			cout << " [success]\n" << flush;
		} else {
			cout << " [failure]\n" << flush;
			return;
		}
	}
	
	cout << '[' << peer<< ']' << " file transfer complete\n" << endl << flush;
}

int main() {	
	try {
		cout << "Creating network\n" << flush;
		async_socket::service_handle s {transfer, 2, 1};
		async_socket as(ANY, 0x1001, s, async_t::SERVER);
		cout << "Server construction succeeded\n" << flush;
		// this example waits, but a real system could do other work (the network is async)
		for (;;);
	} catch (...) {
		cout << "Server construction failed\n";
	}
	
	return 0;
}
