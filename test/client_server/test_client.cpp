#include <iostream>
#include <vector>
#include <cassert>
#include <mutex>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci_controller.hpp"
#include "bluegrass/socket.hpp"

using namespace std;
using namespace bluegrass;

template<proto_t P>
void test(uint16_t n) {
	vector<device> devices;
	
	hci_controller& hci = hci_controller::access();
	hci.device_inquiry(8, devices);
	bdaddr_t self { hci.local_address() }, ret;
	
	for (auto& dev : devices) {
		cout << "\tCreating client\n" << flush;
		try {
			unique_socket us(bluegrass::socket<P>(dev.addr, n));
			if (us.send(&self)) {
				cout << "\tSent:	 " << self << endl << flush;
			} else {
				cout << "\tSend failed\n";
			}
			if(us.receive(&ret)) {
				cout << "\tReceived: " << ret << endl << flush;
			} else {
				cout << "\tReceive failed\n";
			}
		} catch (...) {
			cout << "\tClient construction failed\n";
		}
	}
}

int main() {
	cout << "Starting L2CAP client test\n";
	test<L2CAP>(0x1001);
	cout << "L2CAP server test complete\n\n" << flush;
	
	usleep(100000);

	cout << "Starting RFCOMM client test\n";
	test<RFCOMM>(0x1);
	cout << "RFCOMM server test complete\n";
	
	return 0;
}
