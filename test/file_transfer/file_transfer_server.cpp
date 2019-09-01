#include <iostream>
#include <fstream>
#include "file_transfer.hpp"
#include "bluegrass/service_queue.hpp"

using namespace std;
using namespace bluegrass;

void transfer(bluegrass::socket<L2CAP>& conn) {	
	unique_socket us(std::move(conn));
	
	bdaddr_t peer{ 0 };
	struct packet_t packet{ 0, 0 };
	uint8_t count{ '1' };
	
	cout << "Receiving address of client" << endl << flush;
	us.receive(&peer);
	cout << "Connection received from " << peer << endl << flush;
	std::ifstream file("../../test_files/zimmermann.txt", std::ios::binary);
	cout << "Transfering file \"zimmermann.txt\" to client" << endl << flush;
	
	while(file.good()) {
		file.read((char*) packet.data, sizeof packet.data);
		packet.size = file.gcount();
		cout << '[' << peer << ']' << " sending packet " << count;
		
		if(us.send(&packet)) {
			cout << " [success]"<< endl << flush;
		} else {
			cout << " [failure]" << endl << flush;
			return;
		}
		
		++count;
	}
	
	cout << '[' << peer<< ']' << " file transfer complete" << endl << endl << flush;
}

int main() {
	cout << "Creating service_queue" << endl << flush;
	service_queue<bluegrass::socket<L2CAP>, ENQUEUE> sq(transfer, 4, 2);
	
	try {
		cout << "Creating server" << endl << flush;
		server<L2CAP> s(&sq, 0x1001, 4);
		cout << "Server construction succeeded" << endl << flush;
		for(;;);
	} catch(...) {
		cout << "Server construction failed" << endl;
		return 0;
	}
	
	return 0;
}
