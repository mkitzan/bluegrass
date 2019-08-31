#include <iostream>
#include <fstream>
#include "file_transfer.hpp"
#include "bluegrass/controller.hpp"

using namespace std;
using namespace bluegrass;

int main() {
	vector<bdaddr_t> devices;
	struct packet_t packet{ 0, 0 };
	uint8_t count{ 1 };
	
	hci_controller hci = hci_controller::access();
	hci.address_inquiry(8, devices);
	
	for(auto peer : devices) {
		try {
			cout << "Creating client socket to " << peer << endl << flush;
			unique_socket us(bluegrass::socket<L2CAP>(peer, 0x1001));
			cout << "Client construction succeeded" << endl << flush;
			
			while(us.receive(&packet)) {
				cout << '[' << peer << ']' << " packet " << count << " recieved" 
				<< endl << flush;
				cout.write((const char*) packet.data, packet.size) << endl << flush;
				++count;
			}
		} catch(...) {
			cout << "Client construction failed" << endl;
		}
	}
	
	return 0;
}
