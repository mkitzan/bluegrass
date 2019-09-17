#include <iostream>
#include <fstream>

#include "file_transfer.hpp"

#include "bluegrass/service_queue.hpp"
#include "bluegrass/socket.hpp"
#include "bluegrass/unique_socket.hpp"
#include "bluegrass/server.hpp"

using namespace std;
using namespace bluegrass;

void transfer(bluegrass::socket<L2CAP>& conn) {	
	unique_socket us(std::move(conn));
	
	bdaddr_t peer{ 0 };
	struct packet_t packet{ 0, 0 };
	uint8_t count{ '1' };
	
	cout << "Receiving address of client\n" << flush;
	us.receive(&peer);
	cout << "Connection received from " << peer << endl << flush;
	ifstream file("../../test_files/zimmermann.txt", ios::binary);
	cout << "Transfering file \"zimmermann.txt\" to client\n" << flush;
	
	while(file.good()) {
		file.read((char*) packet.data, sizeof packet.data);
		packet.size = file.gcount();
		cout << '[' << peer << ']' << " sending packet " << count++;
		
		if(us.send(&packet)) {
			cout << " [success]\n" << flush;
		} else {
			cout << " [failure]\n" << flush;
			return;
		}
	}
	
	cout << '[' << peer<< ']' << " file transfer complete\n" << endl << flush;
}

int main() {
	cout << "Creating service_queue\n" << flush;
	service_queue<bluegrass::socket<L2CAP>, ENQUEUE> sq(transfer, 4, 2);
	
	try {
		cout << "Creating server\n" << flush;
		server<L2CAP> s(&sq, 0x1001, 4);
		cout << "Server construction succeeded\n" << flush;
		for(;;);
	} catch(...) {
		cout << "Server construction failed\n";
	}
	
	return 0;
}
