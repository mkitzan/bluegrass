#include <iostream>
#include <fstream>
#include "file_transfer.hpp"
#include "bluegrass/service_queue.hpp"

using namespace std;
using namespace bluegrass;

void transfer(bluegrass::socket<RFCOMM>& conn) {	
	unique_socket us(std::move(conn));
	
	bdaddr_t peer{ us.sockaddr().addr.rc_bdaddr };
	struct packet_t packet{ 0, 0 };
	uint8_t count{ 1 };
	
	cout << "Connection recieved from " << peer << endl << flush;
	std::ifstream file("../../test_files/zimmermann.txt", std::ios::binary);
	cout << "File stream opened to \"zimmermann.txt\"" << endl << flush;
	
	while(file.good()) {
		file.read((char*) packet.data, sizeof packet.data);
		packet.size = file.gcount();
		cout << '[' << peer << ']' << " sending packet " << count 
		<< endl << flush;
		
		if(us.send(&packet)) {
			cout << '[' << peer << ']' << " packet " << count << " sent" 
			<< endl << flush;
		} else {
			cout << '[' << peer << ']' << " packet " << count << " failed" 
			<< endl << flush;
			return;
		}
		
		++count;
	}
	
	cout << '[' << peer<< ']' << " file transfer completed" << endl << flush;
}

int main() {
	cout << "Creating service_queue" << endl << flush;
	service_queue<bluegrass::socket<RFCOMM>, ENQUEUE> sq(transfer, 4, 2);
	
	try {
		cout << "Creating server" << endl << flush;
		server<RFCOMM> s(&sq, 0x1, 4);
	} catch(...) {
		cout << "Server construction failed" << endl;
		return 0;
	}
	
	cout << "Server construction succeeded" << endl << flush;
	for(;;);
	
	return 0;
}
