#include <iostream>
#include <vector>
#include <cassert>
#include <mutex>
#include <unistd.h>
#include "bluegrass/bluetooth.hpp"
#include "bluegrass/socket.hpp"
#include "bluegrass/controller.hpp"

using namespace std;
using namespace bluegrass;

template<proto_t P>
void test(uint16_t n) {
	vector<bdaddr_t> devices;
	bdaddr_t self = { 0xDA, 0x33, 0x94, 0xEB, 0x27, 0xB8 }, ret;
	hci_controller hci = hci_controller::access();
	hci.address_inquiry(8, devices);
	
	for(auto peer : devices) {
		cout << "\tCreating client" << endl << flush;
		try {
			unique_socket us(bluegrass::socket<P>(peer, n));
			if(us.send(&self)) {
				cout << "\tSent:     " << self << endl << flush;
			} else {
				cout << "\tSend failed" << endl;
			}
			if(us.receive(&ret)) {
				cout << "\tReceived: " << ret << endl << flush;
			} else {
				cout << "\tReceive failed" << endl;
			}
		} catch(...) {
			cout << "\tClient construction failed" << endl;
		}
	}
}

int main() {
	cout << "Starting L2CAP client test" << endl;
	test<L2CAP>(0x1001);
	cout << "L2CAP server test complete" << endl << endl << flush;
	usleep(100000);
	cout << "Starting RFCOMM client test" << endl;
	test<RFCOMM>(0x1);
	cout << "RFCOMM server test complete" << endl;
	
	return 0;
}
